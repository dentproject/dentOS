/**************************************************************************//**
 *
 *
 *
 *****************************************************************************/
#include <arm64_wnc_qsd61_aom_a_48/arm64_wnc_qsd61_aom_a_48_config.h>

#include "arm64_wnc_qsd61_aom_a_48_log.h"

static int
datatypes_init__(void)
{
#define ARM64_WNC_QSD61_AOM_A_48_ENUMERATION_ENTRY(_enum_name, _desc)     AIM_DATATYPE_MAP_REGISTER(_enum_name, _enum_name##_map, _desc,                               AIM_LOG_INTERNAL);
#include <arm64_wnc_qsd61_aom_a_48/arm64_wnc_qsd61_aom_a_48.x>
    return 0;
}

void __arm64_wnc_qsd61_aom_a_48_module_init__(void)
{
    AIM_LOG_STRUCT_REGISTER();
    datatypes_init__();
}

int __onlp_platform_version__ = 1;
