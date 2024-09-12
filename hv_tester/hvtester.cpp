#include <QDateTime>

#include "logger/logger.h"
#include "hvtester.h"
#include "sysconfigs/sysconfigs.h"

static const char* gs_str_not_init = "tester未正常初始化";
static const char* gs_str_a_new_round = "新一轮测试开始";
static const char* gs_str_mb_op_null_reply = "modbus操作异常，返回空reply";
static const char* gs_str_mb_write_triple = "modbus设置曝光参数";
static const char* gs_str_mb_start_expo = "modbus发起曝光";
static const char* gs_str_mb_read_regs = "modbus读取常规寄存器";
static const char* gs_str_mb_read_distance= "modbus读取距离寄存器";
static const char* gs_str_mb_read_regs_invalid = "modbus读取常规寄存器数据无效";
static const char* gs_str_mb_read_distance_invalid = "modbus读取距离数据无效";
const char* g_str_fail = "失败";
static const char* gs_str_uninit_or_end = "tester未初始化或已停止";
static const char* gs_str_completed  = "tester已完成";

static const char* gs_str_unknown_tester_op = "未知的tester操作";
static const char* gs_str_unexpected_tester_op = "非预期的tester操作";
static const char* gs_str_wait_some_time_then_retry = "等待后重试";
static const char* gs_str_test_paused = "测试暂停";
static const char* gs_str_test_recovered = "测试恢复";

#undef TEST_OP_ITEM
#define TEST_OP_ITEM(op) #op
static const char* gs_tester_op_name_list[] =
{
    TESTER_OP_LIST
};
#define GET_TESTER_OP_NAME_STR(op) \
    (((TEST_OP_NULL <= (op)) && ((op) <= TEST_OP_READ_DISTANCE)) ?\
        gs_tester_op_name_list[(op)] :  gs_str_unknown_tester_op)

#define ALL_OPERATION_COMPLETE(op, proc) \
    ((hv_test_params->other_param_block.read_dist \
               && (TEST_OP_READ_DISTANCE == (op)) \
               && (TESTER_LAST_ONE == (proc))) \
        || (!hv_test_params->other_param_block.read_dist \
               && (TEST_OP_READ_REGS == (op)) \
               && (TESTER_LAST_ONE == (proc))))

HVTester::HVTester(QObject *parent)
    : QObject{parent},
      hv_test_op_timer(this), hv_test_err_retry_timer(this)
{
    qRegisterMetaType<mb_reg_val_map_t>();
    qRegisterMetaType<tester_op_enum_t>();

    connect(this, &HVTester::tester_next_operation_sig,
            this, &HVTester::tester_send_mb_cmd, Qt::QueuedConnection);

    hv_test_op_timer.setSingleShot(true);
    connect(&hv_test_op_timer, &QTimer::timeout, this, &HVTester::hv_test_op_timer_handler,
                Qt::QueuedConnection);

    hv_test_err_retry_timer.setSingleShot(true);
    connect(&hv_test_err_retry_timer, &QTimer::timeout,
            this, &HVTester::hv_test_err_retry_timer_handler, Qt::QueuedConnection);

    connect(this, &HVTester::internal_go_test_sig, this, &HVTester::go_test_sig_handler,
                Qt::QueuedConnection);
}

HVTester::~HVTester()
{
    reset_internal_state();
}

void HVTester::reset_internal_state()
{
    hv_tester_proc = TESTER_IDLE;
    hv_test_paused = false;
    hv_curr_op = TEST_OP_NULL;
    hv_test_idx_in_loop = 0;
    hv_test_idx_in_round = -1;

    hv_test_op_timer.stop();
    hv_test_err_retry_timer.stop();

    m_regs_read_result.clear();

    hv_test_params = nullptr;
    hv_modbus_device = nullptr;

    m_curr_timer = nullptr;
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

    reset_internal_state();

    hv_test_params = test_params;
    hv_modbus_device = modbus_device;
    hv_modbus_srvr_addr = srvr_addr;

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

void HVTester::mb_rw_reply_received(tester_op_enum_t op, QModbusReply* mb_reply,
                                    void (HVTester::*finished_sig_handler)(),
                                    bool sync, bool err_notify)
{
    QString err_str, mb_reply_err_str;
    int timer_ms;
    bool goon = true;

    mb_reply_err_str = (mb_reply) ?
                            QString::number(mb_reply->error()) + " " + mb_reply->errorString()
                          : (QString(gs_str_mb_op_null_reply) + ".");
    switch(op)
    {
    case TEST_OP_SET_EXPO_TRIPLE:
        err_str = QString("%1 %2 %3: %4")
                .arg(gs_str_mb_write_triple, hv_curr_triple_mb_unit_str, g_str_fail,
                     mb_reply_err_str);
        timer_ms = g_sys_configs_block.consec_rw_wait_ms;
        break;

    case TEST_OP_START_EXPO:
        err_str = QString("%1 %2: %3").arg(gs_str_mb_start_expo, g_str_fail, mb_reply_err_str);
        timer_ms = g_sys_configs_block.expo_prepare_time_ms + hv_curr_expo_param_triple.dura_ms;
        break;

    case TEST_OP_READ_REGS:
        err_str = QString("%1 %2: %3").arg(gs_str_mb_read_regs, g_str_fail, mb_reply_err_str);
        if(hv_test_params->other_param_block.read_dist)
        {
            timer_ms = g_sys_configs_block.consec_rw_wait_ms;
        }
        else
        {
            timer_ms = calc_cool_dura_ms();
        }
        break;

    case TEST_OP_READ_DISTANCE:
        err_str = QString("%1 %2: %3").arg(gs_str_mb_read_distance, g_str_fail, mb_reply_err_str);
        timer_ms = calc_cool_dura_ms();
        break;

    default: //TEST_OP_NULL
        err_str = QString("%1 %2, %3").arg(gs_str_unexpected_tester_op,
                                           QString::number(op),
                                           mb_reply_err_str);
        goon = false;
        break;
    }

    if(!goon)
    {
        emit test_info_message_sig(LOG_ERROR, err_str);
        end_test_due_to_exception(err_str);
        return;
    }

    if(!mb_reply)
    {
        /*mb_reply is null, no further err sig. so wait some time and retry.*/
        err_str += QString(" .") + gs_str_wait_some_time_then_retry;
        emit test_info_message_sig(LOG_ERROR, err_str);

        err_str += QString(". ") + GET_TESTER_OP_NAME_STR(op) + ", mb_reply is NULL!!!";
        DIY_LOG(LOG_ERROR, err_str);

        hv_test_err_retry_timer.start(g_sys_configs_block.mb_err_retry_wait_ms);
        return;
    }

    if(!sync || mb_reply->isFinished())
    {//sync and finished; or, async, including finished and error

        QModbusDevice::Error err = mb_reply->error();
        if(QModbusDevice::NoError == err)
        {
            if(TEST_OP_READ_REGS == op || TEST_OP_READ_DISTANCE == op)
            {
                QModbusDataUnit rb_du = mb_reply->result();
                if(!rb_du.isValid())
                {
                    QString err_str = (TEST_OP_READ_REGS == op) ? gs_str_mb_read_regs_invalid
                                                                : gs_str_mb_read_distance_invalid;
                    err_str += QString(". ") + gs_str_wait_some_time_then_retry;
                    emit test_info_message_sig(LOG_ERROR, err_str);
                    DIY_LOG(LOG_ERROR, err_str);
                    hv_test_err_retry_timer.start(g_sys_configs_block.mb_err_retry_wait_ms);
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
                if(ALL_OPERATION_COMPLETE(op, hv_tester_proc))
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
        else if(sync || err_notify) // if((QModbusDevice::NoError != err) && (sync || err_notify))
        {
            err_str += gs_str_wait_some_time_then_retry;
            emit test_info_message_sig(LOG_ERROR, err_str);
            DIY_LOG(LOG_ERROR, err_str);
            hv_test_err_retry_timer.start(g_sys_configs_block.mb_err_retry_wait_ms);

            /*tester just retry and retry. main_dialog is responsible for reconnect.*/
        }
    }
    else
    {//sync op, and not finished.
        if(finished_sig_handler)
        {
            connect(mb_reply, &QModbusReply::finished,
                        this, finished_sig_handler, Qt::QueuedConnection);
        }

        connect(mb_reply, &QModbusReply::errorOccurred,
                    this, &HVTester::mb_rw_error_sig_handler, Qt::QueuedConnection);
    }
    return;
}

void HVTester::end_test_due_to_exception(QString err_str)
{
    DIY_LOG(LOG_ERROR, err_str);

    emit test_info_message_sig(LOG_ERROR, err_str);
    end_test(TEST_END_EXCEPTION);
    return;
}

void HVTester::go_test_sig_handler()
{
    if(!hv_test_params || !hv_modbus_device || !hv_test_params->valid)
    {
        end_test_due_to_exception(gs_str_not_init);
        return;
    }
    update_tester_state();
    switch(hv_tester_proc)
    {
    case TESTER_IDLE:
        end_test_due_to_exception("Tester is IDLE!!!");
        return;

    case TESTER_COMPLETE:
        end_test(TEST_END_NORMAL);
        return;

    case TESTER_A_NEW_ROUND:
        emit test_info_message_sig(LOG_INFO, gs_str_a_new_round);
    case TESTER_LAST_ONE:
    case TESTER_WORKING:
    default:
        {
            hv_curr_op = TEST_OP_SET_EXPO_TRIPLE;
            tester_send_mb_cmd(hv_curr_op);
        }
        break;
    }
}

void HVTester::construct_mb_du(tester_op_enum_t op, QModbusDataUnit &mb_du)
{
    QVector<quint16> mb_reg_vals;
    mb_reg_val_map_t reg_val_map;

    switch(op)
    {
        case TEST_OP_SET_EXPO_TRIPLE:
            mb_reg_vals.append((quint16)(hv_curr_expo_param_triple.cube_volt_kv));
            mb_reg_vals.append((quint16)(1000* hv_curr_expo_param_triple.cube_current_ma));
            mb_reg_vals.append((quint16)(hv_curr_expo_param_triple.dura_ms));

            reg_val_map.insert(VoltSet, mb_reg_vals.at(0));
            reg_val_map.insert(FilamentSet, mb_reg_vals.at(1));
            reg_val_map.insert(ExposureTime, mb_reg_vals.at(2));
            emit rec_mb_regs_sig(TEST_OP_SET_EXPO_TRIPLE, reg_val_map,
                             hv_test_idx_in_loop, hv_test_idx_in_round);

            mb_du.setStartAddress(VoltSet);
            mb_du.setValues(mb_reg_vals);
            break;

        case TEST_OP_START_EXPO:
            mb_reg_vals.append(START_EXPO_DATA);
            mb_du.setStartAddress(ExposureStart);
            mb_du.setValues(mb_reg_vals);
            break;

        case TEST_OP_READ_REGS:
            mb_du.setStartAddress(HSV);
            mb_du.setValueCount(MAX_HV_NORMAL_MB_REG_NUM);
            break;

        case TEST_OP_READ_DISTANCE:
            mb_du.setStartAddress(EXT_MB_REG_DISTANCE);
            mb_du.setValueCount(1);
            break;

        default:
            DIY_LOG(LOG_ERROR, QString("%1 %2").arg(gs_str_unexpected_tester_op,
                                                    QString::number(op)));
            return;
    }
}

void HVTester::tester_send_mb_cmd(tester_op_enum_t op, tester_op_recovery_type_e_t /*r_t*/)
{
    QModbusDataUnit mb_du(QModbusDataUnit::HoldingRegisters);
    QModbusReply *mb_reply;

    if(hv_test_paused)
    {
        //emit test_info_message_sig(LOG_INFO, gs_str_test_paused, true);
        //DIY_LOG(LOG_INFO, gs_str_test_paused);
        return;
    }
    /*
    else if(RECOVER_FROM_PAUSE == r_t)
    {
        emit test_info_message_sig(LOG_INFO, gs_str_test_recovered, true);
        DIY_LOG(LOG_INFO, gs_str_test_recovered);
    }
    */

    DIY_LOG(LOG_INFO, QString("Tester operation: ") + GET_TESTER_OP_NAME_STR(op));
    construct_mb_du(op, mb_du);
    switch(op)
    {
        case TEST_OP_SET_EXPO_TRIPLE:
        case TEST_OP_START_EXPO:
            mb_reply = hv_modbus_device->sendWriteRequest(mb_du, hv_modbus_srvr_addr);
            break;

        case TEST_OP_READ_REGS:
        case TEST_OP_READ_DISTANCE:
            mb_reply = hv_modbus_device->sendReadRequest(mb_du, hv_modbus_srvr_addr);
            break;

        default:
            QString err_str;
            err_str = QString("%1 %2, %3.").arg(gs_str_unexpected_tester_op,
                                                QString::number(op),
                                                gs_str_uninit_or_end);
            end_test_due_to_exception(err_str);
            return;
    }
    mb_rw_reply_received(op, mb_reply, &HVTester::mb_op_finished_sig_handler, true, false);
}

void HVTester::mb_op_finished_sig_handler()
{
    QModbusReply * mb_reply = qobject_cast<QModbusReply *>(sender());
    DIY_LOG(LOG_INFO,
            QString("mb_op_finished_sig_handler: ") + GET_TESTER_OP_NAME_STR(hv_curr_op));
    mb_rw_reply_received(hv_curr_op, mb_reply, nullptr, false, false);
    if(mb_reply)
    {
        mb_reply->deleteLater();
    }
}

void HVTester::mb_rw_error_sig_handler(QModbusDevice::Error error)
{
    QModbusReply * mb_reply = qobject_cast<QModbusReply *>(sender());
    QString err_str = mb_reply ? mb_reply->errorString() : "";
    DIY_LOG(LOG_INFO, QString("Tester op %1 mb_rw_error_sig_handler: %2 ").
            arg(GET_TESTER_OP_NAME_STR(hv_curr_op)).arg(error) + err_str);
    mb_rw_reply_received(hv_curr_op, mb_reply, nullptr, false, true);
    if(mb_reply)
    {
        mb_reply->deleteLater();
    }
}

void HVTester::hv_test_op_timer_handler()
{
    switch(hv_curr_op)
    {
    case TEST_OP_SET_EXPO_TRIPLE:
        hv_curr_op = TEST_OP_START_EXPO;
        emit tester_next_operation_sig(hv_curr_op);
        break;

    case TEST_OP_START_EXPO:
        hv_curr_op = TEST_OP_READ_REGS;
        emit tester_next_operation_sig(hv_curr_op);
        break;

    case TEST_OP_READ_REGS:
        if(hv_test_params->other_param_block.read_dist)
        {
            hv_curr_op = TEST_OP_READ_DISTANCE;
            emit tester_next_operation_sig(hv_curr_op);
        }
        else
        {
            hv_curr_op = TEST_OP_SET_EXPO_TRIPLE;
            emit internal_go_test_sig();
        }
        break;

    case TEST_OP_READ_DISTANCE:
        hv_curr_op = TEST_OP_SET_EXPO_TRIPLE;
        emit internal_go_test_sig();
        break;

    default:
        {
            QString err_str;
            err_str = QString("%1 %2, %3.").arg(gs_str_unexpected_tester_op,
                                                QString::number(hv_curr_op),
                                                gs_str_uninit_or_end);
            end_test_due_to_exception(err_str);
        }
        break;
    }
}

void HVTester::stop_test_sig_handler(tester_end_code_enum_t /*code*/)
{
    reset_internal_state();
}

void HVTester::end_test(tester_end_code_enum_t code)
{
    emit test_complete_sig(code);
    stop_test_sig_handler(code);
}

void HVTester::mb_reconnected_sig_handler()
{
    tester_send_mb_cmd(hv_curr_op, RECOVER_FROM_DISCONN);
}

void HVTester::hv_test_err_retry_timer_handler()
{
    tester_send_mb_cmd(hv_curr_op, RECOVER_FROM_ERROR);
}

void HVTester::pause_restore_test_sig_handler(bool pause)
{
    static QDateTime check_point;
    QString log_str;

    if((TESTER_IDLE == hv_tester_proc) || (TESTER_COMPLETE == hv_tester_proc))
    {
        log_str = ((TESTER_IDLE == hv_tester_proc) ? gs_str_uninit_or_end : gs_str_completed);
        emit test_info_message_sig(LOG_INFO, log_str);
        DIY_LOG(LOG_INFO, log_str);
        return;
    }

    if(pause == hv_test_paused)
    {
        log_str  = QString("tester receives pause-recover instruction,, but tester "
                           "and instructor pause states are both %1. so tester ignore it.")
                            .arg(pause ? "true" : "false");
        DIY_LOG(LOG_WARN, log_str);
        return;
    }


    hv_test_paused = pause;
    if(pause)
    {
        check_point = QDateTime::currentDateTime();

        m_curr_timer = nullptr; m_curr_timer_remaining_time = -1;
        if(hv_test_op_timer.isActive())
        {
            m_curr_timer = &hv_test_op_timer;
            m_curr_timer_handler = &HVTester::hv_test_op_timer_handler;
        }
        else if(hv_test_err_retry_timer.isActive())
        {
            m_curr_timer = &hv_test_err_retry_timer;
            m_curr_timer_handler = &HVTester::hv_test_err_retry_timer_handler;
        }

        if(m_curr_timer)
        {
            m_curr_timer_remaining_time = m_curr_timer->remainingTime();
        }

        hv_test_op_timer.stop();
        hv_test_err_retry_timer.stop();

        emit test_info_message_sig(LOG_INFO, gs_str_test_paused, true);

        log_str = "tester receives pause instruction.";
    }
    else
    {
        hv_test_op_timer.stop();
        hv_test_err_retry_timer.stop();

        emit test_info_message_sig(LOG_INFO, gs_str_test_recovered, true);
        if(m_curr_timer)
        {
            QDateTime curr_datetime = QDateTime::currentDateTime();

            if(check_point.addMSecs(m_curr_timer_remaining_time) >= curr_datetime)
            {
                m_curr_timer->start(m_curr_timer_remaining_time);
            }
            else
            {
                (this->*m_curr_timer_handler)();
            }

            m_curr_timer = nullptr; m_curr_timer_remaining_time = -1;
        }
        else
        {
            if(ALL_OPERATION_COMPLETE(hv_curr_op, hv_tester_proc))
            {
                emit internal_go_test_sig();
            }
            else
            {
                tester_send_mb_cmd(hv_curr_op, RECOVER_FROM_PAUSE);
            }
        }

        log_str = "tester receives recover instruction.";

    }
    DIY_LOG(LOG_INFO, log_str);
}
