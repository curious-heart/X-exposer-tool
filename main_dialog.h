#ifndef DIALOG_H
#define DIALOG_H

#include <QDialog>
#include <QModbusRtuSerialMaster>
#include <QModbusTcpClient>
#include <QFile>
#include <QSet>
#include <QTimer>

#include "test_param_settings.h"
#include "hv_connsettings.h"
#include "hv_tester/hvtester.h"
#include "config_recorder/uiconfigrecorder.h"

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
    QModbusDevice::State m_modbus_state = QModbusDevice::UnconnectedState;
    bool m_dso_connected = false;
    bool m_testing = false, m_self_reconnecting = false, m_asked_for_reconnecting = false;
    QString m_curr_rec_folder_name, m_curr_rec_file_name;
    QFile m_curr_rec_file;
    QTextStream m_curr_txt_stream;
    static const hv_mb_reg_e_t m_mbregs_to_record[];
    QColor m_txt_def_color;
    QFont m_txt_def_font;

    hv_conn_type_enum_t m_curr_conn_type = CONN_TYPE_NONE;
    QModbusClient * m_modbus_device = nullptr;
    UiConfigRecorder m_cfg_recorder;
    qobj_ptr_set_t m_rec_ui_cfg_fin, m_rec_ui_cfg_fout;
    void select_modbus_device();

    void refresh_butoons();

    bool modbus_connect();
    bool modbus_disconnect();

    void record_header();

    QTimer m_reconn_wait_timer, m_gap_between_disconn_conn_timer;

private slots:
    void modbus_error_sig_handler(QModbusDevice::Error error);
    void modbus_state_changed_sig_handler(QModbusDevice::State state);
    void on_hvConnBtn_clicked();
    void on_hvDisconnBtn_clicked();
    void on_startTestBtn_clicked();
    void on_stopTestBtn_clicked();

    void test_info_message_sig_handler(LOG_LEVEL lvl, QString msg);
    void rec_mb_regs_sig_handler(tester_op_enum_t op, mb_reg_val_map_t reg_val_map,
                                 int loop_idx, int round_idx);
    void test_complete_sig_hanler(tester_end_code_enum_t code);
    void mb_op_err_req_reconnect_sig_handler();

    void on_clearTestInfoBtn_clicked();

    /*internal used signal handler*/
    void auto_reconnect_sig_handler();

    void on_dsoSetBtn_clicked();

    void on_Dialog_finished(int result);

    void reconn_wait_timer_sig_handler();
    void gap_between_disconn_conn_timer_sig_handler();

signals:
    void go_test_sig();
    void stop_test_sig(tester_end_code_enum_t code);
    void mb_reconnected_sig();

    /*internal used signal*/
    void auto_reconnect_sig();
};
#endif // DIALOG_H
