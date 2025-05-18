#include "logger/logger.h"
#include "main_dialog.h"
#include "ui_main_dialog.h"

#include <QMessageBox>

extern const char* g_str_plz_set_valid_conn_params;
extern const char* g_str_connected;
extern const char* g_str_disconnected;

static const char* gs_str_sport_open_fail = "串口打开失败";
static const char* gs_str_sport_already_opened = "串口已打开";
static const char* gs_str_plz_conn_dev_firstly = "请先连接设备";
static const char* gs_str_running = "运行";
static const char* gs_str_stopped = "停止";
static const char* gs_str_slp = "休眠";
static const char* gs_str_wkup = "唤醒";

static const char gs_addr_byte = 0x01;
static const char gs_dif_byte_motor_rpm = 0x03;
static const char gs_dif_byte_pwr_st = 0x04;
static const quint16 gs_pwr_chk_val = 0;
static const char gs_dif_byte_wkup_slp = 0x02;
static const quint16 gs_wkup_val = 0, gs_slp_val = 1;
static const qint64 gs_pwr_st_msg_len = 8;

#define CHECK_SPORT_OPEN(ret, silent) \
if(!m_pb_sport_open)\
{\
    if(!silent) QMessageBox::critical(this, "", gs_str_plz_conn_dev_firstly);\
    return ret;\
}

void Dialog::setup_pb_set_and_monitor()
{
    connect(&m_pb_sport, &QSerialPort::readyRead, this, &Dialog::pb_sport_data_received,
            Qt::QueuedConnection);
    connect(&m_pb_monitor_timer, &QTimer::timeout, this, &Dialog::pb_monitor_timer_hdlr,
            Qt::QueuedConnection);
    connect(this, &Dialog::pb_monitor_check_st, this, &Dialog::pb_monitor_check_st_hdlr,
            Qt::QueuedConnection);
}

void Dialog::pb_monitor_check_st_hdlr()
{
    QString log_str;
    LOG_LEVEL log_lvl = LOG_INFO;
    bool set_ok;
    char data_arr[] = {gs_addr_byte,
                    (char)((gs_pwr_chk_val >> 8) & 0xFF), (char)(gs_pwr_chk_val& 0xFF),
                    gs_dif_byte_pwr_st};
    int byte_cnt = ARRAY_COUNT(data_arr);

    set_ok = write_to_sport(data_arr, byte_cnt, true);
    if(set_ok)
    {
        int read_try_cnt = 3, idx = 0;
        char read_data[gs_pwr_st_msg_len];
        qint64 bytes_read_this_op = 0, total_bytes_read = 0;
        while(idx < read_try_cnt && total_bytes_read < gs_pwr_st_msg_len)
        {
            bytes_read_this_op = m_pb_sport.read(&read_data[total_bytes_read],
                                                 gs_pwr_st_msg_len - total_bytes_read);
            total_bytes_read += bytes_read_this_op;
            ++idx;
        }
        log_str += QString("bytes read on monitor: ");
        log_str += QByteArray::fromRawData(read_data,
                                           total_bytes_read > gs_pwr_st_msg_len ?
                                           gs_pwr_st_msg_len : total_bytes_read).toHex(' ').toUpper();
        log_str += "\n";
        do
        {
            if(total_bytes_read < gs_pwr_st_msg_len)
            {
                log_lvl = LOG_ERROR;
                log_str += "read power st bytes less than expected.";
                break;
            }
            if(read_data[0] != gs_addr_byte
                    || read_data[gs_pwr_st_msg_len - 1] != gs_dif_byte_pwr_st)
            {
                log_lvl = LOG_ERROR;
                log_str += "bytes array format error.\n";
                break;
            }
            break;
        }while(true);
    }
}

void Dialog::pb_monitor_timer_hdlr()
{
    emit pb_monitor_check_st();
}

void Dialog::pb_sport_data_received()
{}

void Dialog::on_pbConnPbt_clicked()
{
    if(!m_pb_conn_params.valid)
    {
        QMessageBox::critical(this, "", g_str_plz_set_valid_conn_params);
        return;
    }

    if(!m_pb_sport_open)
    {
        if (m_pb_sport.open(QIODevice::ReadWrite))
        {
            m_pb_sport_open = true;
            ui->pbConnStDispLbl->setText(g_str_connected);

            m_pb_monitor_timer.start(g_sys_configs_block.pb_monitor_period_ms);
        }
        else
        {
            QMessageBox::critical(this, "", gs_str_sport_open_fail);
        }
    }
    else
    {
        QMessageBox::critical(this, "", gs_str_sport_already_opened);
    }
}


void Dialog::on_pbDisconnPbt_clicked()
{
    if(!m_pb_conn_params.valid)
    {
        QMessageBox::critical(this, "", g_str_plz_set_valid_conn_params);
        return;
    }

    m_pb_sport.close();
    m_pb_sport_open = false;
    ui->pbConnStDispLbl->setText(g_str_disconnected);

    m_pb_monitor_timer.stop();
}

bool Dialog::write_to_sport(char* data_arr, qint64 byte_cnt, bool silent)
{
    bool set_ok = false;
    CHECK_SPORT_OPEN(set_ok, silent);

    QString log_str;
    LOG_LEVEL log_lvl;

    qint64 bytes_written;

    bytes_written = m_pb_sport.write(data_arr, byte_cnt);
    log_lvl = LOG_INFO;
    log_str = QString("Write data to %1: ")
            + QByteArray::fromRawData(data_arr, byte_cnt).toHex(' ').toUpper() + "\n";
    if(bytes_written < byte_cnt)
    {
        log_str += QString("Write data to %1 error: expect write %2 bytes, "
                          "but in fact written %3 bytes.")
                    .arg(m_pb_conn_params.com_port_s).arg(byte_cnt, bytes_written);
        log_lvl = LOG_ERROR;
    }
    else
    {
        set_ok = true;
    }
    DIY_LOG(log_lvl, log_str);
    append_str_with_color_and_weight(ui->testInfoDisplayTxt, log_str,
                                     g_log_lvl_fonts_arr[log_lvl]);
    return set_ok;
}

void Dialog::on_motorRPMSetPBtn_clicked()
{
    bool set_ok = true;

    quint16 rpm_val = (quint16)ui->motorRPMSpinBox->value();
    char data_arr[] = {gs_addr_byte,
                    (char)((rpm_val >> 8) & 0xFF), (char)(rpm_val & 0xFF),
                    gs_dif_byte_motor_rpm};
    int byte_cnt = ARRAY_COUNT(data_arr);

    set_ok = write_to_sport(data_arr, byte_cnt);

    if(set_ok)
    {
        /* updata motor run st on GUI. just set the st-display on write-ret.*/
        ui->motorRunStDispLbl->setText(rpm_val ? gs_str_running : gs_str_stopped);
    }
}


void Dialog::on_hostSleepPBtn_clicked()
{
    bool set_ok = true;

    char data_arr[] = {gs_addr_byte,
                    (char)((gs_slp_val >> 8) & 0xFF), (char)(gs_slp_val & 0xFF),
                    gs_dif_byte_wkup_slp};
    int byte_cnt = ARRAY_COUNT(data_arr);

    set_ok = write_to_sport(data_arr, byte_cnt);

    if(set_ok)
    {
        /* updata disp on GUI. just set the st-display on write-ret.*/
        ui->hostWorkStDispLbl->setText(gs_str_slp);
    }
}

void Dialog::on_hostWakeupPBtn_clicked()
{
    bool set_ok = true;

    char data_arr[] = {gs_addr_byte,
                    (char)((gs_wkup_val >> 8) & 0xFF), (char)(gs_wkup_val & 0xFF),
                    gs_dif_byte_wkup_slp};
    int byte_cnt = ARRAY_COUNT(data_arr);

    set_ok = write_to_sport(data_arr, byte_cnt);

    if(set_ok)
    {
        /* updata disp on GUI. just set the st-display on write-ret.*/
        ui->hostWorkStDispLbl->setText(gs_str_wkup);
    }
}
