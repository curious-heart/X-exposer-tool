#ifndef SYSCONFIGS_H
#define SYSCONFIGS_H

typedef struct
{
    float cool_dura_factor;
}sys_configs_struct_t;

extern sys_configs_struct_t g_sys_configs_block;

void fill_sys_configs();

#endif // SYSCONFIGS_H
