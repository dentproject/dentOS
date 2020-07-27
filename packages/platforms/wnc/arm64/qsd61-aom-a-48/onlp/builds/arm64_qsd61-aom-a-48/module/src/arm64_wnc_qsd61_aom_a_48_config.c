/**************************************************************************//**
 *
 *
 *
 *****************************************************************************/
#include <arm64_wnc_qsd61_aom_a_48/arm64_wnc_qsd61_aom_a_48_config.h>

/* <auto.start.cdefs(arm64_wnc_qsd61_aom_a_48_CONFIG_HEADER).source> */
#define __arm64_wnc_qsd61_aom_a_48_config_STRINGIFY_NAME(_x) #_x
#define __arm64_wnc_qsd61_aom_a_48_config_STRINGIFY_VALUE(_x) __arm64_wnc_qsd61_aom_a_48_config_STRINGIFY_NAME(_x)
arm64_wnc_qsd61_aom_a_48_config_settings_t arm64_wnc_qsd61_aom_a_48_config_settings[] =
{
#ifdef ARM64_WNC_QSD61_AOM_A_48_CONFIG_INCLUDE_LOGGING
    { __arm64_wnc_qsd61_aom_a_48_config_STRINGIFY_NAME(ARM64_WNC_QSD61_AOM_A_48_CONFIG_INCLUDE_LOGGING), __arm64_wnc_qsd61_aom_a_48_config_STRINGIFY_VALUE(ARM64_WNC_QSD61_AOM_A_48_CONFIG_INCLUDE_LOGGING) },
#else
{ ARM64_WNC_QSD61_AOM_A_48_CONFIG_INCLUDE_LOGGING(__arm64_wnc_qsd61_aom_a_48_config_STRINGIFY_NAME), "__undefined__" },
#endif
#ifdef ARM64_WNC_QSD61_AOM_A_48_CONFIG_LOG_OPTIONS_DEFAULT
    { __arm64_wnc_qsd61_aom_a_48_config_STRINGIFY_NAME(ARM64_WNC_QSD61_AOM_A_48_CONFIG_LOG_OPTIONS_DEFAULT), __arm64_wnc_qsd61_aom_a_48_config_STRINGIFY_VALUE(ARM64_WNC_QSD61_AOM_A_48_CONFIG_LOG_OPTIONS_DEFAULT) },
#else
{ ARM64_WNC_QSD61_AOM_A_48_CONFIG_LOG_OPTIONS_DEFAULT(__arm64_wnc_qsd61_aom_a_48_config_STRINGIFY_NAME), "__undefined__" },
#endif
#ifdef ARM64_WNC_QSD61_AOM_A_48_CONFIG_LOG_BITS_DEFAULT
    { __arm64_wnc_qsd61_aom_a_48_config_STRINGIFY_NAME(ARM64_WNC_QSD61_AOM_A_48_CONFIG_LOG_BITS_DEFAULT), __arm64_wnc_qsd61_aom_a_48_config_STRINGIFY_VALUE(ARM64_WNC_QSD61_AOM_A_48_CONFIG_LOG_BITS_DEFAULT) },
#else
{ ARM64_WNC_QSD61_AOM_A_48_CONFIG_LOG_BITS_DEFAULT(__arm64_wnc_qsd61_aom_a_48_config_STRINGIFY_NAME), "__undefined__" },
#endif
#ifdef ARM64_WNC_QSD61_AOM_A_48_CONFIG_LOG_CUSTOM_BITS_DEFAULT
    { __arm64_wnc_qsd61_aom_a_48_config_STRINGIFY_NAME(ARM64_WNC_QSD61_AOM_A_48_CONFIG_LOG_CUSTOM_BITS_DEFAULT), __arm64_wnc_qsd61_aom_a_48_config_STRINGIFY_VALUE(ARM64_WNC_QSD61_AOM_A_48_CONFIG_LOG_CUSTOM_BITS_DEFAULT) },
#else
{ ARM64_WNC_QSD61_AOM_A_48_CONFIG_LOG_CUSTOM_BITS_DEFAULT(__arm64_wnc_qsd61_aom_a_48_config_STRINGIFY_NAME), "__undefined__" },
#endif
#ifdef ARM64_WNC_QSD61_AOM_A_48_CONFIG_PORTING_STDLIB
    { __arm64_wnc_qsd61_aom_a_48_config_STRINGIFY_NAME(ARM64_WNC_QSD61_AOM_A_48_CONFIG_PORTING_STDLIB), __arm64_wnc_qsd61_aom_a_48_config_STRINGIFY_VALUE(ARM64_WNC_QSD61_AOM_A_48_CONFIG_PORTING_STDLIB) },
#else
{ ARM64_WNC_QSD61_AOM_A_48_CONFIG_PORTING_STDLIB(__arm64_wnc_qsd61_aom_a_48_config_STRINGIFY_NAME), "__undefined__" },
#endif
#ifdef ARM64_WNC_QSD61_AOM_A_48_CONFIG_PORTING_INCLUDE_STDLIB_HEADERS
    { __arm64_wnc_qsd61_aom_a_48_config_STRINGIFY_NAME(ARM64_WNC_QSD61_AOM_A_48_CONFIG_PORTING_INCLUDE_STDLIB_HEADERS), __arm64_wnc_qsd61_aom_a_48_config_STRINGIFY_VALUE(ARM64_WNC_QSD61_AOM_A_48_CONFIG_PORTING_INCLUDE_STDLIB_HEADERS) },
#else
{ ARM64_WNC_QSD61_AOM_A_48_CONFIG_PORTING_INCLUDE_STDLIB_HEADERS(__arm64_wnc_qsd61_aom_a_48_config_STRINGIFY_NAME), "__undefined__" },
#endif
#ifdef ARM64_WNC_QSD61_AOM_A_48_CONFIG_INCLUDE_UCLI
    { __arm64_wnc_qsd61_aom_a_48_config_STRINGIFY_NAME(ARM64_WNC_QSD61_AOM_A_48_CONFIG_INCLUDE_UCLI), __arm64_wnc_qsd61_aom_a_48_config_STRINGIFY_VALUE(ARM64_WNC_QSD61_AOM_A_48_CONFIG_INCLUDE_UCLI) },
#else
{ ARM64_WNC_QSD61_AOM_A_48_CONFIG_INCLUDE_UCLI(__arm64_wnc_qsd61_aom_a_48_config_STRINGIFY_NAME), "__undefined__" },
#endif
#ifdef ARM64_WNC_QSD61_AOM_A_48_CONFIG_INCLUDE_DEFAULT_FAN_DIRECTION
    { __arm64_wnc_qsd61_aom_a_48_config_STRINGIFY_NAME(ARM64_WNC_QSD61_AOM_A_48_CONFIG_INCLUDE_DEFAULT_FAN_DIRECTION), __arm64_wnc_qsd61_aom_a_48_config_STRINGIFY_VALUE(ARM64_WNC_QSD61_AOM_A_48_CONFIG_INCLUDE_DEFAULT_FAN_DIRECTION) },
#else
{ ARM64_WNC_QSD61_AOM_A_48_CONFIG_INCLUDE_DEFAULT_FAN_DIRECTION(__arm64_wnc_qsd61_aom_a_48_config_STRINGIFY_NAME), "__undefined__" },
#endif
    { NULL, NULL }
};
#undef __arm64_wnc_qsd61_aom_a_48_config_STRINGIFY_VALUE
#undef __arm64_wnc_qsd61_aom_a_48_config_STRINGIFY_NAME

const char*
arm64_wnc_qsd61_aom_a_48_config_lookup(const char* setting)
{
    int i;
    for(i = 0; arm64_wnc_qsd61_aom_a_48_config_settings[i].name; i++) {
        if(strcmp(arm64_wnc_qsd61_aom_a_48_config_settings[i].name, setting)) {
            return arm64_wnc_qsd61_aom_a_48_config_settings[i].value;
        }
    }
    return NULL;
}

int
arm64_wnc_qsd61_aom_a_48_config_show(struct aim_pvs_s* pvs)
{
    int i;
    for(i = 0; arm64_wnc_qsd61_aom_a_48_config_settings[i].name; i++) {
        aim_printf(pvs, "%s = %s\n", arm64_wnc_qsd61_aom_a_48_config_settings[i].name, arm64_wnc_qsd61_aom_a_48_config_settings[i].value);
    }
    return i;
}

/* <auto.end.cdefs(ARM64_WNC_QSD61_AOM_A_48_CONFIG_HEADER).source> */

