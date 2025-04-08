#ifndef DIALOG_H
#define DIALOG_H

#include <QDialog>
#include <QModbusRtuSerialMaster>
#include <QModbusTcpClient>
#include <QFile>
#include <QSet>
#include <QTimer>
#include <QDateTime>

#include "common_tools/common_tool_func.h"
#include "test_param_settings.h"
#include "hv_connsettings.h"
#include "hv_tester/hvtester.h"
#include "config_recorder/uiconfigrecorder.h"
#include "test_result_judge/test_result_judge.h"

QT_BEGIN_NAMESPACE
namespace Ui { class Dialog; }
QT_END_NAMESPACE

class Dialog : public QDialog
{
    Q_OBJECT

public:
    Dialog(QWidget *parent = nullptr);
    ~Dialog();
    bool m_init_ok = false;

private slots:
    void on_testParamSetBtn_clicked();

    void on_hvConnSetBtn_clicked();

private:
    Ui::Dialog *ui;
    HVTester m_hv_tester;
    testParamSettingsDialog * m_testParamSettingsDialog = nullptr;
    hvConnSettings * m_hvConnSettingsDialog = nullptr;

    void reset_time_stat_vars();
    void reset_internal_flags();

    test_params_struct_t m_test_params;
    modbus_conn_parameters_struct_t m_hv_conn_params;
    QModbusDevice::State m_modbus_state = QModbusDevice::UnconnectedState;
    bool m_dso_connected = false;
    bool m_testing = false, m_test_paused = false, m_self_reconnecting = false;
    bool m_asked_for_reconnecting = false;
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

    QTimer m_reconn_wait_timer;

    QDateTime m_test_start_time, m_pause_dura_check_point;
    int m_pause_cnt = 0, m_pause_dura_ms = 0, m_act_test_dura_ms = 0;
    int m_expt_test_dura_ms = 0, m_expt_test_remain_dura_ms = 0;
    QTimer m_time_stat_timer; //this is a periodical timer.
    void refresh_time_stat_display(bool total_dura = false, bool start_test = false,
                                   bool pause_resumed = false);

    void set_man_test_grp_visible(test_mode_enum_t mode);

    TestResultJudge m_test_judge;
    mb_reg_judge_result_map_t m_judge_reg_ret_map;
    void reset_judge_reg_ret_map();
    void map_judge_result_to_style(judge_result_e_t judge_result, str_with_style_s_t &style_str);
    void rec_judge_result(tester_end_code_enum_t code);

    CToolKeyFilter m_key_filter;
    expo_params_ui_sync_ctrls_s_t m_ui_sync_ctrls;

private slots:
    void modbus_error_sig_handler(QModbusDevice::Error error);
    void modbus_state_changed_sig_handler(QModbusDevice::State state);
    void on_hvConnBtn_clicked();
    void on_hvDisconnBtn_clicked();
    void on_startTestBtn_clicked();
    void on_stopTestBtn_clicked();

    void test_info_message_sig_handler(LOG_LEVEL lvl, QString msg,
                                       bool always_rec = false,
                                       QColor set_color = QColor(), int set_font_w = -1);
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

    void on_pauseTestBtn_clicked();

    void time_stat_timer_sig_handler();

    void on_manTestSettingBtn_clicked();

signals:
    void go_test_sig();
    void stop_test_sig(tester_end_code_enum_t code);
    void mb_reconnected_sig();
    void pause_resume_test_sig(bool pause);

    /*internal used signal*/
    void auto_reconnect_sig();
    void refresh_time_stat_display_sig(bool total_dura = false, bool start_test = false,
                                       bool from_timer = false);
};
#endif // DIALOG_H
