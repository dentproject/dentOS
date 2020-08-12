/**************************************************************************//**
 *
 *
 *
 *****************************************************************************/
#include <arm64_wnc_qsa72_aom_a_48p/arm64_wnc_qsa72_aom_a_48p_config.h>

#include "arm64_wnc_qsa72_aom_a_48p_log.h"

static int
datatypes_init__(void)
{
#define ARM64_WNC_QSA72_AOM_A_48P_ENUMERATION_ENTRY(_enum_name, _desc)     AIM_DATATYPE_MAP_REGISTER(_enum_name, _enum_name##_map, _desc,                               AIM_LOG_INTERNAL);
#include <arm64_wnc_qsa72_aom_a_48p/arm64_wnc_qsa72_aom_a_48p.x>
    return 0;
}

void __arm64_wnc_qsa72_aom_a_48p_module_init__(void)
{
    AIM_LOG_STRUCT_REGISTER();
    datatypes_init__();
}

int __onlp_platform_version__ = 1;
