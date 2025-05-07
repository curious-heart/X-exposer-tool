#include <QMessageBox>
#include <QSerialPort>
#include <QSerialPortInfo>

#include "logger/logger.h"
#include "common_tools/common_tool_func.h"
#include "serialportsetdlg.h"
#include "ui_serialportsetdlg.h"

static const char* gs_str_no_available_com = "无可用串口";

const SerialPortSetDlg::combobox_item_struct_t SerialPortSetDlg::baudrate_list[] =
{
    {QSerialPort::Baud1200 ,"1200"},
    {QSerialPort::Baud2400 ,"2400"},
    {QSerialPort::Baud4800 ,"4800"},
    {QSerialPort::Baud9600 ,"9600"},
    {QSerialPort::Baud19200 ,"19200"},
    {QSerialPort::Baud38400 ,"38400"},
    {QSerialPort::Baud57600 ,"57600"},
    {QSerialPort::Baud115200 ,"115200"},
};

const SerialPortSetDlg::combobox_item_struct_t SerialPortSetDlg::databits_list[] =
{
    {QSerialPort::Data7, "7"},
    {QSerialPort::Data8, "8"},
};

const SerialPortSetDlg::combobox_item_struct_t SerialPortSetDlg::parity_list[] =
{
    {QSerialPort::NoParity ,"无校验"},
    {QSerialPort::EvenParity ,"偶校验"},
    {QSerialPort::OddParity ,"奇校验"},
};

const SerialPortSetDlg::combobox_item_struct_t SerialPortSetDlg::stopbits_list[] =
{
    {QSerialPort::OneStop, "1"},
    {QSerialPort::TwoStop, "2"},
};

SerialPortSetDlg::SerialPortSetDlg(QWidget *parent,
                                   serial_port_conn_params_struct_t * conn_params,
                                   UiConfigRecorder * cfg_recorder ,
                                   QString dlg_id_str, QString dlg_name_str,
                                   QSerialPort::BaudRate baudrate,
                                   QSerialPort::DataBits databits,
                                   QSerialPort::Parity parity,
                                   QSerialPort::StopBits stopbits) :
    QDialog(parent), ui(new Ui::SerialPortSetDlg),
    m_conn_params(conn_params),
    m_dlg_id_str(dlg_id_str), m_dlg_name_str(dlg_name_str),
    m_def_baudrate(baudrate), m_def_databits(databits), m_def_parity(parity),
    m_def_stopbits(stopbits),
    m_cfg_recorder(cfg_recorder)
{
    ui->setupUi(this);

    typedef struct
    {
        QComboBox * ctrl;
        const combobox_item_struct_t * list;
        int cnt, def_val;
    }combobox_list_arr_struct_t;

    const combobox_list_arr_struct_t const_combobox_list[] =
    {
        {ui->sPortBaudrateCBox, baudrate_list, ARRAY_COUNT(baudrate_list), m_def_baudrate},
        {ui->sPortDatabitsCBox, databits_list, ARRAY_COUNT(databits_list), m_def_databits},
        {ui->sPortParityCBox, parity_list, ARRAY_COUNT(parity_list), m_def_parity},
        {ui->sPortStopbitsCBox, stopbits_list, ARRAY_COUNT(stopbits_list), m_def_stopbits},
    };
    for(int idx = 0; idx < ARRAY_COUNT(const_combobox_list); ++idx)
    {
        QComboBox * ctrl;
        int def_idx = 0;
        ctrl = const_combobox_list[idx].ctrl;
        for(int i = 0; i < const_combobox_list[idx].cnt; ++i)
        {
            ctrl->addItem(const_combobox_list[idx].list[i].s,const_combobox_list[idx].list[i].val);
            if(const_combobox_list[idx].list[i].val == const_combobox_list[idx].def_val)
            {
                def_idx = i;
            }
        }
        ctrl->setCurrentIndex(def_idx);
    }

    foreach (QSerialPortInfo info, QSerialPortInfo::availablePorts())
    {
        ui->sPortCOMNumCBox->addItem(info.portName());
    }
    ui->sPortCOMNumCBox->setCurrentIndex(0);

    setWindowTitle(m_dlg_name_str);

    m_rec_ui_cfg_fin.clear();
    m_rec_ui_cfg_fout.clear();
    if(m_cfg_recorder)
    {
        m_cfg_recorder->load_configs_to_ui(this, m_rec_ui_cfg_fin, m_rec_ui_cfg_fout);
    }
}

SerialPortSetDlg::~SerialPortSetDlg()
{
    delete ui;
}

void SerialPortSetDlg::on_sPortButtonBox_clicked(QAbstractButton *button)
{
    if(!m_conn_params) return;

    if(button == ui->sPortButtonBox->button(QDialogButtonBox::Ok))
    {
        QString ret_str = collect_conn_params();

        if(m_conn_params->valid)
        {
            if(m_cfg_recorder) m_cfg_recorder->record_ui_configs(this,
                                                         m_rec_ui_cfg_fin, m_rec_ui_cfg_fout);
            accept();
        }
        else
        {
            QMessageBox::critical(this, "Error", ret_str);
        }
    }
}

QString SerialPortSetDlg::collect_conn_params()
{
    QString ret_str;

    if(!m_conn_params)
    {
        DIY_LOG(LOG_ERROR, "conn params ptr is NULL.");
        return ret_str;
    }

    m_conn_params->valid = false;
    m_conn_params->info_str.clear();

    m_conn_params->com_port_s = ui->sPortCOMNumCBox->currentText();
    if(m_conn_params->com_port_s.isEmpty())
    {
        ret_str += gs_str_no_available_com;
        return ret_str;
    }
    m_conn_params->boudrate = ui->sPortBaudrateCBox->currentData().toInt();
    m_conn_params->databits = ui->sPortDatabitsCBox->currentData().toInt();
    m_conn_params->parity = ui->sPortParityCBox->currentData().toInt();
    m_conn_params->stopbits = ui->sPortStopbitsCBox->currentData().toInt();

    m_conn_params->valid = true;

    format_params_info_str();

    return ret_str;
}

void SerialPortSetDlg::format_params_info_str()
{
    if(!m_conn_params)
    {
        DIY_LOG(LOG_ERROR, "conn params ptr is NULL.");
        return;
    }

    QString &info_str = m_conn_params->info_str;

    info_str.clear();

    info_str += ui->sPortCOMNumLbl->text() + ":" + ui->sPortCOMNumCBox->currentText() + "\n";
    info_str += ui->sPortBaudrateLbl->text() + ":" + ui->sPortBaudrateCBox->currentText() + "\n";
    info_str += ui->sPortDatabitsLbl->text() + ":" + ui->sPortDatabitsCBox->currentText() + "\n";
    info_str += ui->sPortParityLbl->text() + ":" + ui->sPortParityCBox->currentText() + "\n";
    info_str += ui->sPortStopbitsLbl->text() + ":" + ui->sPortStopbitsCBox->currentText() + "\n";
}
