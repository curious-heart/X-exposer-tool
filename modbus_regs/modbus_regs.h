#ifndef MODBUS_REGS_H
#define MODBUS_REGS_H

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
    C(MB_REG_COUNT)\
}
#undef C
#define C(a) a
typedef enum MB_REG_ENUM hv_mb_reg_e_t;

#define VALID_MB_REG_ADDR(addr) (HSV <= (addr) && (addr) < MB_REG_COUNT)

const char* get_hv_mb_reg_str(hv_mb_reg_e_t reg_addr);

#endif // MODBUS_REGS_H
