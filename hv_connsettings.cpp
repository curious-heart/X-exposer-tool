#include <QSerialPort>
#include <QSerialPortInfo>
#include <QMessageBox>
#include <QAxObject>
#include <QDir>

#include "common_tools/common_tool_func.h"
#include "logger/logger.h"
#include "sysconfigs/sysconfigs.h"
#include "hv_connsettings.h"
#include "ui_hv_connsettings.h"

static const char* gs_str_no_available_com = "无可用串口";
static const char* gs_str_should_be_p_int = "应为正整数";
extern const char* gs_str_data_item_invalid;

static const char* gs_str_tube_no = "射线管编号";
static const char* gs_str_oilbox_no = "油盒编号";

const hvConnSettings::combobox_item_struct_t hvConnSettings::hv_conn_type_list[] =
{
    {CONN_TYPE_SERIAL, "串口"},
    {CONN_TYPE_TCPIP, "TCPIP", true},
    {CONN_TYPE_RTU_OVER_TCP, "RTU-Over-TCP"},
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


hvConnSettings::hvConnSettings(QWidget *parent, modbus_conn_parameters_struct_t* param_ptr,
                               dev_info_struct_t *dev_info_ptr,
                               UiConfigRecorder * cfg_recorder) :
    QDialog(parent),
    ui(new Ui::hvConnSettings),
    modbus_conn_params(param_ptr),
    dev_info_block(dev_info_ptr),
    m_cfg_recorder(cfg_recorder)
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

    /*------------------------------------------*/
    ui->swVerStrLbl->setVisible(g_sys_configs_block.sw_ver_disp);
    ui->swVerStrEdit->setVisible(g_sys_configs_block.sw_ver_disp);

    ui->hwVerStrLbl->setVisible(g_sys_configs_block.hw_ver_disp);
    ui->hwVerStrEdit->setVisible(g_sys_configs_block.hw_ver_disp);

    ui->hvCtrlBoardNoLbl->setVisible(g_sys_configs_block.hv_ctrl_board_no_disp);
    ui->hvCtrlBoardNoEdit->setVisible(g_sys_configs_block.hv_ctrl_board_no_disp);

    ui->oilBoxNoLbl->setText(UI_DISP_TUBE_NO == g_sys_configs_block.tube_or_oilbox_no_disp ?
                gs_str_tube_no : gs_str_oilbox_no);

    /*------------------------------------------*/
    setup_pdt_cboxes();
    /*------------------------------------------*/

    m_rec_ui_cfg_fin.clear();
    m_rec_ui_cfg_fout.clear();
    if(m_cfg_recorder) m_cfg_recorder->load_configs_to_ui(this,
                                                          m_rec_ui_cfg_fin, m_rec_ui_cfg_fout);
    select_conn_type_param_block();
}

hvConnSettings::~hvConnSettings()
{
    m_rec_ui_cfg_fin.clear();
    m_rec_ui_cfg_fout.clear();
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
    modbus_conn_params->srvr_address = ui->modBusSrvrAddrEdit->text().toUInt(&tr_ok);
    if(!tr_ok)
    {
        ret_str += ui->modBusSrvrAddrLbl->text() + gs_str_should_be_p_int;
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

    if(dev_info_block)
    {
        dev_info_block->oil_box_number_str = ui->oilBoxNoEdit->text();
        dev_info_block->hv_ctrl_board_number_str = ui->hvCtrlBoardNoEdit->text();
        dev_info_block->sw_ver_str = ui->swVerStrEdit->text();
        dev_info_block->hw_ver_str = ui->hwVerStrEdit->text();
        dev_info_block->pdt_code = ui->pdtCodeCBox->currentText();
        dev_info_block->pdt_name = ui->pdtNameCBox->currentText();
        dev_info_block->pdt_model = ui->pdtMdlCBox->currentText();
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

    info_str += "\n";
    info_str += ui->oilBoxNoLbl->text() + ":" + ui->oilBoxNoEdit->text() + "\n";
    info_str += ui->hvCtrlBoardNoLbl->text() + ":" + ui->hvCtrlBoardNoEdit->text() + "\n";
    info_str += ui->swVerStrLbl->text() + ":" + ui->swVerStrEdit->text() + "\n";
    info_str += ui->hwVerStrLbl->text() + ":" + ui->hwVerStrEdit->text() + "\n";
    info_str += ui->pdtCodeLbl->text() + ":" + ui->pdtCodeCBox->currentText() + "\n";
    info_str += ui->pdtNameLbl->text() + ":" + ui->pdtNameCBox->currentText() + "\n";
    info_str += ui->pdtMdlLbl->text() + ":" + ui->pdtMdlCBox->currentText() + "\n";
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

#define EXCEL_OP_CHECK(obj, obj_name, fn, next_op) \
if(!(obj) || (obj)->isNull())\
{\
    DIY_LOG(LOG_WARN, QString("read %1 fail. obj_name: %2").arg(fn, (obj_name)));\
    next_op;\
}
bool hvConnSettings::load_pdt_info()
{
    QString fn = g_sys_configs_block.dev_code_infos.pdt_file_name;
    QString f_path = QDir::current().absoluteFilePath(fn);
    QVariantList params;
    params << f_path              // Filename
       << QVariant(0)       // UpdateLinks
       << QVariant(true);   // ReadOnly = true

    QAxObject excel("Excel.Application");
    excel.setProperty("Visible", false);
    QAxObject *workbooks = excel.querySubObject("Workbooks");
    EXCEL_OP_CHECK(workbooks, "workbooks", fn, {return false;});

    QAxObject *workbook = workbooks->querySubObject(
                                    "Open(const QString&, QVariant, QVariant)",
                                    params.at(0), params.at(1), params.at(2)
                                );
    EXCEL_OP_CHECK(workbook, "workbook", fn, {return false;});

    bool ret = true;
    do
    {
        QAxObject *worksheet = workbook->querySubObject("Worksheets(int)", 1);
        EXCEL_OP_CHECK(worksheet, "worksheet", fn, {ret = false; break;});

        QAxObject *usedRange = worksheet->querySubObject("UsedRange");
        EXCEL_OP_CHECK(usedRange, "usedRange", fn, {ret = false; break;});

        QAxObject *rows = usedRange->querySubObject("Rows");
        EXCEL_OP_CHECK(rows, "rows", fn, {ret = false; break;});

        int rowCount = rows->property("Count").toInt();

        int data_start_row = g_sys_configs_block.dev_code_infos.pdt_title_start_row + 1;
        for (int i = data_start_row; i < data_start_row + rowCount - 1; i++)
        {
            for(int c = 1; c <= m_pdt_cboxes.count(); c++)
            {
                int col = m_pdt_cboxes[c-1].col;
                QAxObject *cell = worksheet->querySubObject("Cells(int,int)", i, col);
                EXCEL_OP_CHECK(cell, QString("cells: %1,%2").arg(i).arg(col),
                               fn, {ret = false; break;});

                QString value = cell->property("Value").toString();

                m_pdt_cboxes[c-1].cbox->addItem(value);
            }
            if(!ret) break;
        }
        break;
    }while(true);

    workbook->dynamicCall("Close()");
    excel.dynamicCall("Quit()");
    return ret;
}

void hvConnSettings::setup_pdt_cboxes()
{
    m_pdt_cboxes.append({ui->pdtCodeCBox, g_sys_configs_block.dev_code_infos.pdt_code_col});
    m_pdt_cboxes.append({ui->pdtNameCBox, g_sys_configs_block.dev_code_infos.pdt_name_col});
    m_pdt_cboxes.append({ui->pdtMdlCBox, g_sys_configs_block.dev_code_infos.pdt_model_col});
    if(!load_pdt_info())
    {
        QString fn = g_sys_configs_block.dev_code_infos.pdt_file_name;
        QMessageBox::warning(this, "", QString("读取 %1 失败。需要手动输入产品编码信息").arg(fn));

        QString cbox_editor_name_str = "_editor";
        ui->pdtCodeCBox->setEditable(true);
        ui->pdtCodeCBox->lineEdit()->setObjectName(ui->pdtCodeCBox->objectName() + cbox_editor_name_str);
        ui->pdtNameCBox->setEditable(true);
        ui->pdtNameCBox->lineEdit()->setObjectName(ui->pdtNameCBox->objectName() + cbox_editor_name_str);
        ui->pdtMdlCBox->setEditable(true);
        ui->pdtMdlCBox->lineEdit()->setObjectName(ui->pdtMdlCBox->objectName() + cbox_editor_name_str);
    }
}

void hvConnSettings::on_pdtCodeCBox_currentIndexChanged(int index)
{
    if((m_pdt_cboxes[PDT_NAME].cbox->currentIndex() != index)
            && (0 <= index) && (index < m_pdt_cboxes[PDT_NAME].cbox->count()))
        m_pdt_cboxes[PDT_NAME].cbox->setCurrentIndex(index);

    if((m_pdt_cboxes[PDT_MODEL].cbox->currentIndex() != index)
            && (0 <= index) && (index < m_pdt_cboxes[PDT_MODEL].cbox->count()))
        m_pdt_cboxes[PDT_MODEL].cbox->setCurrentIndex(index);
}


void hvConnSettings::on_pdtNameCBox_currentIndexChanged(int index)
{
    if((m_pdt_cboxes[PDT_CODE].cbox->currentIndex() != index)
            && (0 <= index) && (index < m_pdt_cboxes[PDT_CODE].cbox->count()))
        m_pdt_cboxes[PDT_CODE].cbox->setCurrentIndex(index);

    if((m_pdt_cboxes[PDT_MODEL].cbox->currentIndex() != index)
            && (0 <= index) && (index < m_pdt_cboxes[PDT_MODEL].cbox->count()))
        m_pdt_cboxes[PDT_MODEL].cbox->setCurrentIndex(index);
}


void hvConnSettings::on_pdtMdlCBox_currentIndexChanged(int index)
{
    if((m_pdt_cboxes[PDT_CODE].cbox->currentIndex() != index)
            && (0 <= index) && (index < m_pdt_cboxes[PDT_CODE].cbox->count()))
        m_pdt_cboxes[PDT_CODE].cbox->setCurrentIndex(index);

    if((m_pdt_cboxes[PDT_NAME].cbox->currentIndex() != index)
            && (0 <= index) && (index < m_pdt_cboxes[PDT_NAME].cbox->count()))
        m_pdt_cboxes[PDT_NAME].cbox->setCurrentIndex(index);
}
