# X-exposer-tool
自动化曝光测试工具

## 单位说明
管电压：使用kv。

管电流：
- GUI：uA或mA。与modbus接口单位一致，由配置文件mb_cube_current_intf_unit项决定。无GUI修改接口。
- 软件内部：使用mA。
- modbus接口：参考GUI。

曝光时间：
- GUI：ms、s或min。GUI上用户可选，但同一版本只有2项可选。配置文件中的mb_dura_intf_unit项决定隐藏哪一项。
- 软件内部：使用ms。
- modbus接口：ms、s或min。配置文件mb_dura_intf_unit决定。

软件内部的range checker，也同样固定使用kv、mA和ms。

软件内部的对modbus回读数据是否超标的判断：TestResultJudge内实际是没有单位的。在目前的实现中没有问题：回读数据目前只有管电压、管电流和测距值，这三者在GUI和modbus接口的单位都是一样的，所以可以直接将modbus回读的数据与GUI设置的范围进行比较，不需要转换。

