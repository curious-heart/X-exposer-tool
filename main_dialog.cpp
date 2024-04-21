#include "main_dialog.h"
#include "ui_main_dialog.h"

Dialog::Dialog(QWidget *parent)
    : QDialog(parent)
    , ui(new Ui::Dialog)
{
    ui->setupUi(this);

    m_testParamSettingsDialog = new testParamSettingsDialog(this);
    m_hvConnSettingsDialog = new hvConnSettings(this);
}

Dialog::~Dialog()
{
    delete ui;
}


void Dialog::on_testParamSetBtn_clicked()
{
    m_testParamSettingsDialog->exec();
}


void Dialog::on_hvConnSetBtn_clicked()
{
    m_hvConnSettingsDialog->exec();
}

