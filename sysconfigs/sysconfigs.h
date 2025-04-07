#ifndef SYSCONFIGS_H
#define SYSCONFIGS_H

#include <QString>

#define ENUM_NAME_DEF(e) e,
#define MB_CUBE_CURRENT_UNIT_E \
    ENUM_NAME_DEF(MB_CUBE_CURRENT_UNIT_UA)\
    ENUM_NAME_DEF(MB_CUBE_CURRENT_UNIT_MA)
typedef enum
{
    MB_CUBE_CURRENT_UNIT_E
}mb_cube_current_unit_e_t;

#define MB_DURA_UNIT_E \
    ENUM_NAME_DEF(MB_DURA_UNIT_MS) \
    ENUM_NAME_DEF(MB_DURA_UNIT_SEC) \
    ENUM_NAME_DEF(MB_DURA_UNIT_MIN)
typedef enum
{
    MB_DURA_UNIT_E
}mb_dura_unit_e_t;

typedef struct
{
    int log_level;

    float cool_dura_factor;
    int extra_cool_time_ms;
    int expo_prepare_time_ms, consec_rw_wait_ms;

    int cube_volt_kv_min;
    int cube_volt_kv_max;
    float cube_current_ma_min;
    float cube_current_ma_max;
    float dura_ms_min;
    float dura_ms_max;

    int mb_reconnect_wait_ms, mb_err_retry_wait_ms;
    int test_time_stat_grain_sec;
    int mb_one_cmd_round_time_ms;

    mb_cube_current_unit_e_t mb_cube_current_intf_unit;
    mb_dura_unit_e_t mb_dura_intf_unit, hidden_ui_mb_dura_unit;
}sys_configs_struct_t;

extern sys_configs_struct_t g_sys_configs_block;

bool fill_sys_configs(QString *);

#endif // SYSCONFIGS_H
