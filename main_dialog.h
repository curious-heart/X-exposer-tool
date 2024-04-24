#ifndef DIALOG_H
#define DIALOG_H

#include <QDialog>
#include "test_param_settings.h"
#include "hv_connsettings.h"

QT_BEGIN_NAMESPACE
namespace Ui { class Dialog; }
QT_END_NAMESPACE

class Dialog : public QDialog
{
    Q_OBJECT

public:
    Dialog(QWidget *parent = nullptr);
    ~Dialog();

private slots:
    void on_testParamSetBtn_clicked();

    void on_hvConnSetBtn_clicked();

private:
    Ui::Dialog *ui;
    testParamSettingsDialog * m_testParamSettingsDialog = nullptr;
    hvConnSettings * m_hvConnSettingsDialog = nullptr;

    test_params_struct_t m_test_params;
    modbus_conn_parameters_struct_t m_hv_conn_params;
};
#endif // DIALOG_H
