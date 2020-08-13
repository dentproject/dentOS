/**************************************************************************//**
 *
 * @file
 * @brief arm64_wnc_qsa72_aom_a_48p Porting Macros.
 *
 * @addtogroup arm64_wnc_qsa72_aom_a_48p-porting
 * @{
 *
 *****************************************************************************/
#ifndef __ARM64_WNC_QSA72_AOM_A_48P_PORTING_H__
#define __ARM64_WNC_QSA72_AOM_A_48P_PORTING_H__


/* <auto.start.portingmacro(ALL).define> */
#if ARM64_WNC_QSA72_AOM_A_48P_CONFIG_PORTING_INCLUDE_STDLIB_HEADERS == 1
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <memory.h>
#endif

#ifndef ARM64_WNC_QSA72_AOM_A_48P_MALLOC
    #if defined(GLOBAL_MALLOC)
        #define ARM64_WNC_QSA72_AOM_A_48P_MALLOC GLOBAL_MALLOC
    #elif ARM64_WNC_QSA72_AOM_A_48P_CONFIG_PORTING_STDLIB == 1
        #define ARM64_WNC_QSA72_AOM_A_48P_MALLOC malloc
    #else
        #error The macro ARM64_WNC_QSA72_AOM_A_48P_MALLOC is required but cannot be defined.
    #endif
#endif

#ifndef ARM64_WNC_QSA72_AOM_A_48P_FREE
    #if defined(GLOBAL_FREE)
        #define ARM64_WNC_QSA72_AOM_A_48P_FREE GLOBAL_FREE
    #elif ARM64_WNC_QSA72_AOM_A_48P_CONFIG_PORTING_STDLIB == 1
        #define ARM64_WNC_QSA72_AOM_A_48P_FREE free
    #else
        #error The macro ARM64_WNC_QSA72_AOM_A_48P_FREE is required but cannot be defined.
    #endif
#endif

#ifndef ARM64_WNC_QSA72_AOM_A_48P_MEMSET
    #if defined(GLOBAL_MEMSET)
        #define ARM64_WNC_QSA72_AOM_A_48P_MEMSET GLOBAL_MEMSET
    #elif ARM64_WNC_QSA72_AOM_A_48P_CONFIG_PORTING_STDLIB == 1
        #define ARM64_WNC_QSA72_AOM_A_48P_MEMSET memset
    #else
        #error The macro ARM64_WNC_QSA72_AOM_A_48P_MEMSET is required but cannot be defined.
    #endif
#endif

#ifndef ARM64_WNC_QSA72_AOM_A_48P_MEMCPY
    #if defined(GLOBAL_MEMCPY)
        #define ARM64_WNC_QSA72_AOM_A_48P_MEMCPY GLOBAL_MEMCPY
    #elif ARM64_WNC_QSA72_AOM_A_48P_CONFIG_PORTING_STDLIB == 1
        #define ARM64_WNC_QSA72_AOM_A_48P_MEMCPY memcpy
    #else
        #error The macro ARM64_WNC_QSA72_AOM_A_48P_MEMCPY is required but cannot be defined.
    #endif
#endif

#ifndef ARM64_WNC_QSA72_AOM_A_48P_STRNCPY
    #if defined(GLOBAL_STRNCPY)
        #define ARM64_WNC_QSA72_AOM_A_48P_STRNCPY GLOBAL_STRNCPY
    #elif ARM64_WNC_QSA72_AOM_A_48P_CONFIG_PORTING_STDLIB == 1
        #define ARM64_WNC_QSA72_AOM_A_48P_STRNCPY strncpy
    #else
        #error The macro ARM64_WNC_QSA72_AOM_A_48P_STRNCPY is required but cannot be defined.
    #endif
#endif

#ifndef ARM64_WNC_QSA72_AOM_A_48P_VSNPRINTF
    #if defined(GLOBAL_VSNPRINTF)
        #define ARM64_WNC_QSA72_AOM_A_48P_VSNPRINTF GLOBAL_VSNPRINTF
    #elif ARM64_WNC_QSA72_AOM_A_48P_CONFIG_PORTING_STDLIB == 1
        #define ARM64_WNC_QSA72_AOM_A_48P_VSNPRINTF vsnprintf
    #else
        #error The macro ARM64_WNC_QSA72_AOM_A_48P_VSNPRINTF is required but cannot be defined.
    #endif
#endif

#ifndef ARM64_WNC_QSA72_AOM_A_48P_SNPRINTF
    #if defined(GLOBAL_SNPRINTF)
        #define ARM64_WNC_QSA72_AOM_A_48P_SNPRINTF GLOBAL_SNPRINTF
    #elif ARM64_WNC_QSA72_AOM_A_48P_CONFIG_PORTING_STDLIB == 1
        #define ARM64_WNC_QSA72_AOM_A_48P_SNPRINTF snprintf
    #else
        #error The macro ARM64_WNC_QSA72_AOM_A_48P_SNPRINTF is required but cannot be defined.
    #endif
#endif

#ifndef ARM64_WNC_QSA72_AOM_A_48P_STRLEN
    #if defined(GLOBAL_STRLEN)
        #define ARM64_WNC_QSA72_AOM_A_48P_STRLEN GLOBAL_STRLEN
    #elif ARM64_WNC_QSA72_AOM_A_48P_CONFIG_PORTING_STDLIB == 1
        #define ARM64_WNC_QSA72_AOM_A_48P_STRLEN strlen
    #else
        #error The macro ARM64_WNC_QSA72_AOM_A_48P_STRLEN is required but cannot be defined.
    #endif
#endif

/* <auto.end.portingmacro(ALL).define> */


#endif /* __ARM64_WNC_QSA72_AOM_A_48P_PORTING_H__ */
/* @} */
