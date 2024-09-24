#include <QMessageBox>
#include <QFile>
#include <QTextStream>
#include <QFileDialog>
#include <QtMath>
#include "common_tools/common_tool_func.h"
#include "logger/logger.h"
#include "test_param_settings.h"
#include "ui_test_param_settings.h"
#include "sysconfigs/sysconfigs.h"

static RangeChecker<int> gs_valid_cube_volt_kv_range;
static RangeChecker<float> gs_valid_cube_current_ma_range;
static RangeChecker<float> gs_valid_expo_dura_ms_range;
static RangeChecker<int> gs_valid_expo_cnt_range(1, 0, "", EDGE_INCLUDED, EDGE_INFINITE);
static RangeChecker<float> gs_valid_cool_dura_range;
static RangeChecker<float> gs_valid_cool_dura_factor;

static const char* gs_str_test_mode = "测试模式";
static const char* gs_str_cube_volt = "管电压";
static const char* gs_str_cube_current = "管电流";
static const char* gs_str_expo_dura = "曝光时间";
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
static const char* gs_str_volt_unit_kv = "kV";
static const char* gs_str_current_unit_ma = "mA";
static const char* gs_str_dura_unit_s = "s";
static const char* gs_str_dura_unit_ms = "ms";
static const char* gs_str_cust_file = "自定义曝光参数文件";
static const char* gs_str_1st_line_should_be = "首行应为";
static const char* gs_str_or = "或";
static const char* gs_str_expo_params = "曝光参数";
static const char* gs_str_group = "组";
const char* g_str_loop = "轮";
const char* g_str_time_ci = "次";
const char* g_str_time_bei = "倍";
const char* g_str_no_bu = "不";

static const char* gs_cust_expo_file_filter = "CSV File (*.csv)";
static const char* gs_valid_header_line_ms = "volt-kv,current-ma,dura-ms";
static const char* gs_valid_header_line_s = "volt-kv,current-ma,dura-s";
static const char* gs_cust_expo_file_item_sep_in_line = ",";
static const int gs_cust_expo_file_item_num_in_line = 3;

const char* g_str_the_line_pron = "第";
static const char* gs_str_line = "行";
static const char* gs_str_item = "项";
static const char* gs_str_no_valid_data_item = "无有效数据";
static const char* gs_str_format_error = "格式错误";
const char* gs_str_data_item_invalid = "数据无效";

static const char* gs_str_should_be_le = "应小于或等于";

static const char* gs_info_str_seperator = "--------------------";

static const char* gs_str_cust1_notes =
"文件格式：文本文件，第一行指定曝光时间单位（s或ms），之后每行一组电压、电流、时间值，ASCII逗号隔开";

static const char* gs_str_cust2_notes =
"文件格式：共3行的文本文件，分别指定ASSCII逗号分开的电压、电流、时间值。每行第一个字段为表头；第三行表头指定s或ms";

static const char* gs_str_cust2_err_msg_format = "第一项为表头，第二项开始为数据";

const testParamSettingsDialog::test_mode_espair_struct_t
      testParamSettingsDialog::test_mode_list[] =
{
    {TEST_MODE_SINGLE, QString("单次")},
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
    ui->cubeVoltStartEdit->setWhatsThis(QString("%1%2").arg(gs_str_cube_volt, gs_str_start_val));\
    ARRANGE_CTRLS_ABILITY(ui->cubeVoltEndEdit, false, false, true, false, false)\
    ui->cubeVoltEndEdit->setWhatsThis(QString("%1%2").arg(gs_str_cube_volt, gs_str_end_val));\
    ARRANGE_CTRLS_ABILITY(ui->cubeVoltStepEdit, false, false, true, false, false)\
    ui->cubeVoltStepEdit->setWhatsThis(QString("%1%2").arg(gs_str_cube_volt, gs_str_step));\
    ARRANGE_CTRLS_ABILITY(ui->cubeCurrentStartEdit, true, true, true, false, false)\
    ui->cubeCurrentStartEdit->setWhatsThis(QString("%1%2").arg(gs_str_cube_current, gs_str_start_val));\
    ARRANGE_CTRLS_ABILITY(ui->cubeCurrentEndEdit, false, false, true, false, false)\
    ui->cubeCurrentEndEdit->setWhatsThis(QString("%1%2").arg(gs_str_cube_current, gs_str_end_val));\
    ARRANGE_CTRLS_ABILITY(ui->cubeCurrentStepEdit, false, false, true, false, false)\
    ui->cubeCurrentStepEdit->setWhatsThis(QString("%1%2").arg(gs_str_cube_current, gs_str_step));\
    ARRANGE_CTRLS_ABILITY(ui->expoDuraStartEdit, true, true, true, false, false)\
    ui->expoDuraStartEdit->setWhatsThis(QString("%1%2").arg(gs_str_expo_dura, gs_str_start_val));\
    ARRANGE_CTRLS_ABILITY(ui->expoDuraEndEdit, false, false, true, false, false)\
    ui->expoDuraEndEdit->setWhatsThis(QString("%1%2").arg(gs_str_expo_dura, gs_str_end_val));\
    ARRANGE_CTRLS_ABILITY(ui->expoDuraStepEdit, false, false, true, false, false)\
    ui->expoDuraStepEdit->setWhatsThis(QString("%1%2").arg(gs_str_expo_dura, gs_str_step));\
    ARRANGE_CTRLS_ABILITY(ui->repeatsNumEdit, false, true, true, true, true)\
    ui->repeatsNumEdit->setWhatsThis(QString(gs_str_repeats_num));\
    ARRANGE_CTRLS_ABILITY(ui->coolDuraEdit, false, true, true, true, true)\
    if(ui->fixedCoolDuraRButton->isChecked())\
    {ui->coolDuraEdit->setWhatsThis(QString(gs_str_cool_dura));}\
    else\
    {ui->coolDuraEdit->setWhatsThis(ui->timesCoolDuraRButton->text());}\
    ARRANGE_CTRLS_ABILITY(ui->expoDuraUnitmsRButton, true, true, true, false, false)\
    ARRANGE_CTRLS_ABILITY(ui->expoDuraUnitsecRButton, true, true, true, false, false)\
    ARRANGE_CTRLS_ABILITY(ui->fixedCoolDuraRButton, false, true, true, true, true)\
    ARRANGE_CTRLS_ABILITY(ui->timesCoolDuraRButton, false, true, true, true, true)\
    ARRANGE_CTRLS_ABILITY(ui->limitShortestCoolDuraChBox, false, true, true, true, true)\
    \
    ARRANGE_CTRLS_ABILITY(ui->custExpoParamFileSelBtn, false, false, false, true, true)\
    ARRANGE_CTRLS_ABILITY(ui->custExpoParamFileNoteEdit, false, false, false, true, true)\
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

    ui->setupUi(this);
    m_expoDuraUnitBtnGrp = new QButtonGroup(this);
    m_expoDuraUnitBtnGrp->addButton(ui->expoDuraUnitmsRButton);
    m_expoDuraUnitBtnGrp->addButton(ui->expoDuraUnitsecRButton);

    m_coolDuraModeBtnGrp = new QButtonGroup(this);
    m_coolDuraModeBtnGrp->addButton(ui->fixedCoolDuraRButton);
    m_coolDuraModeBtnGrp->addButton(ui->timesCoolDuraRButton);

    int idx;
    for(idx = 0; idx < ARRAY_COUNT(test_mode_list); ++idx)
    {
        ui->testModeComboBox->addItem(test_mode_list[idx].s, test_mode_list[idx].e);
    }

    ui->expoDuraUnitmsRButton->setChecked(true);
    ui->timesCoolDuraRButton->setChecked(true);
    ui->testModeComboBox->setCurrentIndex(0);
    ui->limitShortestCoolDuraChBox->setChecked(true);
    ui->readDistChBox->setChecked(true);

    m_rec_ui_cfg_fin.clear();
    m_rec_ui_cfg_fout.insert(ui->custExpoParamFileNoteEdit);
    if(m_cfg_recorder) m_cfg_recorder->load_configs_to_ui(this,
                                                          m_rec_ui_cfg_fin, m_rec_ui_cfg_fout);

    TEST_PARAMS_CTRLS_ABT_TBL;

    setup_judge_ctrls();

    refresh_controls_display();

    gs_valid_cube_volt_kv_range.set_min_max(g_sys_configs_block.cube_volt_kv_min,
                                            g_sys_configs_block.cube_volt_kv_max);
    gs_valid_cube_volt_kv_range.set_unit_str(gs_str_volt_unit_kv);

    gs_valid_cube_current_ma_range.set_min_max(g_sys_configs_block.cube_current_ma_min,
                                               g_sys_configs_block.cube_current_ma_max);
    gs_valid_cube_current_ma_range.set_unit_str(gs_str_current_unit_ma);

    gs_valid_expo_dura_ms_range.set_min_max(g_sys_configs_block.dura_ms_min,
                                            g_sys_configs_block.dura_ms_max);
    gs_valid_expo_dura_ms_range.set_unit_str(gs_str_dura_unit_ms);

    gs_valid_cool_dura_range.set_min_max(g_sys_configs_block.dura_ms_max
                                          * g_sys_configs_block.cool_dura_factor /1000, 0);
    gs_valid_cool_dura_range.set_edge(EDGE_INCLUDED, EDGE_INFINITE);
    gs_valid_cool_dura_range.set_unit_str(gs_str_dura_unit_s);

    gs_valid_cool_dura_factor.set_min_max(g_sys_configs_block.cool_dura_factor, 0);
    gs_valid_cool_dura_factor.set_edge(EDGE_INCLUDED, EDGE_INFINITE);
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
get_one_expo_param(QLineEdit * ctrl, common_data_type_enum_t d_type, float factor,
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

    tr_val = ui_val * factor;
    ret = true;
    if(range && !range->range_check(tr_val))
    {
        ret_str = QString("%1%2 ").arg(ctrl_str, gs_str_exceeds_valid_range);

        ret_str += range->range_str(d_type, 1/factor, new_unit_str);
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

void testParamSettingsDialog::get_expo_param_vals_from_ui()
{
    float factor;
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

    m_expo_params_from_ui.valid_cube_current_start =
        get_one_expo_param<float>(ui->cubeCurrentStartEdit, FLOAT_DATA, 1,
                           &gs_valid_cube_current_ma_range,
                           &m_expo_params_from_ui.vals.cube_current_ma_start,
                           m_expo_params_from_ui.err_msg_cube_current_start);
    m_expo_params_from_ui.valid_cube_current_end =
        get_one_expo_param<float>(ui->cubeCurrentEndEdit, FLOAT_DATA, 1,
                           &gs_valid_cube_current_ma_range,
                           &m_expo_params_from_ui.vals.cube_current_ma_end,
                           m_expo_params_from_ui.err_msg_cube_current_end);
    m_expo_params_from_ui.valid_cube_current_step =
        get_one_expo_param<float>(ui->cubeCurrentStepEdit, FLOAT_DATA, 1,
                           nullptr,
                           &m_expo_params_from_ui.vals.cube_current_ma_step,
                           m_expo_params_from_ui.err_msg_cube_current_step);

    QString new_unit_str = "";
    if(ui->expoDuraUnitsecRButton->isChecked())
    {
        factor = 1000;
        new_unit_str = gs_str_dura_unit_s;
    }
    else
    {
        factor = 1;
    }
    m_expo_params_from_ui.valid_expo_dura_start =
        get_one_expo_param<float>(ui->expoDuraStartEdit, FLOAT_DATA, factor,
                           &gs_valid_expo_dura_ms_range,
                           &m_expo_params_from_ui.vals.expo_dura_ms_start,
                           m_expo_params_from_ui.err_msg_expo_dura_start, new_unit_str);
    m_expo_params_from_ui.valid_expo_dura_end =
        get_one_expo_param<float>(ui->expoDuraEndEdit, FLOAT_DATA, factor,
                           &gs_valid_expo_dura_ms_range,
                           &m_expo_params_from_ui.vals.expo_dura_ms_end,
                           m_expo_params_from_ui.err_msg_expo_dura_end, new_unit_str);
    m_expo_params_from_ui.valid_expo_dura_step =
        get_one_expo_param<float>(ui->expoDuraStepEdit, FLOAT_DATA, factor,
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

bool testParamSettingsDialog::judge_dura_factor_from_str(QString h_s, float *ret_factor)
{
    QString qstr_s = QString(gs_str_dura_unit_s), qstr_ms = QString(gs_str_dura_unit_ms);
    int s_len = qstr_s.length(), ms_len = qstr_ms.length();
    int h_s_len = h_s.length();
    float factor = 1;
    bool ret = false;

    if(h_s_len >= ms_len)
    {
        if(h_s.endsWith(qstr_ms))
        {
            ret = true;
            factor = 1;
        }
        else if(h_s.endsWith(qstr_s))
        {
            ret = true;
            factor = 1000;
        }
    }
    else if(h_s_len >= s_len)
    {
        if(h_s.endsWith(qstr_s))
        {
            ret = true;
            factor = 1000;
        }
    }
    if(ret_factor) *ret_factor = factor;
    return ret;
}

bool testParamSettingsDialog::get_expo_param_vals_from_cust_file(QString file_fpn,
                                        QVector<expo_param_triple_struct_t> &param_vector,
                                        float * max_expo_dura_ms,
                                        QString &ret_str, QString &file_content)
{
    /*
     * The 1st line of file should be as gs_valid_header_line_ms or gs_valid_header_line_s,
     * where the last string "ms" or "s" indicates the unit of duration.
     *
     * Then the subsequent lines are volt, current and duration time, seperated by ",".
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
    QString line;
    float factor;
    bool valid_line = false;
    float max_ms = 0;
    if(file_stream.readLineInto(&line))
    {
        valid_line = judge_dura_factor_from_str(line, &factor);
    }
    file_content += line + "\n";
    if(!valid_line)
    {
        DIY_LOG(LOG_ERROR,
                QString("%1%2%3%4%5%6").
                arg(file_fpn, ":The 1st line should be as\n\t",
                    gs_valid_header_line_ms, "or\n\t",
                    gs_valid_header_line_s,
                    "\nbut it is\n\t", line));
        ret_str = QString("%1%2\n%3\n%4\n%5").
                arg(gs_str_cust_file, gs_str_1st_line_should_be,
                    gs_valid_header_line_ms, gs_str_or, gs_valid_header_line_s);
        cust_file.close();
        return false;
    }

    int line_no = 2;
    QStringList line_items;
    QString item_str;
    expo_param_triple_struct_t expo_param_triple;
    bool c2n_ret;
    valid_line = true;
    while(valid_line && file_stream.readLineInto(&line))
    {
        file_content += line + "\n";
        line_items = line.split(gs_cust_expo_file_item_sep_in_line, Qt::SkipEmptyParts);
        while(true)
        {
            if(line_items.count() == gs_cust_expo_file_item_num_in_line)
            {
                item_str = line_items.at(0);
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
                                  gs_str_cube_volt, item_str, gs_str_exceeds_valid_range,
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
                                  gs_str_cube_volt, item_str, gs_str_should_be_int);
                    DIY_LOG(LOG_ERROR, QString("%1%2%3%4").arg(file_fpn, ":the ",
                               QString::number(line_no), " line, cube volt should be int."));
                    break;
                }

                item_str = line_items.at(1);
                expo_param_triple.cube_current_ma = item_str.toFloat(&c2n_ret);
                if(c2n_ret)
                {
                    if(!gs_valid_cube_current_ma_range.
                            range_check(expo_param_triple.cube_current_ma))
                    {
                        valid_line = false;
                        ret_str = QString("%1%2%3%4%5\n:%6%7,%8%9").arg(gs_str_cust_file,
                                  g_str_the_line_pron, QString::number(line_no), gs_str_line,
                                  gs_str_data_item_invalid,
                                  gs_str_cube_current, item_str, gs_str_exceeds_valid_range,
                                  gs_valid_cube_current_ma_range.range_str(FLOAT_DATA));
                        DIY_LOG(LOG_ERROR, QString("%1%2%3%4%5").arg(file_fpn,
                                       ":the ", QString::number(line_no),
                                       "cube current exceeds valid range:",
                                       gs_valid_cube_current_ma_range.range_str(FLOAT_DATA)));
                        break;
                    }
                }
                else
                {
                    valid_line = false;
                    ret_str = QString("%1%2%3%4%5:\n%6%7,%8").arg(gs_str_cust_file,
                                  g_str_the_line_pron, QString::number(line_no), gs_str_line,
                                  gs_str_data_item_invalid,
                                  gs_str_cube_current, item_str, gs_str_should_be_number);
                    DIY_LOG(LOG_ERROR, QString("%1%2%3%4").arg(file_fpn, ":the ",
                           QString::number(line_no), " line, cube current should be number."));
                    break;
                }

                item_str = line_items.at(2);
                expo_param_triple.dura_ms = item_str.toFloat(&c2n_ret);
                if(c2n_ret)
                {
                    expo_param_triple.dura_ms *= factor;
                    max_ms = expo_param_triple.dura_ms > max_ms ? expo_param_triple.dura_ms : max_ms;
                    if(!gs_valid_expo_dura_ms_range.
                            range_check(expo_param_triple.dura_ms))
                    {
                        valid_line = false;
                        ret_str = QString("%1%2%3%4%5:\n%6%7,%8%9").arg(gs_str_cust_file,
                                  g_str_the_line_pron, QString::number(line_no), gs_str_line,
                                  gs_str_data_item_invalid,
                                  gs_str_expo_dura, item_str, gs_str_exceeds_valid_range,
                                  gs_valid_expo_dura_ms_range.range_str(FLOAT_DATA, 1/factor,
                                                                        gs_str_dura_unit_s));
                        DIY_LOG(LOG_ERROR, QString("%1%2%3%4%5").arg(file_fpn,
                                       ":the ", QString::number(line_no),
                                       "expo duration exceeds valid range:",
                                       gs_valid_expo_dura_ms_range.range_str(FLOAT_DATA, 1/factor,
                                                                             gs_str_dura_unit_s)));
                        break;
                    }
                }
                else
                {
                    valid_line = false;
                    ret_str = QString("%1%2%3%4%5:\n%6%7,%8").arg(gs_str_cust_file,
                                  g_str_the_line_pron, QString::number(line_no), gs_str_line,
                                  gs_str_data_item_invalid,
                                  gs_str_expo_dura, item_str, gs_str_should_be_number);
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
    float factor;
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

    /*2nd line: cube volt*/
    if(!file_stream.readLineInto(&line)) READ_CUST_FILE_ERR;
    file_content += line + "\n";
    ++line_no;
    line_items = line.split(gs_cust_expo_file_item_sep_in_line, Qt::SkipEmptyParts);
    item_cnt = line_items.count();
    if(item_cnt < min_items_per_line) CUST_FILE_LINE_FORMAT_ERROR;
    QVector<float> cube_current_ma_v;
    for(item_idx = 1; item_idx < item_cnt; ++item_idx)
    {
        float ma;
        ma = line_items[item_idx].toFloat(&tr_ok);
        if(!tr_ok) CUST_FILE_ITEM_FORMAT_ERROR(gs_str_should_be_number);
        if(!gs_valid_cube_current_ma_range.range_check(ma))
            CUST_FILE_ITEM_FORMAT_ERROR(line_items[item_idx]
                                        + gs_str_exceeds_valid_range
                                        + gs_valid_cube_current_ma_range.range_str(FLOAT_DATA));
        cube_current_ma_v.append(ma);
    }
    current_cnt = cube_current_ma_v.count();

    /*3rd line: cube volt*/
    if(!file_stream.readLineInto(&line)) READ_CUST_FILE_ERR;
    file_content += line + "\n";
    ++line_no;
    line_items = line.split(gs_cust_expo_file_item_sep_in_line, Qt::SkipEmptyParts);
    item_cnt = line_items.count();
    if(item_cnt < min_items_per_line) CUST_FILE_LINE_FORMAT_ERROR;
    if(!judge_dura_factor_from_str(line_items[0], &factor)) CUST_FILE_LINE_FORMAT_ERROR;
    QVector<float> dura_ms_v;
    max_ms = 0;
    for(item_idx = 1; item_idx < item_cnt; ++item_idx)
    {
        float ms_or_s, ms;
        ms_or_s = line_items[item_idx].toFloat(&tr_ok);
        if(!tr_ok) CUST_FILE_ITEM_FORMAT_ERROR(gs_str_should_be_number);
        if(!gs_valid_expo_dura_ms_range.range_check(ms_or_s * factor))
            CUST_FILE_ITEM_FORMAT_ERROR(line_items[item_idx]
                                        + gs_str_exceeds_valid_range
                                        + gs_valid_expo_dura_ms_range.range_str(FLOAT_DATA,
                                                                              1/factor,
                                                                              gs_str_dura_unit_s));
        ms = ms_or_s * factor;
        dura_ms_v.append(ms);
        if(ms > max_ms) max_ms = ms;
    }
    dura_cnt = dura_ms_v.count();

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

#undef CUST_FILE_ITEM_FORMAT_ERROR
#undef CUST_FILE_LINE_FORMAT_ERROR
#undef READ_CUST_FILE_ERR
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
                    ret_str += QString("%1%2、%3、%4、%5").arg(gs_str_cube_volt,
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
                    ret_str += QString("%1%2、%3、%4、%5").arg(gs_str_cube_current,
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
                    ret_str += QString("%1%2、%3、%4、%5").arg(gs_str_expo_dura,
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
    QString dura_unit_s;
    float factor;
    int times_in_one_loop = 1;

    test_mode = (test_mode_enum_t)(ui->testModeComboBox->currentData().toInt());

    if(!m_test_params || !m_test_params->valid) return;
    QString &info_str = m_test_params->info_str;

    if(ui->expoDuraUnitmsRButton->isChecked())
    {
        factor = 1;
        dura_unit_s = gs_str_dura_unit_ms;
    }
    else
    {
        factor = (float)(1.0/1000);
        dura_unit_s = gs_str_dura_unit_s;
    }

    info_str = QString("%1:").arg(gs_str_test_mode);
    info_str += test_mode_list[test_mode].s + "\n";
    info_str += QString(gs_info_str_seperator) + "\n";

    QString start_val_str("");
    if(TEST_MODE_TRAVERSE == test_mode) start_val_str = gs_str_start_val;
    if((TEST_MODE_CUST1_TRIPLES != test_mode) && (TEST_MODE_CUST2_DISCRETE != test_mode))
    {
        info_str += QString("%1%2:%3%4\n").
                    arg(gs_str_cube_volt, start_val_str,
                        QString::number(m_test_params->expo_param_block.expo_params.
                                            regular_parms.cube_volt_kv_start),
                        gs_str_volt_unit_kv);
        if(TEST_MODE_TRAVERSE == test_mode)
        {
            info_str += QString("%1%2:%3%4\n").
                        arg(gs_str_cube_volt, gs_str_end_val,
                            QString::number(m_test_params->expo_param_block.expo_params.
                                                regular_parms.cube_volt_kv_end),
                            gs_str_volt_unit_kv);
            info_str += QString("%1%2:%3%4\n").
                        arg(gs_str_cube_volt, gs_str_step,
                            QString::number(m_test_params->expo_param_block.expo_params.
                                                regular_parms.cube_volt_kv_step),
                            gs_str_volt_unit_kv);

            times_in_one_loop *= count_discrete_steps(m_test_params->expo_param_block.expo_params.
                                                          regular_parms.cube_volt_kv_start,
                                                      m_test_params->expo_param_block.expo_params.
                                                          regular_parms.cube_volt_kv_end,
                                                      m_test_params->expo_param_block.expo_params.
                                                          regular_parms.cube_volt_kv_step);
        }
        info_str += QString(gs_info_str_seperator) + "\n";

        info_str += QString("%1%2:%3%4\n").
                    arg(gs_str_cube_current, start_val_str,
                        QString::number(m_test_params->expo_param_block.
                                            expo_params.regular_parms.cube_current_ma_start),
                        gs_str_current_unit_ma);
        if(TEST_MODE_TRAVERSE == test_mode)
        {
            info_str += QString("%1%2:%3%4\n").
                        arg(gs_str_cube_current, gs_str_end_val,
                            QString::number(m_test_params->expo_param_block.expo_params.
                                                regular_parms.cube_current_ma_end),
                            gs_str_current_unit_ma);
            info_str += QString("%1%2:%3%4\n").
                        arg(gs_str_cube_current, gs_str_step,
                            QString::number(m_test_params->expo_param_block.expo_params.
                                                regular_parms.cube_current_ma_step),
                            gs_str_current_unit_ma);

            times_in_one_loop *= count_discrete_steps(m_test_params->expo_param_block.expo_params.
                                                          regular_parms.cube_current_ma_start,
                                                      m_test_params->expo_param_block.expo_params.
                                                          regular_parms.cube_current_ma_end,
                                                      m_test_params->expo_param_block.expo_params.
                                                          regular_parms.cube_current_ma_step);
        }
        info_str += QString(gs_info_str_seperator) + "\n";

        info_str += QString("%1%2:%3%4\n").
                    arg(gs_str_expo_dura, start_val_str,
                        QString::number(m_test_params->expo_param_block.
                                            expo_params.regular_parms.expo_dura_ms_start * factor),
                        dura_unit_s);
        if(TEST_MODE_TRAVERSE == test_mode)
        {
            info_str += QString("%1%2:%3%4\n").
                        arg(gs_str_expo_dura, gs_str_end_val,
                            QString::number(m_test_params->expo_param_block.
                                            expo_params.regular_parms.expo_dura_ms_end * factor),
                            dura_unit_s);
            info_str += QString("%1%2:%3%4\n").
                        arg(gs_str_expo_dura, gs_str_step,
                            QString::number(m_test_params->expo_param_block.expo_params.
                                                regular_parms.expo_dura_ms_step * factor),
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
                    gs_str_dura_unit_s);
            }
            else
            {
                info_str += QString("%1:%2%3%4").
                    arg(gs_str_cool_dura,
                        QString::number(m_test_params->expo_param_block.expo_cool_dura_factor),
                        g_str_time_bei,
                        gs_str_expo_dura);
            }
            info_str += (g_sys_configs_block.extra_cool_time_ms > 0) ?
                        (QString("+(")
                         + QString::number(g_sys_configs_block.extra_cool_time_ms / 1000)
                         + gs_str_dura_unit_s + gs_str_extra_cool_dura + ")") : "";
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
                    gs_str_dura_unit_s);
        }
        else
        {
            info_str += QString("%1:%2%3%4").
                arg(gs_str_cool_dura,
                    QString::number(m_test_params->expo_param_block.expo_cool_dura_factor),
                    g_str_time_bei,
                    gs_str_expo_dura);
        }
        info_str += (g_sys_configs_block.extra_cool_time_ms > 0) ?
                    (QString("+(")
                     + QString::number(g_sys_configs_block.extra_cool_time_ms / 1000)
                     + gs_str_dura_unit_s + gs_str_extra_cool_dura + ")") : "";
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
    info_str += ui->hwVerStrLbl->text() + ":" + ui->hwVerStrEdit->text();
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
             MAX_HV_NORMAL_MB_REG_NUM /*not a valid reg addr*/, EXT_MB_REG_DISTANCE,
            {
                 ui->distmmChkbox,
                 ui->distmmLowEdgePctLEdit, ui->distmmUpEdgePctLEdit,
                 ui->distmmLowEdgeErrValLEdit, ui->distmmUpEdgeErrValLEdit,
                 ui->distmmIsFixedRefChkbox, ui->distmmFixedRefValLEdit
            }
        }
    );

    /*
                 , ui->voltKVUpEdgePctLEdit,
                 ui->voltKVLowEdgeErrValLEdit, ui->voltKVUpEdgeErrValLEdit,
                 ui->voltKVIsFixedRefChkbox, ui->voltKVFixedRefValLEdit
                 */
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

#define GET_JUDGE_PARAM_FROM_CTRL(judge_ctrl, edit_ctrl, judge_param, param_var) \
        if(!CHKBOX_EXIST_AND_CHECKED((judge_ctrl).gui_ctrls.judge_or_not_chbox)) continue;\
        if(!(((judge_ctrl).gui_ctrls).edit_ctrl))\
        {\
            DIY_LOG(LOG_ERROR, "judge ctrl is NULL.");\
            ret = false;\
            func_ret = func_ret && ret;\
            continue;\
        }\
        ctrl_desc = JUDGE_CTRL_DESC_STR(((judge_ctrl).gui_ctrls).edit_ctrl);\
        val_str = ((judge_ctrl).gui_ctrls).edit_ctrl->text();\
        (judge_param).param_var = val_str.toFloat(&tr_ret);\
        if(!tr_ret)\
        {\
            err_str += ctrl_desc + " " + gs_str_should_be_number + "\n";\
            ret = false;\
        }\
        else\
        {\
            info_str += ctrl_desc + ":" + val_str + "\n";\
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

    for(int idx = 0; idx < m_judge_ctrls.count(); ++idx)
    {
        ret = true;

        GET_JUDGE_PARAM_FROM_CTRL(m_judge_ctrls[idx], low_e_pct_ledit, judge_param, low_e_pct);
        GET_JUDGE_PARAM_FROM_CTRL(m_judge_ctrls[idx], up_e_pct_ledit, judge_param, up_e_pct);
        GET_JUDGE_PARAM_FROM_CTRL(m_judge_ctrls[idx], low_e_err_val_ledit, judge_param, low_e_extra_val);
        GET_JUDGE_PARAM_FROM_CTRL(m_judge_ctrls[idx], up_e_err_val_ledit, judge_param, up_e_extra_val);

        if(CHKBOX_EXIST_AND_CHECKED(m_judge_ctrls[idx].gui_ctrls.is_fixed_ref_chbox))
        {
            GET_JUDGE_PARAM_FROM_CTRL(m_judge_ctrls[idx], fixed_ref_val_ledit,
                                      judge_param, fixed_ref_val);
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

void testParamSettingsDialog::on_distmmChkbox_stateChanged(int /*arg1*/)
{
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

