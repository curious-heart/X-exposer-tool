#include "logger/logger.h"
#include "modbus_regs/modbus_regs.h"

#undef C

#define C(a) #a
static const char* gs_hv_mb_reg_str[] = MB_REG_ENUM;

static const char* gs_hv_mb_reg_cnstr[] =
{
   /* 0  */ "HSV",
   /* 1  */ "ota版本",
   /* 2  */ "波特率",
   /* 3  */ "设备地址",
   /* 4  */ "设备状态字",
   /* 5  */ "设置管电压",
   /* 6  */ "设置管电流",
   /* 7  */ "设置管曝光时间",
   /* 8  */ "读取管电压",
   /* 9  */ "读取管电流",
   /* 10 */ "范围指示状态",
   /* 11 */ "曝光状态",
   /* 12 */ "范围指示",
   /* 13 */ "曝光启动申请",
   /* 14 */ "电池电量百分比",
   /* 15 */ "电池电压",
   /* 16 */ "油盒温度",
   /* 17 */ "关机请求",
   /* 18 */ "校准位置",
   /* 19 */ "校准值",
   /* 20 */ "充能状态",
   /* 21 */ "曝光次数",
   /* MAX_HV_NORMAL_MB_REG_NUM */ "分割：普通reg数量",
   /* EXT_MB_REG_START_FLAG = 100 */ "分割:扩展reg开始",

   /* 101 */ "扩展reg:档位调节按键",
   /* 102 */ "扩展reg:充电器插入",
   /* 103 */ "扩展reg:DAP高word",
   /* 104 */ "扩展reg:DAP低word",
   /* 105 */ "扩展reg:测距结果",

   /* HV_MB_REG_END_FLAG */ "reg结束标志",

};
const char* get_hv_mb_reg_str(hv_mb_reg_e_t reg_addr, mbreg_name_lang_enmu_t lang)
{
    const char** arr = (EN_REG_NAME_WITH_NO == lang) ? gs_hv_mb_reg_str : gs_hv_mb_reg_cnstr;
    if(VALID_MB_REG_ADDR(reg_addr))
    {
        return arr[MB_REG_ADDR2IDX(reg_addr)];
    }
    else
    {
        DIY_LOG(LOG_ERROR, "register address %d is invalid.");
        return nullptr;
    }
}
