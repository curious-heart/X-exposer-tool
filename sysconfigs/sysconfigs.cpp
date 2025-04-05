#include <QSettings>

#include "logger/logger.h"
#include "common_tools/common_tool_func.h"
#include "sysconfigs/sysconfigs.h"

static const char* gs_sysconfigs_file_fpn = "configs/configs.ini";

static const char* gs_ini_grp_sys_cfgs = "sys_cfgs";
static const char* gs_ini_key_log_level = "log_level";

static const char* gs_ini_grp_expo_ctrl = "expo_ctrl";
static const char* gs_ini_key_cool_dura_factor = "cool_dura_factor";
static const char* gs_ini_key_extra_cool_time_ms = "extra_cool_time_ms";
static const char* gs_ini_key_expo_prepare_time_ms = "expo_prepare_tims_ms";
static const char* gs_ini_key_mb_consec_rw_wait_ms = "mb_consec_rw_wait_ms";
static const char* gs_ini_key_mb_err_retry_wait_ms = "mb_err_retry_wait_ms";
static const char* gs_ini_key_mb_one_cmd_round_time_ms = "mb_one_cmd_round_time_ms";

static const char* gs_ini_key_cube_volt_kv_min = "cube_volt_kv_min";
static const char* gs_ini_key_cube_volt_kv_max = "cube_volt_kv_max";
static const char* gs_ini_key_cube_current_ma_min = "cube_current_ma_min";
static const char* gs_ini_key_cube_current_ma_max = "cube_current_ma_max";
static const char* gs_ini_key_dura_sec_min = "dura_sec_min";
static const char* gs_ini_key_dura_sec_max = "dura_sec_max";
static const char* gs_ini_key_mb_reconnect_wait_sep_ms = "mb_reconnect_wait_sep_ms";
static const char* gs_ini_key_test_time_stat_grain_sec = "test_time_stat_grain_sec";

static const char* gs_ini_key_mb_cube_current_intf_unit = "mb_cube_current_intf_unit";
static const char* gs_ini_key_mb_dura_intf_unit = "mb_dura_intf_unit";

extern const char* g_str_cube_volt;
extern const char* g_str_cube_current;
extern const char* g_str_expo_dura;
extern const char* g_str_volt_unit_kv;
extern const char* g_str_current_unit_ma;
extern const char* g_str_current_unit_ua;
extern const char* g_str_dura_unit_min;
extern const char* g_str_dura_unit_s;
extern const char* g_str_dura_unit_ms;

sys_configs_struct_t g_sys_configs_block;

static const int gs_def_log_level = LOG_ERROR;

static const int gs_def_cube_volt_kv_min = 40;
static const int gs_def_cube_volt_kv_max = 90;
static const float gs_def_cube_current_ma_min = 0.5;
static const float gs_def_cube_current_ma_max = 5;
static const float gs_def_dura_sec_min = 0.5;
static const float gs_def_dura_sec_max = 1.4;

static const float gs_def_cool_dura_factor = 30;
static const int gs_def_extra_cool_time_ms = 2000;
static const int gs_def_expo_prepare_time_ms = 3500;
static const int gs_def_mb_consec_rw_wait_ms = 500;
static const int gs_def_mb_err_retry_wait_ms = 1000;
static const int gs_def_mb_reconnect_wait_sep_ms = 1000;
static const int gs_def_test_time_stat_grain_sec = 3;
static const int gs_def_mb_one_cmd_round_time_ms = 150;

static const char* gs_cfg_param_limit_error = "参数门限配置错误，请检查！";

static const mb_cube_current_intf_unit_e_t gs_def_mb_cube_current_intf_unit = MB_CUBE_CURRENT_INTF_UNIT_UA;
static const mb_dura_intf_unit_e_t gs_def_mb_dura_intf_unit = MB_DURA_INTF_UNIT_MS;

static RangeChecker<int> gs_cfg_file_log_level_ranger((int)LOG_DEBUG, (int)LOG_ERROR, "",
                     EDGE_INCLUDED, EDGE_INCLUDED);

static RangeChecker<int> gs_cfg_file_value_ge0_int_ranger(0, 0, "",
                           EDGE_INCLUDED, EDGE_INFINITE);

static RangeChecker<float> gs_cfg_file_value_ge0_float_ranger(0, 0, "",
                       EDGE_INCLUDED, EDGE_INFINITE);

static RangeChecker<int> gs_cfg_file_value_gt0_int_ranger(0, 0, "",
                       EDGE_EXCLUDED, EDGE_INFINITE);

static RangeChecker<float> gs_cfg_file_value_gt0_float_ranger(0, 0, "",
                       EDGE_EXCLUDED, EDGE_INFINITE);

/*the __VA_ARGS__ should be empty or a type converter like (cust_type).*/
#define GET_INF_CFG_NUMBER_VAL(settings, key, type_func, var, def, factor, checker, ...)\
{\
    (var) = __VA_ARGS__((factor) * ((settings).value((key), (def)).type_func()));\
    if((checker) && !((checker)->range_check((var))))\
    {\
        (var) = (def);\
    }\
}

#define BEGIN_INT_RANGE_CHECK(low, up, low_inc, up_inc)\
{\
    RangeChecker<int> int_range_checker((low), (up), "", low_inc, up_inc);
#define END_INT_RANGE_CHECK \
}

bool fill_sys_configs(QString * ret_str_ptr)
{
    bool ret = true;
    QString ret_str;
    QSettings settings(gs_sysconfigs_file_fpn, QSettings::IniFormat);

    /*--------------------*/
    settings.beginGroup(gs_ini_grp_sys_cfgs);
    GET_INF_CFG_NUMBER_VAL(settings, gs_ini_key_log_level, toInt,
                           g_sys_configs_block.log_level, gs_def_log_level,
                           1, &gs_cfg_file_log_level_ranger);
    settings.endGroup();

    /*--------------------*/
    settings.beginGroup(gs_ini_grp_expo_ctrl);

    GET_INF_CFG_NUMBER_VAL(settings, gs_ini_key_cube_volt_kv_min, toInt,
                           g_sys_configs_block.cube_volt_kv_min, gs_def_cube_volt_kv_min,
                           1, &gs_cfg_file_value_gt0_int_ranger);

    GET_INF_CFG_NUMBER_VAL(settings, gs_ini_key_cube_volt_kv_max, toInt,
                           g_sys_configs_block.cube_volt_kv_max, gs_def_cube_volt_kv_max,
                           1, &gs_cfg_file_value_gt0_int_ranger);

    GET_INF_CFG_NUMBER_VAL(settings, gs_ini_key_cube_current_ma_min, toFloat,
                           g_sys_configs_block.cube_current_ma_min, gs_def_cube_current_ma_min,
                           1, &gs_cfg_file_value_gt0_float_ranger);

    GET_INF_CFG_NUMBER_VAL(settings, gs_ini_key_cube_current_ma_max, toFloat,
                           g_sys_configs_block.cube_current_ma_max, gs_def_cube_current_ma_max,
                           1, &gs_cfg_file_value_gt0_float_ranger);

    GET_INF_CFG_NUMBER_VAL(settings, gs_ini_key_dura_sec_min, toFloat,
                           g_sys_configs_block.dura_ms_min, gs_def_dura_sec_min,
                           1000, &gs_cfg_file_value_gt0_float_ranger);

    GET_INF_CFG_NUMBER_VAL(settings, gs_ini_key_dura_sec_max, toFloat,
                           g_sys_configs_block.dura_ms_max, gs_def_dura_sec_max,
                           1000, &gs_cfg_file_value_gt0_float_ranger);

    GET_INF_CFG_NUMBER_VAL(settings, gs_ini_key_cool_dura_factor, toFloat,
                           g_sys_configs_block.cool_dura_factor, gs_def_cool_dura_factor,
                           1, &gs_cfg_file_value_ge0_float_ranger);

    GET_INF_CFG_NUMBER_VAL(settings, gs_ini_key_extra_cool_time_ms, toInt,
                           g_sys_configs_block.extra_cool_time_ms, gs_def_extra_cool_time_ms,
                           1, &gs_cfg_file_value_ge0_int_ranger);

    GET_INF_CFG_NUMBER_VAL(settings, gs_ini_key_expo_prepare_time_ms, toInt,
                           g_sys_configs_block.expo_prepare_time_ms, gs_def_expo_prepare_time_ms,
                           1, &gs_cfg_file_value_ge0_int_ranger);

    GET_INF_CFG_NUMBER_VAL(settings, gs_ini_key_mb_consec_rw_wait_ms, toInt,
                           g_sys_configs_block.consec_rw_wait_ms, gs_def_mb_consec_rw_wait_ms,
                           1, &gs_cfg_file_value_gt0_int_ranger);

    GET_INF_CFG_NUMBER_VAL(settings, gs_ini_key_mb_reconnect_wait_sep_ms, toInt,
                           g_sys_configs_block.mb_reconnect_wait_ms, gs_def_mb_reconnect_wait_sep_ms,
                           1, &gs_cfg_file_value_ge0_int_ranger);

    GET_INF_CFG_NUMBER_VAL(settings, gs_ini_key_mb_err_retry_wait_ms, toInt,
                       g_sys_configs_block.mb_err_retry_wait_ms, gs_def_mb_err_retry_wait_ms,
                           1, &gs_cfg_file_value_ge0_int_ranger);

    GET_INF_CFG_NUMBER_VAL(settings, gs_ini_key_test_time_stat_grain_sec, toInt,
                   g_sys_configs_block.test_time_stat_grain_sec, gs_def_test_time_stat_grain_sec,
                           1, &gs_cfg_file_value_ge0_int_ranger);

    GET_INF_CFG_NUMBER_VAL(settings, gs_ini_key_mb_one_cmd_round_time_ms, toInt,
                   g_sys_configs_block.mb_one_cmd_round_time_ms, gs_def_mb_one_cmd_round_time_ms,
                           1, &gs_cfg_file_value_ge0_int_ranger);

    BEGIN_INT_RANGE_CHECK(MB_CUBE_CURRENT_INTF_UNIT_UA, MB_CUBE_CURRENT_INTF_UNIT_MA,
                          EDGE_INCLUDED, EDGE_INCLUDED)
        GET_INF_CFG_NUMBER_VAL(settings, gs_ini_key_mb_cube_current_intf_unit, toInt,
                       g_sys_configs_block.mb_cube_current_intf_unit,
                               gs_def_mb_cube_current_intf_unit,
                               1, &int_range_checker, (mb_cube_current_intf_unit_e_t));
    END_INT_RANGE_CHECK

    BEGIN_INT_RANGE_CHECK(MB_DURA_INTF_UNIT_MS, MB_DURA_INTF_UNIT_SEC,
                          EDGE_INCLUDED, EDGE_INCLUDED)
        GET_INF_CFG_NUMBER_VAL(settings, gs_ini_key_mb_dura_intf_unit, toInt,
                       g_sys_configs_block.mb_dura_intf_unit,
                               gs_def_mb_dura_intf_unit,
                               1, &int_range_checker, (mb_dura_intf_unit_e_t));
    END_INT_RANGE_CHECK

    settings.endGroup();

    /*check the validation of config parameters.*/
#define CHECK_LIMIT(l_name, min_l, max_l, unit_str) \
    if(min_l > max_l)\
    {\
        ret = false;\
        ret_str += QString("\n") + \
                   l_name + " [" + QString::number(min_l) + ", " + QString::number(max_l) + "] " +\
                   unit_str;\
    }

    ret_str = QString(gs_cfg_param_limit_error) + "\n";
    CHECK_LIMIT(g_str_cube_volt,
                g_sys_configs_block.cube_volt_kv_min, g_sys_configs_block.cube_volt_kv_max,
                g_str_volt_unit_kv)
    CHECK_LIMIT(g_str_cube_current,
                g_sys_configs_block.cube_current_ma_min, g_sys_configs_block.cube_current_ma_max,
                g_str_current_unit_ma)
    CHECK_LIMIT(g_str_expo_dura,
                (g_sys_configs_block.dura_ms_min/1000), (g_sys_configs_block.dura_ms_max/1000),
                g_str_dura_unit_s)

    if(ret_str_ptr) *ret_str_ptr = ret_str;
    return ret;
#undef CHECK_LIMIT
}
