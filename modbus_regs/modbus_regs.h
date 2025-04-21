#ifndef MODBUS_REGS_H
#define MODBUS_REGS_H

#include <QObject>
#include <QMap>
#include <QList>

#define MB_REG_ENUM \
{\
    C(HSV = 0),                            /*软硬件版本*/\
    C(OTA = 1),                            /*OTA升级*/\
    C(BaudRate = 2),                       /*波特率*/\
    C(ServerAddress = 3),                  /*设备地址*/\
    C(State = 4),                          /*状态*/\
    C(VoltSet = 5),                        /*5管电压设置值*/\
    C(FilamentSet = 6),                    /*6 管电流设置值*/\
    C(ExposureTime = 7),                   /*曝光时间*/\
    C(Voltmeter = 8),                      /*管电压读出值*/\
    C(Ammeter = 9),                        /*管电流读出值*/\
    C(RangeIndicationStatus = 10),         /*范围指示状态*/\
    C(ExposureStatus = 11),                /*曝光状态*/\
    C(RangeIndicationStart = 12),          /*范围指示启动*/\
    C(ExposureStart = 13),                 /*曝光启动*/\
    C(BatteryLevel = 14),                  /*电池电量*/\
    C(BatteryVoltmeter = 15),              /*电池电压*/\
    C(OilBoxTemperature = 16),             /*电池电压高位*/\
    C(Poweroff = 17),                      /*关机请求*/\
    C(Fixpos = 18),                        /*校准定义*/\
    C(Fixval = 19),                        /*校准值*/\
    C(Workstatus = 20),                    /*充能状态*/\
    C(exposureCount = 21),                 /*曝光次数*/\
\
    C(MAX_HV_NORMAL_MB_REG_NUM),\
\
    C(EXT_MB_REG_START_FLAG = 100), /*extend register start flag.*/\
    /*Below are extend register, that is, they are processed internally by server and not passed to hv controller.*/ \
    C(EXT_MB_REG_DOSE_ADJ = 101),                       /*+/- key event*/\
    C(EXT_MB_REG_CHARGER = 102),                       /*charger plug in/pull out*/\
    C(EXT_MB_REG_DAP_HP = 103),                       /*High part of a float of DAP(Dose Area Product), big endian.*/\
    C(EXT_MB_REG_DAP_LP = 104),                       /*Low part of a float of DAP, big endian.*/\
    C(EXT_MB_REG_DISTANCE = 105),                       /*distance: TOF test result.*/\
\
    C(HV_MB_REG_END_FLAG), /*register end flag.*/\
}
#undef C
#define C(a) a
typedef enum MB_REG_ENUM hv_mb_reg_e_t;
#define START_EXPO_DATA 2
#define START_EXPO_DATA_COOL_HV 3
#define START_EXPO_DATA_ONLY_COIL 4
#define START_EXPO_DATA_DECOUPLE 5

#define NORMAL_MB_REG_ADDR(addr) (HSV <= (addr) && (addr) < MAX_HV_NORMAL_MB_REG_NUM)
#define EXTEND_MB_REG_ADDR(addr) (EXT_MB_REG_START_FLAG < (addr) && (addr) < HV_MB_REG_END_FLAG)
#define VALID_MB_REG_ADDR(addr) (NORMAL_MB_REG_ADDR(addr) || EXTEND_MB_REG_ADDR(addr))
#define NORMAL_MB_REG_AND_CNT(addr, cnt) (NORMAL_MB_REG_ADDR(addr) && NORMAL_MB_REG_ADDR((addr) + (cnt) - 1))
#define EXTEND_MB_REG_AND_CNT(addr, cnt) (EXTEND_MB_REG_ADDR(addr) && EXTEND_MB_REG_ADDR((addr) + (cnt) - 1))
#define VALID_MB_REG_AND_CNT(addr, cnt) (VALID_MB_REG_ADDR(addr) && VALID_MB_REG_ADDR((addr) + (cnt) - 1))

/*those registers that need to communicate with dsp.*/
#define MB_REG_COMM_DSP(addr, cnt) (NORMAL_MB_REG_AND_CNT(addr, cnt) || (EXT_MB_REG_DOSE_ADJ == addr))

/* The number of registers, including the two flag: MAX_HV_NORMAL_MB_REG_NUM and EXT_MB_REG_START_FLAG,
 * but not including the HV_MB_REG_END_FLAG.
 */
#define ALL_MB_REG_NUM (MAX_HV_NORMAL_MB_REG_NUM + (HV_MB_REG_END_FLAG - EXT_MB_REG_START_FLAG))

/* The extend reg addr to its order idx.*/
#define EXTEND_MB_REG_ADDR2IDX(ext_reg_addr) (MAX_HV_NORMAL_MB_REG_NUM + 1 + ((ext_reg_addr) - EXT_MB_REG_START_FLAG))
/* The reg addr to its order idx. refer to its use in function get_hv_mb_reg_str.*/
#define MB_REG_ADDR2IDX(reg_addr) (NORMAL_MB_REG_ADDR(reg_addr) ? (reg_addr) : EXTEND_MB_REG_ADDR2IDX(reg_addr))


typedef enum
{
    EN_REG_NAME_WITH_NO,
    CN_REG_NAME,
}mbreg_name_lang_enmu_t;
const char* get_hv_mb_reg_str(hv_mb_reg_e_t reg_addr,
                              mbreg_name_lang_enmu_t lang = EN_REG_NAME_WITH_NO);

typedef QMap<hv_mb_reg_e_t, quint16> mb_reg_val_map_t;
Q_DECLARE_METATYPE(mb_reg_val_map_t)

typedef QList<hv_mb_reg_e_t> mb_reg_addr_list_t;
Q_DECLARE_METATYPE(mb_reg_addr_list_t)

typedef enum
{
    EXPOSURE_ST_IDLE = 0,
    EXPOSURE_ST_WARNING = 1,
    EXPOSURE_ST_DELAY = 2,
    EXPOSURE_ST_WARMUP = 3,
    EXPOSURE_ST_EXPOSURING = 4,
    EXPOSURE_ST_ENDING = 5,
    EXPOSURE_ST_COOLING = 6,
}mb_reg_exposure_st_e_t;

#endif // MODBUS_REGS_H
