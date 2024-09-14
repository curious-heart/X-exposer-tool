#ifndef SYSCONFIGS_H
#define SYSCONFIGS_H

typedef struct
{
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
}sys_configs_struct_t;

extern sys_configs_struct_t g_sys_configs_block;

void fill_sys_configs();

#endif // SYSCONFIGS_H
