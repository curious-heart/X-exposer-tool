#include "logger/logger.h"
#include "hvtester.h"

static const char* gs_str_not_init = "tester未正常初始化";
static const char* gs_str_a_new_round = "新一轮测试开始";
static const char* gs_str_mb_write_null_reply = "modbus写入异常，返回空reply";
static const char* gs_str_mb_start_expo_null_reply = "modbus启动曝光异常，返回空reply";
static const char* gs_str_mb_read_null_reply = "modbus读取异常，返回空reply";
static const char* gs_str_mb_write_triple = "modbus设置曝光参数";
static const char* gs_str_mb_start_expo = "modbus发起曝光";
static const char* gs_str_mb_read_regs = "modbus读取寄存器";
static const char* gs_str_fail = "失败";

static const int gs_mb_write_triple_error_cool_tims_ms = 1500;
static const int gs_mb_expo_readparm_sep_ms = 1500;

HVTester::HVTester(QObject *parent)
    : QObject{parent}, hv_expo_readback_sep_timer(this), hv_cool_timer(this)
{
    connect(this, &HVTester::start_expo_now_sig, this, &HVTester::start_expo_now_sig_handler,
                Qt::QueuedConnection);

    hv_cool_timer.setSingleShot(true);
    connect(&hv_cool_timer, &QTimer::timeout, this, &HVTester::cool_timer_sig_handler,
                Qt::QueuedConnection);

    hv_expo_readback_sep_timer.setSingleShot(true);
    connect(&hv_expo_readback_sep_timer, &QTimer::timeout, this,
            &HVTester::expo_readback_sep_timer_sig_handler, Qt::QueuedConnection);

    connect(this, &HVTester::start_readback_now_sig,
            this, &HVTester::start_readback_now_sig_handler, Qt::QueuedConnection);

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

    hv_expo_readback_sep_timer.stop();
    hv_cool_timer.stop();
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
    (curr) += (step);\
    if((step) >= 0) {if((curr) > (up_e)) (curr) = (up_e);}\
    else if((curr) < (up_e)) (curr) = (up_e);\
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

bool HVTester::mb_rw_reply_received(tester_op_enum_t op, QModbusReply* mb_reply,
                                    void (HVTester::*finished_sig_handler)(),
                                    bool sync, bool err_notify)
{
    QString err_str;
    QTimer * timer = nullptr;
    int timer_ms;
    bool delete_now = false;
    switch(op)
    {
    case TEST_OP_SET_EXPO_TRIPLE:
        err_str = (mb_reply ? QString("%1 %2 %3: %4").arg(gs_str_mb_write_triple,
                                                  hv_curr_triple_mb_unit_str, gs_str_fail,
                                                  mb_reply->errorString())
                            :
                              QString("%1:%2").arg(gs_str_mb_write_null_reply,
                                              hv_curr_triple_mb_unit_str));
        timer = &hv_cool_timer;
        timer_ms = gs_mb_write_triple_error_cool_tims_ms;
        break;

    case TEST_OP_START_EXPO:
        err_str = (mb_reply ? QString("%1 %2: %3").arg(gs_str_mb_start_expo, gs_str_fail,
                                                  mb_reply->errorString())
                            :
                              QString("%1").arg(gs_str_mb_start_expo_null_reply));
        timer = &hv_expo_readback_sep_timer;
        timer_ms = gs_mb_expo_readparm_sep_ms;
        break;

    case TEST_OP_READ_REGS:
    default:
        err_str = (mb_reply ? QString("%1 %2: %3").arg(gs_str_mb_read_regs, gs_str_fail,
                                                  mb_reply->errorString())
                            :
                              QString("%1").arg(gs_str_mb_read_null_reply));
        timer = &hv_cool_timer;
        timer_ms = hv_test_params->expo_param_block.expo_cool_dura_ms;
        break;
    }

    if(!mb_reply)
    {
        emit test_info_message_sig(LOG_ERROR, err_str);
        if(TESTER_IDLE != hv_tester_proc) timer->start(timer_ms);
    }
    else if(!sync || mb_reply->isFinished())
    {
        QModbusDevice::Error err = mb_reply->error();
        if((QModbusDevice::NoError == err) && (sync || !err_notify))
        {
            if(TESTER_IDLE != hv_tester_proc)
            {
                if(TEST_OP_SET_EXPO_TRIPLE == op)
                {
                    emit start_expo_now_sig();
                }
                else if((TEST_OP_READ_REGS == op) && (TESTER_LAST_ONE == hv_tester_proc))
                {
                    //fot the last one, no need to set timer again.
                    emit internal_go_test_sig();
                }
                else
                {
                    timer->start(timer_ms);
                }
            }
        }
        else if((QModbusDevice::NoError != err) && (sync || err_notify))
        {
            DIY_LOG(LOG_ERROR, err_str);
            emit test_info_message_sig(LOG_ERROR, err_str);
            if(TESTER_IDLE != hv_tester_proc)
            {
                if((TEST_OP_READ_REGS == op) && (TESTER_LAST_ONE == hv_tester_proc))
                {
                    //fot the last one, no need to set timer again.
                    emit internal_go_test_sig();
                }
                else
                {
                    timer->start(timer_ms);
                }
            }
        }
        if(sync) delete_now = true;
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
    if(!hv_test_params || hv_modbus_device || !hv_test_params->valid)
    {
        emit test_info_message_sig(LOG_ERROR, gs_str_not_init);
        end_test();
        return;
    }
    update_tester_state();
    switch(hv_tester_proc)
    {
    case TESTER_IDLE:
        emit test_info_message_sig(LOG_ERROR, "Tester is IDLE!!!");
    case TESTER_COMPLETE:
        end_test();
        return;

    case TESTER_A_NEW_ROUND:
        emit test_info_message_sig(LOG_INFO, gs_str_a_new_round);
    case TESTER_LAST_ONE:
    case TESTER_WORKING:
    default:
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
        if(mb_rw_reply_received(hv_curr_op, mb_reply,
                                &HVTester::mb_write_params_finished_sig_handler,
                                true, false))
        {
            delete mb_reply;
        }
    }
        break;
    }
}

void HVTester::start_expo_now_sig_handler()
{
    QModbusReply * mb_reply;
    QModbusDataUnit mb_du(QModbusDataUnit::HoldingRegisters);
    mb_du.setValue(ExposureStart, START_EXPO_DATA);
    mb_reply = hv_modbus_device->sendWriteRequest(mb_du, hv_modbus_srvr_addr);
    hv_curr_op = TEST_OP_START_EXPO;
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
    mb_du.setValueCount(MB_REG_COUNT);
    mb_reply = hv_modbus_device->sendReadRequest(mb_du, hv_modbus_srvr_addr);
    hv_curr_op = TEST_OP_READ_REGS;
    if(mb_rw_reply_received(hv_curr_op, mb_reply,
                            &HVTester::mb_read_finished_sig_handler,
                            true, false))
    {
        delete mb_reply;
    }
}

void HVTester::mb_write_params_finished_sig_handler()
{
    QModbusReply * mb_reply = qobject_cast<QModbusReply *>(sender());
    mb_rw_reply_received(TEST_OP_SET_EXPO_TRIPLE, mb_reply, nullptr, false, false);
    mb_reply->deleteLater();
}

void HVTester::mb_start_expo_finished_sig_handler()
{
    QModbusReply * mb_reply = qobject_cast<QModbusReply *>(sender());
    mb_rw_reply_received(TEST_OP_START_EXPO, mb_reply, nullptr, false, false);
    mb_reply->deleteLater();
}

void HVTester::mb_read_finished_sig_handler()
{
    QModbusReply * mb_reply = qobject_cast<QModbusReply *>(sender());
    mb_rw_reply_received(TEST_OP_READ_REGS, mb_reply, nullptr, false, false);
    mb_reply->deleteLater();
}

void HVTester::mb_rw_error_sig_handler(QModbusDevice::Error /*error*/)
{
    QModbusReply * mb_reply = qobject_cast<QModbusReply *>(sender());
    mb_rw_reply_received(hv_curr_op, mb_reply, nullptr, false, true);
    mb_reply->deleteLater();
}

void HVTester::cool_timer_sig_handler()
{
    emit internal_go_test_sig();
}

void HVTester::expo_readback_sep_timer_sig_handler()
{
    emit start_readback_now_sig();
}

void HVTester::stop_test_sig_handler()
{
    if(hv_expo_readback_sep_timer.isActive()) hv_expo_readback_sep_timer.stop();
    if(hv_cool_timer.isActive()) hv_cool_timer.stop();

    hv_test_params = nullptr;
    hv_modbus_device = nullptr;
    hv_tester_proc = TESTER_IDLE;
    hv_curr_op = TEST_OP_NULL;
    hv_test_idx_in_loop = 0;
    hv_test_idx_in_round = -1;
}

void HVTester::end_test()
{
    emit test_complete_sig();
    stop_test_sig_handler();
}
