#ifndef TESTRESULTJUDGE_H
#define TESTRESULTJUDGE_H

#include <QObject>
#include <QMap>
#include <QList>

#include "modbus_regs/modbus_regs.h"

/*This class is used to test the modbus result read from hv generator.*/

/*usage:
 * if "is_fixed_ref" is true, input value is judged according to "fixed_ref_val" with the
 * edge parameters; otherwise, input value is judged according to data read from hv generator
 * (and "fixed_ref_val" is not used).
 * Currently, only tof-distance judge uses fixed_ref_val.
 *
 * Say the value to be judged is V, and the referenced value is R, the judge rule is:
 *
 * low_edge_value = R * (1 + low_e_pct) + low_e_extra_val
 * up_edge_value = R * (1 + up_e_pct) + up_e_extra_val
 * judge_result = (low_edge_value <= V <= up_edge_value)
*/
typedef struct
{
    float low_e_pct, up_e_pct; //e.g. -10% (-0.1), 5% (0.05)
    float low_e_extra_val, up_e_extra_val; //e.g. -1, 10
    bool is_fixed_ref;
    float fixed_ref_val;
}judge_params_s_t;

typedef struct
{
    hv_mb_reg_e_t ref_reg_no, val_reg_no;
    judge_params_s_t judge_params;
}mb_reg_judge_map_s_t;

typedef QList<mb_reg_judge_map_s_t> mb_regs_judge_params_list_t;

typedef enum
{
    JUDGE_RESULT_UNKNOWN = -1,
    JUDGE_RESULT_OK = 0,
    JUDGE_RESULT_TOO_LOW,
    JUDGE_RESULT_TOO_HIGH,
    JUDGE_RESULT_REF, //the data is ref data, no need to judge.
}judge_result_e_t;
typedef struct
{
    hv_mb_reg_e_t reg_no;
    judge_result_e_t judge_result;
}mb_reg_judge_result_s_t;
typedef QList<mb_reg_judge_result_s_t> mb_reg_judge_result_list_t;
typedef QMap<hv_mb_reg_e_t, judge_result_e_t> mb_reg_judge_result_map_t;

class TestResultJudge : public QObject
{
    Q_OBJECT
public:
    TestResultJudge();
    ~TestResultJudge();
    bool add_judge_params(hv_mb_reg_e_t ref_reg_no, hv_mb_reg_e_t val_reg_no,
                          judge_params_s_t &param);
    bool add_result_disp_items(hv_mb_reg_e_t reg_no);
    bool add_result_disp_items(const hv_mb_reg_e_t *reg_no_arr, int cnt);
    bool add_result_disp_items(const mb_reg_addr_list_t &arr_list);

    /* reg_val_map: input. the reg values to be judged.
     * bad_val_list: output. the reg numbers whose val can't pass the judge.
     * result_disp_prefix_str: in. for judge result display.
     */
    void judge_mb_regs(const mb_reg_val_map_t &reg_val_map,
                   mb_reg_judge_result_list_t &bad_val_list, QString &result_disp_prefix_str);

    const QStringList & get_judge_result_strs();
    void get_result_disp_header_str(QString &header_str);
    void clear_judge_resut_strs();

private:
    mb_regs_judge_params_list_t m_judge_list;
    mb_reg_addr_list_t m_result_display_items;
    QStringList m_judge_result_strs;
};

#endif // TESTRESULTJUDGE_H
