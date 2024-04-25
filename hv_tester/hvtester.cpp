#include "logger/logger.h"
#include "hvtester.h"

static const char* gs_str_not_init = "tester未正常初始化";
static const char* gs_str_a_new_round = "新一轮测试开始";
static const char* gs_str_mb_write_null_reply = "modbus写入异常，返回空reply";
static const char* gs_str_mb_read_null_reply = "modbus读取异常，返回空reply";
static const char* gs_str_mb_write_triple = "modbus设置曝光参数";
static const char* gs_str_fail = "失败";

static const int gs_mb_write_error_cool_tims_ms = 1000;

HVTester::HVTester(QObject *parent)
    : QObject{parent}, hv_cool_timer(this)
{
    hv_cool_timer.setSingleShot(true);

    connect(this, &HVTester::expo_params_been_set, this, &HVTester::expo_params_been_set_handler,
                Qt::QueuedConnection);
    connect(&hv_cool_timer, &QTimer::timeout, this, &HVTester::cool_timer_handler,
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
    hv_test_idx_in_loop = -1;
    hv_test_idx_in_round = -1;

    hv_cool_timer.stop();
    return true;
}

void HVTester::update_tester_state()
{
    if(!hv_test_params || !hv_test_params->valid)
    {
        hv_tester_proc = TESTER_IDLE;
        return;
    }

    if(hv_test_params->expo_param_block.cust)
    {
        int arr_cnt =  hv_test_params->expo_param_block.expo_params.cust_params_arr.count();
        ++hv_test_idx_in_round;
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
        hv_tester_proc = ((0 == hv_test_idx_in_round) ? TESTER_A_NEW_ROUND : TESTER_WORKING);
    }
    else
    {
        if(hv_test_idx_in_round < 0)
        {
            hv_curr_expo_param_triple.cube_volt_kv =
                  hv_test_params->expo_param_block.expo_params.regular_parms.cube_volt_kv_start;
            hv_curr_expo_param_triple.cube_current_ma =
                  hv_test_params->expo_param_block.expo_params.regular_parms.cube_current_ma_start;
            hv_curr_expo_param_triple.dura_ms =
                  hv_test_params->expo_param_block.expo_params.regular_parms.expo_dura_ms_start;
            hv_test_idx_in_round = 0;
            hv_tester_proc = TESTER_A_NEW_ROUND;
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
                ++hv_test_idx_in_round;
                hv_tester_proc = TESTER_WORKING;
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
                ++hv_test_idx_in_round;
                hv_tester_proc = TESTER_WORKING;
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
                ++hv_test_idx_in_round;
                hv_tester_proc = TESTER_WORKING;
                break;
            }
            ++hv_test_idx_in_loop;
            if(hv_test_idx_in_loop >= hv_test_params->expo_param_block.expo_cnt)
            {
                hv_tester_proc = TESTER_COMPLETE;
                return;
            }
            hv_test_idx_in_round = 0;
            hv_tester_proc = TESTER_A_NEW_ROUND;

            break;
        }
    }
    hv_curr_triple_mb_unit_str = QString("%1,%2,%3").arg(
                   QString::number((quint16)(hv_curr_expo_param_triple.cube_volt_kv)),
                   QString::number((quint16)(1000 * hv_curr_expo_param_triple.cube_current_ma)),
                   QString::number((quint16)(hv_curr_expo_param_triple.dura_ms)));
}

void HVTester::go_test_handler()
{
    if(!hv_test_params || hv_modbus_device || !hv_test_params->valid)
    {
        emit test_info_message(LOG_ERROR, gs_str_not_init);
        end_test();
        return;
    }
    update_tester_state();
    switch(hv_tester_proc)
    {
    case TESTER_IDLE:
        emit test_info_message(LOG_ERROR, "Unkonwn error: tester is IDLE.");
    case TESTER_COMPLETE:
        end_test();
        return;

    case TESTER_A_NEW_ROUND:
        emit test_info_message(LOG_INFO, gs_str_a_new_round);
    case TESTER_WORKING:
    default:
    {
        QModbusDataUnit mb_du(QModbusDataUnit::HoldingRegisters);
        QModbusReply *mb_reply;
        QVector<quint16> mb_reg_vals;
        mb_reg_vals.append((quint16)(hv_curr_expo_param_triple.cube_volt_kv));
        mb_reg_vals.append((quint16)(1000* hv_curr_expo_param_triple.cube_current_ma));
        mb_reg_vals.append((quint16)(hv_curr_expo_param_triple.dura_ms));

        mb_reg_val_map_t reg_val;
        reg_val.insert(VoltSet, mb_reg_vals.at(0));
        reg_val.insert(FilamentSet, mb_reg_vals.at(1));
        reg_val.insert(ExposureTime, mb_reg_vals.at(2));
        emit rec_mb_regs(TEST_OP_SET_EXPO_TRIPLE, reg_val,
                         hv_test_idx_in_loop, hv_test_idx_in_round);

        /*now write to modbus server*/
        mb_du.setStartAddress(VoltSet);
        mb_du.setValues(mb_reg_vals);
        mb_reply = hv_modbus_device->sendWriteRequest(mb_du, hv_modbus_srvr_addr);
        if(!mb_reply)
        {
            QString err_str;
            err_str =  QString("%1:%2").arg(gs_str_mb_write_null_reply,
                                              hv_curr_triple_mb_unit_str);
            DIY_LOG(LOG_ERROR, err_str);
            emit test_info_message(LOG_ERROR, err_str);
            hv_cool_timer.start(gs_mb_write_error_cool_tims_ms);
        }
        else if(mb_reply->isFinished())
        {
            QModbusDevice::Error err = mb_reply->error();
            if(QModbusDevice::NoError == err)
            {
                expo_params_been_set_handler();
            }
            else
            {
                QString err_str = mb_reply->errorString();
                DIY_LOG(LOG_ERROR, QString("Write triple params %1 error:%2").
                                            arg(hv_curr_triple_mb_unit_str, err_str));
                emit test_info_message(LOG_ERROR, QString("%1 %2 %3").arg(gs_str_mb_write_triple,
                                                                    hv_curr_triple_mb_unit_str,
                                                                    gs_str_fail));
                hv_cool_timer.start(gs_mb_write_error_cool_tims_ms);
            }
            delete mb_reply;
        }
        else
        {
            connect(mb_reply, &QModbusReply::finished,
                        this, &HVTester::mb_write_params_finished_handler, Qt::QueuedConnection);
        }
    }
        break;
    }
}

void HVTester::mb_rw_error_handler(QModbusDevice::Error error)
{}

void HVTester::mb_read_finished_handler()
{}

void HVTester::mb_write_params_finished_handler()
{
    QModbusReply * mb_reply = qobject_cast<QModbusReply *>(sender());
    QModbusDevice::Error err = mb_reply->error();
    if(QModbusDevice::NoError == err)
    {
        emit expo_params_been_set();
    }
    else
    {
        QString err_str = mb_reply->errorString();
        DIY_LOG(LOG_ERROR, QString("Write triple params %1 error:%2").
                                    arg(hv_curr_triple_mb_unit_str, err_str));
        emit test_info_message(LOG_ERROR, QString("%1 %2 %3").arg(gs_str_mb_write_triple,
                                                            hv_curr_triple_mb_unit_str,
                                                            gs_str_fail));
        hv_cool_timer.start(gs_mb_write_error_cool_tims_ms);
    }

    mb_reply->deleteLater();
}

void HVTester::mb_start_expo_finished_handler()
{}

void HVTester::expo_params_been_set_handler()
{}

void HVTester::cool_timer_handler()
{
    go_test_handler();
}

void HVTester::stop_test_handler()
{
    hv_test_params = nullptr;
    hv_modbus_device = nullptr;
    hv_tester_proc = TESTER_IDLE;
    hv_test_idx_in_loop = -1;
    hv_test_idx_in_round = -1;
    if(hv_cool_timer.isActive()) hv_cool_timer.stop();
}

void HVTester::end_test()
{
    emit test_complete();
    stop_test_handler();
}
