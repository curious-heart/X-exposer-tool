#ifndef HVTESTER_H
#define HVTESTER_H

#include <QObject>
#include <QModbusClient>
#include <QString>
#include <QMap>
#include <QTimer>
#include <QColor>
#include <QDateTime>

#include "logger/logger.h"
#include "test_params_struct.h"
#include "modbus_regs/modbus_regs.h"

#define TEST_OP_ITEM(op) op
#define TESTER_OP_LIST \
    TEST_OP_ITEM(TEST_OP_NULL),\
    TEST_OP_ITEM(TEST_OP_SET_EXPO_TRIPLE),\
    TEST_OP_ITEM(TEST_OP_START_EXPO),\
    TEST_OP_ITEM(TEST_OP_READ_REGS),\
    TEST_OP_ITEM(TEST_OP_READ_DISTANCE),

typedef enum
{
    TESTER_OP_LIST
}tester_op_enum_t;
Q_DECLARE_METATYPE(tester_op_enum_t)

typedef enum
{
    TEST_END_NORMAL = 0,
    TEST_END_ABORT_BY_USER,
    TEST_END_EXCEPTION,
}tester_end_code_enum_t;
Q_DECLARE_METATYPE(tester_end_code_enum_t)

typedef enum
{
    NORMAL_OP,
    RESUME_FROM_PAUSE,
    RESUME_FROM_DISCONN,
    RESUME_FROM_ERROR,
}tester_op_resume_type_e_t;
Q_DECLARE_METATYPE(tester_op_resume_type_e_t)

class HVTester : public QObject
{
    Q_OBJECT
public:
    explicit HVTester(QObject *parent = nullptr);
    ~HVTester();

private:
    typedef enum
    {
        TESTER_IDLE = -1,
        TESTER_WORKING,
        TESTER_A_NEW_ROUND,
        TESTER_LAST_ONE,
        TESTER_COMPLETE,
    }tester_procedure_enum_t;

    typedef void (HVTester::*tester_simple_handler_t)();

    tester_procedure_enum_t hv_tester_proc = TESTER_IDLE;
    bool hv_test_paused = false;
    tester_op_enum_t hv_curr_op = TEST_OP_NULL;
    test_params_struct_t *hv_test_params = nullptr;
    QModbusClient * hv_modbus_device = nullptr;
    int hv_modbus_srvr_addr;
    int hv_test_idx_in_loop = 0, hv_test_idx_in_round = -1;
    expo_param_triple_struct_t hv_curr_expo_param_triple;
    QString hv_curr_triple_mb_unit_str;
    QTimer hv_test_op_timer, hv_test_err_retry_timer;
    mb_reg_val_map_t m_regs_read_result;

    QTimer * m_curr_timer = nullptr;
    int m_curr_timer_remaining_time_ms;
    tester_simple_handler_t m_curr_timer_handler = nullptr;
    QDateTime m_dt_checkpoint_for_pause;


    bool m_last_jdg_ret = true;
    int m_last_test_retry_cnt = 0;

    bool is_the_last_one_test(test_params_struct_t * ctrl_struct,
                                                expo_param_triple_struct_t &curr_triple,
                                                int &idx_in_loop, int &idx_in_round);
    tester_procedure_enum_t update_tester_state(test_params_struct_t * ctrl_struct,
                                                expo_param_triple_struct_t &curr_triple,
                                                int &idx_in_loop, int &idx_in_round);
    void end_test(tester_end_code_enum_t code);
    typedef void (HVTester::*mb_operation_handler_t)(tester_op_enum_t op);

public:
    bool init(test_params_struct_t *test_params, QModbusClient * modbus_device, int srvr_addr);
    void init_for_time_stat(test_params_struct_t *test_params);
    float expect_remaining_test_dura_ms(bool total = false);
    void set_last_judge_result(bool jdg_ret);
    void increase_test_retry_cnt();
    void reset_test_retry_cnt();
    bool is_last_test_retry();
    bool is_retrying();

private:
    void reset_internal_state();
    void reset_timer_rec();
    void check_pause_to_start_timer(QTimer * c_timer, tester_simple_handler_t timeout_handler,
                                    int dura_ms);
    void mb_rw_reply_received(tester_op_enum_t op, QModbusReply* mb_reply,
                              void (HVTester::*finished_sig_handler)(),
                              bool sync, bool error_notify);
    void construct_mb_du(tester_op_enum_t op, QModbusDataUnit &mb_du);
    void tester_send_mb_cmd(tester_op_enum_t op, tester_op_resume_type_e_t r_t = NORMAL_OP);
    int calc_cool_dura_ms(qint64 time_elapsed_since_cool_start);
    void end_test_due_to_exception(QString err_str);
    void post_test_info_message(LOG_LEVEL lvl, QString msg,
                               bool always_rec = false,
                               QColor set_color = QColor(), int set_font_w = -1);
    quint16 get_start_expo_cmd_from_test_method();

public slots:
    /*user signal handler.*/
    void go_test_sig_handler(); //this handler is also used as internal signal slot.
    void stop_test_sig_handler(tester_end_code_enum_t code);
    void mb_reconnected_sig_handler();
    void pause_resume_test_sig_handler(bool pause);

signals:
    /*signals sent to user.*/
    void test_info_message_sig(LOG_LEVEL lvl, QString msg,
                               bool always_rec = false,
                               QColor set_color = QColor(), int set_font_w = -1);
    void rec_mb_regs_sig(tester_op_enum_t op, mb_reg_val_map_t reg_val_map,
                         int loop_idx, int round_idx, bool proc_monitor = false);
    void test_complete_sig(tester_end_code_enum_t code);
    void begin_exposure_sig(bool start = true);

    /* now we do not use this signal. tester just retry and retry, and main_dialog matain the
       connection.
    */
    void mb_op_err_req_reconnect_sig();

    /*signals used internally.*/
    void tester_next_operation_sig(tester_op_enum_t op, tester_op_resume_type_e_t r_t = NORMAL_OP);
    void internal_go_test_sig();
public slots:
    /*internal signal handler*/
    /*modbus signal handler*/
    void mb_op_finished_sig_handler();
    void mb_rw_error_sig_handler(QModbusDevice::Error error);
    /*timer handler*/
    void hv_test_op_timer_handler();
    void hv_test_err_retry_timer_handler();
};

#endif // HVTESTER_H
