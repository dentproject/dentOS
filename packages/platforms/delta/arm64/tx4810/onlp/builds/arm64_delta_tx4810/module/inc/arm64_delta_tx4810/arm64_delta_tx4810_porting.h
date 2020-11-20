/************************************************************
 * <bsn.cl fy=2016 v=onl>
 *
 *        Copyright 2020 Delta Networks, Inc.
 *
 *
 * Unless required by applicable law or agreed to in writing,
 * software distributed under the License is distributed on an
 * "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND,
 * either express or implied. See the License for the specific
 * language governing permissions and limitations under the
 * License.
 *
 * </bsn.cl>
 ************************************************************
 *
 *
 *
 ***********************************************************/

/********************************************************//**
 *
 * @file
 * @brief arm64_delta_tx4810 Porting Macros.
 *
 * @addtogroup arm64_delta_tx4810-porting
 * @{
 *
 ***********************************************************/
#ifndef __ARM64_DELTA_TX4810_PORTING_H__
#define __ARM64_DELTA_TX4810_PORTING_H__


/* <auto.start.portingmacro(ALL).define> */
#if ARM64_DELTA_TX4810_CONFIG_PORTING_INCLUDE_STDLIB_HEADERS == 1
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <memory.h>
#endif

#ifndef ARM64_DELTA_TX4810_MALLOC
    #if defined(GLOBAL_MALLOC)
        #define ARM64_DELTA_TX4810_MALLOC GLOBAL_MALLOC
    #elif ARM64_DELTA_TX4810_CONFIG_PORTING_STDLIB == 1
        #define ARM64_DELTA_TX4810_MALLOC malloc
    #else
        #error The macro ARM64_DELTA_TX4810_MALLOC is required but cannot be defined.
    #endif
#endif

#ifndef ARM64_DELTA_TX4810_FREE
    #if defined(GLOBAL_FREE)
        #define ARM64_DELTA_TX4810_FREE GLOBAL_FREE
    #elif ARM64_DELTA_TX4810_CONFIG_PORTING_STDLIB == 1
        #define ARM64_DELTA_TX4810_FREE free
    #else
        #error The macro ARM64_DELTA_TX4810_FREE is required but cannot be defined.
    #endif
#endif

#ifndef ARM64_DELTA_TX4810_MEMSET
    #if defined(GLOBAL_MEMSET)
        #define ARM64_DELTA_TX4810_MEMSET GLOBAL_MEMSET
    #elif ARM64_DELTA_TX4810_CONFIG_PORTING_STDLIB == 1
        #define ARM64_DELTA_TX4810_MEMSET memset
    #else
        #error The macro ARM64_DELTA_TX4810_MEMSET is required but cannot be defined.
    #endif
#endif

#ifndef ARM64_DELTA_TX4810_MEMCPY
    #if defined(GLOBAL_MEMCPY)
        #define ARM64_DELTA_TX4810_MEMCPY GLOBAL_MEMCPY
    #elif ARM64_DELTA_TX4810_CONFIG_PORTING_STDLIB == 1
        #define ARM64_DELTA_TX4810_MEMCPY memcpy
    #else
        #error The macro ARM64_DELTA_TX4810_MEMCPY is required but cannot be defined.
    #endif
#endif

#ifndef ARM64_DELTA_TX4810_VSNPRINTF
    #if defined(GLOBAL_VSNPRINTF)
        #define ARM64_DELTA_TX4810_VSNPRINTF GLOBAL_VSNPRINTF
    #elif ARM64_DELTA_TX4810_CONFIG_PORTING_STDLIB == 1
        #define ARM64_DELTA_TX4810_VSNPRINTF vsnprintf
    #else
        #error The macro ARM64_DELTA_TX4810_VSNPRINTF is required but cannot be defined.
    #endif
#endif

#ifndef ARM64_DELTA_TX4810_SNPRINTF
    #if defined(GLOBAL_SNPRINTF)
        #define ARM64_DELTA_TX4810_SNPRINTF GLOBAL_SNPRINTF
    #elif ARM64_DELTA_TX4810_CONFIG_PORTING_STDLIB == 1
        #define ARM64_DELTA_TX4810_SNPRINTF snprintf
    #else
        #error The macro ARM64_DELTA_TX4810_SNPRINTF is required but cannot be defined.
    #endif
#endif

#ifndef ARM64_DELTA_TX4810_STRLEN
    #if defined(GLOBAL_STRLEN)
        #define ARM64_DELTA_TX4810_STRLEN GLOBAL_STRLEN
    #elif ARM64_DELTA_TX4810_CONFIG_PORTING_STDLIB == 1
        #define ARM64_DELTA_TX4810_STRLEN strlen
    #else
        #error The macro ARM64_DELTA_TX4810_STRLEN is required but cannot be defined.
    #endif
#endif

/* <auto.end.portingmacro(ALL).define> */


#endif /* __ARM64_DELTA_TX4810_PORTING_H__ */
/* @} */
