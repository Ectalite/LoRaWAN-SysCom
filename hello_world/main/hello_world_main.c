/*
 * SPDX-FileCopyrightText: 2010-2022 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: CC0-1.0
 */

#include <stdio.h>
#include <inttypes.h>
#include "sdkconfig.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_chip_info.h"
#include "esp_flash.h"
#include "esp_log.h"
#include "led_strip.h"

#define BLINK_GPIO               48 /* GPIO 48 for ESP32-S3 built-in addressable LED */
#define BLINK_LED_RMT_CHANNEL    0
#define BLINK_PERIOD             1000

static const char *TAG = "blink";
static uint8_t s_led_state = 0;
static led_strip_t *pStrip_a;

void app_main(void)
{
    printf("Hello world!\n");

    /* Print chip information */
    esp_chip_info_t chip_info;
    uint32_t flash_size;
    esp_chip_info(&chip_info);
    printf("This is %s chip with %d CPU core(s), WiFi%s%s%s, ",
           CONFIG_IDF_TARGET,
           chip_info.cores,
           (chip_info.features & CHIP_FEATURE_BT) ? "/BT" : "",
           (chip_info.features & CHIP_FEATURE_BLE) ? "/BLE" : "",
           (chip_info.features & CHIP_FEATURE_IEEE802154) ? ", 802.15.4 (Zigbee/Thread)" : "");

    unsigned major_rev = chip_info.revision / 100;
    unsigned minor_rev = chip_info.revision % 100;
    printf("silicon revision v%d.%d, ", major_rev, minor_rev);
    if(esp_flash_get_size(NULL, &flash_size) != ESP_OK) {
        printf("Get flash size failed");
        return;
    }

    printf("%" PRIu32 "MB %s flash\n", flash_size / (uint32_t)(1024 * 1024),
           (chip_info.features & CHIP_FEATURE_EMB_FLASH) ? "embedded" : "external");

    printf("Minimum free heap size: %" PRIu32 " bytes\n", esp_get_minimum_free_heap_size());

    for (int i = 10; i >= 0; i--) {
        printf("Restarting in %d seconds...\n", i);
        vTaskDelay(1000 / portTICK_PERIOD_MS);
    }
    //printf("Restarting now.\n");
    //fflush(stdout);
    //esp_restart();
    
    ESP_LOGI(TAG, "Example configured to blink addressable LED!");
    pStrip_a = led_strip_init(BLINK_LED_RMT_CHANNEL, BLINK_GPIO, 1); /* LED strip initialization with the GPIO and pixels number*/
    pStrip_a->clear(pStrip_a, 50);                                          /* Set all LED off to clear all pixels */

    while (1)
    {
        ESP_LOGI(TAG, "Turning the LED %s!", s_led_state == true ? "ON" : "OFF");

        if (s_led_state)                                    /* If the addressable LED is enabled */
        {
            pStrip_a->set_pixel(pStrip_a, 0, 116, 16, 16);  /* Set the LED pixel using RGB from 0 (0%) to 255 (100%) for each color */
            pStrip_a->refresh(pStrip_a, 100);               /* Refresh the strip to send data */
        }
        else
        {
            pStrip_a->clear(pStrip_a, 50);                  /* Set all LED off to clear all pixels */
        }

        s_led_state = !s_led_state;                         /* Toggle the LED state */
        vTaskDelay(BLINK_PERIOD / portTICK_PERIOD_MS);
    }
}
