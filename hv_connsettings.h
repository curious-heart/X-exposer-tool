#ifndef HV_CONNSETTINGS_H
#define HV_CONNSETTINGS_H

#include <QDialog>
#include <QComboBox>
#include <QDialogButtonBox>
#include <QPushButton>

#include "config_recorder/uiconfigrecorder.h"

namespace Ui {
class hvConnSettings;
}

typedef enum
{
    CONN_TYPE_NONE = -1,
    CONN_TYPE_SERIAL = 0,
    CONN_TYPE_TCPIP,
    CONN_TYPE_RTU_OVER_TCP,
    CONN_TYPE_CNT,
}hv_conn_type_enum_t;

typedef struct
{
    QString com_port_s;
    int boudrate, data_bits, check_parity, stop_bit;
}serial_conn_params_struct_t;

typedef struct
{
    QString ip_addr;
    int port_no;
    int expire_time_ms;
}tcpip_conn_params_struct_t;

typedef struct
{
    bool valid;
    hv_conn_type_enum_t type;
    serial_conn_params_struct_t serial_params;
    tcpip_conn_params_struct_t tcpip_params;
    int resp_wait_time_ms;
    int srvr_address;
    QString info_str;
}modbus_conn_parameters_struct_t;

typedef struct
{
    QString oil_box_number_str, hv_ctrl_board_number_str;
    QString sw_ver_str, hw_ver_str;
    QString pdt_code, pdt_name, pdt_model;
}dev_info_struct_t;

class hvConnSettings : public QDialog
{
    Q_OBJECT

public:
    explicit hvConnSettings(QWidget *parent = nullptr,
                            modbus_conn_parameters_struct_t* param_ptr = nullptr,
                            dev_info_struct_t *dev_info_ptr = nullptr,
                            UiConfigRecorder * cfg_recorder = nullptr);
    ~hvConnSettings();

private slots:
    void on_modBusConnTypeComboBox_currentIndexChanged(int index);

    void on_buttonBox_clicked(QAbstractButton *button);

    void on_pdtCodeCBox_currentIndexChanged(int index);
    void on_pdtNameCBox_currentIndexChanged(int index);
    void on_pdtMdlCBox_currentIndexChanged(int index);

private:
    Ui::hvConnSettings *ui;

    typedef struct
    {
        int val;
        QString s;
        bool is_default;
    }combobox_item_struct_t;
    static const combobox_item_struct_t hv_conn_type_list[];
    static const combobox_item_struct_t baudrate_list[], data_bits_list[], check_parity_list[],
                                        stop_bit_list[];

    modbus_conn_parameters_struct_t * modbus_conn_params;
    dev_info_struct_t * dev_info_block;

    void select_conn_type_param_block();
    void format_hv_conn_info_str();

    UiConfigRecorder * m_cfg_recorder = nullptr;
    qobj_ptr_set_t m_rec_ui_cfg_fin, m_rec_ui_cfg_fout;

    typedef enum
    {
        PDT_CODE = 0,
        PDT_NAME,
        PDT_MODEL,
    }pdt_info_idx_e_t;

    typedef struct
    {
        QComboBox* cbox;
        int col;
    }cbox_col_pair_s_t;
    QList<cbox_col_pair_s_t> m_pdt_cboxes;
    void setup_pdt_cboxes();
    bool load_pdt_info();

public:
    QString collect_conn_params();
};

#endif // HV_CONNSETTINGS_H
