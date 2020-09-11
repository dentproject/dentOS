/************************************************************
 * <bsn.cl fy=2014 v=onl>
 *
 *        Copyright 2020 Delta Networks, Inc.
 *
 * Licensed under the Eclipse Public License, Version 1.0 (the
 * "License"); you may not use this file except in compliance
 * with the License. You may obtain a copy of the License at
 *
 *        http://www.eclipse.org/legal/epl-v10.html
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

#include <arm64_delta_tn48m/arm64_delta_tn48m_config.h>

/* <auto.start.cdefs(ARM64_DELTA_TN48M_CONFIG_HEADER).source> */
#define __arm64_delta_tn48m_config_STRINGIFY_NAME(_x) #_x
#define __arm64_delta_tn48m_config_STRINGIFY_VALUE(_x) __arm64_delta_tn48m_config_STRINGIFY_NAME(_x)
arm64_delta_tn48m_config_settings_t arm64_delta_tn48m_config_settings[] =
{
#ifdef ARM64_DELTA_TN48M_CONFIG_INCLUDE_LOGGING
    { __arm64_delta_tn48m_config_STRINGIFY_NAME(ARM64_DELTA_TN48M_CONFIG_INCLUDE_LOGGING), __arm64_delta_tn48m_config_STRINGIFY_VALUE(ARM64_DELTA_TN48M_CONFIG_INCLUDE_LOGGING) },
#else
{ ARM64_DELTA_TN48M_CONFIG_INCLUDE_LOGGING(__arm64_delta_tn48m_config_STRINGIFY_NAME), "__undefined__" },
#endif
#ifdef ARM64_DELTA_TN48M_CONFIG_LOG_OPTIONS_DEFAULT
    { __arm64_delta_tn48m_config_STRINGIFY_NAME(ARM64_DELTA_TN48M_CONFIG_LOG_OPTIONS_DEFAULT), __arm64_delta_tn48m_config_STRINGIFY_VALUE(ARM64_DELTA_TN48M_CONFIG_LOG_OPTIONS_DEFAULT) },
#else
{ ARM64_DELTA_TN48M_CONFIG_LOG_OPTIONS_DEFAULT(__arm64_delta_tn48m_config_STRINGIFY_NAME), "__undefined__" },
#endif
#ifdef ARM64_DELTA_TN48M_CONFIG_LOG_BITS_DEFAULT
    { __arm64_delta_tn48m_config_STRINGIFY_NAME(ARM64_DELTA_TN48M_CONFIG_LOG_BITS_DEFAULT), __arm64_delta_tn48m_config_STRINGIFY_VALUE(ARM64_DELTA_TN48M_CONFIG_LOG_BITS_DEFAULT) },
#else
{ ARM64_DELTA_TN48M_CONFIG_LOG_BITS_DEFAULT(__arm64_delta_tn48m_config_STRINGIFY_NAME), "__undefined__" },
#endif
#ifdef ARM64_DELTA_TN48M_CONFIG_LOG_CUSTOM_BITS_DEFAULT
    { __arm64_delta_tn48m_config_STRINGIFY_NAME(ARM64_DELTA_TN48M_CONFIG_LOG_CUSTOM_BITS_DEFAULT), __arm64_delta_tn48m_config_STRINGIFY_VALUE(ARM64_DELTA_TN48M_CONFIG_LOG_CUSTOM_BITS_DEFAULT) },
#else
{ ARM64_DELTA_TN48M_CONFIG_LOG_CUSTOM_BITS_DEFAULT(__arm64_delta_tn48m_config_STRINGIFY_NAME), "__undefined__" },
#endif
#ifdef ARM64_DELTA_TN48M_CONFIG_PORTING_STDLIB
    { __arm64_delta_tn48m_config_STRINGIFY_NAME(ARM64_DELTA_TN48M_CONFIG_PORTING_STDLIB), __arm64_delta_tn48m_config_STRINGIFY_VALUE(ARM64_DELTA_TN48M_CONFIG_PORTING_STDLIB) },
#else
{ ARM64_DELTA_TN48M_CONFIG_PORTING_STDLIB(__arm64_delta_tn48m_config_STRINGIFY_NAME), "__undefined__" },
#endif
#ifdef ARM64_DELTA_TN48M_CONFIG_PORTING_INCLUDE_STDLIB_HEADERS
    { __arm64_delta_tn48m_config_STRINGIFY_NAME(ARM64_DELTA_TN48M_CONFIG_PORTING_INCLUDE_STDLIB_HEADERS), __arm64_delta_tn48m_config_STRINGIFY_VALUE(ARM64_DELTA_TN48M_CONFIG_PORTING_INCLUDE_STDLIB_HEADERS) },
#else
{ ARM64_DELTA_TN48M_CONFIG_PORTING_INCLUDE_STDLIB_HEADERS(__arm64_delta_tn48m_config_STRINGIFY_NAME), "__undefined__" },
#endif
#ifdef ARM64_DELTA_TN48M_CONFIG_INCLUDE_UCLI
    { __arm64_delta_tn48m_config_STRINGIFY_NAME(ARM64_DELTA_TN48M_CONFIG_INCLUDE_UCLI), __arm64_delta_tn48m_config_STRINGIFY_VALUE(ARM64_DELTA_TN48M_CONFIG_INCLUDE_UCLI) },
#else
{ ARM64_DELTA_TN48M_CONFIG_INCLUDE_UCLI(__arm64_delta_tn48m_config_STRINGIFY_NAME), "__undefined__" },
#endif
#ifdef ARM64_DELTA_TN48M_CONFIG_SFP_COUNT
    { __arm64_delta_tn48m_config_STRINGIFY_NAME(ARM64_DELTA_TN48M_CONFIG_SFP_COUNT), __arm64_delta_tn48m_config_STRINGIFY_VALUE(ARM64_DELTA_TN48M_CONFIG_SFP_COUNT) },
#else
{ ARM64_DELTA_TN48M_CONFIG_SFP_COUNT(__arm64_delta_tn48m_config_STRINGIFY_NAME), "__undefined__" },
#endif
    { NULL, NULL }
};
#undef __arm64_delta_tn48m_config_STRINGIFY_VALUE
#undef __arm64_delta_tn48m_config_STRINGIFY_NAME

const char*
arm64_delta_tn48m_config_lookup(const char* setting)
{
    int i;
    for(i = 0; arm64_delta_tn48m_config_settings[i].name; i++) {
        if(!strcmp(arm64_delta_tn48m_config_settings[i].name, setting)) {
            return arm64_delta_tn48m_config_settings[i].value;
        }
    }
    return NULL;
}

int
arm64_delta_tn48m_config_show(struct aim_pvs_s* pvs)
{
    int i;
    for(i = 0; arm64_delta_tn48m_config_settings[i].name; i++) {
        aim_printf(pvs, "%s = %s\n", arm64_delta_tn48m_config_settings[i].name, arm64_delta_tn48m_config_settings[i].value);
    }
    return i;
}

/* <auto.end.cdefs(ARM64_DELTA_TN48M_CONFIG_HEADER).source> */

