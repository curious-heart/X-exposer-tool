#include <QSerialPort>
#include <QSerialPortInfo>
#include <QMessageBox>

#include "common_tool_func.h"
#include "logger/logger.h"
#include "hv_connsettings.h"
#include "ui_hv_connsettings.h"

static const char* gs_str_no_available_com = "无可用串口";
static const char* gs_str_should_be_p_int = "应为正整数";
extern const char* gs_str_data_item_invalid;

const hvConnSettings::combobox_item_struct_t hvConnSettings::hv_conn_type_list[] =
{
    {CONN_TYPE_SERIAL, "串口"},
    {CONN_TYPE_TCPIP, "TCPIP", true},
};

const hvConnSettings::combobox_item_struct_t hvConnSettings::baudrate_list[] =
{
    {QSerialPort::Baud1200 ,"1200"},
    {QSerialPort::Baud2400 ,"2400"},
    {QSerialPort::Baud4800 ,"4800"},
    {QSerialPort::Baud9600 ,"9600", true},
    {QSerialPort::Baud19200 ,"19200"},
    {QSerialPort::Baud38400 ,"38400"},
    {QSerialPort::Baud57600 ,"57600"},
    {QSerialPort::Baud115200 ,"115200"},
};

const hvConnSettings::combobox_item_struct_t hvConnSettings::data_bits_list[] =
{
    {QSerialPort::Data7, "7"},
    {QSerialPort::Data8, "8", true},
};

const hvConnSettings::combobox_item_struct_t hvConnSettings::check_parity_list[] =
{
    {QSerialPort::NoParity ,"无校验", true},
    {QSerialPort::EvenParity ,"偶校验"},
    {QSerialPort::OddParity ,"奇校验"},
};

const hvConnSettings::combobox_item_struct_t hvConnSettings::stop_bit_list[] =
{
    {QSerialPort::OneStop, "1", true},
    {QSerialPort::TwoStop, "2"},
};


hvConnSettings::hvConnSettings(QWidget *parent, modbus_conn_parameters_struct_t* param_ptr) :
    QDialog(parent),
    ui(new Ui::hvConnSettings),
    modbus_conn_params(param_ptr)
{
    ui->setupUi(this);

    typedef struct
    {
        QComboBox * ctrl;
        const combobox_item_struct_t * list;
        int cnt;
    }combobox_list_arr_struct_t;
    const combobox_list_arr_struct_t const_combobox_list[] =
    {
        {ui->modBusConnTypeComboBox, hv_conn_type_list, ARRAY_COUNT(hv_conn_type_list)},
        {ui->COMBaudrateComBox, baudrate_list, ARRAY_COUNT(baudrate_list)},
        {ui->COMDataBitsComBox, data_bits_list, ARRAY_COUNT(data_bits_list)},
        {ui->COMCheckBitComBox, check_parity_list, ARRAY_COUNT(check_parity_list)},
        {ui->COMStopBitComBox, stop_bit_list, ARRAY_COUNT(stop_bit_list)},
    };
    for(int idx = 0; idx < ARRAY_COUNT(const_combobox_list); ++idx)
    {
        QComboBox * ctrl;
        int def_idx = 0;
        ctrl = const_combobox_list[idx].ctrl;
        for(int i = 0; i < const_combobox_list[idx].cnt; ++i)
        {
            ctrl->addItem(const_combobox_list[idx].list[i].s,const_combobox_list[idx].list[i].val);
            if(const_combobox_list[idx].list[i].is_default) def_idx = i;
        }
        ctrl->setCurrentIndex(def_idx);
    }

    foreach (QSerialPortInfo info, QSerialPortInfo::availablePorts())
    {
        ui->COMNumComBox->addItem(info.portName());
    }
    ui->COMNumComBox->setCurrentIndex(0);

    select_conn_type_param_block();
}

hvConnSettings::~hvConnSettings()
{
    delete ui;
}

void hvConnSettings::select_conn_type_param_block()
{
    hv_conn_type_enum_t conn_type
            = (hv_conn_type_enum_t)(ui->modBusConnTypeComboBox->currentData().toInt());
    if(CONN_TYPE_SERIAL == conn_type)
    {
        ui->serialGroupBox->setEnabled(true);
        ui->tcpipGroupBox->setEnabled(false);
    }
    else
    {
        ui->serialGroupBox->setEnabled(false);
        ui->tcpipGroupBox->setEnabled(true);
    }
}

void hvConnSettings::on_modBusConnTypeComboBox_currentIndexChanged(int /*index*/)
{
    select_conn_type_param_block();
}

QString hvConnSettings::collect_conn_params()
{
    QString ret_str;
    hv_conn_type_enum_t conn_type
            = (hv_conn_type_enum_t)(ui->modBusConnTypeComboBox->currentData().toInt());

    if(!modbus_conn_params)
    {
        DIY_LOG(LOG_ERROR, "conn params ptr is NULL.");
        return ret_str;
    }

    modbus_conn_params->valid = false;
    modbus_conn_params->info_str.clear();
    bool tr_ok;
    modbus_conn_params->resp_wait_time_ms = ui->modBusRespTimeoutEdit->text().toInt(&tr_ok);
    if(!tr_ok || modbus_conn_params->resp_wait_time_ms <= 0)
    {
        ret_str += ui->modBusRespTimeoutLbl->text() + gs_str_should_be_p_int;
        return ret_str;
    }

    modbus_conn_params->type = conn_type;
    if(CONN_TYPE_SERIAL == conn_type)
    {
        modbus_conn_params->serial_params.com_port_s = ui->COMNumComBox->currentText();
        if(modbus_conn_params->serial_params.com_port_s.isEmpty())
        {
            ret_str += gs_str_no_available_com;
            return ret_str;
        }
        modbus_conn_params->serial_params.boudrate
                = ui->COMBaudrateComBox->currentData().toInt();
        modbus_conn_params->serial_params.data_bits
                = ui->COMDataBitsComBox->currentData().toInt();
        modbus_conn_params->serial_params.check_parity
                = ui->COMCheckBitComBox->currentData().toInt();
        modbus_conn_params->serial_params.stop_bit
                = ui->COMStopBitComBox->currentData().toInt();

        modbus_conn_params->valid = true;
    }
    else
    {
        modbus_conn_params->tcpip_params.port_no = ui->srvrPortEdit->text().toUInt(&tr_ok);
        if(!tr_ok || modbus_conn_params->tcpip_params.port_no < 0 ||
                     modbus_conn_params->tcpip_params.port_no > 65535)
        {
            ret_str += ui->srvrPortLbl->text() + gs_str_data_item_invalid;
            return ret_str;
        }

        modbus_conn_params->tcpip_params.expire_time_ms
                = ui->connSrvrTimeoutEdit->text().toInt(&tr_ok);
        if(!tr_ok)
        {
            ret_str += ui->connSrvrTimeoutLbl->text() + gs_str_should_be_p_int;
            return ret_str;
        }

        modbus_conn_params->tcpip_params.ip_addr = ui->srvrIPEdit->text();

        modbus_conn_params->valid = true;
    }

    if(modbus_conn_params->valid)
    {
        format_hv_conn_info_str();
    }

    return ret_str;
}

void hvConnSettings::format_hv_conn_info_str()
{
    if(!modbus_conn_params)
    {
        DIY_LOG(LOG_ERROR, "conn params ptr is NULL.");
        return;
    }

    hv_conn_type_enum_t conn_type
            = (hv_conn_type_enum_t)(ui->modBusConnTypeComboBox->currentData().toInt());

    QString &info_str = modbus_conn_params->info_str;

    info_str.clear();

    info_str += ui->modBusConnTypeLbl->text() + ":"
                + ui->modBusConnTypeComboBox->currentText() + "\n";
    if(CONN_TYPE_SERIAL == conn_type)
    {
        info_str += ui->COMNumLbl->text() + ":" + ui->COMNumComBox->currentText() + "\n";
        info_str += ui->COMBaudrateLbl->text() + ":" + ui->COMBaudrateComBox->currentText() + "\n";
        info_str += ui->COMDataBitsLbl->text() + ":" + ui->COMDataBitsComBox->currentText() + "\n";
        info_str += ui->COMCheckBitLbl->text() + ":" + ui->COMCheckBitComBox->currentText() + "\n";
        info_str += ui->COMStopBitLbl->text() + ":" + ui->COMStopBitComBox->currentText() + "\n";
    }
    else
    {
        info_str += ui->srvrIPLbl->text() + ":" + ui->srvrIPEdit->text() + "\n";
        info_str += ui->srvrPortLbl->text() + ":" + ui->srvrPortEdit->text() + "\n";
        info_str += ui->connSrvrTimeoutLbl->text() + ":" + ui->connSrvrTimeoutEdit->text() + "\n";
    }
    info_str += ui->modBusRespTimeoutLbl->text() + ":" + ui->modBusRespTimeoutEdit->text() + "\n";
}

void hvConnSettings::on_buttonBox_clicked(QAbstractButton *button)
{
    if(!modbus_conn_params)
    {
        DIY_LOG(LOG_ERROR, "conn params ptr is NULL.");
        return;
    }
    if(button == ui->buttonBox->button(QDialogButtonBox::Ok))
    {
        QString ret_str;
        ret_str = collect_conn_params();
        if(modbus_conn_params->valid)
        {
            accept();
        }
        else
        {
            QMessageBox::critical(this, "Error", ret_str);
        }
    }
}

