/************************************************************
 * <bsn.cl fy=2014 v=onl>
 *
 *           Copyright 2014 Big Switch Networks, Inc.
 *           Copyright 2017 MiTAC Computing Technology Corporation.
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
#include <sys/mman.h>
#include <errno.h>
#include <string.h>
#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <AIM/aim.h>
#include <onlp/onlp.h>
#include <onlplib/file.h>
#include "platform_lib.h"

#define HEX_STR_TO_INT_VALUE_MAX_STR_LEN 32


int deviceNodeWrite(char *filename, char *buffer, int buf_size, int data_len)
{
    int fd;
    int len;

    if ((buffer == NULL) || (buf_size < 0)) {
        return -1;
    }

    if ((fd = open(filename, O_WRONLY, S_IWUSR)) == -1) {
        return -1;
    }

    if ((len = write(fd, buffer, buf_size)) < 0) {
        close(fd);
        return -1;
    }

    if ((close(fd) == -1)) {
        return -1;
    }

    if ((len > buf_size) || (data_len != 0 && len != data_len)) {
        return -1;
    }

    return 0;
}

int deviceNodeWriteInt(char *filename, int value, int data_len)
{
    char buf[8] = {0};
    sprintf(buf, "%d", value);

    return deviceNodeWrite(filename, buf, (int)strlen(buf), data_len);
}

int deviceNodeReadBinary(char *filename, char *buffer, int buf_size, int data_len)
{
    int fd;
    int len;

    if ((buffer == NULL) || (buf_size < 0)) {
        return -1;
    }

    if ((fd = open(filename, O_RDONLY)) == -1) {
        return -1;
    }

    if ((len = read(fd, buffer, buf_size)) < 0) {
        close(fd);
        return -1;
    }

    if ((close(fd) == -1)) {
        return -1;
    }

    if ((len > buf_size) || (data_len != 0 && len != data_len)) {
        return -1;
    }

    return 0;
}

int deviceNodeReadString(char *filename, char *buffer, int buf_size, int data_len)
{
    int ret;

    if (data_len >= buf_size) {
	    return -1;
	}

	ret = deviceNodeReadBinary(filename, buffer, buf_size-1, data_len);

    if (ret == 0) {
        buffer[buf_size-1] = '\0';
    }

    return ret;
}

#define I2C_PSU_MODEL_NAME_LEN 11
#define I2C_PSU_FAN_DIR_LEN    3
#include <ctype.h>
psu_type_t get_psu_type(int id, char* modelname, int modelname_len)
{
    char *node = NULL;
    char  model_name[I2C_PSU_MODEL_NAME_LEN + 1] = {0};
    char  fan_dir[I2C_PSU_FAN_DIR_LEN + 1] = {0};

    /* Check AC model name */
    node = (id == PSU1_ID) ? PSU1_AC_HWMON_NODE(psu_model_name) : PSU2_AC_HWMON_NODE(psu_model_name);

    if (deviceNodeReadString(node, model_name, sizeof(model_name), 0) != 0) {
        return PSU_TYPE_UNKNOWN;
    }

    if(isspace(model_name[strlen(model_name)-1])) {
        model_name[strlen(model_name)-1] = 0;
    }

    if (strncmp(model_name, "YM-2651Y", 8) == 0) {
	    if (modelname) {
			strncpy(modelname, model_name, 8);
	    }

	    node = (id == PSU1_ID) ? PSU1_AC_PMBUS_NODE(psu_fan_dir) : PSU2_AC_PMBUS_NODE(psu_fan_dir);
	    if (deviceNodeReadString(node, fan_dir, sizeof(fan_dir), 0) != 0) {
	        return PSU_TYPE_UNKNOWN;
	    }

	    if (strncmp(fan_dir, "F2B", strlen("F2B")) == 0) {
	        return PSU_TYPE_AC_F2B;
	    }

	    if (strncmp(fan_dir, "B2F", strlen("B2F")) == 0) {
	        return PSU_TYPE_AC_B2F;
	    }
    }

    if (strncmp(model_name, "YM-2651V", 8) == 0) {
	    if (modelname) {
			strncpy(modelname, model_name, 8);
	    }

	    node = (id == PSU1_ID) ? PSU1_AC_PMBUS_NODE(psu_fan_dir) : PSU2_AC_PMBUS_NODE(psu_fan_dir);
	    if (deviceNodeReadString(node, fan_dir, sizeof(fan_dir), 0) != 0) {
	        return PSU_TYPE_UNKNOWN;
	    }

	    if (strncmp(fan_dir, "F2B", strlen("F2B")) == 0) {
	        return PSU_TYPE_DC_48V_F2B;
	    }

	    if (strncmp(fan_dir, "B2F", strlen("B2F")) == 0) {
	        return PSU_TYPE_DC_48V_B2F;
	    }
    }

	if (strncmp(model_name, "PSU-12V-750", 11) == 0) {
	    if (modelname) {
			strncpy(modelname, model_name, 11);
	    }

	    node = (id == PSU1_ID) ? PSU1_AC_HWMON_NODE(psu_fan_dir) : PSU2_AC_HWMON_NODE(psu_fan_dir);
	    if (deviceNodeReadString(node, fan_dir, sizeof(fan_dir), 0) != 0) {
	        return PSU_TYPE_UNKNOWN;
	    }

	    if (strncmp(fan_dir, "F2B", 3) == 0) {
	        return PSU_TYPE_DC_12V_F2B;
	    }

	    if (strncmp(fan_dir, "B2F", 3) == 0) {
	        return PSU_TYPE_DC_12V_B2F;
	    }

	    if (strncmp(fan_dir, "NON", 3) == 0) {
	        return PSU_TYPE_DC_12V_FANLESS;
	    }
	}

    return PSU_TYPE_UNKNOWN;
}

/***********************************************************************
 *  Read data from file and transfer content from "0x" format to integer.
 ***********************************************************************/
int onlp_file_read_int_hex(int* value, const char* node_path)
{
    int len = 0;
    long int tmp_v = 0;
    char *pEnd;
    char buffer[HEX_STR_TO_INT_VALUE_MAX_STR_LEN] = {0};

    if (onlp_file_read((uint8_t *)buffer, sizeof(buffer), &len, node_path) != 0) {
        DEBUG_PRINT("%s(%d)\r\n", __FUNCTION__, __LINE__);
        return ONLP_STATUS_E_INTERNAL;
    }

    /* Transfer from string "0x1" to int "1" */
    tmp_v = strtol (buffer,&pEnd,16);
    *value = (int)tmp_v;

    return ONLP_STATUS_OK;
}
