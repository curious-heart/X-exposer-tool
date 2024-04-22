#ifndef TEST_PARAMS_STRUCT_H
#define TEST_PARAMS_STRUCT_H

#include <QtGlobal>
#include <QString>
#include <QVector>

typedef enum
{
    TEST_MODE_SINGLE = 0,
    TEST_MODE_REPEAT,
    TEST_MODE_TRAVERSE,
    TEST_MODE_CUST,
    TEXT_MODE_CNT
}test_mode_enum_t;

typedef struct
{
    int cube_volt_kv;
    float cube_current_ma, dura_ms;
}expo_param_triple_struct_t;

typedef struct
{
    int cube_volt_kv_start, cube_volt_kv_end, cube_volt_kv_step;
    float cube_current_ma_start, cube_current_ma_end, cube_current_ma_step;
    float expo_dura_ms_start, expo_dura_ms_end, expo_dura_ms_step;
}regular_expo_parms_struct_t;

typedef struct
{
    regular_expo_parms_struct_t regular_parms;
    QVector<expo_param_triple_struct_t> cust_params_arr;
}expo_base_params_struct_t;

typedef struct expo_params_struct
{
    bool cust;
    expo_base_params_struct_t expo_params;
    int expo_cnt;
    float expo_cool_dura_ms;
}expo_params_struct_t;

typedef struct
{
    QString oil_box_number_str, hv_ctrl_board_number_str;
    QString sw_ver_str, hw_ver_str;
}other_test_params_struct_t;

typedef struct
{
    bool valid;
    expo_params_struct_t expo_param_block;
    other_test_params_struct_t other_param_block;
}test_params_struct_t;

#endif // TEST_PARAMS_STRUCT_H
