/* UART Echo Example

   This example code is in the Public Domain (or CC0 licensed, at your option.)

   Unless required by applicable law or agreed to in writing, this
   software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
   CONDITIONS OF ANY KIND, either express or implied.
*/
#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/uart.h"
#include "driver/gpio.h"
#include "sdkconfig.h"
#include "esp_log.h"
#include "led_strip.h"
#include <string.h>

/**
 * This is an example which echos any data it receives on configured UART back to the sender,
 * with hardware flow control turned off. It does not use UART driver event queue.
 *
 * - Port: configured UART
 * - Receive (Rx) buffer: on
 * - Transmit (Tx) buffer: off
 * - Flow control: off
 * - Event queue: off
 * - Pin assignment: see defines below (See Kconfig)
 */

#define ECHO_TEST_TXD (CONFIG_EXAMPLE_UART_TXD)
#define ECHO_TEST_RXD (CONFIG_EXAMPLE_UART_RXD)
#define ECHO_TEST_RTS (UART_PIN_NO_CHANGE)
#define ECHO_TEST_CTS (UART_PIN_NO_CHANGE)

#define ECHO_UART_PORT_NUM      (CONFIG_EXAMPLE_UART_PORT_NUM)
#define ECHO_UART_BAUD_RATE     (CONFIG_EXAMPLE_UART_BAUD_RATE)
#define LORA_TASK_STACK_SIZE    (CONFIG_EXAMPLE_TASK_STACK_SIZE)

#define LORA_TX (CONFIG_UART_LORA_TX)
#define LORA_RX (CONFIG_UART_LORA_RX)
#define LORA_BASE_BAUD_RATE 115200
#define LORA_HIGH_BAUD_RATE 230400
#define UART_LORA_PORT_NUM (CONFIG_UART_LORA_PORT_NUM)

#define BLINK_GPIO (CONFIG_BLINK_GPIO)

#define BUF_SIZE (1024)

void sendLoraModule(char* text, int size);
void joinNetwork(void);
void sendLoraMSG(char* text, int size);

static void lora_task(void *arg)
{
    ESP_LOGI("LoraTask", "Hello!");
    /* Configure parameters of an UART driver,
     * communication pins and install the driver */
    uart_config_t uart_config = {
        .baud_rate = ECHO_UART_BAUD_RATE,
        .data_bits = UART_DATA_8_BITS,
        .parity    = UART_PARITY_DISABLE,
        .stop_bits = UART_STOP_BITS_1,
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
        .source_clk = UART_SCLK_DEFAULT,
    };

    uart_config_t uart_lora = {
        .baud_rate = LORA_BASE_BAUD_RATE,
        .data_bits = UART_DATA_8_BITS,
        .parity    = UART_PARITY_DISABLE,
        .stop_bits = UART_STOP_BITS_1,
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
        .source_clk = UART_SCLK_DEFAULT,
    };

    int intr_alloc_flags = 0;

#if CONFIG_UART_ISR_IN_IRAM
    intr_alloc_flags = ESP_INTR_FLAG_IRAM;
#endif

    //Config UART2
    ESP_ERROR_CHECK(uart_driver_install(ECHO_UART_PORT_NUM, BUF_SIZE * 2, 0, 0, NULL, intr_alloc_flags));
    ESP_ERROR_CHECK(uart_param_config(ECHO_UART_PORT_NUM, &uart_config));
    ESP_ERROR_CHECK(uart_set_pin(ECHO_UART_PORT_NUM, ECHO_TEST_TXD, ECHO_TEST_RXD, ECHO_TEST_RTS, ECHO_TEST_CTS));

    //Config UART1
    ESP_ERROR_CHECK(uart_driver_install(UART_LORA_PORT_NUM, BUF_SIZE * 2, 0, 0, NULL, intr_alloc_flags));
    ESP_ERROR_CHECK(uart_param_config(UART_LORA_PORT_NUM, &uart_lora));
    ESP_ERROR_CHECK(uart_set_pin(UART_LORA_PORT_NUM, LORA_TX, LORA_RX, ECHO_TEST_RTS, ECHO_TEST_CTS));

    //Configure LoRa Module
    //Test si tout est ok
    sendLoraModule("AT\n", 3);
    //On définit l'appkey
    sendLoraModule("AT+KEY= APPKEY,\"14 46 2a 40 45 46 4c 7e a7 29 6c 07 5f c7 ba cc\"\n", 65);
    //On passe en mode OTAA
    sendLoraModule("AT+MODE=LWOTAA\n", 15);
    //On met la pouissance au MAX
    sendLoraModule("AT+POWER=24, FORCE\n", 19);
    //On rejoint le réseau
    joinNetwork();

    static uint16_t u16msgNumber = 0;
    static char buffer[100];

    //On attends que la connection se fasse
    ESP_LOGI("LoraTask", "Start sending");

    while (1) {
        snprintf(buffer, 100, "AT+MSG=\"Blotz-%d\"\n", u16msgNumber);
        ESP_LOGI("LoraTask", "%s", buffer);

        //Send a message per LoRA
        sendLoraMSG(buffer, 100);

        u16msgNumber++;

        //Sleep for 5s 
        vTaskDelay(1000 / portTICK_PERIOD_MS);
    }
}

void sendLoraModule(char* text, int size)
{
    // Configure a temporary buffer for the incoming data
    uint8_t *data = (uint8_t *) malloc(BUF_SIZE);
    int len = 0;

    uart_write_bytes(UART_LORA_PORT_NUM, text, size);

    while(len == 0)
    {
        len = uart_read_bytes(UART_LORA_PORT_NUM, data, (BUF_SIZE - 1), 20 / portTICK_PERIOD_MS);
    }
    ESP_LOGI("LoraTask", "Message length: %d - %.*s", len, len, data);
}

void sendLoraMSG(char* text, int size)
{
    // Configure a temporary buffer for the incoming data
    uint8_t *data = (uint8_t *) malloc(BUF_SIZE);
    int len = 0;

    uart_write_bytes(UART_LORA_PORT_NUM, text, size);
    while(memcmp(data, "+MSG: Done", 10))
    {
        while(len == 0)
        {
            len = uart_read_bytes(UART_LORA_PORT_NUM, data, (BUF_SIZE - 1), 20 / portTICK_PERIOD_MS);
        }
        ESP_LOGI("LoraTask", "Message length: %d - %.*s", len, len, data);
        len = 0;
    }
}

void joinNetwork(void)
{
    // Configure a temporary buffer for the incoming data
    uint8_t *data = (uint8_t *) malloc(BUF_SIZE);
    int len = 0;

    uart_write_bytes(UART_LORA_PORT_NUM, "AT+JOIN\n", 8);

    //Normal response +JOIN: NORMAL
    while(len == 0)
    {
        len = uart_read_bytes(UART_LORA_PORT_NUM, data, (BUF_SIZE - 1), 20 / portTICK_PERIOD_MS);
    }
    ESP_LOGI("LoraTask", "Message length: %d - %.*s", len, len, data);
    len = 0;
    //Join response
    while(len == 0)
    {
        len = uart_read_bytes(UART_LORA_PORT_NUM, data, (BUF_SIZE - 1), 20 / portTICK_PERIOD_MS);
    }
    if(!memcmp(data, "+JOIN: Join failed", 18))
    {
        //On restart l'esp si on a pas réussi à se connecter
        ESP_LOGI("LoraTask", "Join failed");
        esp_restart();
    }
    ESP_LOGI("LoraTask", "Message length: %d - %.*s", len, len, data);
}

void app_main(void)
{
    //Tasks declaration
    xTaskCreate(lora_task, "lora_task", LORA_TASK_STACK_SIZE, NULL, 10, NULL);
}
