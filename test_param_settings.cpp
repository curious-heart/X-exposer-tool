#include <QMessageBox>
#include <QFile>
#include <QTextStream>
#include <QFileDialog>
#include <QtMath>
#include "common_tools/common_tool_func.h"
#include "logger/logger.h"
#include "test_param_settings.h"
#include "ui_test_param_settings.h"
#include "version_def/version_def.h"

static RangeChecker<int> gs_valid_cube_volt_kv_range;
static RangeChecker<float> gs_valid_cube_current_ma_range;
static RangeChecker<float> gs_valid_expo_dura_ms_range;
static RangeChecker<int> gs_valid_expo_cnt_range(1, 0, "", EDGE_INCLUDED, EDGE_INFINITE);
static RangeChecker<float> gs_valid_cool_dura_range;
static RangeChecker<float> gs_valid_cool_dura_factor;

static const char* gs_str_test_mode = "测试模式";
static const char* gs_str_test_content = "测试内容";
const char* g_str_cube_volt = "管电压";
const char* g_str_cube_current = "管电流";
const char* g_str_current = "电流";
const char* g_str_expo_dura = "曝光时间";
const char* g_str_coil_current = "灯丝电流";
static const char* gs_str_current_name = g_str_cube_current;

const char* g_str_fb_cube_current = "读出管电流";
const char* g_str_fb_coil_current = "读出灯丝电流";
static const char* gs_str_fb_current_name = g_str_fb_cube_current;

const char* g_str_should_be = "应为";
static const char* gs_str_the_1st_item = "第一项";
static const char* gs_str_repeats_num = "重复次数";
static const char* gs_str_cool_dura = "冷却时间";
static const char* gs_str_extra_cool_dura = "额外的冷却时间";
static const char* gs_str_start_val = "起始值";
static const char* gs_str_end_val = "结束值";
static const char* gs_str_step = "步长";
static const char* gs_str_exceeds_valid_range = "超出允许范围";
static const char* gs_str_should_be_int = "应为整数";
static const char* gs_str_should_be_number = "应为数字";
static const char* gs_str_start_end_step_err1 = "步长为0时，起始值和结束值必须相等!";
static const char* gs_str_start_end_step_err2 = "组合无效";
static const char* gs_str_read_file_fail = "读取文件失败";
static const char* gs_str_please_select_file = "请选择自定义曝光文件";
static const char* gs_str_cust_file_sel_caption = "选择自定义曝光参数文件";
const char* g_str_volt_unit_kv = "kV";
const char* g_str_current_unit_a = "A";
const char* g_str_current_unit_ma = "mA";
const char* g_str_current_unit_ua = "uA";
const char* g_str_dura_unit_min = "min";
const char* g_str_dura_unit_s = "s";
const char* g_str_dura_unit_ms = "ms";
static const char* gs_str_cust_file = "自定义曝光参数文件";
static const char* gs_str_1st_line_should_be = "首行应为";
const char* g_str_or = "或";
const char* g_str_and = "与";
static const char* gs_str_expo_params = "曝光参数";
static const char* gs_str_group = "组";
const char* g_str_loop = "轮";
const char* g_str_time_ci = "次";
const char* g_str_time_bei = "倍";
const char* g_str_no_bu = "不";
const char* g_str_for_examp = "例如";
const char* g_str_can_be = "可以为";

static const char* gs_cust_expo_file_filter = "CSV File (*.csv)";
static const char* gs_valid_cube_volt_header_str = "volt-kv";
static const char* gs_valid_cube_current_header_prefix_str = "current";
static const char* gs_valid_expo_dura_header_prefix_str = "dura";
static const char* gs_str_header_with_unit = "带参数单位的表头";
static const char* gs_str_sep_by_comma = "以半角逗号分开";
static const char* gs_valid_header_line_s = "volt-kv,current-ma,dura-s";
static const char* gs_cust_expo_file_item_sep_in_line = ",";
static const char* gs_cust_expo_file_v_unit_sep = "-";
static const int gs_cust_expo_file_item_num_in_line = 3;
static const int gs_cust_expo_file_volt_item_idx = 0;
static const int gs_cust_expo_file_current_item_idx = 1;
static const int gs_cust_expo_file_dura_item_idx = 2;

const char* g_str_the_line_pron = "第";
static const char* gs_str_line = "行";
static const char* gs_str_item = "项";
static const char* gs_str_no_valid_data_item = "无有效数据";
static const char* gs_str_format_error = "格式错误";
const char* gs_str_data_item_invalid = "数据无效";

static const char* gs_str_should_be_le = "应小于或等于";
static const char* gs_str_invalid_mb_reg_no = "无效寄存器编号";
static const char* gs_str_cons_judge_param_err1 = "参考值不固定时，必须指定有效的参考寄存器编号";

static const char* gs_info_str_seperator = "--------------------";

static const char* gs_str_cust1_notes =
"文件格式：文本文件，第一行指定曝光时间单位（s或ms），之后每行一组电压、电流、时间值，ASCII逗号隔开";

static const char* gs_str_cust2_notes =
"文件格式：共3行的文本文件，分别指定ASSCII逗号分开的电压、电流、时间值。每行第一个字段为表头；第三行表头指定s或ms";

static const char* gs_str_cust2_err_msg_format = "第一项为表头，第二项开始为数据";
static const char* gs_str_plz_check_first = "请先勾选";

static const char* gs_str_tube_no = "射线管编号";
static const char* gs_str_oilbox_no = "油盒编号";

const testParamSettingsDialog::test_mode_espair_struct_t
      testParamSettingsDialog::test_mode_list[] =
{
    {TEST_MODE_SINGLE, QString("手动")},
    {TEST_MODE_REPEAT, QString("重复")},
    {TEST_MODE_TRAVERSE, QString("遍历")},
    {TEST_MODE_CUST1_TRIPLES, QString("自定义1:三元组列表")},
    {TEST_MODE_CUST2_DISCRETE, QString("自定义2:离散值")},
};

#define CHKBOX_EXIST_AND_CHECKED(ctrl) ((ctrl) && (ctrl)->isChecked())
#define CHECK_AND_SET_ENABLED(ctrl, e)  if(ctrl) (ctrl)->setEnabled((e));
#define JUDGE_CTRL_DESC_STR(ctrl) \
        ctrl_desc = m_judge_ctrl_desc_map[(ctrl)].row_lbl->text() \
                + m_judge_ctrl_desc_map[(ctrl)].col_lbl->text();

/*
 * define which controls is enabled or disabled in every test mode.
 * the a_xx corresponds to the test modes in test_mode_list.
*/
#define ARRANGE_CTRLS_ABILITY(ctrl, a_sg, a_re, a_tr, a_cu1, a_cu2) \
    m_test_params_ctrls_abilities.append({(ctrl), {(a_sg), (a_re), (a_tr), (a_cu1), (a_cu2)}});
#define TEST_PARAMS_CTRLS_ABT_TBL \
{\
    ARRANGE_CTRLS_ABILITY(ui->cubeVoltStartEdit, true, true, true, false, false)\
    ui->cubeVoltStartEdit->setWhatsThis(QString("%1%2").arg(g_str_cube_volt, gs_str_start_val));\
    ARRANGE_CTRLS_ABILITY(ui->cubeVoltEndEdit, false, false, true, false, false)\
    ui->cubeVoltEndEdit->setWhatsThis(QString("%1%2").arg(g_str_cube_volt, gs_str_end_val));\
    ARRANGE_CTRLS_ABILITY(ui->cubeVoltStepEdit, false, false, true, false, false)\
    ui->cubeVoltStepEdit->setWhatsThis(QString("%1%2").arg(g_str_cube_volt, gs_str_step));\
    ARRANGE_CTRLS_ABILITY(ui->cubeCurrentStartEdit, true, true, true, false, false)\
    ui->cubeCurrentStartEdit->setWhatsThis(QString("%1%2").arg(gs_str_current_name, gs_str_start_val));\
    ARRANGE_CTRLS_ABILITY(ui->cubeCurrentEndEdit, false, false, true, false, false)\
    ui->cubeCurrentEndEdit->setWhatsThis(QString("%1%2").arg(gs_str_current_name, gs_str_end_val));\
    ARRANGE_CTRLS_ABILITY(ui->cubeCurrentStepEdit, false, false, true, false, false)\
    ui->cubeCurrentStepEdit->setWhatsThis(QString("%1%2").arg(gs_str_current_name, gs_str_step));\
    ARRANGE_CTRLS_ABILITY(ui->expoDuraStartEdit, true, true, true, false, false)\
    ui->expoDuraStartEdit->setWhatsThis(QString("%1%2").arg(g_str_expo_dura, gs_str_start_val));\
    ARRANGE_CTRLS_ABILITY(ui->expoDuraEndEdit, false, false, true, false, false)\
    ui->expoDuraEndEdit->setWhatsThis(QString("%1%2").arg(g_str_expo_dura, gs_str_end_val));\
    ARRANGE_CTRLS_ABILITY(ui->expoDuraStepEdit, false, false, true, false, false)\
    ui->expoDuraStepEdit->setWhatsThis(QString("%1%2").arg(g_str_expo_dura, gs_str_step));\
    ARRANGE_CTRLS_ABILITY(ui->repeatsNumEdit, false, true, true, true, true)\
    ui->repeatsNumEdit->setWhatsThis(QString(gs_str_repeats_num));\
    ARRANGE_CTRLS_ABILITY(ui->coolDuraEdit, false, true, true, true, true)\
    if(ui->fixedCoolDuraRButton->isChecked())\
    {ui->coolDuraEdit->setWhatsThis(QString(gs_str_cool_dura));}\
    else\
    {ui->coolDuraEdit->setWhatsThis(ui->timesCoolDuraRButton->text());}\
    ARRANGE_CTRLS_ABILITY(ui->expoDuraUnitmsRButton, true, true, true, false, false)\
    ARRANGE_CTRLS_ABILITY(ui->expoDuraUnitsecRButton, true, true, true, false, false)\
    ARRANGE_CTRLS_ABILITY(ui->expoDuraUnitminRButton, true, true, true, false, false)\
    ARRANGE_CTRLS_ABILITY(ui->fixedCoolDuraRButton, false, true, true, true, true)\
    ARRANGE_CTRLS_ABILITY(ui->timesCoolDuraRButton, false, true, true, true, true)\
    ARRANGE_CTRLS_ABILITY(ui->limitShortestCoolDuraChBox, false, true, true, true, true)\
    \
    ARRANGE_CTRLS_ABILITY(ui->custExpoParamFileSelBtn, false, false, false, true, true)\
    ARRANGE_CTRLS_ABILITY(ui->custExpoParamFileNoteEdit, false, false, false, true, true)\
}

void testParamSettingsDialog::arrange_ui_according_to_syscfgs(QRadioButton * &hidden_dura_rb,
                                                           QRadioButton * &mb_intf_dura_rb)
{
    /*This function should only set visiable attribute.*/

    switch(g_sys_configs_block.hidden_ui_mb_dura_unit)
    {
    case MB_DURA_UNIT_MS:
        ui->expoDuraUnitmsRButton->setVisible(false);
        hidden_dura_rb = ui->expoDuraUnitmsRButton;
        break;
    case MB_DURA_UNIT_SEC:
        ui->expoDuraUnitsecRButton->setVisible(false);
        hidden_dura_rb = ui->expoDuraUnitsecRButton;
        break;
    default: //MB_DURA_UNIT_MIN:
        ui->expoDuraUnitminRButton->setVisible(false);
        hidden_dura_rb = ui->expoDuraUnitminRButton;
        break;
    }

    switch(g_sys_configs_block.mb_dura_intf_unit)
    {
    case MB_DURA_UNIT_MS:
        mb_intf_dura_rb = ui->expoDuraUnitmsRButton;
        break;
    case MB_DURA_UNIT_SEC:
        mb_intf_dura_rb = ui->expoDuraUnitsecRButton;
        break;
    default: //MB_DURA_UNIT_MIN:
        mb_intf_dura_rb = ui->expoDuraUnitminRButton;
        break;
    }

    ui->readDistChBox->setVisible(g_sys_configs_block.distance_group_disp);
    ui->distmmChkbox->setVisible(g_sys_configs_block.distance_group_disp);
    ui->distmmLowEdgePctLEdit->setVisible(g_sys_configs_block.distance_group_disp);
    ui->distmmUpEdgePctLEdit->setVisible(g_sys_configs_block.distance_group_disp);
    ui->distmmLowEdgeErrValLEdit->setVisible(g_sys_configs_block.distance_group_disp);
    ui->distmmUpEdgeErrValLEdit->setVisible(g_sys_configs_block.distance_group_disp);
    ui->distmmLowEdgePctLbl->setVisible(g_sys_configs_block.distance_group_disp);
    ui->distmmUpEdgePctLbl->setVisible(g_sys_configs_block.distance_group_disp);
    ui->distmmIsFixedRefChkbox->setVisible(g_sys_configs_block.distance_group_disp);
    ui->distmmFixedRefValLEdit->setVisible(g_sys_configs_block.distance_group_disp);

    ui->swVerStrLbl->setVisible(g_sys_configs_block.sw_ver_disp);
    ui->swVerStrEdit->setVisible(g_sys_configs_block.sw_ver_disp);

    ui->hwVerStrLbl->setVisible(g_sys_configs_block.hw_ver_disp);
    ui->hwVerStrEdit->setVisible(g_sys_configs_block.hw_ver_disp);

    ui->hvCtrlBoardNoLbl->setVisible(g_sys_configs_block.hv_ctrl_board_no_disp);
    ui->hvCtrlBoardNoEdit->setVisible(g_sys_configs_block.hv_ctrl_board_no_disp);

    ui->oilBoxNoLbl->setText(UI_DISP_TUBE_NO == g_sys_configs_block.tube_or_oilbox_no_disp ?
                gs_str_tube_no : gs_str_oilbox_no);
}

#define SAVE_EXPO_DURA_UI_TO_SW(ui_to_sw_f) \
{\
    m_ui_dura_start_in_ms = ui->expoDuraStartEdit->text().toFloat() * (ui_to_sw_f); \
    m_ui_dura_end_in_ms = ui->expoDuraEndEdit->text().toFloat() * (ui_to_sw_f); \
    m_ui_dura_step_in_ms = ui->expoDuraStepEdit->text().toFloat() * (ui_to_sw_f); \
}

#define LOAD_EXPO_DUAR_SW_TO_UI(sw_to_ui_f) \
{\
    static bool init = true; \
    if(init) init = false; \
    else\
    {\
    ui->expoDuraStartEdit->setText(QString::number(m_ui_dura_start_in_ms * (sw_to_ui_f))); \
    ui->expoDuraEndEdit->setText(QString::number(m_ui_dura_end_in_ms * (sw_to_ui_f))); \
    ui->expoDuraStepEdit->setText(QString::number(m_ui_dura_step_in_ms * (sw_to_ui_f))); \
    }\
}

testParamSettingsDialog::testParamSettingsDialog(QWidget *parent,
                                                 test_params_struct_t *test_params_ptr,
                                                 UiConfigRecorder * cfg_recorder,
                                                 TestResultJudge * test_judge_ptr) :
    QDialog(parent),
    ui(new Ui::testParamSettingsDialog),
    m_test_params(test_params_ptr),
    m_cfg_recorder(cfg_recorder), m_test_judge(test_judge_ptr)
{
    if(!m_test_params)
    {
        DIY_LOG(LOG_ERROR, "test_params_ptr is null.");
        return;
    }

    m_used_current_intf_unit = g_sys_configs_block.mb_cube_current_intf_unit;
    m_used_current_ui_unit = g_sys_configs_block.ui_current_unit;

    gs_valid_cube_volt_kv_range.set_min_max(g_sys_configs_block.cube_volt_kv_min,
                                            g_sys_configs_block.cube_volt_kv_max);
    gs_valid_cube_volt_kv_range.set_unit_str(g_str_volt_unit_kv);

    gs_valid_cube_current_ma_range.set_min_max(g_sys_configs_block.cube_current_ma_min,
                                               g_sys_configs_block.cube_current_ma_max);
    gs_valid_cube_current_ma_range.set_unit_str(g_str_current_unit_ma);

    gs_valid_expo_dura_ms_range.set_min_max(g_sys_configs_block.dura_ms_min,
                                            g_sys_configs_block.dura_ms_max);
    gs_valid_expo_dura_ms_range.set_unit_str(g_str_dura_unit_ms);

    gs_valid_cool_dura_range.set_min_max(g_sys_configs_block.dura_ms_max
                                          * g_sys_configs_block.cool_dura_factor /1000, 0);
    gs_valid_cool_dura_range.set_edge(EDGE_INCLUDED, EDGE_INFINITE);
    gs_valid_cool_dura_range.set_unit_str(g_str_dura_unit_s);

    gs_valid_cool_dura_factor.set_min_max(g_sys_configs_block.cool_dura_factor, 0);
    gs_valid_cool_dura_factor.set_edge(EDGE_INCLUDED, EDGE_INFINITE);

    ui->setupUi(this);
    m_expoDuraUnitBtnGrp = new QButtonGroup(this);
    m_expoDuraUnitBtnGrp->addButton(ui->expoDuraUnitmsRButton);
    m_expoDuraUnitBtnGrp->addButton(ui->expoDuraUnitsecRButton);
    m_expoDuraUnitBtnGrp->addButton(ui->expoDuraUnitminRButton);

    m_coolDuraModeBtnGrp = new QButtonGroup(this);
    m_coolDuraModeBtnGrp->addButton(ui->fixedCoolDuraRButton);
    m_coolDuraModeBtnGrp->addButton(ui->timesCoolDuraRButton);

    m_testContentBtnGrp =  new QButtonGroup(this);
    m_testContentBtnGrp->addButton(ui->testContentNormalRButton);
    m_testContentBtnGrp->addButton(ui->testContentCoolHVRButton);
    m_testContentBtnGrp->addButton(ui->testContentOnlyCoilRButton);
    m_testContentBtnGrp->addButton(ui->testContentDecoupleRButton);

    int idx;
    for(idx = 0; idx < ARRAY_COUNT(test_mode_list); ++idx)
    {
        ui->testModeComboBox->addItem(test_mode_list[idx].s, test_mode_list[idx].e);
    }

    ui->timesCoolDuraRButton->setChecked(true);
    ui->testModeComboBox->setCurrentIndex(0);
    ui->limitShortestCoolDuraChBox->setChecked(true);
    ui->readDistChBox->setChecked(true);

    ui->testContentNormalRButton->setChecked(true);

    m_test_params->test_content = get_test_content(nullptr); //default value
    QRadioButton *hidden_dura_rb, *mb_intf_dura_rb;
    arrange_ui_according_to_syscfgs(hidden_dura_rb, mb_intf_dura_rb);
    if(hidden_dura_rb->isChecked() || !(mb_intf_dura_rb->isChecked()))
    {
        mb_intf_dura_rb->setChecked(true);
    }

    /*------------------------------------------*/

    m_rec_ui_cfg_fin.clear();
    m_rec_ui_cfg_fout.insert(ui->custExpoParamFileNoteEdit);
    if(m_cfg_recorder) m_cfg_recorder->load_configs_to_ui(this,
                                                          m_rec_ui_cfg_fin, m_rec_ui_cfg_fout);
    if(hidden_dura_rb->isChecked())
    {
        mb_intf_dura_rb->setChecked(true);
    }

    m_test_params->test_content = get_test_content(nullptr);
    update_current_name_and_unit();
    update_expo_dura_unit(false);

    SAVE_EXPO_DURA_UI_TO_SW(m_test_params->expo_param_block.ui_to_sw_dura_factor);

    TEST_PARAMS_CTRLS_ABT_TBL;

    setup_judge_ctrls();

    if(!g_sys_configs_block.distance_group_disp)
    {
        ui->readDistChBox->setCheckState(Qt::Unchecked);
    }
    refresh_controls_display();

}

#undef ARRANGE_CTRLS_ABILITY
#undef TEST_PARAMS_CTRLS_ABT_TBL

testParamSettingsDialog::~testParamSettingsDialog()
{
    m_rec_ui_cfg_fin.clear();
    m_rec_ui_cfg_fout.clear();

    m_judge_ctrls.clear();
    m_judge_ctrl_desc_map.clear();

    delete ui;
}

template <typename T>
bool testParamSettingsDialog::
get_one_expo_param(QLineEdit * ctrl, common_data_type_enum_t d_type, float ui_to_sw_factor,
               RangeChecker<T>* range, void* val_ptr, QString &ret_str, QString new_unit_str )
{
    QString ctrl_str, d_type_str;
    bool tr_ret, ret;
    float ui_val, tr_val;

    if(!ctrl)
    {
        DIY_LOG(LOG_ERROR, "ctrl is NULL");
        return false;
    }

    ret_str.clear();

    ctrl_str = ctrl->whatsThis();
    (INT_DATA == d_type) ?  ui_val = ctrl->text().toInt(&tr_ret) :
                            ui_val = ctrl->text().toFloat(&tr_ret);
    if(!tr_ret)
    {
        d_type_str = (INT_DATA == d_type) ? gs_str_should_be_int : gs_str_should_be_number;
        ret_str = ctrl_str + d_type_str + "\n";
        return false;
    }

    tr_val = ui_val * ui_to_sw_factor;
    ret = true;
    if(range && !range->range_check(tr_val))
    {
        ret_str = QString("%1%2 ").arg(ctrl_str, gs_str_exceeds_valid_range);

        ret_str += range->range_str(d_type, 1/ui_to_sw_factor, new_unit_str);
        ret_str += "\n";

        ret = false;
    }
    else if(val_ptr)
    {
        (INT_DATA == d_type) ? *((int*)val_ptr) = (int)tr_val :
                               *((float*)val_ptr) = (float)tr_val;
    }

    return ret;
}

float testParamSettingsDialog::cube_current_trans_factor(expo_params_trans_factor_e_t trans,
                                                         QString file_unit_str)

{
    // sw use ma.
    float factor;

    //sw always used ma
    if(EXPO_PARAMS_UI_TO_SW == trans)
    {
        switch(m_used_current_ui_unit)
        {
        case MB_CUBE_CURRENT_UNIT_A:
            factor = 1000;
            break;

        case MB_CUBE_CURRENT_UNIT_MA:
            factor = 1;
            break;

        default: //MB_CUBE_CURRENT_UNIT_UA:
            factor = (float)(1.0/1000);
            break;
        }
    }
    else if(EXPO_PARAMS_SW_TO_MB_INTF == trans)
    {
        switch(m_used_current_intf_unit)
        {
        case MB_CUBE_CURRENT_UNIT_A:
            factor = (float)(1.0/1000);
            break;

        case MB_CUBE_CURRENT_UNIT_MA:
            factor = 1;
            break;

        default: //MB_CUBE_CURRENT_UNIT_UA:
            factor = 1000;
            break;
        }
    }
    else //EXPO_PARAMS_FILE_TO_SW
    {
        if(file_unit_str == g_str_current_unit_a)
        {
            factor = 1000;
        }
        else if(file_unit_str == g_str_current_unit_ma)
        {
            factor = 1;
        }
        else //g_str_current_unit_ua
        {
            factor = (float)(1.0/1000);
            file_unit_str = g_str_current_unit_ua;
        }
    }

    return factor;
}

float testParamSettingsDialog::expo_dura_trans_factor(expo_params_trans_factor_e_t trans,
                                                      QString file_unit_str)
{
    //sw use ma.
    float factor;

    if(EXPO_PARAMS_UI_TO_SW == trans || EXPO_PARAMS_FILE_TO_SW == trans)
    {
        QString str_to_check = ((EXPO_PARAMS_UI_TO_SW == trans) ? m_ui_expo_dura_unit_str : file_unit_str);
        if(g_str_dura_unit_min == str_to_check)
        {
            factor = 60 * 1000;
        }
        else if(g_str_dura_unit_s == str_to_check)
        {
            factor = 1000;
        }
        else //ms
        {
            factor = 1;
        }
    }
    else //EXPO_PARAMS_SW_TO_MB_INTF
    {
        switch(g_sys_configs_block.mb_dura_intf_unit)
        {
            case MB_DURA_UNIT_MIN:
                factor = (float)(1.0/(60*1000));
                break;
            case MB_DURA_UNIT_SEC:
                factor = (float)(1.0/1000);
                break;
            default: //MB_DURA_UNIT_MS:
                factor = 1;
                break;
        }
    }

    return factor;
}

void testParamSettingsDialog::get_expo_param_vals_from_ui()
{
    float ui_to_sw_factor;
    QString new_unit_str = "";

    m_expo_params_from_ui.valid_cube_volt_start =
        get_one_expo_param<int>(ui->cubeVoltStartEdit, INT_DATA, 1,
                           &gs_valid_cube_volt_kv_range,
                           &m_expo_params_from_ui.vals.cube_volt_kv_start,
                           m_expo_params_from_ui.err_msg_cube_volt_start);
    m_expo_params_from_ui.valid_cube_volt_end =
        get_one_expo_param<int>(ui->cubeVoltEndEdit, INT_DATA, 1,
                           &gs_valid_cube_volt_kv_range,
                           &m_expo_params_from_ui.vals.cube_volt_kv_end,
                           m_expo_params_from_ui.err_msg_cube_volt_end);
    m_expo_params_from_ui.valid_cube_volt_step =
        get_one_expo_param<int>(ui->cubeVoltStepEdit, INT_DATA, 1,
                            nullptr,
                           &m_expo_params_from_ui.vals.cube_volt_kv_step,
                           m_expo_params_from_ui.err_msg_cube_volt_step);

    ui_to_sw_factor = m_test_params->expo_param_block.ui_to_sw_current_factor;
    new_unit_str = m_ui_cube_current_unit_str;
    m_expo_params_from_ui.valid_cube_current_start =
        get_one_expo_param<float>(ui->cubeCurrentStartEdit, FLOAT_DATA, ui_to_sw_factor,
                           &gs_valid_cube_current_ma_range,
                           &m_expo_params_from_ui.vals.cube_current_ma_start,
                           m_expo_params_from_ui.err_msg_cube_current_start, new_unit_str);
    m_expo_params_from_ui.valid_cube_current_end =
        get_one_expo_param<float>(ui->cubeCurrentEndEdit, FLOAT_DATA, ui_to_sw_factor,
                           &gs_valid_cube_current_ma_range,
                           &m_expo_params_from_ui.vals.cube_current_ma_end,
                           m_expo_params_from_ui.err_msg_cube_current_end, new_unit_str);
    m_expo_params_from_ui.valid_cube_current_step =
        get_one_expo_param<float>(ui->cubeCurrentStepEdit, FLOAT_DATA, ui_to_sw_factor,
                           nullptr,
                           &m_expo_params_from_ui.vals.cube_current_ma_step,
                           m_expo_params_from_ui.err_msg_cube_current_step, new_unit_str);

    ui_to_sw_factor = m_test_params->expo_param_block.ui_to_sw_dura_factor;
    new_unit_str = m_ui_expo_dura_unit_str;
    m_expo_params_from_ui.valid_expo_dura_start =
        get_one_expo_param<float>(ui->expoDuraStartEdit, FLOAT_DATA, ui_to_sw_factor,
                           &gs_valid_expo_dura_ms_range,
                           &m_expo_params_from_ui.vals.expo_dura_ms_start,
                           m_expo_params_from_ui.err_msg_expo_dura_start, new_unit_str);
    m_expo_params_from_ui.valid_expo_dura_end =
        get_one_expo_param<float>(ui->expoDuraEndEdit, FLOAT_DATA, ui_to_sw_factor,
                           &gs_valid_expo_dura_ms_range,
                           &m_expo_params_from_ui.vals.expo_dura_ms_end,
                           m_expo_params_from_ui.err_msg_expo_dura_end, new_unit_str);
    m_expo_params_from_ui.valid_expo_dura_step =
        get_one_expo_param<float>(ui->expoDuraStepEdit, FLOAT_DATA, ui_to_sw_factor,
                           nullptr,
                           &m_expo_params_from_ui.vals.expo_dura_ms_step,
                           m_expo_params_from_ui.err_msg_expo_dura_step);

    m_expo_params_from_ui.valid_expo_cnt =
        get_one_expo_param<int>(ui->repeatsNumEdit, INT_DATA, 1,
                           &gs_valid_expo_cnt_range,
                           &m_expo_params_from_ui.expo_cnt,
                           m_expo_params_from_ui.err_msg_expo_cnt);

    m_expo_params_from_ui.limit_shortest_cool_dura
            = ui->limitShortestCoolDuraChBox->isChecked();

    m_expo_params_from_ui.fixed_cool_dura = ui->fixedCoolDuraRButton->isChecked();
    if(!m_expo_params_from_ui.fixed_cool_dura)
    {
        ui->coolDuraEdit->setWhatsThis(ui->timesCoolDuraRButton->text());
        if(m_expo_params_from_ui.limit_shortest_cool_dura)
        {
            gs_valid_cool_dura_factor.set_min_max(g_sys_configs_block.cool_dura_factor, 0);
            gs_valid_cool_dura_factor.set_edge(EDGE_INCLUDED, EDGE_INFINITE);
        }
        else
        {
            gs_valid_cool_dura_factor.set_min_max(0, 0);
            gs_valid_cool_dura_factor.set_edge(EDGE_INCLUDED, EDGE_INFINITE);
        }
        m_expo_params_from_ui.valid_cool_dura_factor =
            get_one_expo_param<float>(ui->coolDuraEdit, FLOAT_DATA, 1,
                               &gs_valid_cool_dura_factor,
                               &m_expo_params_from_ui.expo_cool_dura_factor,
                               m_expo_params_from_ui.err_msg_cool_dura_factor);
    }
    else
    {
        ui->coolDuraEdit->setWhatsThis(gs_str_cool_dura);
    }

    /* cool duration is not got here because it is related to the exposure duration.
     * so we count it after checking the latter.
     */
    m_expo_params_from_ui.read_dist = ui->readDistChBox->isChecked();
}

bool is_cube_volt_unit_str_valid(QString s)
{
    return (s == g_str_volt_unit_kv);
}

bool is_cube_current_unit_str_valid(QString s)
{
    return ((s == g_str_current_unit_ua) || (s == g_str_current_unit_ma) || (s == g_str_current_unit_a));
}

bool is_expo_dura_unit_str_valid(QString s)
{
    return ((s == g_str_dura_unit_ms) || (s == g_str_dura_unit_s) || (s == g_str_dura_unit_min));
}

QString valid_cube_volt_header_strs()
{
    return QString("\"%1\"").arg(gs_valid_cube_volt_header_str);
}

QString valid_cube_current_header_strs()
{
    return QString("\"%1%2%3\"").arg(gs_valid_cube_current_header_prefix_str,
                             gs_cust_expo_file_v_unit_sep, g_str_current_unit_ua)
           + " " + g_str_or + " "
           + QString("\"%1%2%3\"").arg(gs_valid_cube_current_header_prefix_str,
                             gs_cust_expo_file_v_unit_sep, g_str_current_unit_ma);
}

QString valid_expo_dura_current_header_str()
{
    return QString("\"%1%2%3\"").arg(gs_valid_expo_dura_header_prefix_str,
                             gs_cust_expo_file_v_unit_sep, g_str_dura_unit_ms)
           + " " + g_str_or + " "
           + QString("\"%1%2%3\"").arg(gs_valid_expo_dura_header_prefix_str,
                             gs_cust_expo_file_v_unit_sep, g_str_dura_unit_s)
           + " " + g_str_or + " "
           + QString("\"%1%2%3\"").arg(gs_valid_expo_dura_header_prefix_str,
                             gs_cust_expo_file_v_unit_sep, g_str_dura_unit_min);

}

bool testParamSettingsDialog::get_expo_param_vals_from_cust_file(QString file_fpn,
                                        QVector<expo_param_triple_struct_t> &param_vector,
                                        float * max_expo_dura_ms,
                                        QString &ret_str, QString &file_content)
{
    /*
     *example:
     *
        volt-kv,current-ma,dura-s
        90,0.5,1
    */
    if(file_fpn.isEmpty())
    {
        DIY_LOG(LOG_ERROR, "Cust expo parameters file name is empty!");
        ret_str = QString(gs_str_please_select_file);
        return false;
    }

    QFile cust_file(file_fpn);
    if(!cust_file.open(QFile::ReadOnly))
    {
        DIY_LOG(LOG_ERROR, QString("%1%2").arg("Read cust file %s error!", file_fpn));
        ret_str = QString("%1:\n%2").arg(gs_str_read_file_fail, file_fpn);
        return false;
    }
    QTextStream file_stream(&cust_file);
    QString line, file_current_unit_str, file_dura_unit_str;
    QStringList h_items, v_u_items;
    float cube_current_file_to_sw_factor, expo_dura_file_to_sw_factor;
    bool valid_line = false;
    float max_ms = 0;

    if(file_stream.readLineInto(&line))
    {
        do
        {
            h_items = line.split(gs_cust_expo_file_item_sep_in_line, Qt::SkipEmptyParts);
            if(h_items.count() < gs_cust_expo_file_item_num_in_line)
            {
                break;
            }

            file_current_unit_str = h_items[gs_cust_expo_file_current_item_idx];
            if(!file_current_unit_str.contains(gs_cust_expo_file_v_unit_sep))
            {
                break;
            }
            v_u_items = file_current_unit_str.split(gs_cust_expo_file_v_unit_sep);
            file_current_unit_str = v_u_items[1];
            if(!is_cube_current_unit_str_valid(file_current_unit_str))
            {
                break;
            }
            cube_current_file_to_sw_factor
                    = cube_current_trans_factor(EXPO_PARAMS_FILE_TO_SW, file_current_unit_str);

            file_dura_unit_str = h_items[gs_cust_expo_file_dura_item_idx];
            if(!file_dura_unit_str.contains(gs_cust_expo_file_v_unit_sep))
            {
                break;
            }
            v_u_items = file_dura_unit_str.split(gs_cust_expo_file_v_unit_sep);
            file_dura_unit_str = v_u_items[1];
            if(!is_expo_dura_unit_str_valid(file_dura_unit_str))
            {
                break;
            }
            expo_dura_file_to_sw_factor = expo_dura_trans_factor(EXPO_PARAMS_FILE_TO_SW, file_dura_unit_str);
            valid_line = true;
            break;
        }while(true);
    }
    file_content += line + "\n";
    if(!valid_line)
    {
        QString header_like_str = gs_str_1st_line_should_be;
        header_like_str += QString(gs_str_header_with_unit) + "，" + gs_str_sep_by_comma + "\n";
        header_like_str += QString(g_str_cube_volt) + g_str_can_be + " " + valid_cube_volt_header_strs() + "\n";
        header_like_str += QString(gs_str_current_name) + g_str_can_be + " " + valid_cube_current_header_strs() + "\n";
        header_like_str += QString(g_str_expo_dura) + g_str_can_be + " " + valid_expo_dura_current_header_str() + "\n";
        header_like_str += QString(g_str_for_examp) + " \"" + gs_valid_header_line_s + "\"\n";

        DIY_LOG(LOG_ERROR,
                QString("%1:%2\nbut it is:%3\n\n").arg(file_fpn, header_like_str, line))
        ret_str = header_like_str;
        cust_file.close();
        return false;
    }

    m_test_params->expo_param_block.file_to_sw_current_factor = cube_current_file_to_sw_factor;
    m_test_params->expo_param_block.file_to_sw_dura_factor = expo_dura_file_to_sw_factor;

    int line_no = 2;
    QStringList line_items;
    QString item_str;
    expo_param_triple_struct_t expo_param_triple;
    bool c2n_ret;
    while(valid_line && file_stream.readLineInto(&line))
    {
        file_content += line + "\n";
        line_items = line.split(gs_cust_expo_file_item_sep_in_line, Qt::SkipEmptyParts);
        while(true)
        {
            if(line_items.count() == gs_cust_expo_file_item_num_in_line)
            {
                item_str = line_items.at(gs_cust_expo_file_volt_item_idx);
                expo_param_triple.cube_volt_kv = item_str.toInt(&c2n_ret);
                if(c2n_ret)
                {
                    if(!gs_valid_cube_volt_kv_range.
                            range_check(expo_param_triple.cube_volt_kv))
                    {
                        valid_line = false;
                        ret_str = QString("%1%2%3%4%5:\n%6%7,%8%9").arg(gs_str_cust_file,
                                  g_str_the_line_pron, QString::number(line_no), gs_str_line,
                                  gs_str_data_item_invalid,
                                  g_str_cube_volt, item_str, gs_str_exceeds_valid_range,
                                  gs_valid_cube_volt_kv_range.range_str(INT_DATA));
                        DIY_LOG(LOG_ERROR, QString("%1%2%3%4%5").arg(file_fpn,
                                           ":the ", QString::number(line_no),
                                           "cube volt exceeds valid range:",
                                           gs_valid_cube_volt_kv_range.range_str(INT_DATA)));
                        break;
                    }
                }
                else
                {
                    valid_line = false;
                    ret_str = QString("%1%2%3%4%5:\n%6%7,%8").arg(gs_str_cust_file,
                                  g_str_the_line_pron, QString::number(line_no), gs_str_line,
                                  gs_str_data_item_invalid,
                                  g_str_cube_volt, item_str, gs_str_should_be_int);
                    DIY_LOG(LOG_ERROR, QString("%1%2%3%4").arg(file_fpn, ":the ",
                               QString::number(line_no), " line, cube volt should be int."));
                    break;
                }

                item_str = line_items.at(gs_cust_expo_file_current_item_idx);
                expo_param_triple.cube_current_ma = item_str.toFloat(&c2n_ret);
                if(c2n_ret)
                {
                    expo_param_triple.cube_current_ma *= cube_current_file_to_sw_factor;
                    if(!gs_valid_cube_current_ma_range.
                            range_check(expo_param_triple.cube_current_ma))
                    {
                        valid_line = false;
                        ret_str = QString("%1%2%3%4%5\n:%6%7,%8%9").arg(gs_str_cust_file,
                                  g_str_the_line_pron, QString::number(line_no), gs_str_line,
                                  gs_str_data_item_invalid,
                                  gs_str_current_name, item_str, gs_str_exceeds_valid_range,
                              gs_valid_cube_current_ma_range.range_str(FLOAT_DATA,
                                                       1/cube_current_file_to_sw_factor,
                                                       file_current_unit_str));
                        DIY_LOG(LOG_ERROR, QString("%1%2%3%4%5").arg(file_fpn,
                                       ":the ", QString::number(line_no),
                                       "cube current exceeds valid range:",
                               gs_valid_cube_current_ma_range.range_str(FLOAT_DATA,
                                                       1/cube_current_file_to_sw_factor,
                                                       file_current_unit_str)));
                        break;
                    }
                }
                else
                {
                    valid_line = false;
                    ret_str = QString("%1%2%3%4%5:\n%6%7,%8").arg(gs_str_cust_file,
                                  g_str_the_line_pron, QString::number(line_no), gs_str_line,
                                  gs_str_data_item_invalid,
                                  gs_str_current_name, item_str, gs_str_should_be_number);
                    DIY_LOG(LOG_ERROR, QString("%1%2%3%4").arg(file_fpn, ":the ",
                           QString::number(line_no), " line, cube current should be number."));
                    break;
                }

                item_str = line_items.at(gs_cust_expo_file_dura_item_idx);
                expo_param_triple.dura_ms = item_str.toFloat(&c2n_ret);
                if(c2n_ret)
                {
                    expo_param_triple.dura_ms *= expo_dura_file_to_sw_factor;
                    max_ms = expo_param_triple.dura_ms > max_ms ? expo_param_triple.dura_ms : max_ms;
                    if(!gs_valid_expo_dura_ms_range.
                            range_check(expo_param_triple.dura_ms))
                    {
                        valid_line = false;
                        ret_str = QString("%1%2%3%4%5:\n%6%7,%8%9").arg(gs_str_cust_file,
                                  g_str_the_line_pron, QString::number(line_no), gs_str_line,
                                  gs_str_data_item_invalid,
                                  g_str_expo_dura, item_str, gs_str_exceeds_valid_range,
                                  gs_valid_expo_dura_ms_range.range_str(FLOAT_DATA, 1/expo_dura_file_to_sw_factor,
                                                                        file_dura_unit_str));
                        DIY_LOG(LOG_ERROR, QString("%1%2%3%4%5").arg(file_fpn,
                                       ":the ", QString::number(line_no),
                                       "expo duration exceeds valid range:",
                                       gs_valid_expo_dura_ms_range.range_str(FLOAT_DATA, 1/expo_dura_file_to_sw_factor,
                                                                             file_dura_unit_str)));
                        break;
                    }
                }
                else
                {
                    valid_line = false;
                    ret_str = QString("%1%2%3%4%5:\n%6%7,%8").arg(gs_str_cust_file,
                                  g_str_the_line_pron, QString::number(line_no), gs_str_line,
                                  gs_str_data_item_invalid,
                                  g_str_expo_dura, item_str, gs_str_should_be_number);
                    DIY_LOG(LOG_ERROR, QString("%1%2%3%4").arg(file_fpn, ":the ",
                           QString::number(line_no), " line, expo duration should be number."));
                    break;
                }
                param_vector.append(expo_param_triple);
                ++line_no;
                break;
            }
            else
            {
                valid_line = false;
                ret_str = QString("%1%2%3%4%5").arg(gs_str_cust_file,
                                  g_str_the_line_pron, QString::number(line_no), gs_str_line,
                                  gs_str_no_valid_data_item);
                DIY_LOG(LOG_ERROR, QString("%1%2%3%4").arg(file_fpn, ":the ",
                                       QString::number(line_no), " line has no valid data."));
                break;
            }
        }
    }
    cust_file.close();
    if(valid_line && param_vector.count())
    {
        if(max_expo_dura_ms) *max_expo_dura_ms = max_ms;
        return true;
    }
    else
    {
        if(valid_line) ret_str += QString("%1%2").arg(gs_str_cust_file, gs_str_no_valid_data_item);
        return false;
    }
}

bool testParamSettingsDialog::get_expo_param_vals_from_cust2_file(QString file_fpn,
                                        QVector<expo_param_triple_struct_t> &param_vector,
                                        float * max_expo_dura_ms,
                                        QString &ret_str, QString &file_content)
{
    /*  example:
     *
        volt-kv,60,70,80,90
        current-ma,3.15,5
        dura-ms,500,625,800

        the 1st item of the last line must be dura-ms or dura-s.
    */
    if(file_fpn.isEmpty())
    {
        DIY_LOG(LOG_ERROR, "Cust expo parameters file name is empty!");
        ret_str = QString(gs_str_please_select_file);
        return false;
    }

#define READ_CUST_FILE_ERR \
{\
    DIY_LOG(LOG_ERROR, QString("%1%2").arg("Read cust file %s error!", file_fpn));\
    ret_str = QString("%1:\n%2").arg(gs_str_read_file_fail, file_fpn);\
    return false;\
}
    QFile cust_file(file_fpn);
    if(!cust_file.open(QFile::ReadOnly)) READ_CUST_FILE_ERR;

    QTextStream file_stream(&cust_file);
    QString line;
    float cube_current_file_to_sw_factor, expo_dura_file_to_sw_factor;
    QString file_current_unit_str, file_dura_unit_str;
    QStringList h_items, v_u_items;
    bool tr_ok;
    float max_ms = 0;
    QStringList line_items;
    int line_no;
    int item_cnt;
    static const int min_items_per_line = 2;

#define CUST_FILE_LINE_FORMAT_ERROR \
{\
    ret_str = QString("%1%2%3%4%5\n%6").arg(gs_str_cust_file,\
                      g_str_the_line_pron, QString::number(line_no), gs_str_line,\
                      gs_str_format_error, gs_str_cust2_err_msg_format);\
    DIY_LOG(LOG_ERROR, QString("%1%2%3%4").arg(file_fpn, ":the ",\
                           QString::number(line_no), " line has no valid data."));\
    cust_file.close();\
    return false;\
}

#define CUST_FILE_ITEM_FORMAT_ERROR(extra_str) \
{\
    ret_str = QString("%1%2%3%4%5%6%7\n%8").arg(gs_str_cust_file,\
                      g_str_the_line_pron, QString::number(line_no), gs_str_line,\
                      g_str_the_line_pron, QString::number(item_idx), gs_str_item,\
                      extra_str);\
    DIY_LOG(LOG_ERROR, QString("%1%2%3%4%5").arg(file_fpn, ":the ",\
                           QString::number(line_no), " line has no valid data.", extra_str));\
    cust_file.close();\
    return false;\
}

#define WRONG_HEADER_STR(valid_str) \
{\
    ret_str = QString("%1%2%3%4").arg(gs_str_cust_file, g_str_the_line_pron,\
                                      QString::number(line_no), gs_str_line);\
    ret_str += QString("%1%2\n%3").arg(gs_str_the_1st_item, g_str_should_be, valid_str);\
    DIY_LOG(LOG_ERROR, QString("%1:\n%2").arg(file_fpn, ret_str));\
    cust_file.close();\
    return false;\
}

    int item_idx, volt_cnt, current_cnt, dura_cnt;
    /*1st line: cube volt*/
    if(!file_stream.readLineInto(&line)) READ_CUST_FILE_ERR;
    file_content += line + "\n";
    line_no = 1;
    line_items = line.split(gs_cust_expo_file_item_sep_in_line, Qt::SkipEmptyParts);
    item_cnt = line_items.count();
    if(item_cnt < min_items_per_line) CUST_FILE_LINE_FORMAT_ERROR;
    QVector<int> cube_volt_kv_v;
    for(item_idx = 1; item_idx < item_cnt; ++item_idx)
    {
        int kv;
        kv = line_items[item_idx].toInt(&tr_ok);
        if(!tr_ok) CUST_FILE_ITEM_FORMAT_ERROR(gs_str_should_be_number);
        if(!gs_valid_cube_volt_kv_range.range_check(kv))
            CUST_FILE_ITEM_FORMAT_ERROR(line_items[item_idx]
                                        + gs_str_exceeds_valid_range
                                        + gs_valid_cube_volt_kv_range.range_str(INT_DATA));
        cube_volt_kv_v.append(kv);
    }
    volt_cnt = cube_volt_kv_v.count();

    /*2nd line: cube current*/
    if(!file_stream.readLineInto(&line)) READ_CUST_FILE_ERR;
    file_content += line + "\n";
    ++line_no;
    line_items = line.split(gs_cust_expo_file_item_sep_in_line, Qt::SkipEmptyParts);
    item_cnt = line_items.count();
    if(item_cnt < min_items_per_line) CUST_FILE_LINE_FORMAT_ERROR;

    if(!line_items[0].contains(gs_cust_expo_file_v_unit_sep)) CUST_FILE_LINE_FORMAT_ERROR;
    v_u_items = line_items[0].split(gs_cust_expo_file_v_unit_sep);
    file_current_unit_str = v_u_items[1];
    if(!is_cube_current_unit_str_valid(file_current_unit_str))
    {
        WRONG_HEADER_STR(valid_cube_current_header_strs());
    }
    cube_current_file_to_sw_factor
            = cube_current_trans_factor(EXPO_PARAMS_FILE_TO_SW, file_current_unit_str);
    QVector<float> cube_current_ma_v;
    for(item_idx = 1; item_idx < item_cnt; ++item_idx)
    {
        float ma;
        ma = line_items[item_idx].toFloat(&tr_ok);
        if(!tr_ok) CUST_FILE_ITEM_FORMAT_ERROR(gs_str_should_be_number);
        ma *= cube_current_file_to_sw_factor;
        if(!gs_valid_cube_current_ma_range.range_check(ma))
            CUST_FILE_ITEM_FORMAT_ERROR(line_items[item_idx]
                                        + gs_str_exceeds_valid_range
                                        + gs_valid_cube_current_ma_range.range_str(FLOAT_DATA,
                                                           1/cube_current_file_to_sw_factor,
                                                           file_current_unit_str));
        cube_current_ma_v.append(ma);
    }
    current_cnt = cube_current_ma_v.count();
    m_test_params->expo_param_block.file_to_sw_current_factor = cube_current_file_to_sw_factor;

    /*3rd line: expo dura*/
    if(!file_stream.readLineInto(&line)) READ_CUST_FILE_ERR;
    file_content += line + "\n";
    ++line_no;
    line_items = line.split(gs_cust_expo_file_item_sep_in_line, Qt::SkipEmptyParts);
    item_cnt = line_items.count();
    if(item_cnt < min_items_per_line) CUST_FILE_LINE_FORMAT_ERROR;
    if(!line_items[0].contains(gs_cust_expo_file_v_unit_sep)) CUST_FILE_LINE_FORMAT_ERROR;
    v_u_items = line_items[0].split(gs_cust_expo_file_v_unit_sep);
    file_dura_unit_str = v_u_items[1];
    if(!is_expo_dura_unit_str_valid(file_dura_unit_str))
    {
        WRONG_HEADER_STR(valid_expo_dura_current_header_str());
    }
    expo_dura_file_to_sw_factor = expo_dura_trans_factor(EXPO_PARAMS_FILE_TO_SW, file_dura_unit_str);
    QVector<float> dura_ms_v;
    max_ms = 0;
    for(item_idx = 1; item_idx < item_cnt; ++item_idx)
    {
        float file_dura_v, ms;
        file_dura_v = line_items[item_idx].toFloat(&tr_ok);
        if(!tr_ok) CUST_FILE_ITEM_FORMAT_ERROR(gs_str_should_be_number);
        if(!gs_valid_expo_dura_ms_range.range_check(file_dura_v * expo_dura_file_to_sw_factor))
            CUST_FILE_ITEM_FORMAT_ERROR(line_items[item_idx]
                                        + gs_str_exceeds_valid_range
                                        + gs_valid_expo_dura_ms_range.range_str(FLOAT_DATA,
                                                                              1/expo_dura_file_to_sw_factor,
                                                                              file_dura_unit_str));
        ms = file_dura_v * expo_dura_file_to_sw_factor;
        dura_ms_v.append(ms);
        if(ms > max_ms) max_ms = ms;
    }
    dura_cnt = dura_ms_v.count();
    m_test_params->expo_param_block.file_to_sw_dura_factor = expo_dura_file_to_sw_factor;

    int i, j, k;
    expo_param_triple_struct_t expo_param_triple;
    for(i = 0; i < volt_cnt; ++i)
    {
        expo_param_triple.cube_volt_kv = cube_volt_kv_v.at(i);
        for(j = 0; j < current_cnt; ++j)
        {
            expo_param_triple.cube_current_ma = cube_current_ma_v.at(j);
            for(k = 0; k < dura_cnt; ++k)
            {
                expo_param_triple.dura_ms = dura_ms_v.at(k);
                param_vector.append(expo_param_triple);
            }
        }
    }

    if(max_expo_dura_ms) *max_expo_dura_ms = max_ms;
    return true;

#undef WRONG_HEADER_STR
#undef CUST_FILE_ITEM_FORMAT_ERROR
#undef CUST_FILE_LINE_FORMAT_ERROR
#undef READ_CUST_FILE_ERR
}

test_content_enum_t testParamSettingsDialog::get_test_content(QString *content_str)
{
    if(ui->testContentCoolHVRButton->isChecked())
    {
        if(content_str) *content_str = ui->testContentCoolHVRButton->text();
        return TEST_CONTENT_COOL_HV;
    }
    else if(ui->testContentOnlyCoilRButton->isChecked())
    {
        if(content_str) *content_str = ui->testContentOnlyCoilRButton->text();
        return TEST_CONTENT_ONLY_COIL;
    }
    else if(ui->testContentDecoupleRButton->isChecked())
    {
        if(content_str) *content_str = ui->testContentDecoupleRButton->text();
        return TEST_CONTENT_DECOUPLE;
    }
    else
    {
        if(content_str) *content_str = ui->testContentNormalRButton->text();
        return TEST_CONTENT_NORMAL;
    }
}

QString testParamSettingsDialog::collect_test_params()
{
    test_mode_enum_t test_mode;
    test_mode = (test_mode_enum_t)(ui->testModeComboBox->currentData().toInt());
    QString ret_str;
    float max_expo_dura;
    QString file_content;

    if(!m_test_params)
    {
        DIY_LOG(LOG_ERROR, "test params pointer is null.");
        return ret_str;
    }
    m_test_params->valid = false;
    m_test_params->test_mode = TEST_MODE_SINGLE;
    m_test_params->info_str.clear();

    get_expo_param_vals_from_ui();

    m_test_params->expo_param_block.fixed_cool_dura = m_expo_params_from_ui.fixed_cool_dura;
    if(!m_expo_params_from_ui.fixed_cool_dura && !m_expo_params_from_ui.valid_cool_dura_factor)
    {
        ret_str += m_expo_params_from_ui.err_msg_cool_dura_factor;
    }
    else if((TEST_MODE_CUST1_TRIPLES == test_mode) || (TEST_MODE_CUST2_DISCRETE == test_mode))
    {
        m_test_params->expo_param_block.cust = false;
        m_test_params->expo_param_block.expo_params.cust_params_arr.clear();
        if(!m_expo_params_from_ui.valid_expo_cnt)
        {
            ret_str += m_expo_params_from_ui.err_msg_expo_cnt;
        }
        else
        {
            m_test_params->expo_param_block.expo_cnt = m_expo_params_from_ui.expo_cnt;

            bool draw_from_file;
            if(TEST_MODE_CUST1_TRIPLES == test_mode)
            {
                draw_from_file =
                        get_expo_param_vals_from_cust_file(ui->custExpoParamFileSelEdit->text(),
                               m_test_params->expo_param_block.expo_params.cust_params_arr,
                               &max_expo_dura,
                               ret_str, file_content);
            }
            else
            {
                draw_from_file =
                        get_expo_param_vals_from_cust2_file(ui->custExpoParamFileSelEdit->text(),
                               m_test_params->expo_param_block.expo_params.cust_params_arr,
                               &max_expo_dura,
                               ret_str, file_content);
            }
            if(draw_from_file)
            {
                m_test_params->expo_param_block.cust = true;
                if(m_expo_params_from_ui.fixed_cool_dura)
                {
                    if(m_expo_params_from_ui.limit_shortest_cool_dura)
                    {
                        gs_valid_cool_dura_range.set_min_max(
                                    max_expo_dura * g_sys_configs_block.cool_dura_factor,
                                     0);
                        gs_valid_cool_dura_range.set_edge(EDGE_INCLUDED, EDGE_INFINITE);
                    }
                    else
                    {
                        gs_valid_cool_dura_range.set_min_max(0, 0);
                        gs_valid_cool_dura_range.set_edge(EDGE_INCLUDED, EDGE_INFINITE);

                    }
                    m_expo_params_from_ui.valid_cool_dura =
                        get_one_expo_param<float>(ui->coolDuraEdit, FLOAT_DATA, 1000,
                                           &gs_valid_cool_dura_range,
                                           &m_expo_params_from_ui.expo_cool_dura_ms,
                                           m_expo_params_from_ui.err_msg_cool_dura);
                    if(m_expo_params_from_ui.valid_cool_dura)
                    {
                        m_test_params->expo_param_block.expo_cool_dura_ms
                                = m_expo_params_from_ui.expo_cool_dura_ms;
                        m_test_params->valid = true;

                    }
                    else
                    {
                        ret_str += m_expo_params_from_ui.err_msg_cool_dura;
                    }
                    m_test_params->expo_param_block.expo_cool_dura_factor = 1; //no use in fact
                }
                else
                {
                    m_test_params->expo_param_block.expo_cool_dura_factor
                            = m_expo_params_from_ui.expo_cool_dura_factor;
                    m_test_params->valid = true;

                    m_expo_params_from_ui.valid_cool_dura = true; // no use in fact
                }
            }
        }
    }
    else
    {
        m_test_params->expo_param_block.cust = false;

        ret_str = m_expo_params_from_ui.err_msg_cube_volt_start
                 + m_expo_params_from_ui.err_msg_cube_current_start
                 + m_expo_params_from_ui.err_msg_expo_dura_start;
        switch(test_mode)
        {
        case TEST_MODE_SINGLE:
        case TEST_MODE_REPEAT:
            if(!m_expo_params_from_ui.valid_cube_volt_start
                || !m_expo_params_from_ui.valid_cube_current_start
                || !m_expo_params_from_ui.valid_expo_dura_start
                || (TEST_MODE_REPEAT == test_mode &&
                    (!m_expo_params_from_ui.valid_expo_cnt)))
            {
                if(TEST_MODE_REPEAT == test_mode)
                {
                    ret_str += m_expo_params_from_ui.err_msg_expo_cnt;
                }
            }
            else
            {
                m_test_params->expo_param_block.expo_params.regular_parms.cube_volt_kv_start
                        = m_expo_params_from_ui.vals.cube_volt_kv_start;
                m_test_params->expo_param_block.expo_params.regular_parms.cube_volt_kv_end
                        = m_expo_params_from_ui.vals.cube_volt_kv_start;
                m_test_params->expo_param_block.expo_params.regular_parms.cube_volt_kv_step
                        = 0;
                m_test_params->expo_param_block.expo_params.regular_parms.cube_current_ma_start
                        = m_expo_params_from_ui.vals.cube_current_ma_start;
                m_test_params->expo_param_block.expo_params.regular_parms.cube_current_ma_end
                        = m_expo_params_from_ui.vals.cube_current_ma_start;
                m_test_params->expo_param_block.expo_params.regular_parms.cube_current_ma_step
                        = 0;
                m_test_params->expo_param_block.expo_params.regular_parms.expo_dura_ms_start
                        = m_expo_params_from_ui.vals.expo_dura_ms_start;
                m_test_params->expo_param_block.expo_params.regular_parms.expo_dura_ms_end
                        = m_expo_params_from_ui.vals.expo_dura_ms_start;
                m_test_params->expo_param_block.expo_params.regular_parms.expo_dura_ms_step
                        = 0;
                m_test_params->expo_param_block.expo_cnt
                        = (TEST_MODE_SINGLE == test_mode) ? 1 : m_expo_params_from_ui.expo_cnt;

                if(TEST_MODE_REPEAT == test_mode)
                {
                    if(m_expo_params_from_ui.fixed_cool_dura)
                    {

                        if(m_expo_params_from_ui.limit_shortest_cool_dura)
                        {
                            gs_valid_cool_dura_range.
                            set_min_max(m_expo_params_from_ui.vals.expo_dura_ms_start
                                        * g_sys_configs_block.cool_dura_factor, 0);

                            gs_valid_cool_dura_range.set_edge(EDGE_INCLUDED, EDGE_INFINITE);
                        }
                        else
                        {
                            gs_valid_cool_dura_range.set_min_max(0, 0);
                            gs_valid_cool_dura_range.set_edge(EDGE_INCLUDED, EDGE_INFINITE);

                        }

                        m_expo_params_from_ui.valid_cool_dura =
                            get_one_expo_param<float>(ui->coolDuraEdit, FLOAT_DATA, 1000,
                                               &gs_valid_cool_dura_range,
                                               &m_expo_params_from_ui.expo_cool_dura_ms,
                                               m_expo_params_from_ui.err_msg_cool_dura);
                        if(!m_expo_params_from_ui.valid_cool_dura)
                        {
                            ret_str += m_expo_params_from_ui.err_msg_cool_dura;
                            break;
                        }
                        m_test_params->expo_param_block.expo_cool_dura_ms
                                = m_expo_params_from_ui.expo_cool_dura_ms;
                        m_test_params->expo_param_block.expo_cool_dura_factor = 1; //no use
                    }
                    else
                    {
                        m_test_params->expo_param_block.expo_cool_dura_factor
                                = m_expo_params_from_ui.expo_cool_dura_factor;
                        m_expo_params_from_ui.valid_cool_dura = true;
                        m_test_params->expo_param_block.expo_cool_dura_ms = 0;
                    }
                }
                else
                {
                    m_expo_params_from_ui.valid_cool_dura = true;
                    m_test_params->expo_param_block.expo_cool_dura_ms = 0;
                    m_test_params->expo_param_block.expo_cool_dura_factor = 1;
                }

                m_test_params->valid = true;
            }
            break;

        case TEST_MODE_TRAVERSE:
        default:
            if(!m_expo_params_from_ui.valid_cube_volt_start
                || !m_expo_params_from_ui.valid_cube_volt_end
                || !m_expo_params_from_ui.valid_cube_volt_step
                || !m_expo_params_from_ui.valid_cube_current_start
                || !m_expo_params_from_ui.valid_cube_current_end
                || !m_expo_params_from_ui.valid_cube_current_step
                || !m_expo_params_from_ui.valid_expo_dura_start
                || !m_expo_params_from_ui.valid_expo_dura_end
                || !m_expo_params_from_ui.valid_expo_dura_step
                || !m_expo_params_from_ui.valid_expo_cnt)
            {
                ret_str += m_expo_params_from_ui.err_msg_cube_volt_end
                        + m_expo_params_from_ui.err_msg_cube_volt_step
                        + m_expo_params_from_ui.err_msg_cube_current_end
                        + m_expo_params_from_ui.err_msg_cube_current_step
                        + m_expo_params_from_ui.err_msg_expo_dura_end
                        + m_expo_params_from_ui.err_msg_expo_dura_step
                        + m_expo_params_from_ui.err_msg_expo_cnt;
            }
            else
            {
                if(0 == m_expo_params_from_ui.vals.cube_volt_kv_step
                        && (m_expo_params_from_ui.vals.cube_volt_kv_end
                             != m_expo_params_from_ui.vals.cube_volt_kv_start))
                {
                    ret_str += gs_str_start_end_step_err1;
                    break;
                }
                else if(m_expo_params_from_ui.vals.cube_volt_kv_step *
                        (m_expo_params_from_ui.vals.cube_volt_kv_end
                             - m_expo_params_from_ui.vals.cube_volt_kv_start) < 0)
                {
                    ret_str += QString("%1%2、%3、%4、%5").arg(g_str_cube_volt,
                                           gs_str_start_val, gs_str_end_val, gs_str_step,
                                                    gs_str_start_end_step_err2);
                    break;
                }
                else
                {
                    m_test_params->expo_param_block.expo_params.regular_parms.cube_volt_kv_start
                            = m_expo_params_from_ui.vals.cube_volt_kv_start;
                    m_test_params->expo_param_block.expo_params.regular_parms.cube_volt_kv_end
                            = m_expo_params_from_ui.vals.cube_volt_kv_end;
                    m_test_params->expo_param_block.expo_params.regular_parms.cube_volt_kv_step
                            = m_expo_params_from_ui.vals.cube_volt_kv_step;
                }


                if(0 == m_expo_params_from_ui.vals.cube_current_ma_step
                        && (m_expo_params_from_ui.vals.cube_current_ma_end
                             != m_expo_params_from_ui.vals.cube_current_ma_start))
                {
                    ret_str += gs_str_start_end_step_err1;
                    break;
                }
                else if(m_expo_params_from_ui.vals.cube_current_ma_step *
                        (m_expo_params_from_ui.vals.cube_current_ma_end
                             - m_expo_params_from_ui.vals.cube_current_ma_start) < 0)
                {
                    ret_str += QString("%1%2、%3、%4、%5").arg(gs_str_current_name,
                                           gs_str_start_val, gs_str_end_val, gs_str_step,
                                                    gs_str_start_end_step_err2);
                    break;
                }
                else
                {
                    m_test_params->expo_param_block.expo_params.regular_parms.cube_current_ma_start
                            = m_expo_params_from_ui.vals.cube_current_ma_start;
                    m_test_params->expo_param_block.expo_params.regular_parms.cube_current_ma_end
                            = m_expo_params_from_ui.vals.cube_current_ma_end;
                    m_test_params->expo_param_block.expo_params.regular_parms.cube_current_ma_step
                            = m_expo_params_from_ui.vals.cube_current_ma_step;
                }

                if(0 == m_expo_params_from_ui.vals.expo_dura_ms_step
                        && (m_expo_params_from_ui.vals.expo_dura_ms_end
                             != m_expo_params_from_ui.vals.expo_dura_ms_start))
                {
                    ret_str += gs_str_start_end_step_err1;
                    break;
                }
                else if(m_expo_params_from_ui.vals.expo_dura_ms_step *
                        (m_expo_params_from_ui.vals.expo_dura_ms_end
                             - m_expo_params_from_ui.vals.expo_dura_ms_start) < 0)
                {
                    ret_str += QString("%1%2、%3、%4、%5").arg(g_str_expo_dura,
                                           gs_str_start_val, gs_str_end_val, gs_str_step,
                                                    gs_str_start_end_step_err2);
                    break;
                }
                else
                {
                    m_test_params->expo_param_block.expo_params.regular_parms.expo_dura_ms_start
                            = m_expo_params_from_ui.vals.expo_dura_ms_start;
                    m_test_params->expo_param_block.expo_params.regular_parms.expo_dura_ms_end
                            = m_expo_params_from_ui.vals.expo_dura_ms_end;
                    m_test_params->expo_param_block.expo_params.regular_parms.expo_dura_ms_step
                            = m_expo_params_from_ui.vals.expo_dura_ms_step;
                }

                if(m_expo_params_from_ui.fixed_cool_dura)
                {
                    max_expo_dura = qMax<float>(m_expo_params_from_ui.vals.expo_dura_ms_start,
                                         m_expo_params_from_ui.vals.expo_dura_ms_end);

                    if(m_expo_params_from_ui.limit_shortest_cool_dura)
                    {
                        gs_valid_cool_dura_range.set_min_max(
                                    max_expo_dura * g_sys_configs_block.cool_dura_factor,
                                     0);
                        gs_valid_cool_dura_range.set_edge(EDGE_INCLUDED, EDGE_INFINITE);
                    }
                    else
                    {
                        gs_valid_cool_dura_range.set_min_max(0, 0);
                        gs_valid_cool_dura_range.set_edge(EDGE_INCLUDED, EDGE_INFINITE);

                    }

                    m_expo_params_from_ui.valid_cool_dura =
                        get_one_expo_param<float>(ui->coolDuraEdit, FLOAT_DATA, 1000,
                                           &gs_valid_cool_dura_range,
                                           &m_expo_params_from_ui.expo_cool_dura_ms,
                                           m_expo_params_from_ui.err_msg_cool_dura);
                    if(!m_expo_params_from_ui.valid_cool_dura)
                    {
                        ret_str += m_expo_params_from_ui.err_msg_cool_dura;
                        break;
                    }

                    m_test_params->expo_param_block.expo_cool_dura_ms
                            = m_expo_params_from_ui.expo_cool_dura_ms;

                    m_test_params->expo_param_block.expo_cool_dura_factor = 1; //no use
                }
                else
                {
                    m_test_params->expo_param_block.expo_cool_dura_factor
                            = m_expo_params_from_ui.expo_cool_dura_factor;
                    m_expo_params_from_ui.valid_cool_dura = true;
                    m_test_params->expo_param_block.expo_cool_dura_ms = 0; // no use
                }

                m_test_params->expo_param_block.expo_cnt = m_expo_params_from_ui.expo_cnt;

                m_test_params->valid = true;
            }
            break;
        }
    }

    m_test_params->test_content = get_test_content(nullptr);

    m_test_params->other_param_block.oil_box_number_str = ui->oilBoxNoEdit->text();
    m_test_params->other_param_block.hv_ctrl_board_number_str = ui->hvCtrlBoardNoEdit->text();
    m_test_params->other_param_block.sw_ver_str = ui->swVerStrEdit->text();
    m_test_params->other_param_block.hw_ver_str = ui->hwVerStrEdit->text();

    m_test_params->other_param_block.read_dist = m_expo_params_from_ui.read_dist;

    format_test_params_info_str(file_content);

    {
        QString err_str, info_str;
        bool ret;

        info_str += QString("%1%2%3").arg("\n", gs_info_str_seperator, "\n");
        ret = construct_judge_params(err_str, info_str);
        m_test_params->valid = m_test_params->valid && ret;

        ret_str += err_str;
        m_test_params->info_str += info_str;
    }

    m_test_params->test_mode
            = (test_mode_enum_t)(ui->testModeComboBox->currentData().toInt());
    return ret_str;
}

void testParamSettingsDialog::refresh_controls_display()
{
    /*enable/disable controls based on test mode.*/
    test_mode_enum_t test_mode;
    test_mode = (test_mode_enum_t)(ui->testModeComboBox->currentData().toInt());

    QVector<test_params_ctrls_ability_struct_t>::ConstIterator it;
    for(it = m_test_params_ctrls_abilities.constBegin();
        it != m_test_params_ctrls_abilities.constEnd();
        ++it)
    {
        it->ctrl->setEnabled(it->ability[test_mode]);
    }

    switch(test_mode)
    {
    case TEST_MODE_CUST1_TRIPLES:
        ui->custExpoParamFileNoteEdit->setText(gs_str_cust1_notes);
        break;

    case TEST_MODE_CUST2_DISCRETE:
        ui->custExpoParamFileNoteEdit->setText(gs_str_cust2_notes);
        break;

    default:
        ui->custExpoParamFileNoteEdit->clear();
        break;
    }

    refresh_judge_ctrls_display();
}

void testParamSettingsDialog::refresh_judge_ctrls_display()
{
    bool ctrl_enabled = false, fixed_ref_enabled = false;

    if(ui->readDistChBox->checkState() == Qt::Unchecked)
    {
        ui->distmmChkbox->setCheckState(Qt::Unchecked);
    }

    ui->distmmIsFixedRefChkbox->setChecked(true);
    for(int idx = 0; idx < m_judge_ctrls.count(); ++idx)
    {
        ctrl_enabled = CHKBOX_EXIST_AND_CHECKED(m_judge_ctrls[idx].gui_ctrls.judge_or_not_chbox);
        fixed_ref_enabled = CHKBOX_EXIST_AND_CHECKED(m_judge_ctrls[idx].gui_ctrls.is_fixed_ref_chbox);

        CHECK_AND_SET_ENABLED(m_judge_ctrls[idx].gui_ctrls.low_e_pct_ledit, ctrl_enabled);
        CHECK_AND_SET_ENABLED(m_judge_ctrls[idx].gui_ctrls.up_e_pct_ledit, ctrl_enabled);
        CHECK_AND_SET_ENABLED(m_judge_ctrls[idx].gui_ctrls.low_e_err_val_ledit, ctrl_enabled);
        CHECK_AND_SET_ENABLED(m_judge_ctrls[idx].gui_ctrls.up_e_err_val_ledit, ctrl_enabled);
        CHECK_AND_SET_ENABLED(m_judge_ctrls[idx].gui_ctrls.is_fixed_ref_chbox, ctrl_enabled);
        CHECK_AND_SET_ENABLED(m_judge_ctrls[idx].gui_ctrls.fixed_ref_val_ledit,
                              ctrl_enabled && fixed_ref_enabled);
    }
    ui->distmmIsFixedRefChkbox->setEnabled(false);
}

void testParamSettingsDialog::on_testModeComboBox_currentIndexChanged(int /*index*/)
{
    refresh_controls_display();
}


void testParamSettingsDialog::on_custExpoParamFileSelBtn_clicked()
{
    QString cust_file_fpn;
    cust_file_fpn = QFileDialog::getOpenFileName(this, gs_str_cust_file_sel_caption,
                                                 ".", gs_cust_expo_file_filter);
    ui->custExpoParamFileSelEdit->setText(cust_file_fpn);
}

void testParamSettingsDialog::on_buttonBox_clicked(QAbstractButton *button)
{
    if(button == ui->buttonBox->button(QDialogButtonBox::Ok))
    {
        QString ret_str = collect_test_params();
        if(m_test_params->valid)
        {
            if(m_cfg_recorder) m_cfg_recorder->record_ui_configs(this,
                                                         m_rec_ui_cfg_fin, m_rec_ui_cfg_fout);
            accept();
        }
        else
        {
            QMessageBox::critical(this, "ERROR!", ret_str);
        }
    }
}

void testParamSettingsDialog::format_test_params_info_str(QString &file_content)
{
    test_mode_enum_t test_mode;
    QString current_unit_s, dura_unit_s;
    float cube_current_sw_to_ui_factor, expo_dura_sw_to_ui_factor;
    int times_in_one_loop = 1;

    test_mode = (test_mode_enum_t)(ui->testModeComboBox->currentData().toInt());

    if(!m_test_params || !m_test_params->valid) return;
    QString &info_str = m_test_params->info_str;

    cube_current_sw_to_ui_factor = 1/(m_test_params->expo_param_block.ui_to_sw_current_factor);
    current_unit_s = m_ui_cube_current_unit_str;

    expo_dura_sw_to_ui_factor = 1/(m_test_params->expo_param_block.ui_to_sw_dura_factor);
    dura_unit_s = m_ui_expo_dura_unit_str;

    info_str = QString("%1:").arg(gs_str_test_mode);
    info_str += test_mode_list[test_mode].s + "\n";
    info_str += QString(gs_info_str_seperator) + "\n";

    info_str += QString("%1:").arg(gs_str_test_content);
    QString tmp_str;
    get_test_content(&tmp_str);
    info_str += tmp_str + "\n" + gs_info_str_seperator + "\n";

    QString start_val_str("");
    if(TEST_MODE_TRAVERSE == test_mode) start_val_str = gs_str_start_val;
    if((TEST_MODE_CUST1_TRIPLES != test_mode) && (TEST_MODE_CUST2_DISCRETE != test_mode))
    {
        info_str += QString("%1%2:%3%4\n").
                    arg(g_str_cube_volt, start_val_str,
                        QString::number(m_test_params->expo_param_block.expo_params.
                                            regular_parms.cube_volt_kv_start),
                        g_str_volt_unit_kv);
        if(TEST_MODE_TRAVERSE == test_mode)
        {
            info_str += QString("%1%2:%3%4\n").
                        arg(g_str_cube_volt, gs_str_end_val,
                            QString::number(m_test_params->expo_param_block.expo_params.
                                                regular_parms.cube_volt_kv_end),
                            g_str_volt_unit_kv);
            info_str += QString("%1%2:%3%4\n").
                        arg(g_str_cube_volt, gs_str_step,
                            QString::number(m_test_params->expo_param_block.expo_params.
                                                regular_parms.cube_volt_kv_step),
                            g_str_volt_unit_kv);

            times_in_one_loop *= count_discrete_steps(m_test_params->expo_param_block.expo_params.
                                                          regular_parms.cube_volt_kv_start,
                                                      m_test_params->expo_param_block.expo_params.
                                                          regular_parms.cube_volt_kv_end,
                                                      m_test_params->expo_param_block.expo_params.
                                                          regular_parms.cube_volt_kv_step);
        }
        info_str += QString(gs_info_str_seperator) + "\n";

        info_str += QString("%1%2:%3%4\n").
                    arg(gs_str_current_name, start_val_str,
                        QString::number(cube_current_sw_to_ui_factor *
                        m_test_params->expo_param_block.expo_params.regular_parms.cube_current_ma_start),
                        current_unit_s);
        if(TEST_MODE_TRAVERSE == test_mode)
        {
            info_str += QString("%1%2:%3%4\n").
                        arg(gs_str_current_name, gs_str_end_val,
                            QString::number(cube_current_sw_to_ui_factor *
                                m_test_params->expo_param_block.expo_params.regular_parms.cube_current_ma_end),
                            current_unit_s);
            info_str += QString("%1%2:%3%4\n").
                        arg(gs_str_current_name, gs_str_step,
                            QString::number(cube_current_sw_to_ui_factor *
                                m_test_params->expo_param_block.expo_params.regular_parms.cube_current_ma_step),
                            current_unit_s);

            times_in_one_loop *= count_discrete_steps(m_test_params->expo_param_block.expo_params.
                                                          regular_parms.cube_current_ma_start,
                                                      m_test_params->expo_param_block.expo_params.
                                                          regular_parms.cube_current_ma_end,
                                                      m_test_params->expo_param_block.expo_params.
                                                          regular_parms.cube_current_ma_step);
        }
        info_str += QString(gs_info_str_seperator) + "\n";

        info_str += QString("%1%2:%3%4\n").
                    arg(g_str_expo_dura, start_val_str,
                        QString::number(expo_dura_sw_to_ui_factor *
                                        m_test_params->expo_param_block.expo_params.regular_parms.expo_dura_ms_start),
                        dura_unit_s);
        if(TEST_MODE_TRAVERSE == test_mode)
        {
            info_str += QString("%1%2:%3%4\n").
                        arg(g_str_expo_dura, gs_str_end_val,
                            QString::number(expo_dura_sw_to_ui_factor *
                                            m_test_params->expo_param_block.expo_params.regular_parms.expo_dura_ms_end),
                            dura_unit_s);
            info_str += QString("%1%2:%3%4\n").
                        arg(g_str_expo_dura, gs_str_step,
                            QString::number(m_test_params->expo_param_block.expo_params.
                                                regular_parms.expo_dura_ms_step * expo_dura_sw_to_ui_factor),
                            dura_unit_s);

            times_in_one_loop *= count_discrete_steps(m_test_params->expo_param_block.expo_params.
                                                          regular_parms.expo_dura_ms_start,
                                                      m_test_params->expo_param_block.expo_params.
                                                          regular_parms.expo_dura_ms_end,
                                                      m_test_params->expo_param_block.expo_params.
                                                          regular_parms.expo_dura_ms_step);
        }
        info_str += QString(gs_info_str_seperator) + "\n";

        if(TEST_MODE_TRAVERSE == test_mode || TEST_MODE_REPEAT == test_mode)
        {
            if(TEST_MODE_REPEAT == test_mode)
            {
                info_str += QString("%1:%2\n").
                            arg(gs_str_repeats_num,
                                QString::number(m_test_params->expo_param_block.expo_cnt));
            }
            else
            {
                info_str += QString("%1:%2*%3=%4\n").
                arg(gs_str_repeats_num,
                    QString::number(m_test_params->expo_param_block.expo_cnt),
                    QString::number(times_in_one_loop),
                    QString::number(m_test_params->expo_param_block.expo_cnt * times_in_one_loop));
            }
            info_str += QString(gs_info_str_seperator) + "\n";

            if(m_test_params->expo_param_block.fixed_cool_dura)
            {
                info_str += QString("%1:%2%3").
                    arg(gs_str_cool_dura,
                    QString::number(m_test_params->expo_param_block.expo_cool_dura_ms / 1000),
                    g_str_dura_unit_s);
            }
            else
            {
                info_str += QString("%1:%2%3%4").
                    arg(gs_str_cool_dura,
                        QString::number(m_test_params->expo_param_block.expo_cool_dura_factor),
                        g_str_time_bei,
                        g_str_expo_dura);
            }
            info_str += (g_sys_configs_block.extra_cool_time_ms > 0) ?
                        (QString("+(")
                         + QString::number(g_sys_configs_block.extra_cool_time_ms / 1000.0)
                         + g_str_dura_unit_s + gs_str_extra_cool_dura + ")") : "";
            info_str += "\n";

            info_str += QString(gs_info_str_seperator) + "\n";
        }
    }
    else
    {
        times_in_one_loop = m_test_params->expo_param_block.expo_params.cust_params_arr.count();
        info_str += file_content + "\n\n";
        info_str += QString("%1%2%3\n").
                        arg(QString::number(m_test_params->expo_param_block.expo_params.
                                            cust_params_arr.count()),
                            gs_str_group, gs_str_expo_params);
        info_str += QString("%1:%2*%3=%4\n").
                arg(gs_str_repeats_num,
                    QString::number(m_test_params->expo_param_block.expo_cnt),
                    QString::number(times_in_one_loop),
                    QString::number(m_test_params->expo_param_block.expo_cnt * times_in_one_loop));
        info_str += QString(gs_info_str_seperator) + "\n";
        if(m_test_params->expo_param_block.fixed_cool_dura)
        {
            info_str += QString("%1:%2%3").
                arg(gs_str_cool_dura,
                    QString::number(m_test_params->expo_param_block.expo_cool_dura_ms / 1000),
                    g_str_dura_unit_s);
        }
        else
        {
            info_str += QString("%1:%2%3%4").
                arg(gs_str_cool_dura,
                    QString::number(m_test_params->expo_param_block.expo_cool_dura_factor),
                    g_str_time_bei,
                    g_str_expo_dura);
        }
        info_str += (g_sys_configs_block.extra_cool_time_ms > 0) ?
                    (QString("+(")
                     + QString::number((g_sys_configs_block.extra_cool_time_ms) / 1000.0)
                     + g_str_dura_unit_s + gs_str_extra_cool_dura + ")") : "";
        info_str += "\n";
        info_str += QString(gs_info_str_seperator) + "\n";
    }

    if(!ui->readDistChBox->isChecked())
    {
        info_str += g_str_no_bu;
    }
    info_str += ui->readDistChBox->text() + "\n";
    info_str += QString(gs_info_str_seperator) + "\n";

    info_str += ui->oilBoxNoLbl->text() + ":" + ui->oilBoxNoEdit->text() + "\n";
    info_str += ui->hvCtrlBoardNoLbl->text() + ":" + ui->hvCtrlBoardNoEdit->text() + "\n";
    info_str += ui->swVerStrLbl->text() + ":" + ui->swVerStrEdit->text() + "\n";
    info_str += ui->hwVerStrLbl->text() + ":" + ui->hwVerStrEdit->text() + "\n";

    info_str += QCoreApplication::applicationName() + ":" + APP_VER_STR;
}

void testParamSettingsDialog::setup_judge_ctrls()
{
    m_judge_ctrls.clear();

    m_judge_ctrls.append(
        {
             VoltSet, Voltmeter,
            {
                 ui->voltKVChkbox,
                 ui->voltKVLowEdgePctLEdit, ui->voltKVUpEdgePctLEdit,
                 ui->voltKVLowEdgeErrValLEdit, ui->voltKVUpEdgeErrValLEdit,
                 ui->voltKVIsFixedRefChkbox, ui->voltKVFixedRefValLEdit
            }
        }
    );
    m_judge_ctrls.append(
        {
             FilamentSet, Ammeter,
            {
                 ui->amtmAChkbox,
                 ui->amtmALowEdgePctLEdit, ui->amtmAUpEdgePctLEdit,
                 ui->amtmALowEdgeErrValLEdit, ui->amtmAUpEdgeErrValLEdit,
                 ui->amtmAIsFixedRefChkbox, ui->amtmAFixedRefValLEdit
            }
        }
    );
    m_judge_ctrls.append(
        {
             EXT_MB_REG_DISTANCE/*use fixed value. ref-reg is not used*/, EXT_MB_REG_DISTANCE,
            {
                 ui->distmmChkbox,
                 ui->distmmLowEdgePctLEdit, ui->distmmUpEdgePctLEdit,
                 ui->distmmLowEdgeErrValLEdit, ui->distmmUpEdgeErrValLEdit,
                 ui->distmmIsFixedRefChkbox, ui->distmmFixedRefValLEdit
            }
        }
    );

    m_judge_ctrl_desc_map.insert(ui->voltKVLowEdgePctLEdit, {ui->voltKVChkbox, ui->lowPctLbl});
    m_judge_ctrl_desc_map.insert(ui->voltKVUpEdgePctLEdit, {ui->voltKVChkbox, ui->upPctLbl});
    m_judge_ctrl_desc_map.insert(ui->voltKVLowEdgeErrValLEdit, {ui->voltKVChkbox, ui->lowErrValLbl});
    m_judge_ctrl_desc_map.insert(ui->voltKVUpEdgeErrValLEdit, {ui->voltKVChkbox, ui->upErrValLbl});
    m_judge_ctrl_desc_map.insert(ui->voltKVIsFixedRefChkbox, {ui->voltKVChkbox, ui->isFixedRefLbl});
    m_judge_ctrl_desc_map.insert(ui->voltKVFixedRefValLEdit, {ui->voltKVChkbox, ui->fixedRefLbl});

    m_judge_ctrl_desc_map.insert(ui->amtmALowEdgePctLEdit, {ui->amtmAChkbox, ui->lowPctLbl});
    m_judge_ctrl_desc_map.insert(ui->amtmAUpEdgePctLEdit, {ui->amtmAChkbox, ui->upPctLbl});
    m_judge_ctrl_desc_map.insert(ui->amtmALowEdgeErrValLEdit, {ui->amtmAChkbox, ui->lowErrValLbl});
    m_judge_ctrl_desc_map.insert(ui->amtmAUpEdgeErrValLEdit, {ui->amtmAChkbox, ui->upErrValLbl});
    m_judge_ctrl_desc_map.insert(ui->amtmAIsFixedRefChkbox, {ui->amtmAChkbox, ui->isFixedRefLbl});
    m_judge_ctrl_desc_map.insert(ui->amtmAFixedRefValLEdit, {ui->amtmAChkbox, ui->fixedRefLbl});

    m_judge_ctrl_desc_map.insert(ui->distmmLowEdgePctLEdit, {ui->distmmChkbox, ui->lowPctLbl});
    m_judge_ctrl_desc_map.insert(ui->distmmUpEdgePctLEdit, {ui->distmmChkbox, ui->upPctLbl});
    m_judge_ctrl_desc_map.insert(ui->distmmLowEdgeErrValLEdit, {ui->distmmChkbox, ui->lowErrValLbl});
    m_judge_ctrl_desc_map.insert(ui->distmmUpEdgeErrValLEdit, {ui->distmmChkbox, ui->upErrValLbl});
    m_judge_ctrl_desc_map.insert(ui->distmmIsFixedRefChkbox, {ui->distmmChkbox, ui->isFixedRefLbl});
    m_judge_ctrl_desc_map.insert(ui->distmmFixedRefValLEdit, {ui->distmmChkbox, ui->fixedRefLbl});
}

#define GET_JUDGE_PARAM_FROM_CTRL(judge_ctrl, edit_ctrl, judge_param, param_var, val_fac, info_str_apx) \
        if(!CHKBOX_EXIST_AND_CHECKED((judge_ctrl).gui_ctrls.judge_or_not_chbox)) \
        {\
            func_ret = func_ret && ret;\
            continue;\
        }\
        if(!(((judge_ctrl).gui_ctrls).edit_ctrl))\
        {\
            DIY_LOG(LOG_ERROR, "judge ctrl is NULL.");\
            ret = false;\
            func_ret = func_ret && ret;\
            continue;\
        }\
        ctrl_desc = JUDGE_CTRL_DESC_STR(((judge_ctrl).gui_ctrls).edit_ctrl);\
        val_str = ((judge_ctrl).gui_ctrls).edit_ctrl->text();\
        (judge_param).param_var = val_str.toFloat(&tr_ret) * (val_fac);\
        if(!tr_ret)\
        {\
            err_str += ctrl_desc + " " + gs_str_should_be_number + "\n";\
            ret = false;\
        }\
        else\
        {\
            info_str += ctrl_desc + ":" + val_str + (info_str_apx) + "\n";\
        }
bool testParamSettingsDialog::construct_judge_params(QString &err_str, QString &info_str)
{
    bool ret = true, func_ret = true;
    QString ctrl_desc, val_str;
    judge_params_s_t judge_param;
    bool tr_ret;

    if(!m_test_judge)
    {
        err_str = "m_test_judge ptr 为空，未正常初始化.";
        DIY_LOG(LOG_ERROR, err_str);
        ret = false;
        return ret;
    }

    m_test_judge->clear_judge_params();
    for(int idx = 0; idx < m_judge_ctrls.count(); ++idx)
    {
        ret = true;

        GET_JUDGE_PARAM_FROM_CTRL(m_judge_ctrls[idx], low_e_pct_ledit, judge_param, low_e_pct,
                                      0.01, "%");
        GET_JUDGE_PARAM_FROM_CTRL(m_judge_ctrls[idx], up_e_pct_ledit, judge_param, up_e_pct,
                                      0.01, "%");
        GET_JUDGE_PARAM_FROM_CTRL(m_judge_ctrls[idx], low_e_err_val_ledit, judge_param,
                                      low_e_extra_val, 1, "");
        GET_JUDGE_PARAM_FROM_CTRL(m_judge_ctrls[idx], up_e_err_val_ledit, judge_param,
                                      up_e_extra_val, 1, "");

        judge_param.is_fixed_ref = false;
        if(CHKBOX_EXIST_AND_CHECKED(m_judge_ctrls[idx].gui_ctrls.is_fixed_ref_chbox))
        {
            judge_param.is_fixed_ref = true;
            GET_JUDGE_PARAM_FROM_CTRL(m_judge_ctrls[idx], fixed_ref_val_ledit,
                                      judge_param, fixed_ref_val, 1, "");
        }
        else if(!VALID_MB_REG_ADDR(m_judge_ctrls[idx].ref_reg_no))
        {
            err_str += QString("%1:%2. %3"). arg(gs_str_invalid_mb_reg_no).
                    arg(m_judge_ctrls[idx].ref_reg_no).arg(gs_str_cons_judge_param_err1);
            ret = false;
        }

        if(ret)
        {
            QString low_desc_str, up_desc_str;
            if(judge_param.low_e_pct > judge_param.up_e_pct)
            {
                low_desc_str = m_judge_ctrl_desc_map[m_judge_ctrls[idx].gui_ctrls.low_e_pct_ledit].row_lbl->text()
                        + m_judge_ctrl_desc_map[m_judge_ctrls[idx].gui_ctrls.low_e_pct_ledit].col_lbl->text();
                up_desc_str = m_judge_ctrl_desc_map[m_judge_ctrls[idx].gui_ctrls.up_e_pct_ledit].row_lbl->text()
                        + m_judge_ctrl_desc_map[m_judge_ctrls[idx].gui_ctrls.up_e_pct_ledit].col_lbl->text();

                err_str += low_desc_str + " " + gs_str_should_be_le + " " + up_desc_str;
                ret = false;

            }
            if(judge_param.low_e_extra_val > judge_param.up_e_extra_val)
            {
                low_desc_str = m_judge_ctrl_desc_map[m_judge_ctrls[idx].gui_ctrls.low_e_err_val_ledit].row_lbl->text()
                        + m_judge_ctrl_desc_map[m_judge_ctrls[idx].gui_ctrls.low_e_err_val_ledit].col_lbl->text();
                up_desc_str = m_judge_ctrl_desc_map[m_judge_ctrls[idx].gui_ctrls.up_e_err_val_ledit].row_lbl->text()
                        + m_judge_ctrl_desc_map[m_judge_ctrls[idx].gui_ctrls.up_e_err_val_ledit].col_lbl->text();

                err_str += low_desc_str + " " + gs_str_should_be_le + " " + up_desc_str;
                ret = false;

            }

            if(ret)
            {
               m_test_judge->add_judge_params(m_judge_ctrls[idx].ref_reg_no,
                                              m_judge_ctrls[idx].val_reg_no, judge_param);
            }
        }

        func_ret = func_ret && ret;
    }

    return func_ret;
}

void testParamSettingsDialog::on_voltKVChkbox_stateChanged(int /*arg1*/)
{
    refresh_judge_ctrls_display();
}

void testParamSettingsDialog::on_amtmAChkbox_stateChanged(int /*arg1*/)
{
    refresh_judge_ctrls_display();
}

void testParamSettingsDialog::on_distmmChkbox_stateChanged(int arg1)
{
    if((Qt::Checked == arg1) && ui->readDistChBox->checkState() == Qt::Unchecked)
    {
        ui->distmmChkbox->setCheckState(Qt::Unchecked);
        QMessageBox::information(this, "Info", QString("%1\"%2\"")
                                 .arg(gs_str_plz_check_first, ui->readDistChBox->text()));
        return;
    }
    refresh_judge_ctrls_display();
}

void testParamSettingsDialog::on_voltKVIsFixedRefChkbox_stateChanged(int /*arg1*/)
{
    refresh_judge_ctrls_display();
}

void testParamSettingsDialog::on_amtmAIsFixedRefChkbox_stateChanged(int /*arg1*/)
{
    refresh_judge_ctrls_display();
}

void testParamSettingsDialog::on_readDistChBox_stateChanged(int arg1)
{
    if(Qt::Unchecked == arg1)
    {
        ui->distmmChkbox->setCheckState(Qt::Unchecked);
    }
}

bool testParamSettingsDialog::expo_params_ui_sync(expo_params_ui_sync_type_e_t direction,
                                      expo_params_ui_sync_ctrls_s_t * main_ctrls, QString *info_str)
{
    QString check_info_str;
    if(!main_ctrls) return false;

    if(EXPO_PARAMS_UI_SYNC_MAIN_TO_SET_DLG == direction)
    {
        /*
        QString set_dlg_ori_volt_str = ui->cubeVoltStartEdit->text(),
                set_dlg_ori_current_str = ui->cubeCurrentStartEdit->text(),
                set_dlg_ori_dura_str = ui->expoDuraStartEdit->text();
        */

        ui->cubeVoltStartEdit->setText(main_ctrls->cube_volt_ctrl->cleanText());
        ui->cubeCurrentStartEdit->setText(main_ctrls->cube_current_ctrl->cleanText());
        ui->expoDuraStartEdit->setText(main_ctrls->expo_dura_ctrl->cleanText());
        check_info_str = collect_test_params();
        if(m_test_params->valid)
        {
            if(m_cfg_recorder) m_cfg_recorder->record_ui_configs(this,
                                                         m_rec_ui_cfg_fin, m_rec_ui_cfg_fout);
            return true;
        }

        /*
        ui->cubeVoltStartEdit->setText(set_dlg_ori_volt_str);
        ui->cubeCurrentStartEdit->setText(set_dlg_ori_current_str);
        ui->expoDuraStartEdit->setText(set_dlg_ori_dura_str);
        collect_test_params();
        */
        if(info_str) *info_str = check_info_str;
        return false;
    }
    else //EXPO_PARAMS_UI_SYNC_SET_DLG_TO_MAIN
    {
        main_ctrls->cube_volt_ctrl->setValue(m_test_params->expo_param_block.expo_params.regular_parms.
                                             cube_volt_kv_start);
        main_ctrls->cube_current_ctrl->setValue(m_test_params->expo_param_block.expo_params.regular_parms.
                    cube_current_ma_start/m_test_params->expo_param_block.ui_to_sw_current_factor);
        main_ctrls->expo_dura_ctrl->setValue(m_test_params->expo_param_block.expo_params.regular_parms.
                     expo_dura_ms_start/m_test_params->expo_param_block.ui_to_sw_dura_factor);
        main_ctrls->cube_current_unit->setText(m_ui_cube_current_unit_str);
        main_ctrls->expo_dura_unit->setText(m_ui_expo_dura_unit_str);
        return true;
    }
}

void testParamSettingsDialog::update_expo_dura_unit(bool from_rb_toggle, QString u_str)
{
    if(!from_rb_toggle)
    {
        if(ui->expoDuraUnitmsRButton->isChecked())
        {
            u_str = g_str_dura_unit_ms;
        }
        else if(ui->expoDuraUnitsecRButton->isChecked())
        {
            u_str = g_str_dura_unit_s;
        }
        else //ui->expoDuraUnitminRButton->isChecked()
        {
            u_str = g_str_dura_unit_min;
        }
    }
    m_ui_expo_dura_unit_str = u_str;

    m_test_params->expo_param_block.sw_to_mb_dura_factor = expo_dura_trans_factor(EXPO_PARAMS_SW_TO_MB_INTF);
    m_test_params->expo_param_block.ui_to_sw_dura_factor = expo_dura_trans_factor(EXPO_PARAMS_UI_TO_SW);
}

void testParamSettingsDialog::update_current_display()
{
    ui->cubeCurrentLbl->setText(QString("%1（%2）").arg(gs_str_current_name,
                                                      m_ui_cube_current_unit_str));
    ui->amtmAChkbox->setText(QString("%1(%2)").arg(gs_str_fb_current_name,
                                                   m_ui_cube_current_unit_str));

    ui->cubeCurrentStartEdit->setWhatsThis(QString("%1%2").arg(gs_str_current_name, gs_str_start_val));
    ui->cubeCurrentEndEdit->setWhatsThis(QString("%1%2").arg(gs_str_current_name, gs_str_end_val));
    ui->cubeCurrentStepEdit->setWhatsThis(QString("%1%2").arg(gs_str_current_name, gs_str_step));
}

#define USE_NEW_CURRENT_UNIT_STR \
    switch(m_used_current_ui_unit) \
    {\
    case MB_CUBE_CURRENT_UNIT_A:\
        m_ui_cube_current_unit_str = g_str_current_unit_a;\
        break;\
    case MB_CUBE_CURRENT_UNIT_MA:\
        m_ui_cube_current_unit_str = g_str_current_unit_ma;\
        break;\
    default: /* MB_CUBE_CURRENT_UNIT_UA */\
        m_ui_cube_current_unit_str = g_str_current_unit_ua;\
        break;\
    }

#define USE_NEW_CURRENT_NAME_STR(coil) \
    if((coil))\
    {\
        gs_str_current_name = g_str_coil_current;\
        gs_str_fb_current_name = g_str_fb_coil_current;\
    }\
    else\
    {\
        gs_str_current_name = g_str_cube_current;\
        gs_str_fb_current_name = g_str_fb_cube_current;\
    }

void testParamSettingsDialog::update_current_name_and_unit()
{
    bool checked = false;

    if(TEST_CONTENT_ONLY_COIL == m_test_params->test_content
            || TEST_CONTENT_DECOUPLE == m_test_params->test_content)
    {
        checked = true;
    }

    m_used_current_ui_unit = checked ? MB_CUBE_CURRENT_UNIT_A : g_sys_configs_block.ui_current_unit;
    m_used_current_intf_unit = checked ? MB_CUBE_CURRENT_UNIT_MA : g_sys_configs_block.mb_cube_current_intf_unit;

    m_test_params->expo_param_block.sw_to_mb_current_factor = cube_current_trans_factor(EXPO_PARAMS_SW_TO_MB_INTF);
    m_test_params->expo_param_block.ui_to_sw_current_factor = cube_current_trans_factor(EXPO_PARAMS_UI_TO_SW);

    USE_NEW_CURRENT_UNIT_STR;

    USE_NEW_CURRENT_NAME_STR(checked);

    update_current_display();

    /*update range*/
    update_current_range_checker();
}

void testParamSettingsDialog::update_current_range_checker()
{
    float cfg_to_sw_factor;
    switch(m_test_params->test_content)
    {
    case TEST_CONTENT_COOL_HV:
        gs_valid_cube_current_ma_range.set_min_max(0, g_sys_configs_block.cube_current_ma_max);
        gs_valid_cube_current_ma_range.set_edge(EDGE_INCLUDED, EDGE_INCLUDED);
        break;

    case TEST_CONTENT_ONLY_COIL:
    case TEST_CONTENT_DECOUPLE:
        cfg_to_sw_factor = 1000;
        gs_valid_cube_current_ma_range.set_min_max(g_sys_configs_block.coil_current_a_min * cfg_to_sw_factor,
                                               g_sys_configs_block.coil_current_a_max * cfg_to_sw_factor);
        gs_valid_cube_current_ma_range.set_edge(EDGE_EXCLUDED, EDGE_INCLUDED);
        break;

    default: //TEST_CONTENT_NORMAL
        gs_valid_cube_current_ma_range.set_min_max(g_sys_configs_block.cube_current_ma_min,
                                                   g_sys_configs_block.cube_current_ma_max);
        gs_valid_cube_current_ma_range.set_edge(EDGE_INCLUDED, EDGE_INCLUDED);
        break;
    }
    gs_valid_cube_current_ma_range.set_unit_str(m_ui_cube_current_unit_str);
}

void testParamSettingsDialog::on_expoDuraUnitmsRButton_toggled(bool checked)
{
    if(checked)
    {
        update_expo_dura_unit(true, g_str_dura_unit_ms);
        LOAD_EXPO_DUAR_SW_TO_UI(1);
    }
    else
    {
        SAVE_EXPO_DURA_UI_TO_SW(1);
    }
}

void testParamSettingsDialog::on_expoDuraUnitsecRButton_toggled(bool checked)
{
    float ui_to_sw_f = 1000.0;
    if(checked)
    {
        update_expo_dura_unit(true, g_str_dura_unit_s);
        LOAD_EXPO_DUAR_SW_TO_UI(1/ui_to_sw_f);
    }
    else
    {
        SAVE_EXPO_DURA_UI_TO_SW(ui_to_sw_f);
    }
}

void testParamSettingsDialog::on_expoDuraUnitminRButton_toggled(bool checked)
{
    float ui_to_sw_f = 60 * 1000.0;
    if(checked)
    {
        update_expo_dura_unit(true, g_str_dura_unit_min);
        LOAD_EXPO_DUAR_SW_TO_UI(1/ui_to_sw_f);
    }
    else
    {
        SAVE_EXPO_DURA_UI_TO_SW(ui_to_sw_f);
    }
}

void testParamSettingsDialog::on_testContentNormalRButton_toggled(bool checked)
{
    if(checked)
    {
        m_test_params->test_content = TEST_CONTENT_NORMAL;
        update_current_name_and_unit();
    }
}

void testParamSettingsDialog::on_testContentCoolHVRButton_toggled(bool checked)
{
    if(checked)
    {
        m_test_params->test_content = TEST_CONTENT_COOL_HV;
        update_current_name_and_unit();
    }
}

void testParamSettingsDialog::on_testContentOnlyCoilRButton_toggled(bool checked)
{
    if(checked)
    {
        m_test_params->test_content = TEST_CONTENT_ONLY_COIL;
        update_current_name_and_unit();
    }
}

void testParamSettingsDialog::on_testContentDecoupleRButton_toggled(bool checked)
{
    if(checked)
    {
        m_test_params->test_content = TEST_CONTENT_DECOUPLE;
        update_current_name_and_unit();
    }
}

void testParamSettingsDialog::get_volt_info_for_chart(QString &name_str, QString &unit_str, int &low, int &up)
{
    name_str = g_str_cube_volt;
    unit_str = g_str_volt_unit_kv;

    low = gs_valid_cube_volt_kv_range.range_min();
    up = gs_valid_cube_volt_kv_range.range_max();
}

void testParamSettingsDialog::get_current_info_for_chart(QString &name_str, QString &unit_str,
                                                         float &low, float &up)
{
    name_str = gs_str_fb_current_name;

    switch(m_used_current_intf_unit)
    {
    case MB_CUBE_CURRENT_UNIT_A:
        unit_str = g_str_current_unit_a;
        break;

    case MB_CUBE_CURRENT_UNIT_MA:
        unit_str = g_str_current_unit_ma;
        break;

    default: //MB_CUBE_CURRENT_UNIT_UA:
        unit_str = g_str_current_unit_ua;
        break;
    }
    low = m_test_params->expo_param_block.sw_to_mb_current_factor
            * gs_valid_cube_current_ma_range.range_min();
    up = m_test_params->expo_param_block.sw_to_mb_current_factor
            * gs_valid_cube_current_ma_range.range_max();
}
