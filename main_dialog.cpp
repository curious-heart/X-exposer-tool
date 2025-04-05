#include <QMessageBox>
#include <QColor>
#include <QtMath>
#include "qtsingleapplication/qtsingleapplication.h"

#include "logger/logger.h"
#include "main_dialog.h"
#include "ui_main_dialog.h"
#include "sysconfigs/sysconfigs.h"
#include "mb_rtu_over_tcp/qmodbusrtuovertcpclient.h"

static const char* gs_str_plz_set_valid_conn_params = "请首先设置有效的连接参数";
static const char* gs_str_plz_set_valid_test_params = "请首先设置有效的测试参数";
static const char* gs_str_init_fail = "初始化失败";
static const char* gs_str_modbus_not_in_disconnected_state = "modbus未处于断开状态";
static const char* gs_str_modbus_not_in_connected_state = "modbus未处于连接状态";
static const char* gs_str_modbus_not_connected = "modbus未连接";
static const char* gs_str_modbus_exceptional_error = "异常的modbus错误";
static const char* gs_str_modbus_unkonwn_state = "未知的modbus连接状态";
static const char* gs_str_modbus_connect_err = "modbus连接失败";
static const char* gs_str_modbus_disconnect_err = "modbus断开连接失败";
static const char* gs_str_init_hvtester_err = "初始化hv_tester失败";
static const char* gs_str_test_complete = "测试完成";
static const char* gs_str_test_abort_by_user = "用户中止测试";
static const char* gs_str_test_end_exception = "测试异常中止";
static const char* gs_str_sep_line = "========================================";

static const char* gs_str_test_rec_name_sufx = "曝光测试结果";
static const char* gs_str_test_rec_file_type = ".csv";
static const char* gs_str_create_folder = "创建文件夹";
static const char* gs_str_create_file = "创建文件";
extern const char* g_str_fail;

static const char* gs_str_date = "日期";
static const char* gs_str_time = "时间";
static const char* gs_str_no = "序号";

static const char* gs_str_pause = "暂停";
static const char* gs_str_resume = "恢复";

static const char* gs_str_test_judge_result = "测试结果判定";
static const char* gs_str_test_pass = "测试通过";
static const char* gs_str_test_begin = "测试开始";
static const char* gs_str_test_end = "测试结束";

static const char* gs_str_start_test = "开始测试";
static const char* gs_str_testing = "正在测试";

extern const char* g_str_loop;
extern const char* g_str_time_ci;
extern const char* g_str_the_line_pron;

/*设置管电压、设置管电流、曝光时间必须连续放置*/
const hv_mb_reg_e_t Dialog::m_mbregs_to_record[] =
{
    HSV, VoltSet, FilamentSet, ExposureTime, Voltmeter, Ammeter, EXT_MB_REG_DISTANCE,
    State, ExposureStatus, BatteryLevel, BatteryVoltmeter, OilBoxTemperature,
    Workstatus, exposureCount,
};
static const hv_mb_reg_e_t gs_judge_result_disp_reg[] =
{VoltSet, FilamentSet, ExposureTime, Voltmeter, Ammeter, EXT_MB_REG_DISTANCE, State,
 ExposureStatus};

static QColor gs_log_lvl_fonts_arr[] =
{
    Qt::gray, //DEBUG, gray
    Qt::black, //INFO, black
    QColor(255, 128, 0), //WARNING, orange
    Qt::red, //ERROR, red
};

/*
 * The following vars MUST be defined in function using the following macros.
 *
    int total_secs, hour, min, sec;
    QString hhmmss_str;
*/
/*sec MUST be of integer type.*/
#define SECS_TO_HHMMSS_STR(secs_var) \
    total_secs = (secs_var);\
    hour = (total_secs) / 3600; (total_secs) = (total_secs) % 3600; \
    min = (total_secs) / 60; (total_secs) = (total_secs) % 60; \
    sec = (total_secs);\
    hhmmss_str = QString("%1:%2:%3").arg(hour, 2, 10, QLatin1Char('0'))\
                                   .arg(min, 2, 10, QLatin1Char('0'))\
                                   .arg(sec, 2, 10, QLatin1Char('0'));

#define SECS_TO_HHMMSS_STR_DISP(secs_var, ctrl_op) \
{\
    int total_secs, hour, min, sec;\
    QString hhmmss_str;\
\
    SECS_TO_HHMMSS_STR(secs_var); \
    ctrl_op(hhmmss_str);\
}

#define REC_INFO_IN_FILE(op_str) \
    if(m_curr_rec_file.isOpen()) {m_curr_txt_stream << op_str;}

void Dialog::refresh_time_stat_display(bool total_dura, bool start_test, bool pause_resumed)
{
    QDateTime curr_dt;
    int curr_pause_dura_ms = 0, pause_dura_till_now_ms = 0, dura_from_resume_ms = 0;
    static int last_calced_dura_ms = 0;
    static QDateTime resume_point;

    if(start_test)
    {
        resume_point = m_test_start_time = QDateTime::currentDateTime();
        QString dtstr = m_test_start_time.toString("yyyy-MM-dd hh:mm:ss");
        ui->startTestTimeDisplayLbl->setText(dtstr);
    }

    if(start_test || total_dura)
    {
        m_expt_test_dura_ms = m_expt_test_remain_dura_ms
                = m_hv_tester.expect_remaining_test_dura_ms(true);
        SECS_TO_HHMMSS_STR_DISP(qCeil(m_expt_test_dura_ms) / 1000,
                                ui->exptTotalTestDuraDisplayLbl->setText);

        last_calced_dura_ms = m_expt_test_dura_ms;
    }

    if(pause_resumed)
    {
        resume_point = QDateTime::currentDateTime();
        last_calced_dura_ms = m_hv_tester.expect_remaining_test_dura_ms(false);
    }

    if(m_testing)
    {
        curr_dt = QDateTime::currentDateTime();
        curr_pause_dura_ms = m_test_paused ? m_pause_dura_check_point.msecsTo(curr_dt) : 0;
        pause_dura_till_now_ms = m_pause_dura_ms + curr_pause_dura_ms;

        if(!m_test_paused)
        {
            curr_dt = QDateTime::currentDateTime();
            dura_from_resume_ms = resume_point.msecsTo(curr_dt);
            m_expt_test_remain_dura_ms = last_calced_dura_ms - dura_from_resume_ms;
            if(m_expt_test_remain_dura_ms < 0) m_expt_test_remain_dura_ms = 0;
        }

        curr_dt = QDateTime::currentDateTime();
        m_act_test_dura_ms = m_test_start_time.msecsTo(curr_dt);
    }

    SECS_TO_HHMMSS_STR_DISP(qCeil(pause_dura_till_now_ms / 1000), ui->pauseDuraDisplayLbl->setText);
    ui->pauseCntDisplayLbl->setText(QString::number(m_pause_cnt));
    SECS_TO_HHMMSS_STR_DISP(qCeil(m_expt_test_remain_dura_ms / 1000),
                            ui->exptRemainTestDuraDisplayLbl->setText);
    SECS_TO_HHMMSS_STR_DISP(qCeil(m_act_test_dura_ms / 1000), ui->actTestDuraDisplayLbl->setText);
}

void Dialog::set_man_test_grp_visible(test_mode_enum_t mode)
{
    bool visible = (mode == TEST_MODE_SINGLE);

    ui->manTestcubeVoltLbl->setVisible(visible);
    ui->manTestcubeVoltSpin->setVisible(visible);
    ui->manTestcubeVoltUnitLbl->setVisible(visible);
    ui->line_1->setVisible(visible);

    ui->manTestcubeCurrentLbl->setVisible(visible);
    ui->manTestcubeCurrentDblspin->setVisible(visible);
    ui->manTestcubeCurrentUnitLbl->setVisible(visible);
    ui->line_2->setVisible(visible);

    ui->manTestexpoDuraLbl->setVisible(visible);
    ui->manTestexpoDuraDblspin->setVisible(visible);
    ui->manTestexpoDuraUnitLbl->setVisible(visible);

    ui->manTestSettingBtn->setVisible(visible);
    ui->line_3->setVisible(visible);
    ui->line_4->setVisible(visible);
}

Dialog::Dialog(QWidget *parent)
    : QDialog(parent)
    , ui(new Ui::Dialog)
    , m_hv_tester(this)
    , m_cfg_recorder(this), m_key_filter(this, this)
{
    QString ret_str;
    bool ret;

    ui->setupUi(this);

    ret = fill_sys_configs(&ret_str);
    if(!ret)
    {
        QMessageBox::critical(this, "", ret_str);
        return;
    }

    m_testParamSettingsDialog = new testParamSettingsDialog(this, &m_test_params,
                                                            &m_cfg_recorder,
                                                            &m_test_judge);
    m_hvConnSettingsDialog = new hvConnSettings(this, &m_hv_conn_params, &m_cfg_recorder);

    if(!m_testParamSettingsDialog || !m_hvConnSettingsDialog)
    {
        DIY_LOG(LOG_ERROR, "new setting dialog fail!");
        QMessageBox::critical(this, "Error", gs_str_init_fail);
        return;
    }

    QString param_collet_ret_str;
    m_txt_def_color = ui->testParamDisplayTxt->textColor();
    ui->testParamDisplayTxt->setProperty(g_prop_name_def_color, m_txt_def_color);
    param_collet_ret_str = m_testParamSettingsDialog->collect_test_params();
    if(m_test_params.valid)
    {
        ui->testParamDisplayTxt->setText(m_test_params.info_str);
    }
    else
    {
        append_str_with_color_and_weight(ui->testParamDisplayTxt, param_collet_ret_str,
                                         gs_log_lvl_fonts_arr[LOG_ERROR]);
    }

    m_txt_def_color = ui->hvConnParamDisplayTxt->textColor();
    ui->hvConnParamDisplayTxt->setProperty(g_prop_name_def_color, m_txt_def_color);
    param_collet_ret_str = m_hvConnSettingsDialog->collect_conn_params();
    if(m_hv_conn_params.valid)
    {
        ui->hvConnParamDisplayTxt->setText(m_hv_conn_params.info_str);
    }
    else
    {
        append_str_with_color_and_weight(ui->hvConnParamDisplayTxt, param_collet_ret_str,
                                         gs_log_lvl_fonts_arr[LOG_ERROR]);
    }
    select_modbus_device();

    m_rec_ui_cfg_fin.insert(nullptr); //no items needs to record.
    m_rec_ui_cfg_fout.clear();
    m_cfg_recorder.load_configs_to_ui(this, m_rec_ui_cfg_fin, m_rec_ui_cfg_fout);

    refresh_butoons();

    connect(this, &Dialog::go_test_sig,
            &m_hv_tester, &HVTester::go_test_sig_handler, Qt::QueuedConnection);
    connect(this, &Dialog::stop_test_sig,
            &m_hv_tester, &HVTester::stop_test_sig_handler, Qt::QueuedConnection);
    connect(this, &Dialog::mb_reconnected_sig,
            &m_hv_tester, &HVTester::mb_reconnected_sig_handler, Qt::QueuedConnection);
    connect(this, &Dialog::pause_resume_test_sig,
            &m_hv_tester, &HVTester::pause_resume_test_sig_handler, Qt::QueuedConnection);

    connect(&m_hv_tester, &HVTester::test_info_message_sig,
            this, &Dialog::test_info_message_sig_handler, Qt::QueuedConnection);
    connect(&m_hv_tester, &HVTester::rec_mb_regs_sig,
            this, &Dialog::rec_mb_regs_sig_handler, Qt::QueuedConnection);
    connect(&m_hv_tester, &HVTester::test_complete_sig,
            this, &Dialog::test_complete_sig_hanler, Qt::QueuedConnection);
    connect(&m_hv_tester, &HVTester::mb_op_err_req_reconnect_sig,
            this, &Dialog::mb_op_err_req_reconnect_sig_handler, Qt::QueuedConnection);

    connect(this, &Dialog::auto_reconnect_sig,
            this, &Dialog::auto_reconnect_sig_handler, Qt::QueuedConnection);

    m_reconn_wait_timer.setSingleShot(true);
    connect(&m_reconn_wait_timer, &QTimer::timeout,
            this, &Dialog::reconn_wait_timer_sig_handler, Qt::QueuedConnection);

    connect(&m_time_stat_timer, &QTimer::timeout,
            this, &Dialog::time_stat_timer_sig_handler, Qt::QueuedConnection);
    connect(this, &Dialog::refresh_time_stat_display_sig,
            this, &Dialog::refresh_time_stat_display, Qt::QueuedConnection);

    m_txt_def_color = ui->testInfoDisplayTxt->textColor();
    ui->testInfoDisplayTxt->setProperty(g_prop_name_def_color, m_txt_def_color);
    m_txt_def_font = ui->testInfoDisplayTxt->currentFont();
    ui->testInfoDisplayTxt->setProperty(g_prop_name_def_font, m_txt_def_font);

    if(m_test_params.valid)
    {
        m_hv_tester.init_for_time_stat(&m_test_params);

        refresh_time_stat_display(true);
    }

    m_test_judge.add_result_disp_items(gs_judge_result_disp_reg,
                                       ARRAY_COUNT(gs_judge_result_disp_reg));

    reset_judge_reg_ret_map();

    connect((QtSingleApplication*)QCoreApplication::instance(), &QtSingleApplication::messageReceived,
            this, [this](const QString & str){QMessageBox::information(this, "", str);});

    m_key_filter.add_keys_to_filter(Qt::Key_Escape);
    installEventFilter(&m_key_filter);

    set_man_test_grp_visible(m_test_params.test_mode);

    m_init_ok = true;
}

Dialog::~Dialog()
{
    if(QModbusDevice::ConnectedState == m_modbus_state)
    {
        modbus_disconnect();
    }

    if(m_curr_rec_file.isOpen())
    {
        m_curr_txt_stream.flush();
        m_curr_rec_file.close();
    }

    m_rec_ui_cfg_fin.clear();
    m_rec_ui_cfg_fout.clear();

    m_judge_reg_ret_map.clear();
    delete ui;
}

void Dialog::reset_judge_reg_ret_map()
{
    for(int idx = 0; idx < ARRAY_COUNT(m_mbregs_to_record); ++idx)
    {
        m_judge_reg_ret_map.insert(m_mbregs_to_record[idx], JUDGE_RESULT_OK);
    }
}

void Dialog::on_testParamSetBtn_clicked()
{
    int dialog_ret = m_testParamSettingsDialog->exec();

    if((QDialog::Accepted == dialog_ret) && m_test_params.valid)
    {
        ui->testParamDisplayTxt->setText(m_test_params.info_str);

        m_hv_tester.init_for_time_stat(&m_test_params);

        reset_time_stat_vars();
        refresh_time_stat_display(true);

        set_man_test_grp_visible(m_test_params.test_mode);
    }
}

void Dialog::on_hvConnSetBtn_clicked()
{
    int dialog_ret = m_hvConnSettingsDialog->exec();

    if((QDialog::Accepted == dialog_ret) && m_hv_conn_params.valid)
    {
        ui->hvConnParamDisplayTxt->setText(m_hv_conn_params.info_str);
        select_modbus_device();
    }
}

void Dialog::refresh_butoons()
{
    ui->hvConnSetBtn->setEnabled((QModbusDevice::UnconnectedState == m_modbus_state)
                                 && !m_testing);
    ui->testParamSetBtn->setEnabled(!m_testing);
    ui->dsoSetBtn->setEnabled(!m_dso_connected && !m_testing);
    ui->hvConnBtn->setEnabled((QModbusDevice::UnconnectedState == m_modbus_state) && !m_testing);
    ui->hvDisconnBtn->setEnabled((QModbusDevice::ConnectedState == m_modbus_state) && !m_testing);

    ui->startTestBtn->setEnabled((QModbusDevice::ConnectedState == m_modbus_state) && !m_testing);
    QString stBtnstr = m_testing ? gs_str_testing : gs_str_start_test;
    if(m_testing && m_test_paused) stBtnstr += QString("-%1").arg(gs_str_pause);
    ui->startTestBtn->setText(stBtnstr);

    ui->stopTestBtn->setEnabled(m_testing);

    ui->pauseTestBtn->setEnabled(m_testing);
    if(m_test_paused) ui->pauseTestBtn->setText(gs_str_resume);
    else ui->pauseTestBtn->setText(gs_str_pause);
}

bool Dialog::modbus_connect()
{
    select_modbus_device();
    if(!m_modbus_device)
    {
        DIY_LOG(LOG_ERROR, "modbus device is NULL.");
        return false;
    }

    if(m_modbus_device->connectDevice())
    {
        return true;
    }
    else
    {
        return false;
    }
}

bool Dialog::modbus_disconnect()
{
    if(!m_modbus_device)
    {
        DIY_LOG(LOG_ERROR, "modbus device is NULL.");
        return false;
    }
    m_modbus_device->disconnectDevice();
    return true;
}

void Dialog::select_modbus_device()
{
    if(m_hv_conn_params.valid)
    {
        if(!m_modbus_device || (m_curr_conn_type != m_hv_conn_params.type))
        {
            if(m_modbus_device) delete m_modbus_device;

            m_curr_conn_type = m_hv_conn_params.type;
            if(CONN_TYPE_TCPIP == m_curr_conn_type)
            {
                m_modbus_device = new QModbusTcpClient(this);
            }
            else if(CONN_TYPE_RTU_OVER_TCP == m_curr_conn_type )
            {
                m_modbus_device = new QModbusRtuOverTcpClient(this) ;
            }
            else
            {
                m_modbus_device = new QModbusRtuSerialMaster(this);
            }

            connect(m_modbus_device, &QModbusClient::errorOccurred,
                    this, &Dialog::modbus_error_sig_handler, Qt::QueuedConnection);
            connect(m_modbus_device, &QModbusClient::stateChanged,
                    this, &Dialog::modbus_state_changed_sig_handler, Qt::QueuedConnection);
        }

        if(CONN_TYPE_TCPIP == m_curr_conn_type || CONN_TYPE_RTU_OVER_TCP == m_curr_conn_type )
        {
            m_modbus_device->setConnectionParameter(QModbusDevice::NetworkAddressParameter,
                                                    m_hv_conn_params.tcpip_params.ip_addr);
            m_modbus_device->setConnectionParameter(QModbusDevice::NetworkPortParameter,
                                                    m_hv_conn_params.tcpip_params.port_no);
        }
        else
        {
            m_modbus_device->setConnectionParameter(QModbusDevice::SerialPortNameParameter,
                                                m_hv_conn_params.serial_params.com_port_s);
            m_modbus_device->setConnectionParameter(QModbusDevice::SerialBaudRateParameter,
                                                    m_hv_conn_params.serial_params.boudrate);
            m_modbus_device->setConnectionParameter(QModbusDevice::SerialDataBitsParameter,
                                                    m_hv_conn_params.serial_params.data_bits);
            m_modbus_device->setConnectionParameter(QModbusDevice::SerialParityParameter,
                                                m_hv_conn_params.serial_params.check_parity);
            m_modbus_device->setConnectionParameter(QModbusDevice::SerialStopBitsParameter,
                                                    m_hv_conn_params.serial_params.stop_bit);
        }
        m_modbus_device->setTimeout(m_hv_conn_params.resp_wait_time_ms);
    }
}

void Dialog::modbus_error_sig_handler(QModbusDevice::Error error)
{
    /*The strings below are in the same order of enum QModbusDevice::Error.*/
    static const char* err_str[] =
    {
        "No errors have occurred.",
        "An error occurred during a read operation.",
        "An error occurred during a write operation.",
        "An error occurred when attempting to open the backend.",
        "An error occurred when attempting to set a configuration parameter.",
        "A timeout occurred during I/O. An I/O operation did not finish within a given time frame.",
        "A Modbus specific protocol error occurred.",
        "The reply was aborted due to a disconnection of the device.",
        "An unknown error occurred.",
    };
    QString curr_str;

    curr_str = (error < 0 || error >= ARRAY_COUNT(err_str)) ?
                    QString("%1:%2").arg(gs_str_modbus_exceptional_error, QString::number(error))
                    : err_str[error];

    if((error < 0) || (QModbusDevice::NoError != error))
    {
        DIY_LOG(LOG_ERROR, curr_str);

        test_info_message_sig_handler(LOG_ERROR, curr_str);
    }
}

void Dialog::modbus_state_changed_sig_handler(QModbusDevice::State state)
{
    /*The following strings are in the same order with enum QModbusDevice::State*/
    static const char* state_str[] =
    {
        "The device is disconnected.",
        "The device is being connected.",
        "The device is connected to the Modbus network.",
        "The device is being closed.",
    };

    m_modbus_state = state;
    if(QModbusDevice::UnconnectedState == state || QModbusDevice::ConnectedState== state)
    {
        this->setCursor(Qt::ArrowCursor);
    }

    QString curr_str = (state < 0 || (int)state >= ARRAY_COUNT(state_str)) ?
                QString("%1:%2").arg(gs_str_modbus_unkonwn_state, QString::number(state))
              : state_str[state];

    DIY_LOG(LOG_INFO, curr_str);
    if(QModbusDevice::ConnectedState == state)
    {
        test_info_message_sig_handler(LOG_INFO, curr_str, true, Qt::darkGreen);

        m_self_reconnecting = false;
        if(m_asked_for_reconnecting)
        {
            m_asked_for_reconnecting = false;
            DIY_LOG(LOG_INFO, "user asks for reconnect, connected.");
            emit mb_reconnected_sig();
        }

        if(m_testing)
        {
            refresh_time_stat_display(false, false, true);
        }
    }
    else if(QModbusDevice::UnconnectedState == state)
    {
        test_info_message_sig_handler(LOG_ERROR, curr_str);

        if(m_testing && !m_test_paused)
        {
            if(m_asked_for_reconnecting)
            {
                DIY_LOG(LOG_INFO, "user asks for reconnect."
                                   " wait some time then emit reconnect sig.");
            }
            else
            {
                DIY_LOG(LOG_INFO, "modbus disconnected during testing."
                                   " wait some time then emit reconnect signal.");
            }

            m_self_reconnecting = true;
            m_reconn_wait_timer.start(g_sys_configs_block.mb_reconnect_wait_ms);
        }

        if(m_testing)
        {
            refresh_time_stat_display(false, false, true);
        }
    }
    else
    {
        test_info_message_sig_handler(LOG_INFO, curr_str);
    }

    refresh_butoons();
}

void Dialog::on_hvConnBtn_clicked()
{
    if(!m_hv_conn_params.valid)
    {
        QMessageBox::critical(this, "Error", gs_str_plz_set_valid_conn_params);
        return;
    }
    if(QModbusDevice::UnconnectedState != m_modbus_state)
    {
        QMessageBox::information(this, "", gs_str_modbus_not_in_disconnected_state);
        return;
    }
    ui->hvConnBtn->setDisabled(true);
    this->setCursor(Qt::WaitCursor);
    if(!modbus_connect())
    {
        QMessageBox::critical(this, "Error", gs_str_modbus_connect_err);
    }
}


void Dialog::on_hvDisconnBtn_clicked()
{
    if(QModbusDevice::ConnectedState != m_modbus_state)
    {
        QMessageBox::information(this, "", gs_str_modbus_not_in_connected_state);
        return;
    }
    ui->hvDisconnBtn->setDisabled(true);
    if(!modbus_disconnect())
    {
        QMessageBox::critical(this, "Error", gs_str_modbus_disconnect_err);
    }
}

void Dialog::record_header()
{
    /*test info*/
    REC_INFO_IN_FILE(m_curr_rec_file.fileName() << "\n\n");
    REC_INFO_IN_FILE(m_hv_conn_params.info_str << "\n");
    REC_INFO_IN_FILE(m_test_params.info_str << "\n");
    REC_INFO_IN_FILE(ui->testPsnLbl->text() << ui->testPsnLEdit->text() << "\n\n");

    ui->testInfoDisplayTxt->append("");
    test_info_message_sig_handler(LOG_INFO, QString("%1%2\n")
                                  .arg(gs_str_sep_line, gs_str_test_begin), true);

    /*table header*/
    QString hdr;
    hdr = QString("%1,%2,%3,").arg(gs_str_date, gs_str_time, gs_str_no);
    for(int idx = 0; idx < ARRAY_COUNT(m_mbregs_to_record); ++idx)
    {
        hdr += get_hv_mb_reg_str(m_mbregs_to_record[idx], CN_REG_NAME);
        hdr += ",";
    }
    REC_INFO_IN_FILE(hdr << "\n");
    append_str_with_color_and_weight(ui->testInfoDisplayTxt, hdr,
                                     m_txt_def_color, m_txt_def_font.weight());
}

void Dialog::on_startTestBtn_clicked()
{
    m_cfg_recorder.record_ui_configs(this, m_rec_ui_cfg_fin, m_rec_ui_cfg_fout);

    if(!m_test_params.valid)
    {
        QMessageBox::critical(this, "Error", gs_str_plz_set_valid_test_params);
        return;
    }

    if(QModbusDevice::ConnectedState != m_modbus_state)
    {
        QMessageBox::critical(this, "Error", gs_str_modbus_not_connected);
        return;
    }

    if(!m_hv_tester.init(&m_test_params, m_modbus_device, m_hv_conn_params.srvr_address))
    {
        QMessageBox::critical(this, "Error", gs_str_init_hvtester_err);
        return;
    }

    QString err_str;
    QString curr_dt_str = common_tool_get_curr_dt_str();
    m_curr_rec_folder_name = curr_dt_str  + "-" + gs_str_test_rec_name_sufx;
    m_curr_rec_file_name = m_curr_rec_folder_name + gs_str_test_rec_file_type;
    QString curr_path = QString("./") + m_curr_rec_folder_name;
    QString curr_file_path(curr_path + "/" + m_curr_rec_file_name);
    if(!mkpth_if_not_exists(curr_path))
    {
        err_str = QString("%1%2:%3").arg(gs_str_create_folder, g_str_fail, curr_path);
        DIY_LOG(LOG_ERROR, err_str);
        QMessageBox::critical(this, "Error", err_str);
        return;
    }
    m_curr_rec_file.setFileName(curr_file_path);
    if(!m_curr_rec_file.open(QFile::WriteOnly | QFile::Append))
    {
        err_str = QString("%1%2:%3").arg(gs_str_create_file, g_str_fail, curr_file_path);
        DIY_LOG(LOG_ERROR, err_str);
        QMessageBox::critical(this, "Error", err_str);
        return;
    }
    m_curr_txt_stream.setDevice(&m_curr_rec_file);
    record_header();

    reset_internal_flags();
    m_testing = true;
    refresh_butoons();

    m_test_judge.clear_judge_resut_strs();

    emit go_test_sig();

    refresh_time_stat_display(true, true);

    m_time_stat_timer.start(g_sys_configs_block.test_time_stat_grain_sec * 1000);
}

void Dialog::on_stopTestBtn_clicked()
{
    m_reconn_wait_timer.stop();
    m_time_stat_timer.stop();

    refresh_time_stat_display();
    emit stop_test_sig(TEST_END_ABORT_BY_USER);
}

void Dialog::test_info_message_sig_handler(LOG_LEVEL lvl, QString msg,
                                       bool always_rec, QColor set_color, int set_font_w )
{
    QColor text_color;
    int text_font_w;
    QString line(common_tool_get_curr_date_str() + ","
                 + common_tool_get_curr_time_str() + ",");
    line += ","; //number is null
    line += msg;

    if(always_rec || (lvl >= LOG_WARN))
    {
        REC_INFO_IN_FILE(line << "\n");
    }

    lvl = VALID_LOG_LVL(lvl) ? lvl : LOG_ERROR;
    text_color = set_color.isValid() ? set_color : gs_log_lvl_fonts_arr[lvl];
    text_font_w = (set_font_w > 0) ? set_font_w : m_txt_def_font.weight();

    append_str_with_color_and_weight(ui->testInfoDisplayTxt, line, text_color, text_font_w);

    if(LOG_ERROR == lvl)
    {
        refresh_time_stat_display(false, false, true);
    }
}

void Dialog::map_judge_result_to_style(judge_result_e_t judge_result, str_with_style_s_t &style_str)
{
    switch(judge_result)
    {
        case JUDGE_RESULT_UNKNOWN:
            style_str.color = Qt::darkGray;
            style_str.weight = QFont::DemiBold;
        break;

        case JUDGE_RESULT_TOO_LOW:
            style_str.color = QColor(255, 128, 0); //orange
            style_str.weight = QFont::Bold;
        break;

        case JUDGE_RESULT_TOO_HIGH:
            style_str.color = Qt::red;
            style_str.weight = QFont::Bold;
        break;

        case JUDGE_RESULT_REF:
            style_str.color = m_txt_def_color;
            style_str.weight = QFont::Bold;
        break;

        case JUDGE_RESULT_OK:
        default:
            style_str.color = m_txt_def_color;
            style_str.weight = m_txt_def_font.weight();
        break;
    }
}

void Dialog::rec_mb_regs_sig_handler(tester_op_enum_t op, mb_reg_val_map_t reg_val_map,
                                 int loop_idx, int round_idx)
{
    if(!m_testing)
    {
        DIY_LOG(LOG_WARN, QString("not in test, but still receive reported regs with"
                                  "option %1.").arg(op));
    }

    int idx = 0, base = 10;
    hv_mb_reg_e_t reg_no;
    QString disp_prefix_str(common_tool_get_curr_date_str() + ","
                 + common_tool_get_curr_time_str() + ",");
    disp_prefix_str += QString("%1%2%3%4%5%6,").
            arg(g_str_the_line_pron, QString::number(loop_idx), g_str_loop,
                g_str_the_line_pron, QString::number(round_idx), g_str_time_ci);

    QString line;

    if((0 == round_idx) && (0 != loop_idx) && (TEST_OP_SET_EXPO_TRIPLE == op))
    {//a new round, leave a blank line.
        line += "\n";
    }

    line += disp_prefix_str;

    if(TEST_OP_SET_EXPO_TRIPLE == op)
    {
        idx = 0;
        while(idx < ARRAY_COUNT(m_mbregs_to_record) && m_mbregs_to_record[idx] != VoltSet)
        {
            line += ",";
            ++idx;
        }
        line += QString::number(reg_val_map.value(VoltSet)) + ",";
        line += QString::number(reg_val_map.value(FilamentSet)) + ",";
        line += QString::number(reg_val_map.value(ExposureTime)) + ",";
        REC_INFO_IN_FILE(line << "\n");
        append_str_with_color_and_weight(ui->testInfoDisplayTxt, line,
                                         m_txt_def_color, m_txt_def_font.weight());

        return;
    }

    mb_reg_judge_result_list_t judge_result;
    m_test_judge.judge_mb_regs(reg_val_map, judge_result, disp_prefix_str);
    reset_judge_reg_ret_map();
    for(int idx = 0; idx < judge_result.count(); ++idx)
    {
        m_judge_reg_ret_map[judge_result[idx].val_reg] = judge_result[idx].judge_result;
        if(judge_result[idx].ref_reg != judge_result[idx].val_reg)
        {
            m_judge_reg_ret_map[judge_result[idx].ref_reg] = JUDGE_RESULT_REF;
        }
    }

    QString val_str;
    str_line_with_styles_t style_line;
    str_with_style_s_t style_str,
            last_style = {disp_prefix_str, m_txt_def_color, m_txt_def_font.weight()};
    str_with_style_s_t style_comma = {",", m_txt_def_color, m_txt_def_font.weight()};
    style_line.append(last_style);
    idx = 0;
    while(idx < ARRAY_COUNT(m_mbregs_to_record))
    {
        reg_no = m_mbregs_to_record[idx];
        base = (State == reg_no) ? 2 : 10;
        val_str = QString::number(reg_val_map.value(reg_no), base);
        line += val_str + ",";
        ++idx;

        style_str.str = val_str;
        map_judge_result_to_style(m_judge_reg_ret_map[reg_no], style_str);
        if(last_style.color == style_str.color && last_style.weight == style_str.weight)
        {
            if(last_style.color == style_comma.color && last_style.weight == style_comma.weight)
            {
                style_line.last().str += "," + style_str.str;
            }
            else
            {
                style_line.append(style_comma);
                style_line.append(style_str);
            }
        }
        else
        {
            style_line.append(style_comma);
            style_line.append(style_str);
            last_style.color = style_str.color; last_style.weight = style_str.weight;
        }
    }
    REC_INFO_IN_FILE(line << "\n");
    append_line_with_styles(ui->testInfoDisplayTxt, style_line);

    style_line.clear();
    /*
    append_str_with_color_and_weight(ui->testInfoDisplayTxt, line + "\n",
                                     m_txt_def_color, m_txt_def_font.weight());
     */
}

void Dialog::rec_judge_result(tester_end_code_enum_t code)
{
    QString header_str, title_str;
    const QStringList& judge_result_strs = m_test_judge.get_judge_result_strs();

    if(judge_result_strs.isEmpty() && (TEST_END_NORMAL != code))
    {
        DIY_LOG(LOG_INFO,
                QString("test result is empty, and end with abnormal code %1").arg(code));
        return;
    }

    title_str = QString(gs_str_test_judge_result) + ":";
    REC_INFO_IN_FILE("\n" << title_str << "\n");
    append_str_with_color_and_weight(ui->testInfoDisplayTxt, QString("\n%1").arg(title_str),
                                     m_txt_def_color, QFont::Bold);
    if(judge_result_strs.isEmpty() && (TEST_END_NORMAL == code))
    {
        REC_INFO_IN_FILE(QString(gs_str_test_pass) << "\n");
        append_str_with_color_and_weight(ui->testInfoDisplayTxt,
                                         QString(gs_str_test_pass) + "\n", Qt::green);
        return;
    }

    m_test_judge.get_result_disp_header_str(header_str);
    header_str.prepend(",,,"); //date,time and number
    REC_INFO_IN_FILE(header_str << "\n");
    append_str_with_color_and_weight(ui->testInfoDisplayTxt, header_str);
    for(int idx = 0; idx < judge_result_strs.count(); ++idx)
    {
        REC_INFO_IN_FILE(judge_result_strs[idx] << "\n");
        append_str_with_color_and_weight(ui->testInfoDisplayTxt,judge_result_strs[idx]);
    }
    ui->testInfoDisplayTxt->append("");
}

void Dialog::reset_time_stat_vars()
{
    m_expt_test_dura_ms = 0; m_expt_test_remain_dura_ms = 0;
    m_pause_cnt = 0;
    m_pause_dura_ms = 0;
    m_act_test_dura_ms = 0;
}

void Dialog::reset_internal_flags()
{
    m_testing = false;
    m_test_paused = false;
    m_self_reconnecting = false;
    m_asked_for_reconnecting = false;

    reset_time_stat_vars();
}

void Dialog::test_complete_sig_hanler(tester_end_code_enum_t code)
{
    if(!m_testing)
    {
        DIY_LOG(LOG_WARN,
                QString("not in test, but receive complete sig with code %1.").arg(code));
        return;
    }

    QString complete_str;
    Qt::GlobalColor color;
    QFont::Weight weight = QFont::Bold;

    m_reconn_wait_timer.stop();
    m_time_stat_timer.stop();
    refresh_time_stat_display();

    switch(code)
    {
    case TEST_END_NORMAL:
        complete_str += gs_str_test_complete;
        color = Qt::blue; weight = QFont::Bold;
        break;

    case TEST_END_ABORT_BY_USER:
        complete_str += gs_str_test_abort_by_user;
        color = Qt::darkYellow;
        break;

    case TEST_END_EXCEPTION:
    default:
        complete_str += gs_str_test_end_exception;
        color = Qt::red;
        break;
    }
    ui->testInfoDisplayTxt->append("");
    REC_INFO_IN_FILE("\n");
    test_info_message_sig_handler(LOG_INFO, complete_str, true, color, weight);

    rec_judge_result(code);
    REC_INFO_IN_FILE("\n");
    test_info_message_sig_handler(LOG_INFO, QString("%1%2\n")
                                  .arg(gs_str_sep_line, gs_str_test_end), true,
                                  m_txt_def_color, QFont::Bold);

    if(m_curr_rec_file.isOpen())
    {
        m_curr_txt_stream.flush();
        m_curr_rec_file.close();
    }

    reset_internal_flags();
    refresh_butoons();
}

void Dialog::mb_op_err_req_reconnect_sig_handler()
{
    DIY_LOG(LOG_INFO, "received modbus reconnect req from tester.");
    m_asked_for_reconnecting = true;
    if(!m_self_reconnecting)
    {
        DIY_LOG(LOG_INFO, "now emit reconnect sig on tester req.");
        auto_reconnect_sig_handler();
    }
    else
    {
        DIY_LOG(LOG_INFO, "self reconnecting is already working.");
    }
}

void Dialog::on_clearTestInfoBtn_clicked()
{
    ui->testInfoDisplayTxt->clear();
}

void Dialog::auto_reconnect_sig_handler()
{
    if(m_modbus_device)
    {
        m_modbus_state = m_modbus_device->state();
        switch(m_modbus_state)
        {
            case QModbusDevice::ClosingState:
                break;

            case QModbusDevice::UnconnectedState:
                m_modbus_device->connectDevice();
                break;

            case QModbusDevice::ConnectingState:
                break;

            case QModbusDevice::ConnectedState:
            default:
                m_modbus_device->disconnectDevice();
                break;
        }
    }
}

void Dialog::on_dsoSetBtn_clicked()
{
    QMessageBox::information(this, "", "功能尚未实现！");
}

void Dialog::reconn_wait_timer_sig_handler()
{
    m_self_reconnecting = true;
    if(m_modbus_device)
    {
        m_modbus_device->connectDevice();
    }
}

void Dialog::time_stat_timer_sig_handler()
{
    if(!m_testing) return;

    emit refresh_time_stat_display_sig();
}

void Dialog::on_Dialog_finished(int /*result*/)
{
    modbus_disconnect();
    m_cfg_recorder.record_ui_configs(this, m_rec_ui_cfg_fin, m_rec_ui_cfg_fout);
}


void Dialog::on_pauseTestBtn_clicked()
{
    bool pause_st = !m_test_paused;

    if(m_curr_rec_file.isOpen())
    {
        m_curr_txt_stream.flush();
    }

    emit pause_resume_test_sig(pause_st);

    QDateTime curr_dt = QDateTime::currentDateTime();
    if(pause_st)
    {
        m_pause_dura_check_point = curr_dt;
        ++m_pause_cnt;

        m_reconn_wait_timer.stop();

        refresh_time_stat_display();
    }
    else
    {
        int curr_pause_dura_ms = m_pause_dura_check_point.msecsTo(curr_dt);
        m_pause_dura_ms += curr_pause_dura_ms;

        if(m_modbus_device && (QModbusDevice::ConnectedState != m_modbus_device->state()))
        {
            m_modbus_device->connectDevice();
        }

        refresh_time_stat_display(false, false, true);
    }

    m_test_paused = pause_st;
    refresh_butoons();
}
