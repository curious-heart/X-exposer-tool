#ifndef DIALOG_H
#define DIALOG_H

#include <QDialog>
#include <QModbusRtuSerialMaster>
#include <QModbusTcpClient>

#include "test_param_settings.h"
#include "hv_connsettings.h"
#include "hv_tester/hvtester.h"

QT_BEGIN_NAMESPACE
namespace Ui { class Dialog; }
QT_END_NAMESPACE

class Dialog : public QDialog
{
    Q_OBJECT

public:
    Dialog(QWidget *parent = nullptr);
    ~Dialog();

private slots:
    void on_testParamSetBtn_clicked();

    void on_hvConnSetBtn_clicked();

private:
    Ui::Dialog *ui;
    HVTester m_hv_tester;
    testParamSettingsDialog * m_testParamSettingsDialog = nullptr;
    hvConnSettings * m_hvConnSettingsDialog = nullptr;

    test_params_struct_t m_test_params;
    modbus_conn_parameters_struct_t m_hv_conn_params;
    bool m_modbus_connected = false;
    bool m_dso_connected = false;
    bool m_testing = false;

    hv_conn_type_enum_t m_curr_conn_type = CONN_TYPE_NONE;
    QModbusClient * m_modbus_device = nullptr;
    void select_modbus_device();

    void refresh_butoons();

    bool modbus_connect();
    bool modbus_disconnect();

private slots:
    void modbus_error_handler(QModbusDevice::Error error);
    void modbus_state_changed_handler(QModbusDevice::State state);
    void on_hvConnBtn_clicked();
    void on_hvDisconnBtn_clicked();
    void on_startTestBtn_clicked();
    void on_stopTestBtn_clicked();

    void test_info_message_handler(LOG_LEVEL lvl, QString msg);
    void rec_mb_regs_handler(tester_op_enum_t op, mb_reg_val_map_t reg_val_map, int loop_idx, int round_idx);
    void test_complete_hanler();

signals:
    void go_test();
    void stop_test();

};
#endif // DIALOG_H
