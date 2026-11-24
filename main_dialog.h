#ifndef DIALOG_H
#define DIALOG_H

#include <QDialog>
#include <QModbusRtuSerialMaster>
#include <QModbusTcpClient>
#include <QFile>
#include <QSet>
#include <QTimer>
#include <QDateTime>
#include <functional>

#include "common_tools/common_tool_func.h"
#include "test_param_settings.h"
#include "hv_connsettings.h"
#include "hv_tester/hvtester.h"
#include "config_recorder/uiconfigrecorder.h"
#include "test_result_judge/test_result_judge.h"

#include "mb_regs_chart_display.h"

QT_BEGIN_NAMESPACE
namespace Ui { class Dialog; }
QT_END_NAMESPACE

class Dialog : public QDialog
{
    Q_OBJECT

public:
    explicit Dialog(const QString &device_id, const QString &settings_file,
                    const QString &settings_group, QWidget *parent = nullptr);
    ~Dialog();
    QString deviceId() const { return m_device_id; }
    QString endpoint() const;
    QString endpointKey() const;
    bool endpointInUse() const;
    bool isTesting() const { return m_testing; }
    bool isPaused() const { return m_test_paused; }
    bool canStart() const;
    bool canConnect() const;
    bool canDisconnect() const;
    QString connectionStatus() const;
    QString testStatus() const;
    QString progressText() const;
    int progressPercent() const;
    QString remainingTime() const;
    QString lastResult() const { return m_last_result; }
    QString lastReport() const { return m_last_report; }
    QString lastMessage() const { return m_last_message; }
    QString connectionInfo() const { return m_hv_conn_params.info_str.trimmed(); }
    QString startTest();
    void setConnectionCheck(std::function<QString(const Dialog &)> check);

public slots:
    void on_testParamSetBtn_clicked();
    void on_hvConnSetBtn_clicked();
    void on_hvConnBtn_clicked();
    void on_hvDisconnBtn_clicked();
    void on_stopTestBtn_clicked();
    void on_pauseTestBtn_clicked();
    void reject() override;

private:
    Ui::Dialog *ui;
    const QString m_device_id;
    std::function<QString(const Dialog &)> m_connection_check;
    HVTester m_hv_tester;
    testParamSettingsDialog * m_testParamSettingsDialog = nullptr;
    hvConnSettings * m_hvConnSettingsDialog = nullptr;
    MbRegsChartDisp * m_mbRegsChartWnd = nullptr;

    void reset_time_stat_vars();
    void reset_internal_flags();

    test_params_struct_t m_test_params{};
    modbus_conn_parameters_struct_t m_hv_conn_params{};
    QModbusDevice::State m_modbus_state = QModbusDevice::UnconnectedState;
    bool m_testing = false, m_test_paused = false, m_self_reconnecting = false, m_during_exposuring = false;
    bool m_editing_settings = false;
    QString m_last_result, m_last_report, m_last_message;
    qint64 m_completed_groups = 0, m_planned_groups = 0;
    QFile m_curr_rec_file;
    QTextStream m_curr_txt_stream;
    QList<hv_mb_reg_e_t> m_mbregs_rec_list, m_mbregs_monitor_list, mb_mbregs_result_disp_list;
    QColor m_txt_def_color;
    QFont m_txt_def_font;

    hv_conn_type_enum_t m_curr_conn_type = CONN_TYPE_NONE;
    QModbusClient * m_modbus_device = nullptr;
    UiConfigRecorder m_cfg_recorder;
    qobj_ptr_set_t m_rec_ui_cfg_fin, m_rec_ui_cfg_fout;
    void select_modbus_device();

    void refresh_buttons();

    bool modbus_connect();
    bool modbus_disconnect();

    void record_header();

    void processMonitorReply(QModbusReply *reply);
    QPointer<QModbusReply> m_monitor_reply;

    QTimer m_reconn_wait_timer;

    QDateTime m_test_start_time, m_pause_dura_check_point, m_resume_point;
    int m_last_calced_dura_ms = 0;
    int m_pause_cnt = 0, m_pause_dura_ms = 0, m_act_test_dura_ms = 0;
    int m_expt_test_dura_ms = 0, m_expt_test_remain_dura_ms = 0;
    QTimer m_time_stat_timer; //this is a periodical timer.
    QTimer m_test_proc_monitor_timer;//this is a periodical timer.
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

    void update_regs_to_rec_list();

    void display_mb_regs_chart();

private slots:
    void modbus_error_sig_handler(QModbusDevice::Error error);
    void modbus_state_changed_sig_handler(QModbusDevice::State state);
    void on_startTestBtn_clicked();

    void test_info_message_sig_handler(LOG_LEVEL lvl, QString msg,
                                       bool always_rec = false,
                                       QColor set_color = QColor(), int set_font_w = -1);
    void rec_mb_regs_sig_handler(tester_op_enum_t op, mb_reg_val_map_t reg_val_map,
                                 int loop_idx, int round_idx, bool proc_moniter = false);
    void test_complete_sig_hanler(tester_end_code_enum_t code);

    void on_clearTestInfoBtn_clicked();

    void reconn_wait_timer_sig_handler();

    void time_stat_timer_sig_handler();

    void on_manTestSettingBtn_clicked();

    void test_proc_report_timer_sig_handler();
    void get_test_proc_st_sig_handler();

    void on_testPromMonitorClrBtn_clicked();

    void begin_exposure_sig_handler(bool start = true);

    bool set_mb_expo_triple();
    void on_dispChartBtn_clicked();

    void endisable_test_proc_monitor_btns();

    void on_testProcMonitorChkBox_stateChanged(int arg1);

signals:
    void deviceChanged();
};
#endif // DIALOG_H
