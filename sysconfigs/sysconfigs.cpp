#include <QSettings>

#include "common_tools/common_tool_func.h"
#include "sysconfigs/sysconfigs.h"

static const char* gs_sysconfigs_file_fpn = "configs/configs.ini";
static const char* gs_ini_grp_expo_ctrl = "expo_ctrl";
static const char* gs_ini_key_cool_dura_factor = "cool_dura_factor";
static const char* gs_ini_key_extra_cool_time_ms = "extra_cool_time_ms";
static const char* gs_ini_key_expo_prepare_time_ms = "expo_prepare_tims_ms";
static const char* gs_ini_key_mb_consec_rw_wait_ms = "mb_consec_rw_wait_ms";
static const char* gs_ini_key_mb_err_retry_wait_ms = "mb_err_retry_wait_ms";

static const char* gs_ini_key_cube_volt_kv_min = "cube_volt_kv_min";
static const char* gs_ini_key_cube_volt_kv_max = "cube_volt_kv_max";
static const char* gs_ini_key_cube_current_ma_min = "cube_current_ma_min";
static const char* gs_ini_key_cube_current_ma_max = "cube_current_ma_max";
static const char* gs_ini_key_dura_ms_min = "dura_ms_min";
static const char* gs_ini_key_dura_ms_max = "dura_ms_max";
static const char* gs_ini_key_mb_reconnect_wait_sep_ms = "mb_reconnect_wait_sep_ms";

static RangeChecker gs_cfg_file_value_ge0_ranger(0, 0, "",
                                       RangeChecker::EDGE_INCLUDED, RangeChecker::EDGE_INFINITE);
static RangeChecker gs_cfg_file_value_gt0_ranger(0, 0, "",
                                       RangeChecker::EDGE_EXCLUDED, RangeChecker::EDGE_INFINITE);

sys_configs_struct_t g_sys_configs_block;

static const int gs_def_cube_volt_kv_min = 40;
static const int gs_def_cube_volt_kv_max = 90;
static const float gs_def_cube_current_ma_min = 0.5;
static const float gs_def_cube_current_ma_max = 5;
static const float gs_def_dura_ms_min = 500;
static const float gs_def_dura_ms_max = 1400;

static const float gs_def_cool_dura_factor = 30;
static const int gs_def_extra_cool_time_ms = 2000;
static const int gs_def_expo_prepare_time_ms = 3500;
static const int gs_def_mb_consec_rw_wait_ms = 500;
static const int gs_def_mb_err_retry_wait_ms = 1000;
static const int gs_def_mb_reconnect_wait_sep_ms = 1000;

#define GET_INF_CFG_NUMBER_VAL(settings, key, type_func, var, def, checker)\
{\
    (var) = (settings).value((key), (def)).type_func();\
    if((checker) && !((checker)->range_check((var))))\
    {\
        (var) = (def);\
    }\
}

void fill_sys_configs()
{
    QSettings settings(gs_sysconfigs_file_fpn, QSettings::IniFormat);

    settings.beginGroup(gs_ini_grp_expo_ctrl);

    GET_INF_CFG_NUMBER_VAL(settings, gs_ini_key_cube_volt_kv_min, toInt,
                           g_sys_configs_block.cube_volt_kv_min, gs_def_cube_volt_kv_min,
                               &gs_cfg_file_value_gt0_ranger);

    GET_INF_CFG_NUMBER_VAL(settings, gs_ini_key_cube_volt_kv_max, toInt,
                           g_sys_configs_block.cube_volt_kv_max, gs_def_cube_volt_kv_max,
                               &gs_cfg_file_value_gt0_ranger);

    GET_INF_CFG_NUMBER_VAL(settings, gs_ini_key_cube_current_ma_min, toFloat,
                           g_sys_configs_block.cube_current_ma_min, gs_def_cube_current_ma_min,
                               &gs_cfg_file_value_gt0_ranger);

    GET_INF_CFG_NUMBER_VAL(settings, gs_ini_key_cube_current_ma_max, toFloat,
                           g_sys_configs_block.cube_current_ma_max, gs_def_cube_current_ma_max,
                               &gs_cfg_file_value_gt0_ranger);

    GET_INF_CFG_NUMBER_VAL(settings, gs_ini_key_dura_ms_min, toFloat,
                           g_sys_configs_block.dura_ms_min, gs_def_dura_ms_min,
                               &gs_cfg_file_value_gt0_ranger);

    GET_INF_CFG_NUMBER_VAL(settings, gs_ini_key_dura_ms_max, toFloat,
                           g_sys_configs_block.dura_ms_max, gs_def_dura_ms_max,
                               &gs_cfg_file_value_gt0_ranger);

    GET_INF_CFG_NUMBER_VAL(settings, gs_ini_key_cool_dura_factor, toFloat,
                               g_sys_configs_block.cool_dura_factor, gs_def_cool_dura_factor,
                               &gs_cfg_file_value_ge0_ranger);

    GET_INF_CFG_NUMBER_VAL(settings, gs_ini_key_extra_cool_time_ms, toInt,
                               g_sys_configs_block.extra_cool_time_ms, gs_def_extra_cool_time_ms,
                               &gs_cfg_file_value_ge0_ranger);

    GET_INF_CFG_NUMBER_VAL(settings, gs_ini_key_expo_prepare_time_ms, toInt,
                               g_sys_configs_block.expo_prepare_time_ms, gs_def_expo_prepare_time_ms,
                               &gs_cfg_file_value_ge0_ranger);

    GET_INF_CFG_NUMBER_VAL(settings, gs_ini_key_mb_consec_rw_wait_ms, toInt,
                           g_sys_configs_block.consec_rw_wait_ms, gs_def_mb_consec_rw_wait_ms,
                           &gs_cfg_file_value_gt0_ranger);

    GET_INF_CFG_NUMBER_VAL(settings, gs_ini_key_mb_reconnect_wait_sep_ms, toInt,
                           g_sys_configs_block.mb_reconnect_wait_ms, gs_def_mb_reconnect_wait_sep_ms,
                           &gs_cfg_file_value_ge0_ranger);

    GET_INF_CFG_NUMBER_VAL(settings, gs_ini_key_mb_err_retry_wait_ms, toInt,
                       g_sys_configs_block.mb_err_retry_wait_ms, gs_def_mb_err_retry_wait_ms,
                           &gs_cfg_file_value_ge0_ranger);

    settings.endGroup();
}
