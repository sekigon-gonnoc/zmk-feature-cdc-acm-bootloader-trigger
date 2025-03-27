/*
 * Copyright (c) 2025 sekigon-gonnoc
 *
 * SPDX-License-Identifier: MIT
 */

#include <zephyr/device.h>
#include <zephyr/init.h>
#include <zephyr/kernel.h>
#include <zephyr/drivers/uart.h>
#include <zephyr/usb/usb_device.h>
#include <zephyr/sys/reboot.h>
#include <zephyr/logging/log.h>
#include <zephyr/devicetree.h>

#include <zmk/events/usb_conn_state_changed.h>

LOG_MODULE_REGISTER(zmk_cdc_acm_bootloader_trigger, CONFIG_ZMK_LOG_LEVEL);

#define DT_DRV_COMPAT zmk_cdc_acm_bootloader_trigger

#if DT_HAS_COMPAT_STATUS_OKAY(DT_DRV_COMPAT)

#define RST_UF2 0x57

#define ZMK_CDC_ACM_BOOTLOADER_TRIGGER_INST(n) DT_INST(n, zmk_cdc_acm_bootloader_trigger)

#if IS_ENABLED(CONFIG_ZMK_SETTINGS)
#include <zephyr/settings/settings.h>
#endif

struct cdc_acm_bootloader_trigger_config {
    const struct device *cdc_acm_dev; /* Can be NULL if auto-detect is used */
};

struct cdc_acm_bootloader_trigger_data {
    uint32_t baud_rate;
    bool port_open;
    bool usb_connected;
    struct k_work_delayable poll_work;
    const struct device *cdc_acm_dev; /* Store the CDC ACM device reference */
};

/* Forward declaration for work handlers */
static void enter_bootloader_work_handler(struct k_work *work);
static void poll_cdc_state_work_handler(struct k_work *work);

K_WORK_DEFINE(enter_bootloader_work, enter_bootloader_work_handler);

/* Reference to the single bootloader trigger instance */
static struct cdc_acm_bootloader_trigger_data *bootloader_trigger_data = NULL;

static void enter_bootloader_work_handler(struct k_work *work) {
    /* Small delay to let USB communications complete */
    k_sleep(K_MSEC(CONFIG_ZMK_CDC_ACM_BOOTLOADER_TRIGGER_DELAY_MS));

    /* Reboot into bootloader mode using the configured reset code */
    sys_reboot(CONFIG_ZMK_CDC_ACM_BOOTLOADER_TRIGGER_RESET_CODE);
}

static void poll_cdc_state_work_handler(struct k_work *work) {
    struct k_work_delayable *dwork = k_work_delayable_from_work(work);
    struct cdc_acm_bootloader_trigger_data *data = 
        CONTAINER_OF(dwork, struct cdc_acm_bootloader_trigger_data, poll_work);
    
    const struct device *dev = data->cdc_acm_dev;
    
    uint32_t dtr = 0, baud_rate = 0;
    int ret;

    /* Only poll if USB is connected */
    if (!data->usb_connected) {
        return;
    }

    /* Get current DTR state */
    ret = uart_line_ctrl_get(dev, UART_LINE_CTRL_DTR, &dtr);
    if (ret < 0) {
        LOG_DBG("Failed to get DTR state: %d", ret);
    } else {
        /* If DTR changed to true (port just opened), check baud rate */
        if (dtr) {
            ret = uart_line_ctrl_get(dev, UART_LINE_CTRL_BAUD_RATE, &baud_rate);
            if (ret < 0) {
                LOG_DBG("Failed to get baud rate: %d", ret);
            } else if (baud_rate != data->baud_rate) {
                /* Only log if baud rate changed */
                LOG_DBG("CDC ACM baud rate changed: %d", baud_rate);
                data->baud_rate = baud_rate;
            }
        }
        
        /* If DTR changed to false (port just closed) and baud rate was 1200, trigger bootloader */
        if (!dtr && data->baud_rate == 1200) {
            LOG_INF("CDC ACM port closed after 1200 baud set, triggering bootloader");
            k_work_submit(&enter_bootloader_work);
        }
        
        /* Update port open state */
        data->port_open = dtr;
    }
    
    /* Schedule next poll only if USB is connected */
    if (data->usb_connected) {
        k_work_schedule(dwork, K_MSEC(CONFIG_ZMK_CDC_ACM_BOOTLOADER_TRIGGER_POLL_MS));
    }
}

/* Handle USB connection state change events */
static int cdc_acm_bootloader_on_usb_conn_state_changed(const zmk_event_t *eh) {
    const struct zmk_usb_conn_state_changed *ev = as_zmk_usb_conn_state_changed(eh);
    
    if (bootloader_trigger_data == NULL) {
        return 0;
    }

    if (ev->conn_state == ZMK_USB_CONN_HID) {
        LOG_DBG("USB connected event");
        bootloader_trigger_data->usb_connected = true;
        /* Start polling when USB is connected */
        k_work_schedule(&bootloader_trigger_data->poll_work, 
                        K_MSEC(CONFIG_ZMK_CDC_ACM_BOOTLOADER_TRIGGER_POLL_MS));
    } else if (ev->conn_state == ZMK_USB_CONN_NONE) {
        LOG_DBG("USB disconnected event");
        bootloader_trigger_data->usb_connected = false;
        /* Reset state when USB is disconnected */
        bootloader_trigger_data->port_open = false;
        bootloader_trigger_data->baud_rate = 0;
        /* Cancel polling when USB is disconnected */
        k_work_cancel_delayable(&bootloader_trigger_data->poll_work);
    }

    return 0;
}

ZMK_LISTENER(cdc_acm_bootloader, cdc_acm_bootloader_on_usb_conn_state_changed);
ZMK_SUBSCRIPTION(cdc_acm_bootloader, zmk_usb_conn_state_changed);

static int cdc_acm_bootloader_trigger_init(const struct device *dev) {
    const struct cdc_acm_bootloader_trigger_config *config = dev->config;
    struct cdc_acm_bootloader_trigger_data *data = dev->data;
    const struct device *cdc_dev = NULL;
    
    /* If we have a configured CDC ACM device, use it */
    if (config->cdc_acm_dev != NULL) {
        cdc_dev = config->cdc_acm_dev;
        if (!device_is_ready(cdc_dev)) {
            LOG_ERR("Configured CDC ACM device not ready");
            return -ENODEV;
        }
    } else {
        /* Auto-detect a zephyr,cdc-acm-uart compatible device */
        #define CDC_ACM_UART_NODE DT_COMPAT_GET_ANY_STATUS_OKAY(zephyr_cdc_acm_uart)
        #if DT_NODE_EXISTS(CDC_ACM_UART_NODE)
            cdc_dev = DEVICE_DT_GET(CDC_ACM_UART_NODE);
            if (!device_is_ready(cdc_dev)) {
                LOG_ERR("Found CDC ACM UART device is not ready");
                return -ENODEV;
            }
            LOG_INF("Auto-detected zephyr,cdc-acm-uart device: %s", cdc_dev->name);
        #else
            LOG_ERR("No zephyr,cdc-acm-uart compatible device found in device tree");
            return -ENODEV;
        #endif
    }
    
    /* Initial state setup */
    data->port_open = false;
    data->baud_rate = 0;
    data->usb_connected = false;
    data->cdc_acm_dev = cdc_dev; /* Store detected or provided CDC ACM device */

    /* Initialize the polling work */
    k_work_init_delayable(&data->poll_work, poll_cdc_state_work_handler);

    /* Store reference to this instance for polling */
    bootloader_trigger_data = data;

    /* Polling will be started when USB connect event is received */
    LOG_INF("CDC ACM bootloader trigger initialized with ZMK USB event listener");
    return 0;
}

#define CDC_ACM_BOOTLOADER_TRIGGER_INIT(n)                                                    \
    static struct cdc_acm_bootloader_trigger_data cdc_acm_bootloader_trigger_data_##n = {     \
        .baud_rate = 0,                                                                       \
        .port_open = false,                                                                   \
        .usb_connected = false,                                                               \
    };                                                                                         \
                                                                                               \
    static const struct cdc_acm_bootloader_trigger_config cdc_acm_bootloader_trigger_config_##n = { \
        .cdc_acm_dev = COND_CODE_1(DT_INST_NODE_HAS_PROP(n, cdc_port),                        \
                                 (DEVICE_DT_GET(DT_INST_PHANDLE(n, cdc_port))),               \
                                 (NULL)),                                                       \
    };                                                                                         \
                                                                                               \
    DEVICE_DT_INST_DEFINE(n, cdc_acm_bootloader_trigger_init, NULL,                           \
                         &cdc_acm_bootloader_trigger_data_##n,                                \
                         &cdc_acm_bootloader_trigger_config_##n, POST_KERNEL,                 \
                         CONFIG_KERNEL_INIT_PRIORITY_DEVICE, NULL);

DT_INST_FOREACH_STATUS_OKAY(CDC_ACM_BOOTLOADER_TRIGGER_INIT)
#endif /* CONFIG_ZMK_CDC_ACM_BOOTLOADER_TRIGGER */
