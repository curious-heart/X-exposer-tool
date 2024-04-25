#include "logger/logger.h"
#include "modbus_regs/modbus_regs.h"

#undef C

#define C(a) #a
static const char* gs_hv_mb_reg_str[] = MB_REG_ENUM;

const char* get_hv_mb_reg_str(hv_mb_reg_e_t reg_addr)
{
    if(VALID_MB_REG_ADDR(reg_addr))
    {
        return gs_hv_mb_reg_str[reg_addr];
    }
    else
    {
        DIY_LOG(LOG_ERROR, "register address %d is invalid.");
        return NULL;
    }
}
