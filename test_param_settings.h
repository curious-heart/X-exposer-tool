#ifndef TEST_PARAM_SETTINGS_H
#define TEST_PARAM_SETTINGS_H

#include <QDialog>
#include <QString>
#include <QButtonGroup>
#include <QLineEdit>

#include "common_tool_func.h"
#include "test_params_struct.h"

namespace Ui {
class testParamSettingsDialog;
}

class testParamSettingsDialog : public QDialog
{
    Q_OBJECT

public:
    explicit testParamSettingsDialog(QWidget *parent = nullptr,
                                     test_params_struct_t * test_params_ptr = nullptr);
    ~testParamSettingsDialog();

private slots:
    void on_testModeComboBox_currentIndexChanged(int index);

    void on_custExpoParamFileSelBtn_clicked();

    void on_buttonBox_clicked(QAbstractButton *button);

private:
    typedef struct
    {
        const test_mode_enum_t e;
        const QString s;
    }test_mode_espair_struct_t;
    static const test_mode_espair_struct_t test_mode_list[];

    typedef struct
    {
        QWidget* ctrl;
        bool ability[TEXT_MODE_CNT];
    }test_params_ctrls_ability_struct_t;

    typedef struct
    {
        regular_expo_parms_struct_t vals;
        int expo_cnt;
        float expo_cool_dura_ms;
        bool valid_cube_volt_start, valid_cube_volt_end, valid_cube_volt_step,
             valid_cube_current_start, valid_cube_current_end, valid_cube_current_step,
             valid_expo_dura_start, valid_expo_dura_end, valid_expo_dura_step,
             valid_expo_cnt, valid_cool_dura;
        QString err_msg_cube_volt_start, err_msg_cube_volt_end, err_msg_cube_volt_step,
             err_msg_cube_current_start, err_msg_cube_current_end, err_msg_cube_current_step,
             err_msg_expo_dura_start, err_msg_expo_dura_end, err_msg_expo_dura_step,
             err_msg_expo_cnt, err_msg_cool_dura;
    }expo_params_from_ui_struct_t;

    Ui::testParamSettingsDialog *ui;
    QButtonGroup *m_expoDuraUnitBtnGrp;
    QVector<test_params_ctrls_ability_struct_t> m_test_params_ctrls_abilities;

    test_params_struct_t * m_test_params;
    expo_params_from_ui_struct_t m_expo_params_from_ui;

    void clear_local_buffer();
    bool get_one_expo_param(QLineEdit * ctrl, common_data_type_enum_t d_type, float factor,
                             RangeChecker* range, void * val_ptr, QString &ret_str);

    void get_expo_param_vals_from_ui();
    bool get_expo_param_vals_from_cust_file(QString file_fpn,
                                            QVector<expo_param_triple_struct_t> &param_vector,
                                            float * max_expo_dura_ms,
                                            QString &ret_str);
    QString collect_test_params();
    void refresh_controls_display();
};

#endif // TEST_PARAM_SETTINGS_H
