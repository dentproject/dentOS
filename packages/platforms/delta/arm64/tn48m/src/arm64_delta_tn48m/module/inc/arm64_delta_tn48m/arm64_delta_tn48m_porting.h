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
 * @brief arm64_delta_tn48m Porting Macros.
 *
 * @addtogroup arm64_delta_tn48m-porting
 * @{
 *
 ***********************************************************/
#ifndef __ARM64_DELTA_TN48M_PORTING_H__
#define __ARM64_DELTA_TN48M_PORTING_H__


/* <auto.start.portingmacro(ALL).define> */
#if ARM64_DELTA_TN48M_CONFIG_PORTING_INCLUDE_STDLIB_HEADERS == 1
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <memory.h>
#endif

#ifndef ARM64_DELTA_TN48M_MALLOC
    #if defined(GLOBAL_MALLOC)
        #define ARM64_DELTA_TN48M_MALLOC GLOBAL_MALLOC
    #elif ARM64_DELTA_TN48M_CONFIG_PORTING_STDLIB == 1
        #define ARM64_DELTA_TN48M_MALLOC malloc
    #else
        #error The macro ARM64_DELTA_TN48M_MALLOC is required but cannot be defined.
    #endif
#endif

#ifndef ARM64_DELTA_TN48M_FREE
    #if defined(GLOBAL_FREE)
        #define ARM64_DELTA_TN48M_FREE GLOBAL_FREE
    #elif ARM64_DELTA_TN48M_CONFIG_PORTING_STDLIB == 1
        #define ARM64_DELTA_TN48M_FREE free
    #else
        #error The macro ARM64_DELTA_TN48M_FREE is required but cannot be defined.
    #endif
#endif

#ifndef ARM64_DELTA_TN48M_MEMSET
    #if defined(GLOBAL_MEMSET)
        #define ARM64_DELTA_TN48M_MEMSET GLOBAL_MEMSET
    #elif ARM64_DELTA_TN48M_CONFIG_PORTING_STDLIB == 1
        #define ARM64_DELTA_TN48M_MEMSET memset
    #else
        #error The macro ARM64_DELTA_TN48M_MEMSET is required but cannot be defined.
    #endif
#endif

#ifndef ARM64_DELTA_TN48M_MEMCPY
    #if defined(GLOBAL_MEMCPY)
        #define ARM64_DELTA_TN48M_MEMCPY GLOBAL_MEMCPY
    #elif ARM64_DELTA_TN48M_CONFIG_PORTING_STDLIB == 1
        #define ARM64_DELTA_TN48M_MEMCPY memcpy
    #else
        #error The macro ARM64_DELTA_TN48M_MEMCPY is required but cannot be defined.
    #endif
#endif

#ifndef ARM64_DELTA_TN48M_VSNPRINTF
    #if defined(GLOBAL_VSNPRINTF)
        #define ARM64_DELTA_TN48M_VSNPRINTF GLOBAL_VSNPRINTF
    #elif ARM64_DELTA_TN48M_CONFIG_PORTING_STDLIB == 1
        #define ARM64_DELTA_TN48M_VSNPRINTF vsnprintf
    #else
        #error The macro ARM64_DELTA_TN48M_VSNPRINTF is required but cannot be defined.
    #endif
#endif

#ifndef ARM64_DELTA_TN48M_SNPRINTF
    #if defined(GLOBAL_SNPRINTF)
        #define ARM64_DELTA_TN48M_SNPRINTF GLOBAL_SNPRINTF
    #elif ARM64_DELTA_TN48M_CONFIG_PORTING_STDLIB == 1
        #define ARM64_DELTA_TN48M_SNPRINTF snprintf
    #else
        #error The macro ARM64_DELTA_TN48M_SNPRINTF is required but cannot be defined.
    #endif
#endif

#ifndef ARM64_DELTA_TN48M_STRLEN
    #if defined(GLOBAL_STRLEN)
        #define ARM64_DELTA_TN48M_STRLEN GLOBAL_STRLEN
    #elif ARM64_DELTA_TN48M_CONFIG_PORTING_STDLIB == 1
        #define ARM64_DELTA_TN48M_STRLEN strlen
    #else
        #error The macro ARM64_DELTA_TN48M_STRLEN is required but cannot be defined.
    #endif
#endif

/* <auto.end.portingmacro(ALL).define> */


#endif /* __ARM64_DELTA_TN48M_PORTING_H__ */
/* @} */
