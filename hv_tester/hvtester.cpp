#include <QDateTime>
#include <QtMath>

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
static const char* gs_str_test_resumed = "测试恢复";

#undef TEST_OP_ITEM
#define TEST_OP_ITEM(op) #op
static const char* gs_tester_op_name_list[] =
{
    TESTER_OP_LIST
};

#define GET_TESTER_OP_NAME_STR(op) \
    (((TEST_OP_NULL <= (op)) && ((op) <= TEST_OP_READ_DISTANCE)) ?\
        gs_tester_op_name_list[(op)] :  gs_str_unknown_tester_op)

#define LAST_OP_IN_WHOLE_TEST(op, proc) \
    ((hv_test_params->other_param_block.read_dist \
               && (TEST_OP_READ_DISTANCE == (op)) \
               && (TESTER_LAST_ONE == (proc))) \
        || (!hv_test_params->other_param_block.read_dist \
               && (TEST_OP_READ_REGS == (op)) \
               && (TESTER_LAST_ONE == (proc))))

#define THIS_IS_A_NEW_ROUND(proc, round_idx) \
    ((TESTER_A_NEW_ROUND == (proc)) || ((TESTER_LAST_ONE == (proc)) && (0 == (round_idx))))

#define EXPO_PREP_AND_WORK_DURA(expo_dura) \
    (g_sys_configs_block.expo_prepare_time_ms + (expo_dura))

#define GAP_BETWEEN_EXPO_FINISH_AND_READ_REG_START(expo_dura) \
    (qMax<float>(EXPO_PREP_AND_WORK_DURA(expo_dura) ,g_sys_configs_block.consec_rw_wait_ms))

#define FULL_COOL_DURA_MS(dura_ms) \
    ((hv_test_params->expo_param_block.fixed_cool_dura ? \
                   hv_test_params->expo_param_block.expo_cool_dura_ms : \
                   hv_test_params->expo_param_block.expo_cool_dura_factor * (dura_ms)) \
    + g_sys_configs_block.extra_cool_time_ms)

#define LAST_ROUND_DURA_MS \
    (hv_test_params->expo_param_block.cust ? \
     hv_test_params->expo_param_block.expo_params.cust_params_arr.last().dura_ms :\
     hv_test_params->expo_param_block.expo_params.regular_parms.expo_dura_ms_end)

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

void HVTester::check_pause_to_start_timer(QTimer * c_timer,
                                      tester_simple_handler_t timeout_handler, int dura_ms)
{
    if(!c_timer) return;

    if(!hv_test_paused)
    {
        c_timer->start(dura_ms);
    }
    else
    {
        m_curr_timer = c_timer;
        m_curr_timer_handler = timeout_handler;
        m_curr_timer_remaining_time_ms = dura_ms;
        m_dt_checkpoint_for_pause = QDateTime::currentDateTime();
    }
}

void HVTester::reset_timer_rec()
{
    m_curr_timer = nullptr;
    m_curr_timer_handler = nullptr;
    m_curr_timer_remaining_time_ms = -1;
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

    reset_timer_rec();
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

void HVTester::init_for_time_stat(test_params_struct_t *test_params)
{
    reset_internal_state();
    hv_test_params = test_params;
}

bool HVTester::is_the_last_one_test(test_params_struct_t * ctrl_struct,
                                                expo_param_triple_struct_t &curr_triple,
                                                int &idx_in_loop, int &idx_in_round)
{
    bool ret;

    if(!ctrl_struct || !ctrl_struct->valid)
    {
        DIY_LOG(LOG_WARN, "tester not init!!!");
        return false;
    }

    if(ctrl_struct->expo_param_block.cust)
    {
        int arr_cnt =  ctrl_struct->expo_param_block.expo_params.cust_params_arr.count();
        ret = ((idx_in_round + 1) >= arr_cnt)
              &&  ((idx_in_loop + 1) >= ctrl_struct->expo_param_block.expo_cnt);
    }
    else
    {
        ret = ((idx_in_loop + 1 >= ctrl_struct->expo_param_block.expo_cnt)
        && (curr_triple.cube_volt_kv ==
              ctrl_struct->expo_param_block.expo_params.regular_parms.cube_volt_kv_end)
        && (curr_triple.cube_current_ma ==
              ctrl_struct->expo_param_block.expo_params.regular_parms.cube_current_ma_end)
        && (curr_triple.dura_ms ==
              ctrl_struct->expo_param_block.expo_params.regular_parms.expo_dura_ms_end));
    }
    return ret;
}

HVTester::tester_procedure_enum_t HVTester::update_tester_state(test_params_struct_t * ctrl_struct,
                                                expo_param_triple_struct_t &curr_triple,
                                                int &idx_in_loop, int &idx_in_round)
{
    HVTester::tester_procedure_enum_t proc;

    if(!ctrl_struct || !ctrl_struct->valid)
    {
        DIY_LOG(LOG_WARN, "tester not init!!!");
        proc = TESTER_IDLE;
        return proc;
    }

    ++idx_in_round;
    if(ctrl_struct->expo_param_block.cust)
    {
        int arr_cnt = ctrl_struct->expo_param_block.expo_params.cust_params_arr.count();
        if(idx_in_round >= arr_cnt)
        {
            ++idx_in_loop;
            if(idx_in_loop >= ctrl_struct->expo_param_block.expo_cnt)
            {
                proc = TESTER_COMPLETE;
                return proc;
            }

            idx_in_round = 0;
        }
        curr_triple = ctrl_struct->expo_param_block.expo_params.cust_params_arr.at(idx_in_round);
        if(is_the_last_one_test(ctrl_struct, curr_triple, idx_in_loop, idx_in_round))
        {
            proc = TESTER_LAST_ONE;
        }
        else
        {
            proc = ((0 == idx_in_round) ? TESTER_A_NEW_ROUND : TESTER_WORKING);
        }
    }
    else
    {
        if(0 == idx_in_round)
        {
            curr_triple.cube_volt_kv =
                  ctrl_struct->expo_param_block.expo_params.regular_parms.cube_volt_kv_start;
            curr_triple.cube_current_ma =
                  ctrl_struct->expo_param_block.expo_params.regular_parms.cube_current_ma_start;
            curr_triple.dura_ms =
                  ctrl_struct->expo_param_block.expo_params.regular_parms.expo_dura_ms_start;

            if(is_the_last_one_test(ctrl_struct, curr_triple, idx_in_loop, idx_in_round))
            {
                proc = TESTER_LAST_ONE;
            }
            else
            {
                proc = TESTER_A_NEW_ROUND;
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
        if((step) >= 0) {if((curr) > (up_e)) (curr) = (up_e);}\
        else if((curr) < (up_e)) (curr) = (up_e);\
    }\
    if((curr) == (low_e)) roundup = true;\
}
            MOVE_A_STEP(
                ctrl_struct->expo_param_block.expo_params.regular_parms.expo_dura_ms_start,
                ctrl_struct->expo_param_block.expo_params.regular_parms.expo_dura_ms_end,
                ctrl_struct->expo_param_block.expo_params.regular_parms.expo_dura_ms_step,
                curr_triple.dura_ms);
            if(!roundup)
            {
                proc = is_the_last_one_test(ctrl_struct, curr_triple, idx_in_loop, idx_in_round) ?
                                            TESTER_LAST_ONE : TESTER_WORKING;
                break;
            }

            roundup = false;
            MOVE_A_STEP(
                ctrl_struct->expo_param_block.expo_params.regular_parms.cube_current_ma_start,
                ctrl_struct->expo_param_block.expo_params.regular_parms.cube_current_ma_end,
                ctrl_struct->expo_param_block.expo_params.regular_parms.cube_current_ma_step,
                curr_triple.cube_current_ma);
            if(!roundup)
            {
                proc = is_the_last_one_test(ctrl_struct, curr_triple, idx_in_loop, idx_in_round) ?
                                            TESTER_LAST_ONE : TESTER_WORKING;
                break;
            }

            roundup = false;
            MOVE_A_STEP(
                ctrl_struct->expo_param_block.expo_params.regular_parms.cube_volt_kv_start,
                ctrl_struct->expo_param_block.expo_params.regular_parms.cube_volt_kv_end,
                ctrl_struct->expo_param_block.expo_params.regular_parms.cube_volt_kv_step,
                curr_triple.cube_volt_kv);
            if(!roundup)
            {
                proc = is_the_last_one_test(ctrl_struct, curr_triple, idx_in_loop, idx_in_round) ?
                                            TESTER_LAST_ONE : TESTER_WORKING;
                break;
            }
            ++idx_in_loop;
            if(idx_in_loop >= ctrl_struct->expo_param_block.expo_cnt)
            {
                proc = TESTER_COMPLETE;
                return proc;
            }
            idx_in_round = 0;
            proc = is_the_last_one_test(ctrl_struct, curr_triple, idx_in_loop, idx_in_round) ?
                                            TESTER_LAST_ONE : TESTER_A_NEW_ROUND;

            break;
#undef MOVE_A_STEP
        }
    }

    return proc;
}

int HVTester::calc_cool_dura_ms(qint64 time_elapsed_since_cool_start)
{
    float cool_dura_ms;

    cool_dura_ms = FULL_COOL_DURA_MS(hv_curr_expo_param_triple.dura_ms);

    if(time_elapsed_since_cool_start >= cool_dura_ms)
    {
        return 0;
    }
    else
    {
        return qCeil(cool_dura_ms - time_elapsed_since_cool_start);
    }
}

void HVTester::mb_rw_reply_received(tester_op_enum_t op, QModbusReply* mb_reply,
                                    void (HVTester::*finished_sig_handler)(),
                                    bool sync, bool err_notify)
{
    static QDateTime ls_cool_start_point = QDateTime::currentDateTime();
    QDateTime curr_dt = QDateTime::currentDateTime();
    qint64 time_elapsed_since_cool_start;
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
        timer_ms = GAP_BETWEEN_EXPO_FINISH_AND_READ_REG_START(hv_curr_expo_param_triple.dura_ms);

        ls_cool_start_point = QDateTime::currentDateTime()
                        .addMSecs(EXPO_PREP_AND_WORK_DURA(hv_curr_expo_param_triple.dura_ms));
        break;

    case TEST_OP_READ_REGS:
        time_elapsed_since_cool_start = ls_cool_start_point.msecsTo(curr_dt);

        err_str = QString("%1 %2: %3").arg(gs_str_mb_read_regs, g_str_fail, mb_reply_err_str);
        if(hv_test_params->other_param_block.read_dist)
        {
            timer_ms = g_sys_configs_block.consec_rw_wait_ms;
        }
        else
        {
            timer_ms = qMax<float>(calc_cool_dura_ms(time_elapsed_since_cool_start),
                                    g_sys_configs_block.consec_rw_wait_ms);
        }
        break;

    case TEST_OP_READ_DISTANCE:
        time_elapsed_since_cool_start = ls_cool_start_point.msecsTo(curr_dt);

        err_str = QString("%1 %2: %3").arg(gs_str_mb_read_distance, g_str_fail, mb_reply_err_str);
        timer_ms = qMax<float>(calc_cool_dura_ms(time_elapsed_since_cool_start),
                                g_sys_configs_block.consec_rw_wait_ms);
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
        end_test_due_to_exception(err_str);
        return;
    }

    if(!mb_reply)
    {
        /*mb_reply is null, no further err sig. so wait some time and retry.*/
        err_str += QString(" .") + gs_str_wait_some_time_then_retry;
        post_test_info_message(LOG_ERROR, err_str);

        err_str += QString(". ") + GET_TESTER_OP_NAME_STR(op) + ", mb_reply is NULL!!!";
        DIY_LOG(LOG_ERROR, err_str);

        check_pause_to_start_timer(&hv_test_err_retry_timer,
                                   &HVTester::hv_test_err_retry_timer_handler,
                                   g_sys_configs_block.mb_err_retry_wait_ms);
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
                    post_test_info_message(LOG_ERROR, err_str);
                    DIY_LOG(LOG_ERROR, err_str);

                    check_pause_to_start_timer(&hv_test_err_retry_timer,
                                               &HVTester::hv_test_err_retry_timer_handler,
                                               g_sys_configs_block.mb_err_retry_wait_ms);
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
                if(LAST_OP_IN_WHOLE_TEST(op, hv_tester_proc))
                {
                    //for the last one, no need to set timer again, and let go_test to end
                    //the loop.
                    emit internal_go_test_sig();
                }
                else
                {
                    check_pause_to_start_timer(&hv_test_op_timer,
                                               &HVTester::hv_test_op_timer_handler,
                                               timer_ms);
                }
            }
        }
        else if(sync || err_notify) // if((QModbusDevice::NoError != err) && (sync || err_notify))
        {
            err_str += gs_str_wait_some_time_then_retry;
            post_test_info_message(LOG_ERROR, err_str);
            DIY_LOG(LOG_ERROR, err_str);
            check_pause_to_start_timer(&hv_test_err_retry_timer,
                                       &HVTester::hv_test_err_retry_timer_handler,
                                       g_sys_configs_block.mb_err_retry_wait_ms);
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

    post_test_info_message(LOG_ERROR, err_str);
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

    if(m_last_jdg_ret
            || m_last_test_retry_cnt >= g_sys_configs_block.test_no_pass_retry_cnt)
    {
        reset_test_retry_cnt();

        hv_tester_proc = update_tester_state(hv_test_params, hv_curr_expo_param_triple,
                            hv_test_idx_in_loop, hv_test_idx_in_round);
    }

    if(hv_tester_proc != TESTER_IDLE && hv_tester_proc != TESTER_COMPLETE)
    {
        hv_curr_triple_mb_unit_str = QString("%1,%2,%3").arg(
                       QString::number((quint16)(hv_curr_expo_param_triple.cube_volt_kv)),
                       QString::number((quint16)(1000 * hv_curr_expo_param_triple.cube_current_ma)),
                       QString::number((quint16)(hv_curr_expo_param_triple.dura_ms)));
    }
    switch(hv_tester_proc)
    {
    case TESTER_IDLE:
        end_test_due_to_exception("Tester is IDLE!!!");
        return;

    case TESTER_COMPLETE:
        end_test(TEST_END_NORMAL);
        return;

    case TESTER_A_NEW_ROUND:
    case TESTER_LAST_ONE:
        if(THIS_IS_A_NEW_ROUND(hv_tester_proc, hv_test_idx_in_round))
        {
            //post_test_info_message(LOG_INFO, gs_str_a_new_round);
        }
    case TESTER_WORKING:
    default:
        {
            hv_curr_op = TEST_OP_SET_EXPO_TRIPLE;
            tester_send_mb_cmd(hv_curr_op);
        }
        break;
    }
}

quint16 HVTester::get_start_expo_cmd_from_test_method()
{
    if(hv_test_params && hv_test_params->valid)
    {
        switch(hv_test_params->test_content)
        {
        case TEST_CONTENT_COOL_HV:
            return START_EXPO_DATA_COOL_HV;
        case TEST_CONTENT_ONLY_COIL:
            return START_EXPO_DATA_ONLY_COIL;
        case TEST_CONTENT_DECOUPLE:
            return START_EXPO_DATA_DECOUPLE;
        default: //TEST_CONTENT_NORMAL:
            return START_EXPO_DATA;
        }
    }
    else
    {
        return START_EXPO_DATA;
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
            mb_reg_vals.append((quint16)(hv_test_params->expo_param_block.sw_to_mb_current_factor
                                         * hv_curr_expo_param_triple.cube_current_ma));
            mb_reg_vals.append((quint16)(hv_test_params->expo_param_block.sw_to_mb_dura_factor
                                         * hv_curr_expo_param_triple.dura_ms));

            reg_val_map.insert(VoltSet, mb_reg_vals.at(0));
            reg_val_map.insert(FilamentSet, mb_reg_vals.at(1));
            reg_val_map.insert(ExposureTime, mb_reg_vals.at(2));
            emit rec_mb_regs_sig(TEST_OP_SET_EXPO_TRIPLE, reg_val_map,
                             hv_test_idx_in_loop, hv_test_idx_in_round);

            mb_du.setStartAddress(VoltSet);
            mb_du.setValues(mb_reg_vals);
            break;

        case TEST_OP_START_EXPO:
            mb_reg_vals.append(get_start_expo_cmd_from_test_method());
            mb_du.setStartAddress(ExposureStart);
            mb_du.setValues(mb_reg_vals);
            emit begin_exposure_sig(true);
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
            emit begin_exposure_sig(false);
            return;
    }
}

void HVTester::tester_send_mb_cmd(tester_op_enum_t op, tester_op_resume_type_e_t /*r_t*/)
{
    QModbusDataUnit mb_du(QModbusDataUnit::HoldingRegisters);
    QModbusReply *mb_reply;

    if(hv_test_paused)
    {
        DIY_LOG(LOG_INFO, QString(gs_str_test_paused));
        return;
    }

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

void HVTester::stop_test_sig_handler(tester_end_code_enum_t code)
{
    reset_internal_state();
    emit test_complete_sig(code);
}

void HVTester::end_test(tester_end_code_enum_t code)
{
    stop_test_sig_handler(code);
}

void HVTester::mb_reconnected_sig_handler()
{
    tester_send_mb_cmd(hv_curr_op, RESUME_FROM_DISCONN);
}

void HVTester::hv_test_err_retry_timer_handler()
{
    emit tester_next_operation_sig(hv_curr_op, RESUME_FROM_ERROR);
}

void HVTester::pause_resume_test_sig_handler(bool pause)
{
    QString log_str;

    if((TESTER_IDLE == hv_tester_proc) || (TESTER_COMPLETE == hv_tester_proc))
    {
        log_str = ((TESTER_IDLE == hv_tester_proc) ? gs_str_uninit_or_end : gs_str_completed);
        post_test_info_message(LOG_INFO, log_str);
        DIY_LOG(LOG_INFO, log_str);
        return;
    }

    if(pause == hv_test_paused)
    {
        log_str  = QString("tester receives pause-resume instruction,, but tester "
                           "and instructor pause states are both %1. so tester ignore it.")
                            .arg(pause ? "true" : "false");
        DIY_LOG(LOG_WARN, log_str);
        return;
    }


    hv_test_paused = pause;
    if(pause)
    {
        m_dt_checkpoint_for_pause = QDateTime::currentDateTime();

        reset_timer_rec();
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
            m_curr_timer_remaining_time_ms = m_curr_timer->remainingTime();
        }

        hv_test_op_timer.stop();
        hv_test_err_retry_timer.stop();

        post_test_info_message(LOG_INFO, gs_str_test_paused, true);

        log_str = "tester receives pause instruction.";
    }
    else
    {
        post_test_info_message(LOG_INFO, gs_str_test_resumed, true);
        if(m_curr_timer)
        {
            QDateTime curr_datetime = QDateTime::currentDateTime();
            QDateTime expire_datetime
                    = m_dt_checkpoint_for_pause.addMSecs(m_curr_timer_remaining_time_ms);

            if(expire_datetime > curr_datetime)
            {
                qint64 rem_dura = curr_datetime.msecsTo(expire_datetime);
                m_curr_timer->start(rem_dura);
            }
            else
            {
                (this->*m_curr_timer_handler)();
            }

            reset_timer_rec();
        }
        else
        {
            tester_send_mb_cmd(hv_curr_op, RESUME_FROM_PAUSE);
        }

        log_str = "tester receives resume instruction.";

    }
    DIY_LOG(LOG_INFO, log_str);
}

#define EXPECT_DURA_AFTER_LAST_CMD_IN_ROUND_MS(expo_dura) \
qMax<float>(g_sys_configs_block.consec_rw_wait_ms, \
    FULL_COOL_DURA_MS(expo_dura) \
- (hv_test_params->other_param_block.read_dist ? 2 : 1) * g_sys_configs_block.mb_one_cmd_round_time_ms \
- (hv_test_params->other_param_block.read_dist ? 1 : 0) * g_sys_configs_block.consec_rw_wait_ms \
- qMax<float>(g_sys_configs_block.consec_rw_wait_ms - EXPO_PREP_AND_WORK_DURA(expo_dura), 0))

float HVTester::expect_remaining_test_dura_ms(bool total)
{
    int one_cmd_round_time_ms = g_sys_configs_block.mb_one_cmd_round_time_ms;
    float remain_dura_ms = 0;

    if(!hv_test_params)
    {
        DIY_LOG(LOG_WARN, "tester not init!!!");
        return 0;
    }

    float fixed_dura_in_one_round_ms;
    if(hv_test_params->other_param_block.read_dist)
    {
        /*
         * set-triple-param + rw_waiit
         * + expo + GAP_BETWEEN_EXPO_FINISH_AND_READ_REG_START
         * + read_regs + rw_wait
         *  + read_dist + EXPECT_DURA_AFTER_LAST_CMD_IN_ROUND_MS
        */
        fixed_dura_in_one_round_ms = 4 * one_cmd_round_time_ms
                + 2 * g_sys_configs_block.consec_rw_wait_ms;
    }
    else
    {
        /*
         * set-triple-param + rw_waiit
         * + expo + GAP_BETWEEN_EXPO_FINISH_AND_READ_REG_START
         * + read_regs + EXPECT_DURA_AFTER_LAST_CMD_IN_ROUND_MS
        */
        fixed_dura_in_one_round_ms = 3 * one_cmd_round_time_ms
                + g_sys_configs_block.consec_rw_wait_ms;
    }

    int start_round_idx = 0, start_loop_idx = 0;
    if(total)
    {
        start_loop_idx = 0;
        start_round_idx = -1;
    }
    else
    {
        start_loop_idx = hv_test_idx_in_loop;
        start_round_idx = hv_test_idx_in_round;
    }

    test_params_struct_t ctrl_struct = (*hv_test_params);
    expo_param_triple_struct_t curr_triple;
    tester_procedure_enum_t proc;

    int loop_cnt = hv_test_params->expo_param_block.expo_cnt;
    float one_round_dura = 0;
    float upper_part_loop_dura = 0, lower_part_loop_dura = 0, one_loop_dura = 0;
    float curr_expo_dura = 0, start_idx_round_dura = 0;

    int loop_idx = 0, round_idx = -1;
    proc = update_tester_state(&ctrl_struct, curr_triple, loop_idx, round_idx);
    while(proc != TESTER_COMPLETE && proc != TESTER_IDLE)
    {
        curr_expo_dura = curr_triple.dura_ms;
        one_round_dura = GAP_BETWEEN_EXPO_FINISH_AND_READ_REG_START(curr_expo_dura)
                            + EXPECT_DURA_AFTER_LAST_CMD_IN_ROUND_MS(curr_expo_dura)
                            + fixed_dura_in_one_round_ms;

        if(round_idx <= start_round_idx)
        {
            upper_part_loop_dura += one_round_dura;
        }
        else
        {
            lower_part_loop_dura += one_round_dura;
        }

        if(round_idx == start_round_idx)
        {
            start_idx_round_dura = curr_expo_dura;
        }

        DIY_LOG(LOG_INFO, QString("round idx: %1, one_round_dura: %2 ms, "
                                  "upper_part_loop_dura: %3 ms, lower_part_dura: %4 ms.")
        .arg(round_idx).arg(one_round_dura).arg(upper_part_loop_dura).arg(lower_part_loop_dura));

        proc = update_tester_state(&ctrl_struct, curr_triple, loop_idx, round_idx);
        if(TESTER_COMPLETE == proc || THIS_IS_A_NEW_ROUND(proc, round_idx)) break;
    }
    one_loop_dura = upper_part_loop_dura+ lower_part_loop_dura;
    DIY_LOG(LOG_INFO,
            QString("lower_part_loop_dura: %1 ms, upper_part_loop_dura: %2 ms, "
                    "one_loop_dura: %3 ms.")
                    .arg(lower_part_loop_dura).arg(upper_part_loop_dura).arg(one_loop_dura));
    remain_dura_ms = lower_part_loop_dura + (loop_cnt - start_loop_idx - 1) * one_loop_dura;
    DIY_LOG(LOG_INFO, QString("complete loops dura: %1 ms.").arg(remain_dura_ms));

    /*now count the partial round dura. this part is not accurate, because we do not
      know if current operation (mb cmd) is completed or not. so we just consider
      the op has not start, and the counted time may be LONGer than actual.
    */
    float part_round_dura = 0;
    curr_expo_dura = start_idx_round_dura;
    if(curr_expo_dura > 0)
    {
        switch(hv_curr_op)
        {
            case TEST_OP_NULL:
            case TEST_OP_SET_EXPO_TRIPLE:
                part_round_dura += one_cmd_round_time_ms
                                    + g_sys_configs_block.consec_rw_wait_ms;

            case TEST_OP_START_EXPO:
                part_round_dura += one_cmd_round_time_ms
                                    + GAP_BETWEEN_EXPO_FINISH_AND_READ_REG_START(curr_expo_dura);

            case TEST_OP_READ_REGS:
                if(hv_test_params->other_param_block.read_dist)
                {
                    part_round_dura += one_cmd_round_time_ms
                                        + g_sys_configs_block.consec_rw_wait_ms;
                }
                else
                {
                    part_round_dura += one_cmd_round_time_ms
                                + EXPECT_DURA_AFTER_LAST_CMD_IN_ROUND_MS(curr_expo_dura);
                    break;
                }

            case TEST_OP_READ_DISTANCE:
                part_round_dura += one_cmd_round_time_ms
                                + EXPECT_DURA_AFTER_LAST_CMD_IN_ROUND_MS(curr_expo_dura);
                break;
        }
    }
    DIY_LOG(LOG_INFO, QString("curr op is %1, part_round_dura is %2 ms.")
            .arg(GET_TESTER_OP_NAME_STR(hv_curr_op)).arg(part_round_dura));
    remain_dura_ms += part_round_dura;

    /*for the last round in the whole test period, no cool waiting is needed. so substract it.*/
    float last_round_dura = LAST_ROUND_DURA_MS;
    remain_dura_ms -= EXPECT_DURA_AFTER_LAST_CMD_IN_ROUND_MS(last_round_dura);
    DIY_LOG(LOG_INFO, QString("last_round_dura is %1 ms.").arg(last_round_dura));
    DIY_LOG(LOG_INFO, QString("expect dura after last cmd is %1 ms.").arg(EXPECT_DURA_AFTER_LAST_CMD_IN_ROUND_MS(last_round_dura)));

    DIY_LOG(LOG_INFO, QString("remain_dura_ms: %1 ms.").arg(remain_dura_ms));

    if(remain_dura_ms < 0) remain_dura_ms = 0;

    return remain_dura_ms;
}

void HVTester::post_test_info_message(LOG_LEVEL lvl, QString msg, bool always_rec,
                               QColor set_color, int set_font_w1)
{
    /*proc being idle means tester is already ended, so the internal is not
      necessary to posted for display.
    */
    if(TESTER_IDLE != hv_tester_proc)
    {
        emit test_info_message_sig(lvl, msg, always_rec, set_color, set_font_w1);
    }
    else
    {
        DIY_LOG(LOG_WARN, QString("tester is idle, so the msg is not posted: %1").arg(msg));
    }
}

void HVTester::set_last_judge_result(bool jdg_ret)
{
    m_last_jdg_ret = jdg_ret;
}

void HVTester::reset_test_retry_cnt()
{
    m_last_test_retry_cnt = 0;
}

void HVTester::increase_test_retry_cnt()
{
    ++m_last_test_retry_cnt;
}

bool HVTester::is_last_test_retry()
{
    return (m_last_test_retry_cnt >= g_sys_configs_block.test_no_pass_retry_cnt);
}

bool HVTester::is_retrying()
{
    return (m_last_test_retry_cnt > 0);
}
