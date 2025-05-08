#include <QMessageBox>
#include <QRegExpValidator>

#include "logger/logger.h"

#include "sc_data_connsettings.h"
#include "ui_sc_data_connsettings.h"

extern const char* gs_str_data_item_invalid;

sc_data_connsettings::sc_data_connsettings(QWidget *parent,
                                           sc_data_conn_params_struct_t * conn_params,
                                           UiConfigRecorder * cfg_recorder) :
    QDialog(parent), ui(new Ui::sc_data_connsettings),
    m_conn_params(conn_params), m_cfg_recorder(cfg_recorder)
{
    ui->setupUi(this);

    QRegExp ipRegex("^(([0-9]|[1-9][0-9]|1[0-9]{2}|2[0-4][0-9]|25[0-5])\\.){3}([0-9]|[1-9][0-9]|1[0-9]{2}|2[0-4][0-9]|25[0-5])$");
    QRegExpValidator *ipValidator = new QRegExpValidator(ipRegex, this);
    ui->dataCollIPAddrLineEdit->setValidator(ipValidator);

    m_rec_ui_cfg_fin.clear();
    m_rec_ui_cfg_fout.clear();
    if(m_cfg_recorder) m_cfg_recorder->load_configs_to_ui(this, m_rec_ui_cfg_fin, m_rec_ui_cfg_fout);
}

sc_data_connsettings::~sc_data_connsettings()
{
    delete ui;
}

QString sc_data_connsettings::collect_conn_params()
{
    bool tr_ok;
    QString ret_str;

    m_conn_params->port_no = ui->dataCollNetPortLineEdit->text().toUInt(&tr_ok);
    if(!tr_ok || m_conn_params->port_no < 0 || m_conn_params->port_no > 65535)
    {
        ret_str += ui->dataCollNetPortLineEdit->text() + gs_str_data_item_invalid;
        return ret_str;
    }

    m_conn_params->ip_addr = ui->dataCollIPAddrLineEdit->text();

    m_conn_params->valid = true;

    format_sc_data_conn_info_str();

    return ret_str;
}

void sc_data_connsettings::format_sc_data_conn_info_str()
{
    if(!m_conn_params)
    {
        DIY_LOG(LOG_ERROR, "conn params ptr is NULL.");
        return;
    }

    QString &info_str = m_conn_params->info_str;

    info_str.clear();

    info_str += ui->dataCollIPAddrLbl->text() + ":" + ui->dataCollIPAddrLineEdit->text() + "\n";
    info_str += ui->dataCollNetPortLbl->text() + ":" + ui->dataCollNetPortLineEdit->text() + "\n";
}

void sc_data_connsettings::on_scDataConnButtonBox_clicked(QAbstractButton *button)
{
    if(!m_conn_params) return;

    if(button == ui->scDataConnButtonBox->button(QDialogButtonBox::Ok))
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

