#include <QThread>
#include <QFile>
#include <QDateTime>
#include <QMutexLocker>
#include <QMessageBox>
#include <QVector>
#include <QTimer>

#include "logger/logger.h"
#include "main_dialog.h"
#include "ui_main_dialog.h"

#include "sc_data_proc.h"

static const char* gs_str_please_set_valid_ip_and_port = "请设置有效的IP地址和端口";
static const char* gs_str_connected = "连接";
static const char* gs_str_disconnected = "断开";
static const char* gs_str_sc_data_test = "抓图测试";
static const char* gs_str_sc_data_test_rec_file_type = ".txt";

const char* g_str_unkonw_st = "未知状态";

extern const char* g_str_create_file;
extern const char* g_str_fail;
extern const char* g_str_test_rec_name_sufx;
extern const char* g_str_create_folder;

#undef RECV_DATA_NOTE_E
#define RECV_DATA_NOTE_E(e) #e
static const char* gs_recv_data_note_str [] = {RECV_DATA_NOTES};

void Dialog::setup_sig_hdlr_for_sc_data()
{
    qRegisterMetaType<collect_rpt_evt_e_t>();

    connect(this, &Dialog::start_collect_sc_data,
            recv_data_worker, &RecvScannedData::start_collect_sc_data, Qt::QueuedConnection);
    connect(this, &Dialog::stop_collect_sc_data,
            recv_data_worker, &RecvScannedData::stop_collect_sc_data, Qt::QueuedConnection);
    connect(recv_data_worker, &RecvScannedData::new_data_ready,
            this, &Dialog::handleNewDataReady, Qt::QueuedConnection);
    connect(recv_data_worker, &RecvScannedData::recv_worker_report_sig,
            this, &Dialog::recv_worker_report_sig_hdlr, Qt::QueuedConnection);

    connect(&m_expo_to_coll_delay_timer, &QTimer::timeout,
            this, &Dialog::expo_to_coll_delay_timer_hdlr, Qt::QueuedConnection);
}

void Dialog::setup_sc_data_curv_wnd()
{
    CurvePlotWidget*window = nullptr;
    QString id_arr[] = {m_ch1_wnd_str_id, m_ch2_wnd_str_id};

    for(int idx = 0; idx < ARRAY_COUNT(id_arr); ++idx)
    {
        window = new CurvePlotWidget(this);
        window->setWindowTitle(QString("Curve Plot [%1]").arg(id_arr[idx]));
        m_plotWindows[id_arr[idx]] = window;
        reset_pt_curves_wnd_pos_size(id_arr[idx]);
    }
}

void Dialog::setup_sc_data_rec_file(QString &curr_path, QString &curr_date_str)
{
    QString err_str, curr_file_path;

    m_curr_sc_data_rec_file_name = curr_date_str + "-" + gs_str_sc_data_test + gs_str_sc_data_test_rec_file_type;
    curr_file_path = curr_path + "/" + m_curr_sc_data_rec_file_name;
    if(!mkpth_if_not_exists(curr_path))
    {
        err_str = QString("%1%2:%3").arg(g_str_create_folder, g_str_fail, curr_path);
        DIY_LOG(LOG_ERROR, err_str);
        QMessageBox::critical(this, "Error", err_str);
        return;
    }

    m_curr_sc_data_rec_file.setFileName(curr_file_path);
    if(!m_curr_sc_data_rec_file.open(QFile::WriteOnly | QFile::Append))
    {
        err_str = QString("%1%2:%3").arg(g_str_create_file, g_str_fail, curr_file_path);
        DIY_LOG(LOG_ERROR, err_str);
        QMessageBox::critical(this, "Error", err_str);
        return;
    }
    m_curr_sc_data_txt_stream.setDevice(&m_curr_sc_data_rec_file);
}

void Dialog::close_sc_data_file_rec()
{
    if(m_curr_sc_data_rec_file.isOpen())
    {
        m_curr_sc_data_txt_stream.flush();
        m_curr_sc_data_rec_file.close();
    }
}

void Dialog::on_dataCollStartPbt_clicked()
{
    if(!m_sc_data_conn_params.valid)
    {
        QMessageBox::critical(this, "", gs_str_please_set_valid_ip_and_port);
        return;
    }

    for (auto &window : m_plotWindows)
    {
        if (window)
        {
            window->clearAllSeries();
            window->hide();
        }
    }

    QString curr_date_str = common_tool_get_curr_date_str();
    QString curr_rec_folder_name = curr_date_str  + "-" + g_str_test_rec_name_sufx;
    QString curr_path = QString("./") + curr_rec_folder_name;
    setup_sc_data_rec_file(curr_path, curr_date_str);

    QString ip = m_sc_data_conn_params.ip_addr;
    quint16 port = m_sc_data_conn_params.port_no;
    int connTimeout = 3;
    int packetCount = ui->dataCollRowCntSpinbox->value();

    emit start_collect_sc_data(ip, port, connTimeout, packetCount);

    sc_data_btns_refresh(true);
}

void Dialog::on_dataCollDispCurvPbt_clicked()
{
    show_pt_curves();
}

void Dialog::on_dataCollStopPbt_clicked()
{
    emit stop_collect_sc_data();

    close_sc_data_file_rec();
    sc_data_btns_refresh(false);
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

    QColor txt_color = (NORMAL == packet.notes || START_ACK == packet.notes
                        || STOP_ACK == packet.notes) ?
                        m_txt_def_color : g_log_lvl_fonts_arr[LOG_WARN];
    append_str_with_color_and_weight(ui->testInfoDisplayTxt, data_str,
                                     txt_color, m_txt_def_font.weight());
    data_str += "\n";
    if(m_curr_sc_data_rec_file.isOpen()) {m_curr_sc_data_txt_stream << data_str;}

    if(NORMAL == packet.notes)
    {
        quint64 pkt_idx;
        m_disp_curv_pt_cnt
                = split_data_into_channels(packet.data, m_ch1_data_vec, m_ch2_data_vec, pkt_idx);


        m_plotWindows[m_ch1_wnd_str_id]->receiveData(m_disp_curv_pt_cnt, m_ch1_data_vec, m_max_pt_value);
        m_plotWindows[m_ch2_wnd_str_id]->receiveData(m_disp_curv_pt_cnt, m_ch2_data_vec, m_max_pt_value);
    }
}

int Dialog::split_data_into_channels(QByteArray& ori_data,
                              QVector<quint32> &dv_ch1, QVector<quint32> &dv_ch2,
                              quint64 &pkt_idx)
{
    /*two channels, all_bytes_cnt_per_pt * 4 bits per point for each channel.*/
    static const int all_bytes_cnt_per_pt = g_sys_configs_block.all_bytes_per_pt,
            suffix_bytes_cnt = g_sys_configs_block.pkt_idx_byte_cnt;
    static const int all_hbs_cnt_per_pt = all_bytes_cnt_per_pt * 2;
    static const bool odd_hb_per_chpt = ((all_bytes_cnt_per_pt % 2) == 1);

    int disp_pt_per_row = ui->dataCollDispRowPtCntSpinbox->value();
    if(disp_pt_per_row > g_sys_configs_block.max_pt_number)
    {//normally this branch would never be reached. just for assurance...
        disp_pt_per_row = g_sys_configs_block.max_pt_number;
    }

    int disp_bytes_per_row = disp_pt_per_row * all_bytes_cnt_per_pt;
    int ori_bytes_cnt = ori_data.count();

    if(ori_bytes_cnt < suffix_bytes_cnt)
    {
        DIY_LOG(LOG_ERROR, QString("received bytes too few: %1").arg(ori_bytes_cnt));
        return 0;
    }
    pkt_idx = 0;
    for(int idx = ori_bytes_cnt - suffix_bytes_cnt; idx < ori_bytes_cnt; ++idx)
    {
        pkt_idx = (pkt_idx << 8) + (quint64)(ori_data.at(idx));
    }

    int ori_data_bytes_cnt = (ori_bytes_cnt - suffix_bytes_cnt);

    if(dv_ch1.size() < disp_pt_per_row) dv_ch1.resize(disp_pt_per_row);
    if(dv_ch2.size() < disp_pt_per_row) dv_ch2.resize(disp_pt_per_row);
    dv_ch1.fill(0); dv_ch2.fill(0);

    unsigned char byte;
    QVector<quint32> * curr_dv = nullptr;
    for(int idx = 0, hb_idx = 0, hb_idx_in_pt = 0, pt_idx = 0;
        idx < disp_bytes_per_row; ++idx)
    {
        byte = idx < ori_data_bytes_cnt ? ori_data[idx] : 0xFF;
        pt_idx = idx / all_bytes_cnt_per_pt;

        hb_idx = idx * 2;
        hb_idx_in_pt = hb_idx % all_hbs_cnt_per_pt;

        curr_dv = (hb_idx_in_pt < all_bytes_cnt_per_pt) ? &dv_ch1 : &dv_ch2;

        if(odd_hb_per_chpt)
        {
            if(hb_idx_in_pt < all_bytes_cnt_per_pt - 1 ||
                    hb_idx_in_pt > all_bytes_cnt_per_pt) //whole byte
            {
                (*curr_dv)[pt_idx] <<= 8;
                (*curr_dv)[pt_idx] += byte;
            }
            else //half byte
            {
                dv_ch1[pt_idx] <<= 4;
                dv_ch1[pt_idx] += (byte >> 4);

                dv_ch2[pt_idx] <<= 4;
                dv_ch2[pt_idx] += (byte & 0x0F);
            }
        }
        else
        {
            (*curr_dv)[pt_idx] <<= 8;
            (*curr_dv)[pt_idx] += byte;
        }
    }

    return disp_pt_per_row;
}

void Dialog::reset_pt_curves_wnd_pos_size(QString wnd_id)
{
    static const float ls_size_w_ratio = (float)0.8, ls_size_h_ratio = (float)0.8;
    static const float ls_pos_x_diff_ratio = 0, ls_pos_y_diff_ratio = (float)0.2;
    static const float ls_x_diff_between_wnds_ratio = (float)0.2;
    CurvePlotWidget *wnd =  m_plotWindows.contains(wnd_id) ? m_plotWindows[wnd_id] : nullptr;
    if(!wnd) return;

    QSize parent_size = this->size();
    wnd->resize(parent_size.width() * ls_size_w_ratio, parent_size.height() * ls_size_h_ratio);

    QPoint relativePos(parent_size.width() * ls_pos_x_diff_ratio,
                       parent_size.height() * ls_pos_y_diff_ratio);
    QPoint globalPos = this->mapToGlobal(relativePos);
    if(wnd_id == m_ch2_wnd_str_id)
    {
        globalPos.setX(globalPos.x() + parent_size.width() * ls_x_diff_between_wnds_ratio);
    }
    wnd->move(globalPos);
}

void Dialog::show_pt_curves()
{
    for (auto &window : m_plotWindows)
    {
        if (window)
        {
            window->show();
            window->raise();
            window->activateWindow();
        }
    }
}

void Dialog::recv_worker_report_sig_hdlr(LOG_LEVEL lvl, QString report_str,
                                         collect_rpt_evt_e_t evt )
{
    lvl = VALID_LOG_LVL(lvl) ? lvl : LOG_ERROR;

    QString disp_str = log_disp_prepender_str() + QString(g_log_level_strs[lvl]) + ",";

    disp_str += report_str;

    append_str_with_color_and_weight(ui->testInfoDisplayTxt, disp_str,
                                     g_log_lvl_fonts_arr[lvl]);
    disp_str += "\n";
    if(m_curr_sc_data_rec_file.isOpen()) {m_curr_sc_data_txt_stream << disp_str;}

    switch(evt)
    {
    case COLLECT_RPT_EVT_CONNECTED:
        ui->dataCollConnStDispLbl->setText(gs_str_connected);
        break;

    case COLLECT_RPT_EVT_DISCONNECTED:
        ui->dataCollConnStDispLbl->setText(gs_str_disconnected);

        sc_data_btns_refresh(false);
        break;

    default:
        break;
    }
}

void Dialog::sc_data_btns_refresh(bool start_collec)
{
    ui->dataCollStartPbt->setEnabled(!start_collec);
    ui->dataCollStopPbt->setEnabled(start_collec);
}

void Dialog::expo_to_coll_delay_timer_hdlr()
{
    on_dataCollStartPbt_clicked();
}

QString Dialog::hv_work_st_str(quint16 st_reg_val)
{
    static bool ls_first = true;
    typedef struct
    {
        quint16 val; QString str;
    }st_val_to_str_s_t;
    static const st_val_to_str_s_t ls_st_val_to_str_arr[] =
    {
        {0x11, "空闲"},
        {0x22, "散热"},
        {0xE1, "input1状态反馈异常"},
        {0xE2, "电流反馈异常"},
        {0xE3, "电压反馈异常"},
        {0xE4, "多个异常同时发生"},
    };
    static QMap<quint16, QString> ls_st_val_to_str_map;

    if(ls_first)
    {
        for(int i = 0; i < ARRAY_COUNT(ls_st_val_to_str_arr); ++i)
        {
            ls_st_val_to_str_map.insert(ls_st_val_to_str_arr[i].val,
                                        ls_st_val_to_str_arr[i].str);
        }
        ls_first = false;
    }

    QString st_str;
    if(ls_st_val_to_str_map.contains(st_reg_val))
    {
        st_str = ls_st_val_to_str_map[st_reg_val];
    }
    else
    {
        st_str = QString(g_str_unkonw_st) + ":0x" + QString::number(st_reg_val, 16).toUpper();
    }
    return st_str;
}
