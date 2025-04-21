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
    TEST_MODE_CUST1_TRIPLES,
    TEST_MODE_CUST2_DISCRETE,
    TEXT_MODE_CNT
}test_mode_enum_t;

typedef enum
{
    TEST_CONTENT_NORMAL = 0,
    TEST_CONTENT_COOL_HV,
    TEST_CONTENT_ONLY_COIL,
    TEST_CONTENT_DECOUPLE,
}test_content_enum_t;

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
    bool fixed_cool_dura;
    float expo_cool_dura_ms, expo_cool_dura_factor;
    float ui_to_sw_current_factor, ui_to_sw_dura_factor;
    float sw_to_mb_current_factor, sw_to_mb_dura_factor;
}expo_params_struct_t;

typedef struct
{
    QString oil_box_number_str, hv_ctrl_board_number_str;
    QString sw_ver_str, hw_ver_str;
    bool read_dist;
}other_test_params_struct_t;

typedef struct
{
    bool valid;
    test_mode_enum_t test_mode;
    test_content_enum_t test_content;
    expo_params_struct_t expo_param_block;
    other_test_params_struct_t other_param_block;
    QString info_str;
}test_params_struct_t;

#endif // TEST_PARAMS_STRUCT_H
