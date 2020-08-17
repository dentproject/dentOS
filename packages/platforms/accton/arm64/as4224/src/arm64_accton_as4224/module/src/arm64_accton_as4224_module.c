/**************************************************************************//**
 *
 *
 *
 *****************************************************************************/
#include <arm64_accton_as4224/arm64_accton_as4224_config.h>

#include "arm64_accton_as4224_log.h"

static int
datatypes_init__(void)
{
#define ARM64_ACCTON_AS4224_ENUMERATION_ENTRY(_enum_name, _desc)     AIM_DATATYPE_MAP_REGISTER(_enum_name, _enum_name##_map, _desc,                               AIM_LOG_INTERNAL);
#include <arm64_accton_as4224/arm64_accton_as4224.x>
    return 0;
}

void __arm64_accton_as4224_module_init__(void)
{
    AIM_LOG_STRUCT_REGISTER();
    datatypes_init__();
}

int __onlp_platform_version__ = 1;