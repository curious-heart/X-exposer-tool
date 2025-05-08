#include <QThread>
#include <QFile>
#include <QDateTime>
#include <QMutexLocker>
#include <QMessageBox>

#include "logger/logger.h"
#include "main_dialog.h"
#include "ui_main_dialog.h"

#include "sc_data_proc.h"

static const char* gs_str_please_set_valid_ip_and_port = "请设置有效的IP地址和端口";

#undef RECV_DATA_NOTE_E
#define RECV_DATA_NOTE_E(e) #e
static const char* gs_recv_data_note_str [] = {RECV_DATA_NOTES};

void Dialog::on_dataCollStartPbt_clicked()
{
    if(!m_sc_data_conn_params.valid)
    {
        QMessageBox::critical(this, "", gs_str_please_set_valid_ip_and_port);
        return;
    }

    QString ip = m_sc_data_conn_params.ip_addr;
    quint16 port = m_sc_data_conn_params.port_no;
    int connTimeout = 3;
    int packetCount = ui->dataCollRowCntSpinbox->value();

    emit start_collect_sc_data(ip, port, connTimeout, packetCount);
}

void Dialog::on_dataCollStopPbt_clicked()
{
    emit stop_collect_sc_data();
}

void Dialog::collect_data_conn_timeout()
{
    DIY_LOG(LOG_WARN, "connect to scanner timeout.");
}

void Dialog::collect_data_disconn_timeout()
{
    DIY_LOG(LOG_WARN, "disconnect from scanner timeout.");
}

void Dialog::handleNewDataReady()
{
    recv_data_with_notes_s_t packet;
    {
        QMutexLocker locker(&queueMutex);
        packet = dataQueue.dequeue();
    }

    QString data_str = log_disp_prepender_str();

    data_str += QString(gs_recv_data_note_str[packet.notes]) + ":";
    data_str += packet.data.toHex(' ').toUpper();

    append_str_with_color_and_weight(ui->testInfoDisplayTxt, data_str,
                                     m_txt_def_color, m_txt_def_font.weight());
}
