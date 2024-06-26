﻿#include "logger/logger.h"
#include "hvtester.h"
#include "sysconfigs/sysconfigs.h"

static const char* gs_str_not_init = "tester未正常初始化";
static const char* gs_str_a_new_round = "新一轮测试开始";
static const char* gs_str_mb_write_null_reply = "modbus写入异常，返回空reply";
static const char* gs_str_mb_start_expo_null_reply = "modbus启动曝光异常，返回空reply";
static const char* gs_str_mb_read_null_reply = "modbus读取异常，返回空reply";
static const char* gs_str_mb_write_triple = "modbus设置曝光参数";
static const char* gs_str_mb_start_expo = "modbus发起曝光";
static const char* gs_str_mb_read_regs = "modbus读取常规寄存器";
static const char* gs_str_mb_read_distance= "modbus读取距离寄存器";
static const char* gs_str_mb_read_regs_invalid = "modbus读取常规寄存器数据无效";
static const char* gs_str_mb_read_distance_invalid = "modbus读取距离数据无效";
const char* g_str_fail = "失败";

HVTester::HVTester(QObject *parent)
    : QObject{parent},
      hv_test_op_timer(this)
{
    qRegisterMetaType<mb_reg_val_map_t>();
    qRegisterMetaType<tester_op_enum_t>();

    connect(this, &HVTester::start_expo_now_sig, this, &HVTester::start_expo_now_sig_handler,
                Qt::QueuedConnection);

    hv_test_op_timer.setSingleShot(true);
    connect(&hv_test_op_timer, &QTimer::timeout, this, &HVTester::hv_test_op_timer_handler,
                Qt::QueuedConnection);

    connect(this, &HVTester::start_readback_now_sig,
            this, &HVTester::start_readback_now_sig_handler, Qt::QueuedConnection);

    connect(this, &HVTester::start_read_distance_sig,
            this, &HVTester::start_read_distance_sig_handler, Qt::QueuedConnection);

    connect(this, &HVTester::internal_go_test_sig, this, &HVTester::go_test_sig_handler,
                Qt::QueuedConnection);
}

bool HVTester::init(test_params_struct_t *test_params, QModbusClient * modbus_device, int srvr_addr)
{
    if(!test_params || !modbus_device || !test_params->valid)
    {
        DIY_LOG(LOG_ERROR,
                QString("%1%0x2,0x%3,%4").arg("tester init fails: ",
                                           QString::number((qlonglong)test_params, 16),
                                           QString::number((qlonglong)modbus_device, 16),
                                           QString::number(test_params ? test_params->valid : 0)));
        return false;
    }

    hv_test_params = test_params;
    hv_modbus_device = modbus_device;
    hv_modbus_srvr_addr = srvr_addr;

    hv_tester_proc = TESTER_IDLE;
    hv_curr_op = TEST_OP_NULL;
    hv_test_idx_in_loop = 0;
    hv_test_idx_in_round = -1;

    hv_test_op_timer.stop();

    m_regs_read_result.clear();
    return true;
}

bool HVTester::is_the_last_one_test()
{
    bool ret;
    if(hv_test_params->expo_param_block.cust)
    {
        int arr_cnt =  hv_test_params->expo_param_block.expo_params.cust_params_arr.count();
        ret = ((hv_test_idx_in_round + 1) >= arr_cnt)
              &&  ((hv_test_idx_in_loop + 1) >= hv_test_params->expo_param_block.expo_cnt);
    }
    else
    {
        ret = ((hv_test_idx_in_loop + 1 >= hv_test_params->expo_param_block.expo_cnt)
        && (hv_curr_expo_param_triple.cube_volt_kv ==
              hv_test_params->expo_param_block.expo_params.regular_parms.cube_volt_kv_end)
        && (hv_curr_expo_param_triple.cube_current_ma ==
              hv_test_params->expo_param_block.expo_params.regular_parms.cube_current_ma_end)
        && (hv_curr_expo_param_triple.dura_ms ==
              hv_test_params->expo_param_block.expo_params.regular_parms.expo_dura_ms_end));
    }
    return ret;
}

void HVTester::update_tester_state()
{
    if(!hv_test_params || !hv_test_params->valid)
    {
        hv_tester_proc = TESTER_IDLE;
        return;
    }

    ++hv_test_idx_in_round;
    if(hv_test_params->expo_param_block.cust)
    {
        int arr_cnt =  hv_test_params->expo_param_block.expo_params.cust_params_arr.count();
        if(hv_test_idx_in_round >= arr_cnt)
        {
            ++hv_test_idx_in_loop;
            if(hv_test_idx_in_loop >= hv_test_params->expo_param_block.expo_cnt)
            {
                hv_tester_proc = TESTER_COMPLETE;
                return;
            }

            hv_test_idx_in_round = 0;
        }
        hv_curr_expo_param_triple =
            hv_test_params->expo_param_block.expo_params.cust_params_arr.at(hv_test_idx_in_round);
        if(is_the_last_one_test())
        {
             hv_tester_proc = TESTER_LAST_ONE;
        }
        else
        {
            hv_tester_proc = ((0 == hv_test_idx_in_round) ? TESTER_A_NEW_ROUND : TESTER_WORKING);
        }
    }
    else
    {
        if(0 == hv_test_idx_in_round)
        {
            hv_curr_expo_param_triple.cube_volt_kv =
                  hv_test_params->expo_param_block.expo_params.regular_parms.cube_volt_kv_start;
            hv_curr_expo_param_triple.cube_current_ma =
                  hv_test_params->expo_param_block.expo_params.regular_parms.cube_current_ma_start;
            hv_curr_expo_param_triple.dura_ms =
                  hv_test_params->expo_param_block.expo_params.regular_parms.expo_dura_ms_start;

            if(is_the_last_one_test())
            {
                hv_tester_proc = TESTER_LAST_ONE;
            }
            else
            {
                hv_tester_proc = TESTER_A_NEW_ROUND;
            }
        }
        else while(true)
        {
            bool roundup = false;
#define MOVE_A_STEP(low_e, up_e, step, curr) \
{\
    if((curr) == (up_e))\
    {\
        (curr) = (low_e);\
    }\
    else\
    {\
        (curr) += (step);\
        if((step) >= 0) {if((curr) >= (up_e)) (curr) = (up_e);}\
        else if((curr) < (up_e)) (curr) = (up_e);\
    }\
    if((curr) == (low_e)) roundup = true;\
}
            MOVE_A_STEP(
                hv_test_params->expo_param_block.expo_params.regular_parms.expo_dura_ms_start,
                hv_test_params->expo_param_block.expo_params.regular_parms.expo_dura_ms_end,
                hv_test_params->expo_param_block.expo_params.regular_parms.expo_dura_ms_step,
                hv_curr_expo_param_triple.dura_ms);
            if(!roundup)
            {
                hv_tester_proc = is_the_last_one_test() ? TESTER_LAST_ONE : TESTER_WORKING;
                break;
            }

            roundup = false;
            MOVE_A_STEP(
                hv_test_params->expo_param_block.expo_params.regular_parms.cube_current_ma_start,
                hv_test_params->expo_param_block.expo_params.regular_parms.cube_current_ma_end,
                hv_test_params->expo_param_block.expo_params.regular_parms.cube_current_ma_step,
                hv_curr_expo_param_triple.cube_current_ma);
            if(!roundup)
            {
                hv_tester_proc = is_the_last_one_test() ? TESTER_LAST_ONE : TESTER_WORKING;
                break;
            }

            roundup = false;
            MOVE_A_STEP(
                hv_test_params->expo_param_block.expo_params.regular_parms.cube_volt_kv_start,
                hv_test_params->expo_param_block.expo_params.regular_parms.cube_volt_kv_end,
                hv_test_params->expo_param_block.expo_params.regular_parms.cube_volt_kv_step,
                hv_curr_expo_param_triple.cube_volt_kv);
            if(!roundup)
            {
                hv_tester_proc = is_the_last_one_test() ? TESTER_LAST_ONE : TESTER_WORKING;
                break;
            }
            ++hv_test_idx_in_loop;
            if(hv_test_idx_in_loop >= hv_test_params->expo_param_block.expo_cnt)
            {
                hv_tester_proc = TESTER_COMPLETE;
                return;
            }
            hv_test_idx_in_round = 0;
            hv_tester_proc = is_the_last_one_test() ? TESTER_LAST_ONE : TESTER_A_NEW_ROUND;

            break;
#undef MOVE_A_STEP
        }
    }
    hv_curr_triple_mb_unit_str = QString("%1,%2,%3").arg(
                   QString::number((quint16)(hv_curr_expo_param_triple.cube_volt_kv)),
                   QString::number((quint16)(1000 * hv_curr_expo_param_triple.cube_current_ma)),
                   QString::number((quint16)(hv_curr_expo_param_triple.dura_ms)));
}

int HVTester::calc_cool_dura_ms()
{
    float cool_dura_ms = hv_test_params->expo_param_block.fixed_cool_dura ?
                   hv_test_params->expo_param_block.expo_cool_dura_ms :
                   hv_test_params->expo_param_block.expo_cool_dura_factor
                   * hv_curr_expo_param_triple.dura_ms;
    cool_dura_ms += g_sys_configs_block.extra_cool_time_ms;
    return (int)cool_dura_ms;
}

bool HVTester::mb_rw_reply_received(tester_op_enum_t op, QModbusReply* mb_reply,
                                    void (HVTester::*finished_sig_handler)(),
                                    bool sync, bool err_notify)
{
    QString err_str;
    int timer_ms;
    bool delete_now = false;
    bool goon = true;
    switch(op)
    {
    case TEST_OP_SET_EXPO_TRIPLE:
        err_str = (mb_reply ? QString("%1 %2 %3: %4").arg(gs_str_mb_write_triple,
                                                  hv_curr_triple_mb_unit_str, g_str_fail,
                                                  mb_reply->errorString())
                            :
                              QString("%1:%2").arg(gs_str_mb_write_null_reply,
                                              hv_curr_triple_mb_unit_str));
        timer_ms = g_sys_configs_block.consec_rw_wait_ms;
        m_current_handler = &HVTester::set_expo_parameters;
        break;

    case TEST_OP_START_EXPO:
        err_str = (mb_reply ? QString("%1 %2: %3").arg(gs_str_mb_start_expo, g_str_fail,
                                                  mb_reply->errorString())
                            :
                              QString("%1").arg(gs_str_mb_start_expo_null_reply));
        timer_ms = g_sys_configs_block.expo_prepare_time_ms + hv_curr_expo_param_triple.dura_ms;
        m_current_handler = &HVTester::start_expo_now_sig_handler;
        break;

    case TEST_OP_READ_REGS:
        err_str = (mb_reply ? QString("%1 %2: %3").arg(gs_str_mb_read_regs, g_str_fail,
                                                  mb_reply->errorString())
                            :
                              QString("%1").arg(gs_str_mb_read_null_reply));
        if(hv_test_params->other_param_block.read_dist)
        {
            timer_ms = g_sys_configs_block.consec_rw_wait_ms;
        }
        else
        {
            timer_ms = calc_cool_dura_ms();
        }
        m_current_handler = &HVTester::start_readback_now_sig_handler;
        break;

    case TEST_OP_READ_DISTANCE:
    default:
        err_str = (mb_reply ? QString("%1 %2: %3").arg(gs_str_mb_read_distance, g_str_fail,
                                                  mb_reply->errorString())
                            :
                              QString("%1").arg(gs_str_mb_read_null_reply));
        timer_ms = calc_cool_dura_ms();
        m_current_handler = &HVTester::start_read_distance_sig_handler;
        break;
    }

    if(!mb_reply)
    {
        emit test_info_message_sig(LOG_ERROR, err_str);
        DIY_LOG(LOG_ERROR, "tester sends reconnect req because of null reply");
        emit mb_op_err_req_reconnect_sig();
        /*
        if(TESTER_IDLE != hv_tester_proc) hv_test_op_timer.start(timer_ms);
        */
    }
    else if(!sync || mb_reply->isFinished())
    {
        if(sync) delete_now = true;

        QModbusDevice::Error err = mb_reply->error();
        if((QModbusDevice::NoError == err) && (sync || !err_notify))
        {
            if(TEST_OP_READ_REGS == op || TEST_OP_READ_DISTANCE == op)
            {
                QModbusDataUnit rb_du = mb_reply->result();
                if(!rb_du.isValid())
                {
                    QString err_str = (TEST_OP_READ_REGS == op) ? gs_str_mb_read_regs_invalid
                                                                : gs_str_mb_read_distance_invalid;
                    emit test_info_message_sig(LOG_ERROR, err_str);
                    DIY_LOG(LOG_ERROR, "tester sends reconnect req because of invalid du");
                    emit mb_op_err_req_reconnect_sig();
                    goon = false;
                }
                else
                {
                    if(TEST_OP_READ_REGS == op) m_regs_read_result.clear();
                    int st_addr = rb_du.startAddress(), idx = 0, cnt = rb_du.valueCount();
                    for(; idx < cnt; ++idx)
                    {
                        m_regs_read_result.insert(hv_mb_reg_e_t(st_addr + idx), rb_du.value(idx));
                    }
                    if((hv_test_params->other_param_block.read_dist
                            && TEST_OP_READ_DISTANCE == op)
                       ||!hv_test_params->other_param_block.read_dist)
                    {
                        emit rec_mb_regs_sig(TEST_OP_READ_REGS, m_regs_read_result,
                                             hv_test_idx_in_loop, hv_test_idx_in_round);
                    }
                }
            }

            if(goon && (TESTER_IDLE != hv_tester_proc))
            {
                if((hv_test_params->other_param_block.read_dist
                           && (TEST_OP_READ_DISTANCE == op)
                           && (TESTER_LAST_ONE == hv_tester_proc))
                    || (!hv_test_params->other_param_block.read_dist
                           && (TEST_OP_READ_REGS == op)
                           && (TESTER_LAST_ONE == hv_tester_proc)))
                {
                    //for the last one, no need to set timer again, and let go_test to end
                    //the loop.
                    emit internal_go_test_sig();
                }
                else
                {
                    hv_test_op_timer.start(timer_ms);
                }
            }
        }
        else if((QModbusDevice::NoError != err) && (sync || err_notify))
        {
            DIY_LOG(LOG_ERROR, QString("tester sends reconnect req because of error:") + err_str);
            emit test_info_message_sig(LOG_ERROR, err_str);
            emit mb_op_err_req_reconnect_sig();
            /*
            if(TESTER_IDLE != hv_tester_proc)
            {
                if((hv_test_params->other_param_block.read_dist
                           && (TEST_OP_READ_DISTANCE == op)
                           && (TESTER_LAST_ONE == hv_tester_proc))
                    || (!hv_test_params->other_param_block.read_dist
                           && (TEST_OP_READ_REGS == op)
                           && (TESTER_LAST_ONE == hv_tester_proc)))
                {
                    //for the last one, no need to set timer again, and let go_test to end
                    //the loop.
                    emit internal_go_test_sig();
                }
                else
                {
                    hv_test_op_timer.start(timer_ms);
                }
            }
            */
        }
    }
    else
    {
        if(finished_sig_handler)
        {
            connect(mb_reply, &QModbusReply::finished,
                        this, finished_sig_handler, Qt::QueuedConnection);
        }

        connect(mb_reply, &QModbusReply::errorOccurred,
                    this, &HVTester::mb_rw_error_sig_handler, Qt::QueuedConnection);
    }
    return delete_now;
}

void HVTester::go_test_sig_handler()
{
    if(!hv_test_params || !hv_modbus_device || !hv_test_params->valid)
    {
        DIY_LOG(LOG_ERROR, gs_str_not_init);
        emit test_info_message_sig(LOG_ERROR, gs_str_not_init);
        end_test(TEST_END_EXCEPTION);
        return;
    }
    update_tester_state();
    switch(hv_tester_proc)
    {
    case TESTER_IDLE:
        emit test_info_message_sig(LOG_ERROR, "Tester is IDLE!!!");
    case TESTER_COMPLETE:
        end_test(TEST_END_NORMAL);
        return;

    case TESTER_A_NEW_ROUND:
        emit test_info_message_sig(LOG_INFO, gs_str_a_new_round);
    case TESTER_LAST_ONE:
    case TESTER_WORKING:
    default:
        {
            set_expo_parameters();
        }
        break;
    }
}

void HVTester::set_expo_parameters()
{
    QModbusDataUnit mb_du(QModbusDataUnit::HoldingRegisters);
    QModbusReply *mb_reply;
    QVector<quint16> mb_reg_vals;
    mb_reg_vals.append((quint16)(hv_curr_expo_param_triple.cube_volt_kv));
    mb_reg_vals.append((quint16)(1000* hv_curr_expo_param_triple.cube_current_ma));
    mb_reg_vals.append((quint16)(hv_curr_expo_param_triple.dura_ms));

    mb_reg_val_map_t reg_val_map;
    reg_val_map.insert(VoltSet, mb_reg_vals.at(0));
    reg_val_map.insert(FilamentSet, mb_reg_vals.at(1));
    reg_val_map.insert(ExposureTime, mb_reg_vals.at(2));
    emit rec_mb_regs_sig(TEST_OP_SET_EXPO_TRIPLE, reg_val_map,
                     hv_test_idx_in_loop, hv_test_idx_in_round);

    /*now write to modbus server*/
    mb_du.setStartAddress(VoltSet);
    mb_du.setValues(mb_reg_vals);
    mb_reply = hv_modbus_device->sendWriteRequest(mb_du, hv_modbus_srvr_addr);
    hv_curr_op = TEST_OP_SET_EXPO_TRIPLE;
    DIY_LOG(LOG_INFO, "call mb_rw_reply_received.");
    if(mb_rw_reply_received(hv_curr_op, mb_reply,
                            &HVTester::mb_write_params_finished_sig_handler,
                            true, false))
    {
        delete mb_reply;
    }
}

void HVTester::start_expo_now_sig_handler()
{
    QModbusReply * mb_reply;
    QModbusDataUnit mb_du(QModbusDataUnit::HoldingRegisters, ExposureStart, 1);
    mb_du.setValue(0, START_EXPO_DATA);
    mb_reply = hv_modbus_device->sendWriteRequest(mb_du, hv_modbus_srvr_addr);
    hv_curr_op = TEST_OP_START_EXPO;
    DIY_LOG(LOG_INFO, "call mb_rw_reply_received.");
    if(mb_rw_reply_received(hv_curr_op, mb_reply,
                            &HVTester::mb_start_expo_finished_sig_handler,
                            true, false))
    {
        delete mb_reply;
    }
}

void HVTester::start_readback_now_sig_handler()
{
    QModbusReply * mb_reply;
    QModbusDataUnit mb_du(QModbusDataUnit::HoldingRegisters);
    mb_du.setStartAddress(HSV);
    mb_du.setValueCount(MAX_HV_NORMAL_MB_REG_NUM);
    mb_reply = hv_modbus_device->sendReadRequest(mb_du, hv_modbus_srvr_addr);
    hv_curr_op = TEST_OP_READ_REGS;
    DIY_LOG(LOG_INFO, "call mb_rw_reply_received.");
    if(mb_rw_reply_received(hv_curr_op, mb_reply,
                            &HVTester::mb_read_finished_sig_handler,
                            true, false))
    {
        delete mb_reply;
    }
}

void HVTester::start_read_distance_sig_handler()
{
    QModbusReply * mb_reply;
    QModbusDataUnit mb_du(QModbusDataUnit::HoldingRegisters);
    mb_du.setStartAddress(EXT_MB_REG_DISTANCE);
    mb_du.setValueCount(1);
    mb_reply = hv_modbus_device->sendReadRequest(mb_du, hv_modbus_srvr_addr);
    hv_curr_op = TEST_OP_READ_DISTANCE;
    DIY_LOG(LOG_INFO, "call mb_rw_reply_received.");
    if(mb_rw_reply_received(hv_curr_op, mb_reply,
                            &HVTester::mb_read_distance_finish_sig_handler,
                            true, false))
    {
        delete mb_reply;
    }
}

void HVTester::mb_write_params_finished_sig_handler()
{
    QModbusReply * mb_reply = qobject_cast<QModbusReply *>(sender());
    DIY_LOG(LOG_INFO, "call mb_rw_reply_received.");
    mb_rw_reply_received(TEST_OP_SET_EXPO_TRIPLE, mb_reply, nullptr, false, false);
    mb_reply->deleteLater();
}

void HVTester::mb_start_expo_finished_sig_handler()
{
    QModbusReply * mb_reply = qobject_cast<QModbusReply *>(sender());
    DIY_LOG(LOG_INFO, "call mb_rw_reply_received.");
    mb_rw_reply_received(TEST_OP_START_EXPO, mb_reply, nullptr, false, false);
    mb_reply->deleteLater();
}

void HVTester::mb_read_finished_sig_handler()
{
    QModbusReply * mb_reply = qobject_cast<QModbusReply *>(sender());
    DIY_LOG(LOG_INFO, "call mb_rw_reply_received.");
    mb_rw_reply_received(TEST_OP_READ_REGS, mb_reply, nullptr, false, false);
    mb_reply->deleteLater();
}

void HVTester::mb_read_distance_finish_sig_handler()
{
    QModbusReply * mb_reply = qobject_cast<QModbusReply *>(sender());
    DIY_LOG(LOG_INFO, "call mb_rw_reply_received.");
    mb_rw_reply_received(TEST_OP_READ_DISTANCE, mb_reply, nullptr, false, false);
    mb_reply->deleteLater();
}

void HVTester::mb_rw_error_sig_handler(QModbusDevice::Error /*error*/)
{
    QModbusReply * mb_reply = qobject_cast<QModbusReply *>(sender());
    DIY_LOG(LOG_INFO, "call mb_rw_reply_received.");
    mb_rw_reply_received(hv_curr_op, mb_reply, nullptr, false, true);
    mb_reply->deleteLater();
}

void HVTester::hv_test_op_timer_handler()
{
    switch(hv_curr_op)
    {
    case TEST_OP_SET_EXPO_TRIPLE:
        emit start_expo_now_sig();
        break;

    case TEST_OP_START_EXPO:
        emit start_readback_now_sig();
        break;

    case TEST_OP_READ_REGS:
        if(hv_test_params->other_param_block.read_dist)
        {
            emit start_read_distance_sig();
        }
        else
        {
            emit internal_go_test_sig();
        }
        break;

    case TEST_OP_READ_DISTANCE:
        emit internal_go_test_sig();
        break;

    default:
        break;
    }
}

void HVTester::stop_test_sig_handler(tester_end_code_enum_t /*code*/)
{
    if(hv_test_op_timer.isActive()) hv_test_op_timer.stop();

    hv_test_params = nullptr;
    hv_modbus_device = nullptr;
    hv_tester_proc = TESTER_IDLE;
    hv_curr_op = TEST_OP_NULL;
    hv_test_idx_in_loop = 0;
    hv_test_idx_in_round = -1;

    m_regs_read_result.clear();
}

void HVTester::end_test(tester_end_code_enum_t code)
{
    emit test_complete_sig(code);
    stop_test_sig_handler(code);
}

void HVTester::mb_reconnected_sig_handler()
{
    if(m_current_handler) (this->*m_current_handler)();
}
