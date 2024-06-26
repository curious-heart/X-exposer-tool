﻿#ifndef HVTESTER_H
#define HVTESTER_H

#include <QObject>
#include <QModbusClient>
#include <QString>
#include <QMap>
#include <QTimer>

#include "logger/logger.h"
#include "test_params_struct.h"
#include "modbus_regs/modbus_regs.h"

typedef QMap<hv_mb_reg_e_t, quint16> mb_reg_val_map_t;
Q_DECLARE_METATYPE(mb_reg_val_map_t)

typedef enum
{
    TEST_OP_NULL,
    TEST_OP_SET_EXPO_TRIPLE,
    TEST_OP_START_EXPO,
    TEST_OP_READ_REGS,
    TEST_OP_READ_DISTANCE,
}tester_op_enum_t;
Q_DECLARE_METATYPE(tester_op_enum_t)

typedef enum
{
    TEST_END_NORMAL = 0,
    TEST_END_ABORT_BY_USER,
    TEST_END_EXCEPTION,
}tester_end_code_enum_t;
Q_DECLARE_METATYPE(tester_end_code_enum_t)

class HVTester : public QObject
{
    Q_OBJECT
public:
    explicit HVTester(QObject *parent = nullptr);

private:
    typedef enum
    {
        TESTER_IDLE = -1,
        TESTER_WORKING,
        TESTER_A_NEW_ROUND,
        TESTER_LAST_ONE,
        TESTER_COMPLETE,
    }tester_procedure_enum_t;

    tester_procedure_enum_t hv_tester_proc = TESTER_IDLE;
    tester_op_enum_t hv_curr_op = TEST_OP_NULL;
    test_params_struct_t *hv_test_params = nullptr;
    QModbusClient * hv_modbus_device = nullptr;
    int hv_modbus_srvr_addr;
    int hv_test_idx_in_loop = 0, hv_test_idx_in_round = -1;
    expo_param_triple_struct_t hv_curr_expo_param_triple;
    QString hv_curr_triple_mb_unit_str;
    QTimer hv_test_op_timer;
    mb_reg_val_map_t m_regs_read_result;

    bool is_the_last_one_test();
    void update_tester_state();
    void end_test(tester_end_code_enum_t code);
    typedef void (HVTester::*mb_operation_handler_t)();
    mb_operation_handler_t m_current_handler = nullptr;

public:
    bool init(test_params_struct_t *test_params, QModbusClient * modbus_device, int srvr_addr);

private:
    bool mb_rw_reply_received(tester_op_enum_t op, QModbusReply* mb_reply,
                              void (HVTester::*finished_sig_handler)(),
                              bool sync, bool error_notify);
    void set_expo_parameters();
    int calc_cool_dura_ms();

public slots:
    /*user signal handler.*/
    void go_test_sig_handler(); //this handler is also used as internal signal slot.
    void stop_test_sig_handler(tester_end_code_enum_t code);
    void mb_reconnected_sig_handler();

signals:
    /*signals sent to user.*/
    void test_info_message_sig(LOG_LEVEL lvl, QString msg);
    void rec_mb_regs_sig(tester_op_enum_t op, mb_reg_val_map_t reg_val_map,
                         int loop_idx, int round_idx);
    void test_complete_sig(tester_end_code_enum_t code);
    void mb_op_err_req_reconnect_sig();
    /*signals used internally.*/
    void start_expo_now_sig();
    void start_readback_now_sig();
    void start_read_distance_sig();
    void internal_go_test_sig();
public slots:
    /*internal signal handler*/
    void start_expo_now_sig_handler();
    void start_readback_now_sig_handler();
    void start_read_distance_sig_handler();
    /*modbus signal handler*/
    void mb_write_params_finished_sig_handler();
    void mb_start_expo_finished_sig_handler();
    void mb_read_finished_sig_handler();
    void mb_read_distance_finish_sig_handler();
    void mb_rw_error_sig_handler(QModbusDevice::Error error);
    /*timer handler*/
    void hv_test_op_timer_handler();
};

#endif // HVTESTER_H
