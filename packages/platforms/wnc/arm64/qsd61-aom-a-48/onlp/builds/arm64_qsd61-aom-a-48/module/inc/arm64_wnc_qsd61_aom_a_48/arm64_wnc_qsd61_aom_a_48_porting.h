/**************************************************************************//**
 *
 * @file
 * @brief arm64_wnc_qsd61_aom_a_48 Porting Macros.
 *
 * @addtogroup arm64_wnc_qsd61_aom_a_48-porting
 * @{
 *
 *****************************************************************************/
#ifndef __ARM64_WNC_QSD61_AOM_A_48_PORTING_H__
#define __ARM64_WNC_QSD61_AOM_A_48_PORTING_H__


/* <auto.start.portingmacro(ALL).define> */
#if ARM64_WNC_QSD61_AOM_A_48_CONFIG_PORTING_INCLUDE_STDLIB_HEADERS == 1
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <memory.h>
#endif

#ifndef ARM64_WNC_QSD61_AOM_A_48_MALLOC
    #if defined(GLOBAL_MALLOC)
        #define ARM64_WNC_QSD61_AOM_A_48_MALLOC GLOBAL_MALLOC
    #elif ARM64_WNC_QSD61_AOM_A_48_CONFIG_PORTING_STDLIB == 1
        #define ARM64_WNC_QSD61_AOM_A_48_MALLOC malloc
    #else
        #error The macro ARM64_WNC_QSD61_AOM_A_48_MALLOC is required but cannot be defined.
    #endif
#endif

#ifndef ARM64_WNC_QSD61_AOM_A_48_FREE
    #if defined(GLOBAL_FREE)
        #define ARM64_WNC_QSD61_AOM_A_48_FREE GLOBAL_FREE
    #elif ARM64_WNC_QSD61_AOM_A_48_CONFIG_PORTING_STDLIB == 1
        #define ARM64_WNC_QSD61_AOM_A_48_FREE free
    #else
        #error The macro ARM64_WNC_QSD61_AOM_A_48_FREE is required but cannot be defined.
    #endif
#endif

#ifndef ARM64_WNC_QSD61_AOM_A_48_MEMSET
    #if defined(GLOBAL_MEMSET)
        #define ARM64_WNC_QSD61_AOM_A_48_MEMSET GLOBAL_MEMSET
    #elif ARM64_WNC_QSD61_AOM_A_48_CONFIG_PORTING_STDLIB == 1
        #define ARM64_WNC_QSD61_AOM_A_48_MEMSET memset
    #else
        #error The macro ARM64_WNC_QSD61_AOM_A_48_MEMSET is required but cannot be defined.
    #endif
#endif

#ifndef ARM64_WNC_QSD61_AOM_A_48_MEMCPY
    #if defined(GLOBAL_MEMCPY)
        #define ARM64_WNC_QSD61_AOM_A_48_MEMCPY GLOBAL_MEMCPY
    #elif ARM64_WNC_QSD61_AOM_A_48_CONFIG_PORTING_STDLIB == 1
        #define ARM64_WNC_QSD61_AOM_A_48_MEMCPY memcpy
    #else
        #error The macro ARM64_WNC_QSD61_AOM_A_48_MEMCPY is required but cannot be defined.
    #endif
#endif

#ifndef ARM64_WNC_QSD61_AOM_A_48_STRNCPY
    #if defined(GLOBAL_STRNCPY)
        #define ARM64_WNC_QSD61_AOM_A_48_STRNCPY GLOBAL_STRNCPY
    #elif ARM64_WNC_QSD61_AOM_A_48_CONFIG_PORTING_STDLIB == 1
        #define ARM64_WNC_QSD61_AOM_A_48_STRNCPY strncpy
    #else
        #error The macro ARM64_WNC_QSD61_AOM_A_48_STRNCPY is required but cannot be defined.
    #endif
#endif

#ifndef ARM64_WNC_QSD61_AOM_A_48_VSNPRINTF
    #if defined(GLOBAL_VSNPRINTF)
        #define ARM64_WNC_QSD61_AOM_A_48_VSNPRINTF GLOBAL_VSNPRINTF
    #elif ARM64_WNC_QSD61_AOM_A_48_CONFIG_PORTING_STDLIB == 1
        #define ARM64_WNC_QSD61_AOM_A_48_VSNPRINTF vsnprintf
    #else
        #error The macro ARM64_WNC_QSD61_AOM_A_48_VSNPRINTF is required but cannot be defined.
    #endif
#endif

#ifndef ARM64_WNC_QSD61_AOM_A_48_SNPRINTF
    #if defined(GLOBAL_SNPRINTF)
        #define ARM64_WNC_QSD61_AOM_A_48_SNPRINTF GLOBAL_SNPRINTF
    #elif ARM64_WNC_QSD61_AOM_A_48_CONFIG_PORTING_STDLIB == 1
        #define ARM64_WNC_QSD61_AOM_A_48_SNPRINTF snprintf
    #else
        #error The macro ARM64_WNC_QSD61_AOM_A_48_SNPRINTF is required but cannot be defined.
    #endif
#endif

#ifndef ARM64_WNC_QSD61_AOM_A_48_STRLEN
    #if defined(GLOBAL_STRLEN)
        #define ARM64_WNC_QSD61_AOM_A_48_STRLEN GLOBAL_STRLEN
    #elif ARM64_WNC_QSD61_AOM_A_48_CONFIG_PORTING_STDLIB == 1
        #define ARM64_WNC_QSD61_AOM_A_48_STRLEN strlen
    #else
        #error The macro ARM64_WNC_QSD61_AOM_A_48_STRLEN is required but cannot be defined.
    #endif
#endif

/* <auto.end.portingmacro(ALL).define> */


#endif /* __ARM64_WNC_QSD61_AOM_A_48_PORTING_H__ */
/* @} */
