#include <QMessageBox>
#include <QColor>

#include "logger/logger.h"
#include "main_dialog.h"
#include "ui_main_dialog.h"
#include "sysconfigs/sysconfigs.h"

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

extern const char* g_str_loop;
extern const char* g_str_time_ci;
extern const char* g_str_the_line_pron;

static const char* gs_cfg_recorder_file_fpn = ".cfg_settings.ini";

/*设置管电压、设置管电流、曝光时间必须连续放置*/
const hv_mb_reg_e_t Dialog::m_mbregs_to_record[] =
{
    HSV, VoltSet, FilamentSet, ExposureTime, Voltmeter, Ammeter, EXT_MB_REG_DISTANCE,
    State, ExposureStatus, BatteryLevel, BatteryVoltmeter, OilBoxTemperature,
    Workstatus, exposureCount,
};

Dialog::Dialog(QWidget *parent)
    : QDialog(parent)
    , ui(new Ui::Dialog)
    , m_hv_tester(this)
    , m_cfg_recorder(this, gs_cfg_recorder_file_fpn)
{
    ui->setupUi(this);

    fill_sys_configs();

    m_testParamSettingsDialog = new testParamSettingsDialog(this, &m_test_params,
                                                            &m_cfg_recorder);
    m_hvConnSettingsDialog = new hvConnSettings(this, &m_hv_conn_params, &m_cfg_recorder);

    if(!m_testParamSettingsDialog || !m_hvConnSettingsDialog)
    {
        DIY_LOG(LOG_ERROR, "new setting dialog fail!");
        QMessageBox::critical(this, "Error", gs_str_init_fail);
        return;
    }

    m_testParamSettingsDialog->collect_test_params();
    if(m_test_params.valid)
    {
        ui->testParamDisplayTxt->setText(m_test_params.info_str);
    }
    m_hvConnSettingsDialog->collect_conn_params();
    if(m_hv_conn_params.valid)
    {
        ui->hvConnParamDisplayTxt->setText(m_hv_conn_params.info_str);
    }
    select_modbus_device();

    refresh_butoons();

    connect(this, &Dialog::go_test_sig,
            &m_hv_tester, &HVTester::go_test_sig_handler, Qt::QueuedConnection);
    connect(this, &Dialog::stop_test_sig,
            &m_hv_tester, &HVTester::stop_test_sig_handler, Qt::QueuedConnection);
    connect(this, &Dialog::mb_reconnected_sig,
            &m_hv_tester, &HVTester::mb_reconnected_sig_handler, Qt::QueuedConnection);

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

    m_txt_def_color = ui->testInfoDisplayTxt->textColor();
    m_txt_def_font = ui->testInfoDisplayTxt->currentFont();
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
    delete ui;
}

void Dialog::on_testParamSetBtn_clicked()
{
    m_testParamSettingsDialog->exec();

    if(m_test_params.valid)
    {
        ui->testParamDisplayTxt->setText(m_test_params.info_str);
    }
}

void Dialog::on_hvConnSetBtn_clicked()
{
    m_hvConnSettingsDialog->exec();

    if(m_hv_conn_params.valid)
    {
        ui->hvConnParamDisplayTxt->setText(m_hv_conn_params.info_str);
    }
    select_modbus_device();
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
    ui->stopTestBtn->setEnabled(m_testing);
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
                m_modbus_device  =  new QModbusTcpClient(this) ;
                m_modbus_device->setConnectionParameter(QModbusDevice::NetworkAddressParameter,
                                                        m_hv_conn_params.tcpip_params.ip_addr);
                m_modbus_device->setConnectionParameter(QModbusDevice::NetworkPortParameter,
                                                        m_hv_conn_params.tcpip_params.port_no);
            }
            else
            {
                m_modbus_device = new QModbusRtuSerialMaster(this);
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

            connect(m_modbus_device, &QModbusClient::errorOccurred,
                    this, &Dialog::modbus_error_sig_handler, Qt::QueuedConnection);
            connect(m_modbus_device, &QModbusClient::stateChanged,
                    this, &Dialog::modbus_state_changed_sig_handler, Qt::QueuedConnection);
        }
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
                    QString("%d:%2").arg(gs_str_modbus_exceptional_error, QString::number(error))
                    : err_str[error];

    if((error < 0) || (QModbusDevice::NoError != error))
    {
        DIY_LOG(LOG_ERROR, curr_str);

        append_str_with_color_and_weight(ui->testInfoDisplayTxt, curr_str, Qt::red);
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

    QString curr_str;
    curr_str = (state < 0 || (int)state >= ARRAY_COUNT(state_str)) ?
                QString("%1:%2").arg(gs_str_modbus_unkonwn_state, QString::number(state))
              : state_str[state];

    DIY_LOG(LOG_INFO, curr_str);
    if(QModbusDevice::ConnectedState == state)
    {
        append_str_with_color_and_weight(ui->testInfoDisplayTxt, curr_str, Qt::darkGreen);

        m_self_reconnecting = false;
        if(m_asked_for_reconnecting)
        {
            m_asked_for_reconnecting = false;
            DIY_LOG(LOG_INFO, "user asks for reconnect, connected.");
            emit mb_reconnected_sig();
        }
    }
    else if(QModbusDevice::UnconnectedState == state)
    {
        append_str_with_color_and_weight(ui->testInfoDisplayTxt, curr_str, Qt::darkGray);

        if(m_testing)
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

            QThread::msleep(g_sys_configs_block.mb_reconnect_wait_ms);
            m_self_reconnecting = true;
            emit auto_reconnect_sig();
        }
    }
    else
    {
        append_str_with_color_and_weight(ui->testInfoDisplayTxt, curr_str,
                                         m_txt_def_color, m_txt_def_font.weight());
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
    m_curr_txt_stream << m_curr_rec_file.fileName() << "\n\n";
    m_curr_txt_stream << m_hv_conn_params.info_str << "\n";
    m_curr_txt_stream << m_test_params.info_str << "\n\n";

    /*table header*/
    QString hdr;
    hdr = QString("%1,%2,%3,").arg(gs_str_date, gs_str_time, gs_str_no);
    for(int idx = 0; idx < ARRAY_COUNT(m_mbregs_to_record); ++idx)
    {
        hdr += get_hv_mb_reg_str(m_mbregs_to_record[idx], CN_REG_NAME);
        hdr += ",";
    }
    m_curr_txt_stream << hdr << "\n";
    append_str_with_color_and_weight(ui->testInfoDisplayTxt, hdr,
                                     m_txt_def_color, m_txt_def_font.weight());
}

void Dialog::on_startTestBtn_clicked()
{
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

    m_testing = true;
    m_self_reconnecting = false;
    m_asked_for_reconnecting = false;
    refresh_butoons();
    emit go_test_sig();
}

void Dialog::on_stopTestBtn_clicked()
{
    emit stop_test_sig(TEST_END_ABORT_BY_USER);
    test_complete_sig_hanler(TEST_END_ABORT_BY_USER);
}

void Dialog::test_info_message_sig_handler(LOG_LEVEL lvl, QString msg)
{
    if(lvl >= LOG_WARN)
    {
        QString line(common_tool_get_curr_date_str() + ","
                     + common_tool_get_curr_time_str() + ",");
        line += ","; //number is null
        line += msg + "\n";
        m_curr_txt_stream << line;

        if(LOG_ERROR == lvl)
        {
            append_str_with_color_and_weight(ui->testInfoDisplayTxt, line, Qt::red);
        }
        else
        {
            append_str_with_color_and_weight(ui->testInfoDisplayTxt, line,
                                             m_txt_def_color, m_txt_def_font.weight());
        }
    }
}

void Dialog::rec_mb_regs_sig_handler(tester_op_enum_t op, mb_reg_val_map_t reg_val_map,
                                 int loop_idx, int round_idx)
{
    int idx = 0, base = 10;
    hv_mb_reg_e_t reg_no;
    QString line(common_tool_get_curr_date_str() + ","
                 + common_tool_get_curr_time_str() + ",");
    line += QString("%1%2%3%4%5%6,").
            arg(g_str_the_line_pron, QString::number(loop_idx), g_str_loop,
                g_str_the_line_pron, QString::number(round_idx), g_str_time_ci);
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
        m_curr_txt_stream << line << "\n";
        append_str_with_color_and_weight(ui->testInfoDisplayTxt, line,
                                         m_txt_def_color, m_txt_def_font.weight());

        return;
    }
    idx = 0;
    while(idx < ARRAY_COUNT(m_mbregs_to_record))
    {
        reg_no = m_mbregs_to_record[idx];
        base = (State == reg_no) ? 2 : 10;
        line += QString::number(reg_val_map.value(reg_no), base) + ",";
        ++idx;
    }
    m_curr_txt_stream << line << "\n";
    append_str_with_color_and_weight(ui->testInfoDisplayTxt, line,
                                     m_txt_def_color, m_txt_def_font.weight());
}

void Dialog::test_complete_sig_hanler(tester_end_code_enum_t code)
{
    QString complete_str(common_tool_get_curr_date_str() + ","
                 + common_tool_get_curr_time_str() + ",");
    Qt::GlobalColor color;
    QFont::Weight weight = QFont::Bold;

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

    append_str_with_color_and_weight(ui->testInfoDisplayTxt, complete_str, color, weight);
    append_str_with_color_and_weight(ui->testInfoDisplayTxt, QString(gs_str_sep_line) + "\n\n",
                                     m_txt_def_color, m_txt_def_font.weight());

    m_testing = false;
    m_self_reconnecting = false;
    m_asked_for_reconnecting = false;
    refresh_butoons();

    if(m_curr_rec_file.isOpen())
    {
        m_curr_txt_stream << complete_str << "\n" << gs_str_sep_line << "\n";
        m_curr_txt_stream.flush();
        m_curr_rec_file.close();
    }
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
        if(QModbusDevice::ConnectedState == m_modbus_state)
        {
            m_modbus_device->disconnectDevice();
        }
        m_modbus_device->connectDevice();
    }
}

void Dialog::on_dsoSetBtn_clicked()
{
    QMessageBox::information(this, "", "功能尚未实现！");
}

