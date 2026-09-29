#include <stdio.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "nvs_flash.h"
#include "esp_wifi.h"
#include "esp_now.h"
#include "esp_adc/adc_oneshot.h"

// REPLACE THIS WITH YOUR RECEIVER's MAC ADDRESS
uint8_t receiver_mac_address[] = {0x7C, 0x4F, 0xAD, 0xB7, 0xBC, 0x64};

typedef struct struct_message {
    int16_t steering;
    int16_t throttle;
} struct_message;

struct_message myData;

void OnDataSent(const uint8_t *mac_addr, esp_now_send_status_t status) {
    // Suppress printing every 50ms to keep the serial monitor readable
}

extern "C" void app_main(void) {
    // 1. Initialize Wi-Fi and ESP-NOW
    nvs_flash_init();
    esp_netif_init();
    esp_event_loop_create_default();
    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    esp_wifi_init(&cfg);
    esp_wifi_set_mode(WIFI_MODE_STA);
    esp_wifi_start();
    esp_now_init();
    
    esp_now_register_send_cb((esp_now_send_cb_t)OnDataSent);
    esp_now_peer_info_t peerInfo = {};
    memcpy(peerInfo.peer_addr, receiver_mac_address, 6);
    peerInfo.channel = 0;
    peerInfo.encrypt = false;
    esp_now_add_peer(&peerInfo);

    // 2. Initialize the ADC for the Potentiometer (GPIO 4)
    adc_oneshot_unit_handle_t adc1_handle;
    adc_oneshot_unit_init_cfg_t init_config = {};
    init_config.unit_id = ADC_UNIT_1;
    adc_oneshot_new_unit(&init_config, &adc1_handle);

    adc_oneshot_chan_cfg_t config = {};
    config.bitwidth = ADC_BITWIDTH_DEFAULT; // 12-bit resolution (0-4095)
    config.atten = ADC_ATTEN_DB_12;         // 12dB attenuation to read full 0-3.3V range
    adc_oneshot_config_channel(adc1_handle, ADC_CHANNEL_3, &config);

    adc_oneshot_chan_cfg_t config2 = {};
    config2.bitwidth = ADC_BITWIDTH_DEFAULT; // 12-bit resolution (0-4095)
    config2.atten = ADC_ATTEN_DB_12;         // 12dB attenuation to read full 0-3.3V range
    adc_oneshot_config_channel(adc1_handle, ADC_CHANNEL_4, &config2);

    while (1) {
        int adc_raw_ch3 = 0;
        int adc_raw_ch4 = 0;
        adc_oneshot_read(adc1_handle, ADC_CHANNEL_3, &adc_raw_ch3);
        adc_oneshot_read(adc1_handle, ADC_CHANNEL_4, &adc_raw_ch4);

        // Map the 0-4095 ADC reading to -100 to 100 for the steering
        myData.steering = (int16_t)((adc_raw_ch3 * 200 / 4095) - 100);
        myData.throttle = (int16_t)((adc_raw_ch4 * 200 / 4095) - 100); // Fixed at 0 for now

        printf("Potentiometer Raw: %d, %d | Steering Data: %d | Throttle Data: %d\n", adc_raw_ch3, adc_raw_ch4, myData.steering, myData.throttle);

        esp_now_send(receiver_mac_address, (uint8_t *) &myData, sizeof(myData));
        vTaskDelay(50 / portTICK_PERIOD_MS); // Update 20 times per second
    }
}