#ifndef DIALOG_H
#define DIALOG_H

#include <QDialog>
#include <QModbusRtuSerialMaster>
#include <QModbusTcpClient>
#include <QFile>
#include <QSet>
#include <QTimer>
#include <QDateTime>
#include <QMutexLocker>

#include "common_tools/common_tool_func.h"
#include "test_param_settings.h"
#include "hv_connsettings.h"
#include "hv_tester/hvtester.h"
#include "config_recorder/uiconfigrecorder.h"
#include "test_result_judge/test_result_judge.h"
#include "mb_regs_chart_display.h"

#include "serialportsetdlg.h"
#include "sc_data_connsettings.h"
#include "sc_data_proc.h"
#include "recvscanneddata.h"
#include "curveplotwidget.h"

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
    SerialPortSetDlg * m_pb_conn_settings_dlg = nullptr;
    MbRegsChartDisp * m_mbRegsChartWnd = nullptr;
    sc_data_connsettings * m_sc_data_conn_settings_dlg = nullptr;

    void reset_time_stat_vars();
    void reset_internal_flags();

    test_params_struct_t m_test_params;
    modbus_conn_parameters_struct_t m_hv_conn_params;
    QModbusDevice::State m_modbus_state = QModbusDevice::UnconnectedState;
    bool m_dso_connected = false;
    bool m_testing = false, m_test_paused = false, m_self_reconnecting = false, m_during_exposuring = false;
    bool m_asked_for_reconnecting = false;
    QString m_curr_rec_folder_name, m_curr_rec_file_name;
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

    serial_port_conn_params_struct_t m_pb_conn_params;
    sc_data_conn_params_struct_t m_sc_data_conn_params;

    void refresh_butoons();

    bool modbus_connect();
    bool modbus_disconnect();

    void record_header();

    void mb_rw_reply_received(QModbusReply* mb_reply, void (Dialog::*finished_sig_handler)(),
                              bool sync);

    QTimer m_reconn_wait_timer;

    QDateTime m_test_start_time, m_pause_dura_check_point;
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

    void arrange_ui_disp_according_to_syscfgs();

    QQueue<recv_data_with_notes_s_t> dataQueue;
    QMutex queueMutex;
    RecvScannedData *recv_data_worker;
    QThread *recv_data_workerThread;

    quint32 m_max_pt_value;
    int m_disp_curv_pt_cnt;
    QString log_disp_prepender_str();
    QVector<quint32> m_ch1_data_vec, m_ch2_data_vec;
    QString m_ch1_wnd_str_id = "channel-1", m_ch2_wnd_str_id = "channel-2";
    int split_data_into_channels(QByteArray& ori_data,
                              QVector<quint32> &dv_ch1, QVector<quint32> &dv_ch2,
                              quint64 &pkt_idx);
    void openOrActivatePlotWindow(const QString &id,
                                  int pt_cnt, const QVector<quint32>& row_data,
                                  quint32 range_max);
    void show_pt_curves();
    void reset_pt_curves_wnd_pos_size(QString wnd_id);
    QMap<QString, CurvePlotWidget*> m_plotWindows;

private slots:
    void modbus_error_sig_handler(QModbusDevice::Error error);
    void modbus_state_changed_sig_handler(QModbusDevice::State state);
    void modbus_op_finished_sig_handler();
    void on_hvConnBtn_clicked();
    void on_hvDisconnBtn_clicked();
    void on_startTestBtn_clicked();
    void on_stopTestBtn_clicked();

    void test_info_message_sig_handler(LOG_LEVEL lvl, QString msg,
                                       bool always_rec = false,
                                       QColor set_color = QColor(), int set_font_w = -1);
    void rec_mb_regs_sig_handler(tester_op_enum_t op, mb_reg_val_map_t reg_val_map,
                                 int loop_idx, int round_idx, bool proc_moniter = false);
    void test_complete_sig_hanler(tester_end_code_enum_t code);
    void mb_op_err_req_reconnect_sig_handler();

    void on_clearTestInfoBtn_clicked();

    /*internal used signal handler*/
    void auto_reconnect_sig_handler();

    void on_Dialog_finished(int result);

    void reconn_wait_timer_sig_handler();

    void on_pauseTestBtn_clicked();

    void time_stat_timer_sig_handler();

    void on_manTestSettingBtn_clicked();

    void test_proc_report_timer_sig_handler();
    void get_test_proc_st_sig_handler();

    void on_testPromMonitorClrBtn_clicked();

    void begin_exposure_sig_handler(bool start = true);

    bool set_mb_expo_triple();
    void on_dispChartBtn_clicked();

    void on_pbConnSetPbt_clicked();

    void on_dataCollConnSetPbt_clicked();

    void on_dataCollStartPbt_clicked();
    void on_dataCollStopPbt_clicked();
    void handleNewDataReady();
    void collect_data_conn_timeout();
    void collect_data_disconn_timeout();

    void on_dataCollDispCurvPbt_clicked();

signals:
    void go_test_sig();
    void stop_test_sig(tester_end_code_enum_t code);
    void mb_reconnected_sig();
    void pause_resume_test_sig(bool pause);

    void start_collect_sc_data(QString ip, quint16 port, int connTimeout, int packetCount);
    void stop_collect_sc_data();

    /*internal used signal*/
    void auto_reconnect_sig();
    void refresh_time_stat_display_sig(bool total_dura = false, bool start_test = false,
                                       bool from_timer = false);
    void get_test_proc_st_sig();
};
#endif // DIALOG_H
