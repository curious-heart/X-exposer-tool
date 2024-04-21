#include "common_tool_func.h"
#include "logger/logger.h"
#include "test_param_settings.h"

static const int gs_cool_dura_factor = 30; //cool time shuld not be less than this times of expp dura.
static const float gs_min_expo_dura_ms = 1;
static RangeChecker gs_valid_cube_volt_kv_range(10, 1000);
static RangeChecker gs_valid_cube_current_ma_range(0.1, 1000);
static RangeChecker gs_valid_expo_dura_ms_range(gs_min_expo_dura_ms, 3600*1000);
static RangeChecker gs_valid_expo_cnt_range(1, 0,
                                            RangeChecker::EDGE_INCLUDED,
                                            RangeChecker::EDGE_INFINITE);
static RangeChecker gs_valid_cool_dura_range(gs_min_expo_dura_ms * gs_cool_dura_factor,
                                             0,
                                             RangeChecker::EDGE_INCLUDED,
                                             RangeChecker::EDGE_INFINITE);

static const char* gs_str_cube_volt = "管电压";
static const char* gs_str_cube_current = "管电压流";
static const char* gs_str_expo_dura = "曝光时间";
static const char* gs_str_repeats_num = "重复次数";
static const char* gs_str_cool_dura = "冷却时间";
static const char* gs_str_start_val = "起始值";
static const char* gs_str_end_val = "结束值";
static const char* gs_str_step = "步长";
static const char* gs_str_exceeds_valid_range = "超出允许范围";
static const char* gs_str_should_be_int = "应为整数";
static const char* gs_str_should_be_number = "应为数字";
static const char* gs_str_cust_file = "自定义曝光参数文件";

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

bool testParamSettingsDialog::
get_one_expo_param(QLineEdit * ctrl, ui_data_type_enum_t d_type, int factor,
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

    if(range)
    {
        tr_val = ui_val * factor;
        if(range->range_check(tr_val))
        {
            if(val_ptr)
            {
                (INT_DATA == d_type) ? *((int*)val_ptr) = (int)tr_val :
                                       *((float*)val_ptr) = (float)tr_val;
            }
            ret = true;
        }
        else
        {
            ret_str = QString("%1%2 ").arg(ctrl_str, gs_str_exceeds_valid_range);

            ret_str += (RangeChecker::EDGE_INCLUDED == range->range_low_edge()) ? "[" : "(";
            ret_str += (RangeChecker::EDGE_INFINITE == range->range_low_edge()) ? "" :
                        ((INT_DATA == d_type) ? QString::number((int)(range->range_min())) :
                                                QString::number((float)(range->range_min())));
            ret_str += ", ";
            ret_str += (RangeChecker::EDGE_INFINITE == range->range_up_edge()) ? "" :
                        ((INT_DATA == d_type) ? QString::number((int)(range->range_max())) :
                                                QString::number((float)(range->range_max())));
            ret_str += (RangeChecker::EDGE_INCLUDED == range->range_up_edge()) ? "]" : ")";
            ret_str += "\n";

            ret = false;
        }
    }
    else
    {
        ret = true;
    }

    return ret;
}

void testParamSettingsDialog::get_expo_param_vals_from_ui()
{
    int factor;
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

    m_expo_params_from_ui.valid_cool_dura =
        get_one_expo_param(ui->coolDuraEdit, FLOAT_DATA, 1000,
                           &gs_valid_cool_dura_range,
                           &m_expo_params_from_ui.expo_cool_dura_ms,
                           m_expo_params_from_ui.err_msg_cool_dura);
}

bool testParamSettingsDialog::get_expo_param_vals_from_cust_file(QString file_fpn,
                                        QVector<expo_param_triple_struct_t> &param_vector,
                                        QString &ret_str)
{
    return false;
}

QString testParamSettingsDialog::collect_test_params()
{
    test_mode_enum_t test_mode;
    test_mode = (test_mode_enum_t)(ui->testModeComboBox->currentData().toInt());
    QString ret_str;

    if(!m_test_params)
    {
        DIY_LOG(LOG_ERROR, "test params pointer is null.");
        return ret_str;
    }
    m_test_params->valid = false;

    get_expo_param_vals_from_ui();
    if(TEST_MODE_CUST == test_mode)
    {
        get_expo_param_vals_from_cust_file(ui->custExpoParamFileSelEdit->text(),
                               m_test_params->expo_param_block.expo_params.cust_params_arr,
                               ret_str);
        m_test_params->expo_param_block.cust = true;
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
                    (!m_expo_params_from_ui.valid_expo_cnt
                     || !m_expo_params_from_ui.valid_cool_dura)))
            {
                if(TEST_MODE_REPEAT == test_mode)
                {
                    ret_str += m_expo_params_from_ui.err_msg_expo_cnt
                             + m_expo_params_from_ui.err_msg_cool_dura;
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
                m_test_params->expo_param_block.expo_cool_dura_ms
                        = (TEST_MODE_SINGLE == test_mode) ?
                            0 : m_expo_params_from_ui.expo_cool_dura_ms;
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
                || !m_expo_params_from_ui.valid_expo_cnt
                || !m_expo_params_from_ui.valid_cool_dura)
            {
                ret_str += m_expo_params_from_ui.err_msg_cube_volt_end
                        + m_expo_params_from_ui.err_msg_cube_volt_step
                        + m_expo_params_from_ui.err_msg_cube_current_end
                        + m_expo_params_from_ui.err_msg_cube_current_step
                        + m_expo_params_from_ui.err_msg_expo_dura_end
                        + m_expo_params_from_ui.err_msg_expo_dura_step
                        + m_expo_params_from_ui.err_msg_expo_cnt
                        + m_expo_params_from_ui.err_msg_cool_dura;
            }
            else
            {
                m_test_params->expo_param_block.expo_params.regular_parms.cube_volt_kv_start
                        = m_expo_params_from_ui.vals.cube_volt_kv_start;
                m_test_params->expo_param_block.expo_params.regular_parms.cube_volt_kv_end
                        = m_expo_params_from_ui.vals.cube_volt_kv_end;
                m_test_params->expo_param_block.expo_params.regular_parms.cube_volt_kv_step
                        = m_expo_params_from_ui.vals.cube_volt_kv_step;
                m_test_params->expo_param_block.expo_params.regular_parms.cube_current_ma_start
                        = m_expo_params_from_ui.vals.cube_current_ma_start;
                m_test_params->expo_param_block.expo_params.regular_parms.cube_current_ma_end
                        = m_expo_params_from_ui.vals.cube_current_ma_end;
                m_test_params->expo_param_block.expo_params.regular_parms.cube_current_ma_step
                        = m_expo_params_from_ui.vals.cube_current_ma_step;
                m_test_params->expo_param_block.expo_params.regular_parms.expo_dura_ms_start
                        = m_expo_params_from_ui.vals.expo_dura_ms_start;
                m_test_params->expo_param_block.expo_params.regular_parms.expo_dura_ms_end
                        = m_expo_params_from_ui.vals.expo_dura_ms_end;
                m_test_params->expo_param_block.expo_params.regular_parms.expo_dura_ms_step
                        = m_expo_params_from_ui.vals.expo_dura_ms_step;
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

