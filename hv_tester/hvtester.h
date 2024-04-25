#ifndef HVTESTER_H
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
typedef enum
{
    TEST_OP_SET_EXPO_TRIPLE,
    TEST_OP_READ_REGS,
}tester_op_enum_t;
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
        TESTER_COMPLETE,
    }tester_procedure_enum_t;

    tester_procedure_enum_t hv_tester_proc = TESTER_IDLE;
    test_params_struct_t *hv_test_params = nullptr;
    QModbusClient * hv_modbus_device = nullptr;
    int hv_modbus_srvr_addr;
    int hv_test_idx_in_loop = -1, hv_test_idx_in_round = -1;
    expo_param_triple_struct_t hv_curr_expo_param_triple;
    QString hv_curr_triple_mb_unit_str;
    QTimer hv_cool_timer;

    void update_tester_state();
    void end_test();

public:
    bool init(test_params_struct_t *test_params, QModbusClient * modbus_device, int srvr_addr);

public slots:
    /*user signal handler.*/
    void go_test_handler();
    void stop_test_handler();
public slots:
    /*modbus signal handler*/
    void mb_rw_error_handler(QModbusDevice::Error error);
    void mb_read_finished_handler();
    void mb_write_params_finished_handler();
    void mb_start_expo_finished_handler();
    /*timer handler*/
    void cool_timer_handler();

signals:
    /*signals to send to its user.*/
    void test_info_message(LOG_LEVEL lvl, QString msg);
    void rec_mb_regs(tester_op_enum_t op, mb_reg_val_map_t reg_val_map, int loop_idx, int round_idx);
    void test_complete();

signals:
    /*signals used internally.*/
    void expo_params_been_set();
public slots:
    void expo_params_been_set_handler();
};

#endif // HVTESTER_H
