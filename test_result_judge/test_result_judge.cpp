#include "logger/logger.h"
#include "test_result_judge.h"

static const char* gs_str_too_low = "过低";
static const char* gs_str_too_high= "过高";
static const char* gs_str_judge_range_invalid = "判定范围无效";
static const char* gs_str_cannot_judge = "无法判定有效性";

TestResultJudge::TestResultJudge()
{
}

TestResultJudge::~TestResultJudge()
{
    m_judge_list.clear();
    m_judge_result_strs.clear();
    m_result_display_items.clear();
}

#define RETURN_ON_INVALID_REG(reg_no, str) \
    if(!VALID_MB_REG_ADDR((reg_no))) \
    {\
        DIY_LOG(LOG_WARN, QString((str)).arg(get_hv_mb_reg_str(reg_no)));\
        return false;\
    }

bool TestResultJudge::add_judge_params(hv_mb_reg_e_t ref_reg_no, hv_mb_reg_e_t val_reg_no,
                                       judge_params_s_t &param)
{
    RETURN_ON_INVALID_REG(ref_reg_no, "ref_reg_no %1 for judge is invalid!");
    RETURN_ON_INVALID_REG(val_reg_no, "val_reg_no %1 for judge is invalid!");

    mb_reg_judge_map_s_t a_map;
    a_map.ref_reg_no = ref_reg_no;
    a_map.val_reg_no = val_reg_no;
    a_map.judge_params = param;
    m_judge_list.append(a_map);
    return true;
}

bool TestResultJudge::add_result_disp_items(hv_mb_reg_e_t reg_no)
{
    RETURN_ON_INVALID_REG(reg_no, "reg_no %1 for result_disp is invalid!");

    m_result_display_items.append(reg_no);
    return true;
}

bool TestResultJudge::add_result_disp_items(const hv_mb_reg_e_t *reg_no_arr, int cnt)
{
    if(!reg_no_arr) return false;

    for(int idx = 0; idx < cnt; ++idx)
    {
        RETURN_ON_INVALID_REG(reg_no_arr[idx], "reg_no %1 for result_disp is invalid!");
        m_result_display_items.append(reg_no_arr[idx]);
    }
    return true;
}

bool TestResultJudge::add_result_disp_items(const mb_reg_addr_list_t &arr_list)
{
    for(int idx = 0; idx < arr_list.count(); ++idx)
    {
        RETURN_ON_INVALID_REG(arr_list[idx], "reg_no %1 for result_disp is invalid!");
    }

    m_result_display_items.append(arr_list);
    return true;
}

void TestResultJudge::judge_mb_regs(const mb_reg_val_map_t &reg_val_map,
                   mb_reg_judge_result_list_t &bad_val_list, QString &result_disp_prefix_str)
{
    float val, ref_val, low_limit, up_limit;
    QString bad_str;
    const char* reg_cn_name_str = nullptr;
    judge_result_e_t e_result = JUDGE_RESULT_OK;

    for(int idx = 0; idx < m_judge_list.count(); ++ idx)
    {
        if(!reg_val_map.contains(m_judge_list[idx].val_reg_no))
        {
            DIY_LOG(LOG_WARN,
                    QString("the val_reg_no %1 in judge_list is not in input reg_val_map.")
                    .arg(get_hv_mb_reg_str(m_judge_list[idx].val_reg_no)));
            bad_str += ",";
            continue;
        }

        if(!m_judge_list[idx].judge_params.is_fixed_ref
                && !reg_val_map.contains(m_judge_list[idx].ref_reg_no))
        {
            DIY_LOG(LOG_WARN,
                    QString("the ref_reg_no %1 in judge_list is not in input reg_val_map.")
                    .arg(get_hv_mb_reg_str(m_judge_list[idx].ref_reg_no)));
            bad_str += ",";
            continue;
        }

        val = reg_val_map[m_judge_list[idx].val_reg_no];
        ref_val = m_judge_list[idx].judge_params.is_fixed_ref ?
                  m_judge_list[idx].judge_params.fixed_ref_val :
                  reg_val_map[m_judge_list[idx].ref_reg_no];

        low_limit = ref_val * (1 + m_judge_list[idx].judge_params.low_e_pct)
                            + m_judge_list[idx].judge_params.low_e_extra_val;
        up_limit = ref_val * (1 + m_judge_list[idx].judge_params.up_e_pct)
                            + m_judge_list[idx].judge_params.up_e_extra_val;
        reg_cn_name_str = get_hv_mb_reg_str(m_judge_list[idx].val_reg_no, CN_REG_NAME);

        if(low_limit > up_limit)
        {
            bad_str += QString("%1 %2(%3~%4). %5")
                    .arg(reg_cn_name_str, gs_str_judge_range_invalid,
                         QString::number(low_limit), QString::number(up_limit),
                         gs_str_cannot_judge);
            e_result = JUDGE_RESULT_UNKNOWN;
        }
        else if(val < low_limit)
        {
            bad_str += QString("%1%2(<%3),").arg(reg_cn_name_str,gs_str_too_low
                                            ,QString::number(low_limit));
            e_result = JUDGE_RESULT_TOO_LOW;
        }
        else if(val > up_limit)
        {
            bad_str += QString("%1%2(>%3),").arg(reg_cn_name_str, gs_str_too_high
                                            ,QString::number(up_limit));
            e_result = JUDGE_RESULT_TOO_HIGH;
        }
        else
        {
            /*this is a good value. check the next.*/
            bad_str += ",";
            continue;
        }

        /*record the bad value reg no.*/
        bad_val_list.append({m_judge_list[idx].val_reg_no, e_result});
    }

    if(bad_str.isEmpty())
    {
        /*No bad value.*/
        return;
    }

    /*construct the report string.*/
    QString ret_line_str;
    QString disp_str;
    ret_line_str += result_disp_prefix_str + ",";
    for(int d_idx = 0; d_idx < m_result_display_items.count(); ++d_idx)
    {
        if(!reg_val_map.contains(m_result_display_items[d_idx]))
        {
            DIY_LOG(LOG_WARN, QString("the reg_no %1 in result_display_items is"
                                      "not in reg_val_map")
                                    .arg(get_hv_mb_reg_str(m_result_display_items[d_idx])));
            disp_str = "";
        }
        else
        {
            disp_str = QString::number(reg_val_map[m_result_display_items[d_idx]]);
        }

        ret_line_str += disp_str + ",";
    }
    ret_line_str += bad_str;

    m_judge_result_strs.append(ret_line_str);
}

const QList<QString> & TestResultJudge::get_judge_result_strs()
{
    return m_judge_result_strs;
}
