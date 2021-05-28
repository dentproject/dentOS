/**************************************************************************//**
 *
 *
 *
 *****************************************************************************/
#include <arm64_accton_as4564_26p/arm64_accton_as4564_26p_config.h>

#include "arm64_accton_as4564_26p_log.h"

static int
datatypes_init__(void)
{
#define ARM64_ACCTON_AS4564_26P_ENUMERATION_ENTRY(_enum_name, _desc)     AIM_DATATYPE_MAP_REGISTER(_enum_name, _enum_name##_map, _desc,                               AIM_LOG_INTERNAL);
#include <arm64_accton_as4564_26p/arm64_accton_as4564_26p.x>
    return 0;
}

void __arm64_accton_as4564_26p_module_init__(void)
{
    AIM_LOG_STRUCT_REGISTER();
    datatypes_init__();
}

int __onlp_platform_version__ = 1;