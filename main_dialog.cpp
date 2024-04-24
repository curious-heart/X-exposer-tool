#include "main_dialog.h"
#include "ui_main_dialog.h"

Dialog::Dialog(QWidget *parent)
    : QDialog(parent)
    , ui(new Ui::Dialog)
{
    ui->setupUi(this);

    m_testParamSettingsDialog = new testParamSettingsDialog(this, &m_test_params);
    m_hvConnSettingsDialog = new hvConnSettings(this, &m_hv_conn_params);
}

Dialog::~Dialog()
{
    delete ui;
}

void Dialog::on_testParamSetBtn_clicked()
{
    m_test_params.valid = false;
    m_test_params.expo_param_block.expo_params.cust_params_arr.clear();
    m_test_params.info_str.clear();
    m_testParamSettingsDialog->exec();

    if(m_test_params.valid)
    {
        ui->testParamDisplayTxt->setText(m_test_params.info_str);
    }
}


void Dialog::on_hvConnSetBtn_clicked()
{
    m_hv_conn_params.valid = false;
    m_hv_conn_params.info_str.clear();

    m_hvConnSettingsDialog->exec();

    if(m_hv_conn_params.valid)
    {
        ui->hvConnParamDisplayTxt->setText(m_hv_conn_params.info_str);
    }
}

