﻿#ifndef COMMON_TOOL_FUNC_H
#define COMMON_TOOL_FUNC_H

#include <QString>
#include <QNetworkInterface>

typedef enum
{
    IP_SET_TYPE_IPV4_DYNAMIC = 0,
    IP_SET_TYPE_IPV4_FIXED,
}ip_set_type_t;;

typedef enum
{
    /*every bit corresponds to one kind of intf.*/
    IP_INTF_OTHER = 0,
    IP_INTF_WIFI = 0x01,
    IP_INTF_ETHERNET = 0x02,
}ip_intf_type_t;

QString common_tool_get_curr_dt_str();
QString common_tool_get_curr_date_str();

/*intf_l contains the result. caller should pass in a pointer to a list.*/
void get_q_network_intf_by_type(ip_intf_type_t targ_l_intf_t, QList<QNetworkInterface> * intf_l,
                                bool is_up = true);
QString get_ip_addr_by_if_idx(int if_idx);
bool set_fixed_ip_address(QString ipaddr_str, QString addr_mask = "255.255.255.0", QString gw = "");
bool set_dynamic_ip();
bool set_host_ip_address(int if_idx, ip_set_type_t set_type,
                         QString ip_addr = "", QString ip_mask = "255.255.255.0", QString gw = "");

/*return the if idx*/
int set_host_wifi_or_eth_ip_addr(ip_set_type_t set_type, ip_intf_type_t intf_t = IP_INTF_WIFI,
                         QString ip_addr = "", QString ip_mask = "255.255.255.0", QString gw = "");

bool mkpth_if_not_exists(QString &pth_str);

#define DEF_SHUTDOWN_WAIT_TIME 3
/*return the shutdown command line.*/
QString shutdown_system(QString reason_str = "", int wait_time = DEF_SHUTDOWN_WAIT_TIME);

#define ROUNDUP_UINT16_TO_10(x) ((quint16)(((quint16)(((x) + 5) / 10)) * 10))
#define ARRAY_COUNT(a) (sizeof((a)) / sizeof((a)[0]))

typedef enum
{
    INT_DATA, FLOAT_DATA,
}common_data_type_enum_t;
class RangeChecker
{
public:
#define EDGE_ITEM(a) a
#define EDGE_LIST \
        EDGE_ITEM(EDGE_INCLUDED),\
        EDGE_ITEM(EDGE_EXCLUDED),\
        EDGE_ITEM(EDGE_INFINITE),\
        EDGE_ITEM(EDGE_COUNT),
    typedef enum
    {
        EDGE_LIST
    }range_edge_enum_t;
#undef EDGE_ITEM
static  const char* range_edge_strs[];

private:
    bool valid;
    double min, max;
    range_edge_enum_t low_edge, up_edge;
public:
    RangeChecker(double min = -1, double max = 1,
                 range_edge_enum_t low_edge = EDGE_INCLUDED, range_edge_enum_t up_edge = EDGE_INCLUDED);
public:
    bool range_check(double val);
    range_edge_enum_t range_low_edge();
    range_edge_enum_t range_up_edge();
    double range_min();
    double range_max();
    QString range_str(common_data_type_enum_t d_type);
};

#endif // COMMON_TOOL_FUNC_H
