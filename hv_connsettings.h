#ifndef HV_CONNSETTINGS_H
#define HV_CONNSETTINGS_H

#include <QDialog>
#include <QComboBox>
#include <QDialogButtonBox>
#include <QPushButton>
#include <QVector>

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
}modbus_conn_parameters_struct_t;

typedef struct
{
    QString oil_box_number_str, hv_ctrl_board_number_str;
    QString sw_ver_str, hw_ver_str;
    QString pdt_code, pdt_name, pdt_model;
}dev_info_struct_t;

typedef struct
{
    modbus_conn_parameters_struct_t mb_conn_parameters;
    dev_info_struct_t dev_infos;
    QString info_str;
}dev_and_conn_info_s_t;

typedef QVector<dev_and_conn_info_s_t> dev_and_conn_info_vec_t;

typedef enum
{
    CONN_COLLET_OK,
    CONN_COLLET_DUP,
    CONN_COLLET_ERR,
}conn_collect_ret_e_t;

class hvConnSettings : public QDialog
{
    Q_OBJECT

public:
    explicit hvConnSettings(QWidget *parent = nullptr,
                            dev_and_conn_info_vec_t * dev_and_conn_vec = nullptr,
                            UiConfigRecorder * cfg_recorder = nullptr);
    ~hvConnSettings();

private slots:
    void on_modBusConnTypeComboBox_currentIndexChanged(int index);

    void on_buttonBox_clicked(QAbstractButton *button);

    void on_pdtCodeCBox_currentIndexChanged(int index);
    void on_pdtNameCBox_currentIndexChanged(int index);
    void on_pdtMdlCBox_currentIndexChanged(int index);

    void on_addDevPBtn_clicked();

    void on_delDevPBtn_clicked();

    void on_devListWidget_currentRowChanged(int currentRow);

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

    dev_and_conn_info_vec_t * dev_and_conn_info_vec;
    modbus_conn_parameters_struct_t curr_modbus_conn_params;
    dev_info_struct_t curr_dev_info_block;

    conn_collect_ret_e_t collect_conn_params(QString &ret_str);
    void select_conn_type_param_block();
    void format_hv_conn_info_str(QString &info_str);
    int curr_conn_id_in_vec(const modbus_conn_parameters_struct_t conn_params,
                             const dev_and_conn_info_vec_t &vect);
    QString get_conn_id_str(const modbus_conn_parameters_struct_t &conn_params);
    QString get_dev_id_str(const dev_and_conn_info_s_t &dev);
    void update_dev_conn_info_disp();
    bool load_params_to_ui(const dev_and_conn_info_s_t &dev);

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
    bool add_one_dev(QString *ret_str_ptr = nullptr);
    QString get_all_dev_info_strs();
};

#endif // HV_CONNSETTINGS_H
