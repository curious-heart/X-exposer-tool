#ifndef DIALOG_H
#define DIALOG_H

#include <QDialog>
#include <QModbusRtuSerialMaster>
#include <QModbusTcpClient>

#include "test_param_settings.h"
#include "hv_connsettings.h"

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
};
#endif // DIALOG_H
