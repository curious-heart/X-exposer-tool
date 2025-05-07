#ifndef SC_DATA_CONNSETTINGS_H
#define SC_DATA_CONNSETTINGS_H

#include <QPushButton>
#include <QDialog>

#include "config_recorder/uiconfigrecorder.h"

typedef struct
{
    bool valid;
    QString ip_addr;
    int port_no;
    QString info_str;
}sc_data_conn_params_struct_t;

namespace Ui {
class sc_data_connsettings;
}

class sc_data_connsettings : public QDialog
{
    Q_OBJECT

public:
    explicit sc_data_connsettings(QWidget *parent = nullptr,
                                  sc_data_conn_params_struct_t * conn_params = nullptr,
                                  UiConfigRecorder * cfg_recorder = nullptr);
    ~sc_data_connsettings();

    QString collect_conn_params();

private slots:
    void on_scDataConnButtonBox_clicked(QAbstractButton *button);

private:
    Ui::sc_data_connsettings *ui;

    sc_data_conn_params_struct_t * m_conn_params;

    void format_sc_data_conn_info_str();

    UiConfigRecorder * m_cfg_recorder = nullptr;
    qobj_ptr_set_t m_rec_ui_cfg_fin, m_rec_ui_cfg_fout;
};

#endif // SC_DATA_CONNSETTINGS_H
