#include <QSettings>

#include "sysconfigs/sysconfigs.h"

static const char* gs_sysconfigs_file_fpn = "configs/configs.ini";
static const char* gs_ini_grp_expo_ctrl = "expo_ctrl";
static const char* gs_ini_key_cool_dura_factor = "cool_dura_factor";

sys_configs_struct_t g_sys_configs_block;

static const float gs_def_cool_dura_factor = 30;

void fill_sys_configs()
{
    QSettings settings(gs_sysconfigs_file_fpn, QSettings::IniFormat);
    settings.beginGroup(gs_ini_grp_expo_ctrl);
    g_sys_configs_block.cool_dura_factor
            = settings.value(gs_ini_key_cool_dura_factor, gs_def_cool_dura_factor).toFloat();
    settings.endGroup();
}
