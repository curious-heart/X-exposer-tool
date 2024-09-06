#ifndef TEST_PARAM_SETTINGS_H
#define TEST_PARAM_SETTINGS_H

#include <QDialog>
#include <QString>
#include <QButtonGroup>
#include <QLineEdit>

#include "common_tools/common_tool_func.h"
#include "test_params_struct.h"
#include "config_recorder/uiconfigrecorder.h"

namespace Ui {
class testParamSettingsDialog;
}

class testParamSettingsDialog : public QDialog
{
    Q_OBJECT

public:
    explicit testParamSettingsDialog(QWidget *parent = nullptr,
                                     test_params_struct_t * test_params_ptr = nullptr,
                                     UiConfigRecorder * cfg_recorder = nullptr );
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

#define UI_PARAM_ITEM(v, c) valid_##c
#define LIST_PROC_UI_PARAM_ITEM(v) \
UI_PARAM_ITEM(v, cube_volt_start), UI_PARAM_ITEM(v, cube_volt_end), UI_PARAM_ITEM(v, cube_volt_step),\
UI_PARAM_ITEM(v, cube_current_start), UI_PARAM_ITEM(v, cube_current_end), UI_PARAM_ITEM(v, cube_current_step),\
UI_PARAM_ITEM(v, expo_dura_start), UI_PARAM_ITEM(v, expo_dura_end), UI_PARAM_ITEM(v, expo_dura_step),\
UI_PARAM_ITEM(v, expo_cnt), UI_PARAM_ITEM(v, cool_dura), UI_PARAM_ITEM(v, cool_dura_factor)
    typedef struct
    {
        regular_expo_parms_struct_t vals;
        int expo_cnt;
        float expo_cool_dura_ms, expo_cool_dura_factor;
        bool fixed_cool_dura;
        bool limit_shortest_cool_dura, read_dist;
        bool LIST_PROC_UI_PARAM_ITEM(_);
#undef UI_PARAM_ITEM
#define UI_PARAM_ITEM(v, c) err_msg_##c
        QString LIST_PROC_UI_PARAM_ITEM(_);
    }expo_params_from_ui_struct_t;
#undef UI_PARAM_ITEM
/* maybe used in future...
#define UI_PARAM_ITEM(v, c) (v).err_msg_##c.clear()
#define CLEAR_UI_PARAM_ERR_MSGS(v) UI_PARAM_ITEM(v)
*/

    Ui::testParamSettingsDialog *ui;
    QButtonGroup *m_expoDuraUnitBtnGrp, *m_coolDuraModeBtnGrp;
    QVector<test_params_ctrls_ability_struct_t> m_test_params_ctrls_abilities;

    test_params_struct_t * m_test_params;
    expo_params_from_ui_struct_t m_expo_params_from_ui;

    bool get_one_expo_param(QLineEdit * ctrl, common_data_type_enum_t d_type, float factor,
                             RangeChecker* range, void * val_ptr, QString &ret_str,
                             QString new_unit_str = "");

    void get_expo_param_vals_from_ui();
    bool judge_dura_factor_from_str(QString h_s, float *factor);
    bool get_expo_param_vals_from_cust_file(QString file_fpn,
                                            QVector<expo_param_triple_struct_t> &param_vector,
                                            float * max_expo_dura_ms,
                                            QString &ret_str, QString &file_content);

    bool get_expo_param_vals_from_cust2_file(QString file_fpn,
                                            QVector<expo_param_triple_struct_t> &param_vector,
                                            float * max_expo_dura_ms,
                                            QString &ret_str, QString &file_content);
    void refresh_controls_display();
    void format_test_params_info_str(QString &file_content);

    UiConfigRecorder * m_cfg_recorder = nullptr;
public:
    QString collect_test_params();
};

#endif // TEST_PARAM_SETTINGS_H
