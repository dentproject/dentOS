/*
 * A LED driver for the delta_tx4810_dn_led
 *
 * Copyright (C) 2021 Delta Networks, Inc.
 *
 * Chenglin Tsai <chenglin.tsai@deltaww.com>
 *
 * Base on:
 * accton_as4610_leds.c
 * Copyright (C) 2016 Accton Technology Corporation.
 * Brandon Chuang <brandon_chuang@accton.com.tw>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.     See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/platform_device.h>
#include <linux/err.h>
#include <linux/leds.h>
#include <linux/slab.h>
#include <linux/gpio.h>
#include <linux/bitops.h>


#define DRVNAME "tx4810_dn_led"

/* LED related data
 */
enum led_type {
    LED_TYPE_PSU,
    LED_TYPE_FAN,
    LED_TYPE_SYS,
    NUM_OF_LED
};

enum led_light_mode {
    LED_MODE_OFF = 0,
    LED_MODE_GREEN,
    LED_MODE_AMBER,
    LED_MODE_UNKNOWN,
};

struct tx4810_dn_gpio {
    const char *name;
    unsigned int gpio;
};

static struct tx4810_dn_gpio tx4810_dn_led_gpio[] = {
    {"LED PSU Green", 35},
    {"LED PSU Amber", 34},
    {"LED FAN Green", 46},
    {"LED FAN Amber", 43},
    {"LED SYS Green", 63},
    {"LED SYS Amber", 66},
};

#define TX4810_DN_COMMON_LED_MAP  (BIT(LED_TYPE_PSU) | BIT(LED_TYPE_FAN) | \
                                 BIT(LED_TYPE_SYS))

struct tx4810_dn_led_data {
    struct platform_device *pdev;
    struct mutex     update_lock;
    char             valid;             /* != 0 if registers are valid */
    unsigned long    last_updated;      /* In jiffies */
    int              led_map;
    u8               led_gpio_val[6];
};

static struct tx4810_dn_led_data  *ledctl = NULL;


static enum led_light_mode gpio_val_to_led_mode(int led_green, int led_amber)
{
    if (led_green && !led_amber)
        return LED_MODE_GREEN;
    else if (!led_green && led_amber)
        return LED_MODE_AMBER;
    else
        return LED_MODE_OFF;
}

static void led_mode_to_gpio_value(enum led_light_mode mode,
                                               int *led_green, int *led_amber)
{
    switch (mode)
    {
    case LED_MODE_GREEN:
        *led_green = 1;
        *led_amber = 0;
        break;
    case LED_MODE_AMBER:
        *led_green = 0;
        *led_amber = 1;
        break;
    default:
        *led_green = 0;
        *led_amber = 0;
        break;
    }
}

static int tx4810_dn_gpio_map(struct platform_device *pdev)
{
    int i, err;

    for (i = 0; i < ARRAY_SIZE(tx4810_dn_led_gpio); i++) {
        err = gpio_request(tx4810_dn_led_gpio[i].gpio, tx4810_dn_led_gpio[i].name);
        if (err) {
            dev_err(&pdev->dev, "error mapping gpio %s: %d\n",
                    tx4810_dn_led_gpio[i].name, err);
            goto err_request;
        }
        gpio_direction_output(tx4810_dn_led_gpio[i].gpio, 0);
    }

    return 0;

err_request:
    while (--i >= 0)
        gpio_free(tx4810_dn_led_gpio[i].gpio);
    return err;
}

static void tx4810_dn_gpio_unmap(void)
{
    int i;

    for (i = 0; i < ARRAY_SIZE(tx4810_dn_led_gpio); i++)
        gpio_free(tx4810_dn_led_gpio[i].gpio);
}

static int tx4810_dn_led_read_value(unsigned int gpio)
{
    return gpio_get_value(gpio);
}

static void tx4810_dn_led_write_value(unsigned int gpio, u8 value)
{
    return gpio_set_value(gpio, value);
}

static void tx4810_dn_led_update(void)
{
    mutex_lock(&ledctl->update_lock);

    if (time_after(jiffies, ledctl->last_updated + HZ + HZ / 2)
        || !ledctl->valid) {
        int i;

        dev_dbg(&ledctl->pdev->dev, "Starting tx4810_dn_led update\n");

        /* Update LED data
         */
        for (i = 0; i < ARRAY_SIZE(tx4810_dn_led_gpio); i++) {
            int status = tx4810_dn_led_read_value(tx4810_dn_led_gpio[i].gpio);

            if (status < 0) {
                ledctl->valid = 0;
                goto exit;
            }
            else
            {
                ledctl->led_gpio_val[i] = status;
            }
        }

        ledctl->last_updated = jiffies;
        ledctl->valid = 1;
    }

exit:
    mutex_unlock(&ledctl->update_lock);
}

static void tx4810_dn_led_normal_set(struct led_classdev *cdev,
                                            enum led_brightness mode,
                                            enum led_type type)
{
    int gpio_green_val, gpio_amber_val;
    int gpio_green_idx, gpio_amber_idx;

    /* Validate brightness */
    if (mode > cdev->max_brightness) {
        return;
    }

    led_mode_to_gpio_value(mode, &gpio_green_val, &gpio_amber_val);

    gpio_green_idx = 2 * type;
    gpio_amber_idx = 2 * type + 1;

    tx4810_dn_led_write_value(tx4810_dn_led_gpio[gpio_green_idx].gpio, gpio_green_val);
    tx4810_dn_led_write_value(tx4810_dn_led_gpio[gpio_amber_idx].gpio, gpio_amber_val);
}

static enum led_brightness tx4810_dn_led_normal_get(enum led_type type)
{
    enum led_brightness mode;
    int gpio_green_idx, gpio_amber_idx;

    tx4810_dn_led_update();

    gpio_green_idx = 2 * type;
    gpio_amber_idx = 2 * type + 1;

    mode = gpio_val_to_led_mode(ledctl->led_gpio_val[gpio_green_idx],
                                ledctl->led_gpio_val[gpio_amber_idx]);

    return mode;
}

static void tx4810_dn_led_psu_set(struct led_classdev *cdev,
                                       enum led_brightness mode)
{
    tx4810_dn_led_normal_set(cdev, mode, LED_TYPE_PSU);
}

static enum led_brightness tx4810_dn_led_psu_get(struct led_classdev *cdev)
{
    return tx4810_dn_led_normal_get(LED_TYPE_PSU);
}

static void tx4810_dn_led_fan_set(struct led_classdev *cdev,
                                          enum led_brightness mode)
{
    tx4810_dn_led_normal_set(cdev, mode, LED_TYPE_FAN);
}

static enum led_brightness tx4810_dn_led_fan_get(struct led_classdev *cdev)
{
    return tx4810_dn_led_normal_get(LED_TYPE_FAN);
}

static void tx4810_dn_led_sys_set(struct led_classdev *cdev,
                                          enum led_brightness mode)
{
    tx4810_dn_led_normal_set(cdev, mode, LED_TYPE_SYS);
}

static enum led_brightness tx4810_dn_led_sys_get(struct led_classdev *cdev)
{
    return tx4810_dn_led_normal_get(LED_TYPE_SYS);
}

static struct led_classdev tx4810_dn_leds[] = {
    [LED_TYPE_PSU] = {
        .name            = "tx4810_dn::psu",
        .default_trigger = "unused",
        .brightness_set  = tx4810_dn_led_psu_set,
        .brightness_get  = tx4810_dn_led_psu_get,
        .max_brightness  = LED_MODE_AMBER,
    },
    [LED_TYPE_FAN] = {
        .name            = "tx4810_dn::fan",
        .default_trigger = "unused",
        .brightness_set  = tx4810_dn_led_fan_set,
        .brightness_get  = tx4810_dn_led_fan_get,
        .max_brightness  = LED_MODE_AMBER,
    },
    [LED_TYPE_SYS] = {
        .name            = "tx4810_dn::sys",
        .default_trigger = "unused",
        .brightness_set  = tx4810_dn_led_sys_set,
        .brightness_get  = tx4810_dn_led_sys_get,
        .max_brightness  = LED_MODE_AMBER,
    },
};

static int tx4810_dn_led_probe(struct platform_device *pdev)
{
    int ret, i;

    ret = tx4810_dn_gpio_map(pdev);
    if (ret)
        return ret;

    for (i = 0; i < NUM_OF_LED; i++) {
        if (!(ledctl->led_map & BIT(i))) {
            continue;
        }

        ret = led_classdev_register(&pdev->dev, &tx4810_dn_leds[i]);
        if (ret < 0) {
            goto error;
        }
    }

    return 0;

error:
    for (i = i - 1; i >= 0; i--) {
        /* only unregister the LEDs that were successfully registered */
        if (!(ledctl->led_map & BIT(i))) {
            continue;
        }

        led_classdev_unregister(&tx4810_dn_leds[i]);
    }

    return ret;
}

static int tx4810_dn_led_remove(struct platform_device *pdev)
{
    int i;

    tx4810_dn_gpio_unmap();

    for (i = 0; i < NUM_OF_LED; i++) {
        if (!(ledctl->led_map & BIT(i))) {
            continue;
        }

        led_classdev_unregister(&tx4810_dn_leds[i]);
    }

    return 0;
}

static struct platform_driver tx4810_dn_led_driver = {
    .driver = {
        .name  = DRVNAME,
        .owner = THIS_MODULE,
    },
    .probe     = tx4810_dn_led_probe,
    .remove    = tx4810_dn_led_remove,
};

static int __init tx4810_dn_led_init(void)
{
    int ret;

    ret = platform_driver_register(&tx4810_dn_led_driver);
    if (ret < 0) {
        goto exit;
    }

    ledctl = kzalloc(sizeof(struct tx4810_dn_led_data), GFP_KERNEL);
    if (!ledctl) {
        ret = -ENOMEM;
        platform_driver_unregister(&tx4810_dn_led_driver);
        goto exit;
    }

    ledctl->led_map = TX4810_DN_COMMON_LED_MAP;
    mutex_init(&ledctl->update_lock);

    ledctl->pdev = platform_device_register_simple(DRVNAME, -1, NULL, 0);
    if (IS_ERR(ledctl->pdev)) {
        ret = PTR_ERR(ledctl->pdev);
        platform_driver_unregister(&tx4810_dn_led_driver);
        kfree(ledctl);
        goto exit;
    }

exit:
    return ret;
}

static void __exit tx4810_dn_led_exit(void)
{
    if (!ledctl) {
        return;
    }

    platform_device_unregister(ledctl->pdev);
    platform_driver_unregister(&tx4810_dn_led_driver);
    kfree(ledctl);
}

late_initcall(tx4810_dn_led_init);
module_exit(tx4810_dn_led_exit);

MODULE_AUTHOR("Chenglin Tsai <chenglin.tsai@deltaww.com>");
MODULE_DESCRIPTION("tx4810_dn_led driver");
MODULE_LICENSE("GPL");

