#include <QMessageBox>
#include <QFile>
#include <QTextStream>
#include <QFileDialog>
#include "common_tools/common_tool_func.h"
#include "logger/logger.h"
#include "test_param_settings.h"
#include "ui_test_param_settings.h"

static const float gs_cool_dura_factor = 30; //cool time shuld not be less than this times of expp dura.
static RangeChecker gs_valid_cube_volt_kv_range(10, 1000);
static RangeChecker gs_valid_cube_current_ma_range(0.1, 1000);
static const float gs_min_expo_dura_ms = 1;
static RangeChecker gs_valid_expo_dura_ms_range(gs_min_expo_dura_ms, 3600*1000);
static RangeChecker gs_valid_expo_cnt_range(1, 0, "",
                                            RangeChecker::EDGE_INCLUDED,
                                            RangeChecker::EDGE_INFINITE);
static RangeChecker gs_valid_cool_dura_range(gs_min_expo_dura_ms * gs_cool_dura_factor,
                                             0,
                                             "s",
                                             RangeChecker::EDGE_INCLUDED,
                                             RangeChecker::EDGE_INFINITE);

static const char* gs_str_test_mode = "测试模式";
static const char* gs_str_cube_volt = "管电压";
static const char* gs_str_cube_current = "管电流";
static const char* gs_str_expo_dura = "曝光时间";
static const char* gs_str_repeats_num = "重复次数";
static const char* gs_str_cool_dura = "冷却时间";
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

static const char* gs_cust_expo_file_filter = "CSV File (*.csv)";
static const char* gs_valid_header_line_ms = "volt-kv,current-ma,dura-ms";
static const char* gs_valid_header_line_s = "volt-kv,current-ma,dura-s";
static const char* gs_cust_expo_file_item_sep_in_line = ",";
static const int gs_cust_expo_file_item_num_in_line = 3;

static const char* gs_str_the_line_pron = "第";
static const char* gs_str_line = "行";
static const char* gs_str_no_valid_data_item = "无有效数据";
const char* gs_str_data_item_invalid = "数据无效";

const testParamSettingsDialog::test_mode_espair_struct_t
      testParamSettingsDialog::test_mode_list[] =
{
    {TEST_MODE_SINGLE, QString("单次")},
    {TEST_MODE_REPEAT, QString("重复")},
    {TEST_MODE_TRAVERSE, QString("遍历")},
    {TEST_MODE_CUST, QString("自定义")},
};

/*
 * define which controls is enabled or disabled in every test mode.
 * the a_xx corresponds to the test modes in test_mode_list.
*/
#define ARRANGE_CTRLS_ABILITY(ctrl, a_sg, a_re, a_tr, a_cu) \
    m_test_params_ctrls_abilities.append({(ctrl), {(a_sg), (a_re), (a_tr), (a_cu)}});
#define TEST_PARAMS_CTRLS_ABT_TBL \
{\
    ARRANGE_CTRLS_ABILITY(ui->cubeVoltStartEdit, true, true, true, false)\
    ui->cubeVoltStartEdit->setWhatsThis(QString("%1%2").arg(gs_str_cube_volt, gs_str_start_val));\
    ARRANGE_CTRLS_ABILITY(ui->cubeVoltEndEdit, false, false, true, false)\
    ui->cubeVoltEndEdit->setWhatsThis(QString("%1%2").arg(gs_str_cube_volt, gs_str_end_val));\
    ARRANGE_CTRLS_ABILITY(ui->cubeVoltStepEdit, false, false, true, false)\
    ui->cubeVoltStepEdit->setWhatsThis(QString("%1%2").arg(gs_str_cube_volt, gs_str_step));\
    ARRANGE_CTRLS_ABILITY(ui->cubeCurrentStartEdit, true, true, true, false)\
    ui->cubeCurrentStartEdit->setWhatsThis(QString("%1%2").arg(gs_str_cube_current, gs_str_start_val));\
    ARRANGE_CTRLS_ABILITY(ui->cubeCurrentEndEdit, false, false, true, false)\
    ui->cubeCurrentEndEdit->setWhatsThis(QString("%1%2").arg(gs_str_cube_current, gs_str_end_val));\
    ARRANGE_CTRLS_ABILITY(ui->cubeCurrentStepEdit, false, false, true, false)\
    ui->cubeCurrentStepEdit->setWhatsThis(QString("%1%2").arg(gs_str_cube_current, gs_str_step));\
    ARRANGE_CTRLS_ABILITY(ui->expoDuraStartEdit, true, true, true, false)\
    ui->expoDuraStartEdit->setWhatsThis(QString("%1%2").arg(gs_str_expo_dura, gs_str_start_val));\
    ARRANGE_CTRLS_ABILITY(ui->expoDuraEndEdit, false, false, true, false)\
    ui->expoDuraEndEdit->setWhatsThis(QString("%1%2").arg(gs_str_expo_dura, gs_str_end_val));\
    ARRANGE_CTRLS_ABILITY(ui->expoDuraStepEdit, false, false, true, false)\
    ui->expoDuraStepEdit->setWhatsThis(QString("%1%2").arg(gs_str_expo_dura, gs_str_step));\
    ARRANGE_CTRLS_ABILITY(ui->repeatsNumEdit, false, true, true, true)\
    ui->repeatsNumEdit->setWhatsThis(QString(gs_str_repeats_num));\
    ARRANGE_CTRLS_ABILITY(ui->coolDuraEdit, false, true, true, true)\
    ui->coolDuraEdit->setWhatsThis(QString(gs_str_cool_dura));\
    \
    ARRANGE_CTRLS_ABILITY(ui->custExpoParamFileSelBtn, false, false, false, true)\
    ARRANGE_CTRLS_ABILITY(ui->custExpoParamFileCommentLbl, false, false, false, true)\
}

testParamSettingsDialog::testParamSettingsDialog(QWidget *parent, test_params_struct_t *test_params_ptr) :
    QDialog(parent),
    ui(new Ui::testParamSettingsDialog),
    m_test_params(test_params_ptr)
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
    ui->expoDuraUnitmsRButton->setChecked(true);

    int idx;
    for(idx = 0; idx < ARRAY_COUNT(test_mode_list); ++idx)
    {
        ui->testModeComboBox->addItem(test_mode_list[idx].s, test_mode_list[idx].e);
    }
    ui->testModeComboBox->setCurrentIndex(0);

    TEST_PARAMS_CTRLS_ABT_TBL;

    refresh_controls_display();
}
#undef ARRANGE_CTRLS_ABILITY
#undef TEST_PARAMS_CTRLS_ABT_TBL

testParamSettingsDialog::~testParamSettingsDialog()
{
    delete ui;
}

void testParamSettingsDialog::clear_local_buffer()
{
    m_expo_params_from_ui.err_msg_cube_volt_start.clear();
    m_expo_params_from_ui.err_msg_cube_volt_end.clear();
    m_expo_params_from_ui.err_msg_cube_volt_step.clear();
    m_expo_params_from_ui.err_msg_cube_current_start.clear();
    m_expo_params_from_ui.err_msg_cube_current_end.clear();
    m_expo_params_from_ui.err_msg_cube_current_step.clear();
    m_expo_params_from_ui.err_msg_expo_dura_start.clear();
    m_expo_params_from_ui.err_msg_expo_dura_end.clear();
    m_expo_params_from_ui.err_msg_expo_dura_step.clear();
    m_expo_params_from_ui.err_msg_expo_cnt.clear();
    m_expo_params_from_ui.err_msg_cool_dura.clear();
}

bool testParamSettingsDialog::
get_one_expo_param(QLineEdit * ctrl, common_data_type_enum_t d_type, float factor,
                   RangeChecker* range, void* val_ptr, QString &ret_str)
{
    QString ctrl_str, d_type_str;
    bool tr_ret, ret;
    float ui_val, tr_val;

    if(!ctrl)
    {
        DIY_LOG(LOG_ERROR, "ctrl is NULL");
        return false;
    }

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

        ret_str += range->range_str(d_type, 1/factor);
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
        get_one_expo_param(ui->cubeVoltStartEdit, INT_DATA, 1,
                           &gs_valid_cube_volt_kv_range,
                           &m_expo_params_from_ui.vals.cube_volt_kv_start,
                           m_expo_params_from_ui.err_msg_cube_volt_start);
    m_expo_params_from_ui.valid_cube_volt_end =
        get_one_expo_param(ui->cubeVoltEndEdit, INT_DATA, 1,
                           &gs_valid_cube_volt_kv_range,
                           &m_expo_params_from_ui.vals.cube_volt_kv_end,
                           m_expo_params_from_ui.err_msg_cube_volt_end);
    m_expo_params_from_ui.valid_cube_volt_step =
        get_one_expo_param(ui->cubeVoltStepEdit, INT_DATA, 1,
                           nullptr,
                           &m_expo_params_from_ui.vals.cube_volt_kv_step,
                           m_expo_params_from_ui.err_msg_cube_volt_step);

    m_expo_params_from_ui.valid_cube_current_start =
        get_one_expo_param(ui->cubeCurrentStartEdit, FLOAT_DATA, 1,
                           &gs_valid_cube_current_ma_range,
                           &m_expo_params_from_ui.vals.cube_current_ma_start,
                           m_expo_params_from_ui.err_msg_cube_current_start);
    m_expo_params_from_ui.valid_cube_current_end =
        get_one_expo_param(ui->cubeCurrentEndEdit, FLOAT_DATA, 1,
                           &gs_valid_cube_current_ma_range,
                           &m_expo_params_from_ui.vals.cube_current_ma_end,
                           m_expo_params_from_ui.err_msg_cube_current_end);
    m_expo_params_from_ui.valid_cube_current_step =
        get_one_expo_param(ui->cubeCurrentStepEdit, FLOAT_DATA, 1,
                           nullptr,
                           &m_expo_params_from_ui.vals.cube_current_ma_step,
                           m_expo_params_from_ui.err_msg_cube_current_step);

    if(ui->expoDuraUnitsecRButton->isChecked())
    {
        factor = 1000;
    }
    else
    {
        factor = 1;
    }
    m_expo_params_from_ui.valid_expo_dura_start =
        get_one_expo_param(ui->expoDuraStartEdit, FLOAT_DATA, factor,
                           &gs_valid_expo_dura_ms_range,
                           &m_expo_params_from_ui.vals.expo_dura_ms_start,
                           m_expo_params_from_ui.err_msg_expo_dura_start);
    m_expo_params_from_ui.valid_expo_dura_end =
        get_one_expo_param(ui->expoDuraEndEdit, FLOAT_DATA, factor,
                           &gs_valid_expo_dura_ms_range,
                           &m_expo_params_from_ui.vals.expo_dura_ms_end,
                           m_expo_params_from_ui.err_msg_expo_dura_end);
    m_expo_params_from_ui.valid_expo_dura_step =
        get_one_expo_param(ui->expoDuraStepEdit, FLOAT_DATA, factor,
                           nullptr,
                           &m_expo_params_from_ui.vals.expo_dura_ms_step,
                           m_expo_params_from_ui.err_msg_expo_dura_step);

    m_expo_params_from_ui.valid_expo_cnt =
        get_one_expo_param(ui->repeatsNumEdit, INT_DATA, 1,
                           &gs_valid_expo_cnt_range,
                           &m_expo_params_from_ui.expo_cnt,
                           m_expo_params_from_ui.err_msg_expo_cnt);

    /* cool duration is not got here because it is related to the exposure duration.
     * so we count it after checking the latter.
     */
}

bool testParamSettingsDialog::get_expo_param_vals_from_cust_file(QString file_fpn,
                                        QVector<expo_param_triple_struct_t> &param_vector,
                                        float * max_expo_dura_ms,
                                        QString &ret_str)
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
        QString qstr_s = QString(gs_str_dura_unit_s), qstr_ms = QString(gs_str_dura_unit_ms);
        int s_len = qstr_s.length(), ms_len = qstr_ms.length();
        int l_len = line.length();
        if(l_len >= ms_len)
        {
            if(line.endsWith(qstr_ms))
            {
                valid_line = true;
                factor = 1;
            }
            else if(line.endsWith(qstr_s))
            {
                valid_line = true;
                factor = 1000;
            }
        }
        else if(l_len >= s_len)
        {
            if(line.endsWith(qstr_s))
            {
                valid_line = true;
                factor = 1000;
            }
        }
    }
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
                                  gs_str_the_line_pron, QString::number(line_no), gs_str_line,
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
                                  gs_str_the_line_pron, QString::number(line_no), gs_str_line,
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
                                  gs_str_the_line_pron, QString::number(line_no), gs_str_line,
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
                                  gs_str_the_line_pron, QString::number(line_no), gs_str_line,
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
                                  gs_str_the_line_pron, QString::number(line_no), gs_str_line,
                                  gs_str_data_item_invalid,
                                  gs_str_expo_dura, item_str, gs_str_exceeds_valid_range,
                                  gs_valid_expo_dura_ms_range.range_str(FLOAT_DATA, 1/factor));
                        DIY_LOG(LOG_ERROR, QString("%1%2%3%4%5").arg(file_fpn,
                                       ":the ", QString::number(line_no),
                                       "expo duration exceeds valid range:",
                                       gs_valid_expo_dura_ms_range.range_str(FLOAT_DATA, 1/factor)));
                        break;
                    }
                }
                else
                {
                    valid_line = false;
                    ret_str = QString("%1%2%3%4%5:\n%6%7,%8").arg(gs_str_cust_file,
                                  gs_str_the_line_pron, QString::number(line_no), gs_str_line,
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
                                  gs_str_the_line_pron, QString::number(line_no), gs_str_line,
                                  gs_str_no_valid_data_item);
                DIY_LOG(LOG_ERROR, QString("%1%2%3%4").arg(file_fpn, ":the ",
                                       QString::number(line_no), " line has no valid data."));
                break;
            }
        }
    }
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

QString testParamSettingsDialog::collect_test_params()
{
    test_mode_enum_t test_mode;
    test_mode = (test_mode_enum_t)(ui->testModeComboBox->currentData().toInt());
    QString ret_str;
    float max_expo_dura;

    if(!m_test_params)
    {
        DIY_LOG(LOG_ERROR, "test params pointer is null.");
        return ret_str;
    }
    m_test_params->valid = false;

    get_expo_param_vals_from_ui();
    if(TEST_MODE_CUST == test_mode)
    {
        m_test_params->expo_param_block.cust = false;
        m_test_params->expo_param_block.expo_params.cust_params_arr.clear();
        if(!m_expo_params_from_ui.valid_expo_cnt)
        {
            ret_str += m_expo_params_from_ui.err_msg_expo_cnt;
        }
        else if(get_expo_param_vals_from_cust_file(ui->custExpoParamFileSelEdit->text(),
                               m_test_params->expo_param_block.expo_params.cust_params_arr,
                               &max_expo_dura,
                               ret_str))
        {
            gs_valid_cool_dura_range.set_min_max(max_expo_dura * gs_cool_dura_factor, 0);
            m_expo_params_from_ui.valid_cool_dura =
                get_one_expo_param(ui->coolDuraEdit, FLOAT_DATA, 1000,
                                   &gs_valid_cool_dura_range,
                                   &m_expo_params_from_ui.expo_cool_dura_ms,
                                   m_expo_params_from_ui.err_msg_cool_dura);
            if(m_expo_params_from_ui.valid_cool_dura)
            {
                m_test_params->expo_param_block.cust = true;

                m_test_params->expo_param_block.expo_cool_dura_ms
                        = m_expo_params_from_ui.expo_cool_dura_ms;
                m_test_params->expo_param_block.expo_cnt = m_expo_params_from_ui.expo_cnt;
                m_test_params->valid = true;
            }
            else
            {
                ret_str += m_expo_params_from_ui.err_msg_cool_dura;
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
                if(TEST_MODE_REPEAT == test_mode)
                {
                    gs_valid_cool_dura_range.
                    set_min_max(m_expo_params_from_ui.vals.expo_dura_ms_start * gs_cool_dura_factor, 0);
                    m_expo_params_from_ui.valid_cool_dura =
                        get_one_expo_param(ui->coolDuraEdit, FLOAT_DATA, 1000,
                                           &gs_valid_cool_dura_range,
                                           &m_expo_params_from_ui.expo_cool_dura_ms,
                                           m_expo_params_from_ui.err_msg_cool_dura);
                    if(!m_expo_params_from_ui.valid_cool_dura)
                    {
                        ret_str += m_expo_params_from_ui.err_msg_cool_dura;
                        break;
                    }
                }
                else
                {
                    m_expo_params_from_ui.valid_cool_dura = true;
                    m_test_params->expo_param_block.expo_cool_dura_ms = 0;
                }

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

                max_expo_dura = qMax(m_expo_params_from_ui.vals.expo_dura_ms_start,
                                     m_expo_params_from_ui.vals.expo_dura_ms_end);
                gs_valid_cool_dura_range.set_min_max(max_expo_dura * gs_cool_dura_factor, 0);
                m_expo_params_from_ui.valid_cool_dura =
                    get_one_expo_param(ui->coolDuraEdit, FLOAT_DATA, 1000,
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

    format_test_params_info_str();
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
            accept();
        }
        else
        {
            QMessageBox::critical(this, "ERROR!", ret_str);
        }
    }
}

void testParamSettingsDialog::format_test_params_info_str()
{
    test_mode_enum_t test_mode;
    QString dura_unit_s;
    float factor;
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

    QString start_val_str("");
    if(TEST_MODE_TRAVERSE == test_mode) start_val_str = gs_str_start_val;
    if(TEST_MODE_CUST != test_mode)
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
        }

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
        }

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
                                                regular_parms.expo_dura_ms_step),
                            dura_unit_s);
        }

        if(TEST_MODE_TRAVERSE == test_mode || TEST_MODE_REPEAT == test_mode)
        {
            info_str += QString("%1:%2\n").
                        arg(gs_str_repeats_num,
                            QString::number(m_test_params->expo_param_block.expo_cnt));
            info_str += QString("%1:%2%3\n").
                        arg(gs_str_cool_dura,
                            QString::number(m_test_params->expo_param_block.expo_cool_dura_ms / 1000),
                            gs_str_dura_unit_s);
        }
    }
    else
    {
        info_str += QString("%1%2%3\n").
                        arg(QString::number(m_test_params->expo_param_block.expo_params.
                                            cust_params_arr.count()),
                            gs_str_group, gs_str_expo_params);
        info_str += QString("%1:%2\n").
                    arg(gs_str_repeats_num,
                        QString::number(m_test_params->expo_param_block.expo_cnt));
        info_str += QString("%1:%2%3\n").
                    arg(gs_str_cool_dura,
                        QString::number(m_test_params->expo_param_block.expo_cool_dura_ms / 1000),
                        gs_str_dura_unit_s);
    }
    info_str += ui->oilBoxNoLbl->text() + ":" + ui->oilBoxNoEdit->text() + "\n";
    info_str += ui->hvCtrlBoardNoLbl->text() + ":" + ui->hvCtrlBoardNoEdit->text() + "\n";
    info_str += ui->swVerStrLbl->text() + ":" + ui->swVerStrEdit->text() + "\n";
    info_str += ui->hwVerStrLbl->text() + ":" + ui->hwVerStrEdit->text();
}
