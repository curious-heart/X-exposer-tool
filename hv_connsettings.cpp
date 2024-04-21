#include "hv_connsettings.h"
#include "ui_hv_connsettings.h"

hvConnSettings::hvConnSettings(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::hvConnSettings)
{
    ui->setupUi(this);
}

hvConnSettings::~hvConnSettings()
{
    delete ui;
}
