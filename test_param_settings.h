#ifndef TEST_PARAM_SETTINGS_H
#define TEST_PARAM_SETTINGS_H

#include <QDialog>
#include <QString>
#include <QButtonGroup>
#include <QLineEdit>
#include <QCheckBox>
#include <QLabel>
#include <QMap>
#include <QSpinBox>
#include <QDoubleSpinBox>
#include <QLabel>
#include <QRadioButton>

#include "common_tools/common_tool_func.h"
#include "test_params_struct.h"
#include "sysconfigs/sysconfigs.h"
#include "config_recorder/uiconfigrecorder.h"
#include "test_result_judge/test_result_judge.h"

namespace Ui {
class testParamSettingsDialog;
}

typedef struct
{
    QCheckBox * judge_or_not_chbox;
    QLineEdit * low_e_pct_ledit, *up_e_pct_ledit, *low_e_err_val_ledit, *up_e_err_val_ledit;
    QCheckBox * is_fixed_ref_chbox;
    QLineEdit * fixed_ref_val_ledit;
}judge_gui_ctrls_s_t;
typedef struct
{
    hv_mb_reg_e_t ref_reg_no, val_reg_no;
    judge_gui_ctrls_s_t gui_ctrls;
}judge_ctrls_s_t;

typedef struct
{
    QCheckBox * row_lbl;
    QLabel * col_lbl;
}judge_ctrl_labels_s_t;
typedef QMap<QWidget*, judge_ctrl_labels_s_t> judge_ctrl_desc_map_t;

typedef enum
{
    EXPO_PARAMS_UI_TO_SW,
    EXPO_PARAMS_FILE_TO_SW,
    EXPO_PARAMS_SW_TO_MB_INTF,
}expo_params_trans_factor_e_t;

typedef enum
{
    EXPO_PARAMS_UI_SYNC_MAIN_TO_SET_DLG,
    EXPO_PARAMS_UI_SYNC_SET_DLG_TO_MAIN,
}expo_params_ui_sync_type_e_t;

typedef struct
{
    QSpinBox * cube_volt_ctrl;
    QDoubleSpinBox * cube_current_ctrl, *expo_dura_ctrl;
    QLabel *cube_current_unit, *expo_dura_unit;
}expo_params_ui_sync_ctrls_s_t;

class testParamSettingsDialog : public QDialog
{
    Q_OBJECT

public:
    explicit testParamSettingsDialog(QWidget *parent = nullptr,
                                     test_params_struct_t * test_params_ptr = nullptr,
                                     UiConfigRecorder * cfg_recorder = nullptr,
                                     TestResultJudge * test_judge_ptr = nullptr);
    ~testParamSettingsDialog();

private slots:
    void on_testModeComboBox_currentIndexChanged(int index);

    void on_custExpoParamFileSelBtn_clicked();

    void on_buttonBox_clicked(QAbstractButton *button);

    void on_voltKVChkbox_stateChanged(int arg1);

    void on_amtmAChkbox_stateChanged(int arg1);

    void on_distmmChkbox_stateChanged(int arg1);

    void on_voltKVIsFixedRefChkbox_stateChanged(int arg1);

    void on_amtmAIsFixedRefChkbox_stateChanged(int arg1);

    void on_readDistChBox_stateChanged(int arg1);

    void on_expoDuraUnitmsRButton_toggled(bool checked);

    void on_expoDuraUnitsecRButton_toggled(bool checked);

    void on_expoDuraUnitminRButton_toggled(bool checked);

    void on_testContentDecoupleRButton_toggled(bool checked);

    void on_testContentCoolHVRButton_toggled(bool checked);

    void on_testContentOnlyCoilRButton_toggled(bool checked);

    void on_testContentNormalRButton_toggled(bool checked);

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
    QButtonGroup *m_expoDuraUnitBtnGrp, *m_coolDuraModeBtnGrp, *m_testContentBtnGrp;
    QVector<test_params_ctrls_ability_struct_t> m_test_params_ctrls_abilities;

    test_params_struct_t * m_test_params;
    expo_params_from_ui_struct_t m_expo_params_from_ui;

    QString m_ui_cube_current_unit_str, m_ui_expo_dura_unit_str;

    template <typename T>
    bool get_one_expo_param(QLineEdit * ctrl, common_data_type_enum_t d_type, float factor,
                             RangeChecker<T>* range, void * val_ptr, QString &ret_str,
                             QString new_unit_str = "");

    void get_expo_param_vals_from_ui();
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
    qobj_ptr_set_t m_rec_ui_cfg_fin, m_rec_ui_cfg_fout;

    TestResultJudge * m_test_judge;
    QList<judge_ctrls_s_t> m_judge_ctrls;
    judge_ctrl_desc_map_t m_judge_ctrl_desc_map;
    void setup_judge_ctrls();
    void refresh_judge_ctrls_display();
    bool construct_judge_params(QString &err_str, QString &info_str);

    void arrange_ui_according_to_syscfgs(QRadioButton * &hidden_dura_rb, QRadioButton * &mb_intf_dura_rb);
    test_content_enum_t get_test_content(QString *content_str);
    float cube_current_trans_factor(expo_params_trans_factor_e_t trans, QString file_unit_str = "");
    float expo_dura_trans_factor(expo_params_trans_factor_e_t trans, QString file_unit_str = "");

    float m_ui_dura_start_in_ms = 0, m_ui_dura_end_in_ms = 0, m_ui_dura_step_in_ms = 0;

    /* the actuall used current intf unit.
     * it is initiated by g_sys_configs_block.mb_cube_current_intf_unit (i.e
     * config file). But it may change when user select different test content.
    */
    mb_cube_current_unit_e_t m_used_current_intf_unit, m_used_current_ui_unit;
    void update_current_display();

    /* from_rb_toggle: indicating if this function is called from radio button toggle
     * handler. if it is true, "checked" indicates radio button state; else, "checked" is ignored.
     */
    void update_current_name_and_unit();
    /* from_rb_toggle: indicating if this function is called from radio button toggle
     * handler. if it is true, "u_str" indicates the str to be used; else, "u_str" is ignored.
     */
    void update_current_range_checker();
    void update_expo_dura_unit(bool from_rb_toggle, QString u_str = "");

public:
    QString collect_test_params();
    bool expo_params_ui_sync(expo_params_ui_sync_type_e_t direction,
                             expo_params_ui_sync_ctrls_s_t * main_ctrls, QString *info_str = nullptr);
    void get_volt_info_for_chart(QString &name_str, QString &unit_str, int &low, int &up);
    void get_current_info_for_chart(QString &name_str, QString &unit_str, float &low, float &up);
};

#endif // TEST_PARAM_SETTINGS_H
