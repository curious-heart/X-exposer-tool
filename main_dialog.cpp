#include <QMessageBox>
#include <QColor>

#include "logger/logger.h"
#include "main_dialog.h"
#include "ui_main_dialog.h"

static const char* gs_str_plz_set_valid_conn_params = "请首先设置有效的连接参数";
static const char* gs_str_plz_set_valid_test_params = "请首先设置有效的测试参数";
static const char* gs_str_init_fail = "初始化失败";
static const char* gs_str_modbus_already_connected = "modbus已连接";
static const char* gs_str_modbus_already_disconnected = "modbus连接已断开";
static const char* gs_str_modbus_exceptional_error = "异常的modbus错误";
static const char* gs_str_modbus_unkonwn_state = "未知的modbus连接状态";
static const char* gs_str_modbus_connect_err = "modbus连接失败";
static const char* gs_str_modbus_disconnect_err = "modbus断开连接失败";

Dialog::Dialog(QWidget *parent)
    : QDialog(parent)
    , ui(new Ui::Dialog)
{
    ui->setupUi(this);

    m_testParamSettingsDialog = new testParamSettingsDialog(this, &m_test_params);
    m_hvConnSettingsDialog = new hvConnSettings(this, &m_hv_conn_params);

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
}

Dialog::~Dialog()
{
    if(m_modbus_connected && m_modbus_device)
    {
        modbus_disconnect();
    }

    delete ui;
}

void Dialog::on_testParamSetBtn_clicked()
{
    m_test_params.valid = false;
    m_test_params.expo_param_block.expo_params.cust_params_arr.clear();
    m_test_params.info_str.clear();
    m_testParamSettingsDialog->exec();

    if(m_test_params.valid)
    {
        ui->testParamDisplayTxt->setText(m_test_params.info_str);
    }
}

void Dialog::on_hvConnSetBtn_clicked()
{
    m_hv_conn_params.valid = false;
    m_hv_conn_params.info_str.clear();

    m_hvConnSettingsDialog->exec();

    if(m_hv_conn_params.valid)
    {
        ui->hvConnParamDisplayTxt->setText(m_hv_conn_params.info_str);
    }
    select_modbus_device();
}

void Dialog::refresh_butoons()
{
    ui->hvConnSetBtn->setEnabled(!m_modbus_connected && !m_testing);
    ui->testParamSetBtn->setEnabled(!m_testing);
    ui->dsoSetBtn->setEnabled(!m_dso_connected && !m_testing);
    ui->hvConnBtn->setEnabled(!m_modbus_connected && !m_testing);
    ui->hvDisconnBtn->setEnabled(m_modbus_connected && !m_testing);
    ui->startTestBtn->setEnabled(m_modbus_connected && !m_testing);
    ui->stopTestBtn->setEnabled(m_modbus_connected && m_testing);
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
                    this, &Dialog::modbus_error_handler);
            connect(m_modbus_device, &QModbusClient::stateChanged,
                    this, &Dialog::modbus_state_changed_handler);
        }
    }
}

void Dialog::modbus_error_handler(QModbusDevice::Error error)
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
    QColor t_color;

    curr_str = (error < 0 || error >= ARRAY_COUNT(err_str)) ?
                    QString("%d:%2").arg(gs_str_modbus_exceptional_error, QString::number(error))
                    : err_str[error];

    if(QModbusDevice::NoError != error)
    {
        DIY_LOG(LOG_ERROR, curr_str);

        t_color = ui->testInfoDisplayTxt->textColor();
        ui->testInfoDisplayTxt->setTextColor(Qt::red);
        ui->testInfoDisplayTxt->append(curr_str);
        ui->testInfoDisplayTxt->setTextColor(t_color);
    }
}

void Dialog::modbus_state_changed_handler(QModbusDevice::State state)
{
    /*The following strings are in the same order with enum QModbusDevice::State*/
    static const char* state_str[] =
    {
        "The device is disconnected.",
        "The device is being connected.",
        "The device is connected to the Modbus network.",
        "The device is being closed.",
    };

    switch(state)
    {
    case QModbusDevice::ConnectedState:
        m_modbus_connected = true;
        break;

    default:
        m_modbus_connected = false;
        break;
    }

    refresh_butoons();

    QString curr_str;
    curr_str = (state < 0 || (int)state >= ARRAY_COUNT(state_str)) ?
                QString("%1:%2").arg(gs_str_modbus_unkonwn_state, QString::number(state))
              : state_str[state];
    ui->testInfoDisplayTxt->append(curr_str);
}

void Dialog::on_hvConnBtn_clicked()
{
    if(!m_hv_conn_params.valid)
    {
        QMessageBox::critical(this, "Error", gs_str_plz_set_valid_conn_params);
        return;
    }
    if(m_modbus_connected)
    {
        QMessageBox::information(this, "", gs_str_modbus_already_connected);
        return;
    }
    if(!modbus_connect())
    {
        QMessageBox::critical(this, "Error", gs_str_modbus_connect_err);
    }
}


void Dialog::on_hvDisconnBtn_clicked()
{
    if(!m_modbus_connected)
    {
        QMessageBox::information(this, "", gs_str_modbus_already_disconnected);
        return;
    }
    if(!modbus_disconnect())
    {
        QMessageBox::critical(this, "Error", gs_str_modbus_disconnect_err);
    }
}
