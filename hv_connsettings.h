#ifndef HV_CONNSETTINGS_H
#define HV_CONNSETTINGS_H

#include <QDialog>
#include <QComboBox>
#include <QDialogButtonBox>
#include <QPushButton>

namespace Ui {
class hvConnSettings;
}

typedef enum
{
    CONN_TYPE_NONE = -1,
    CONN_TYPE_SERIAL = 0,
    CONN_TYPE_TCPIP,
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
    QString info_str;
}modbus_conn_parameters_struct_t;

class hvConnSettings : public QDialog
{
    Q_OBJECT

public:
    explicit hvConnSettings(QWidget *parent = nullptr, modbus_conn_parameters_struct_t* param_ptr = nullptr);
    ~hvConnSettings();

private slots:
    void on_modBusConnTypeComboBox_currentIndexChanged(int index);

    void on_buttonBox_clicked(QAbstractButton *button);

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

    void select_conn_type_param_block();
    void format_hv_conn_info_str();

public:
    QString collect_conn_params();
};

#endif // HV_CONNSETTINGS_H
