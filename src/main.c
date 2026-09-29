#include <stdio.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "nvs_flash.h"
#include "esp_wifi.h"
#include "esp_now.h"

// REPLACE THIS WITH YOUR RECEIVER's MAC ADDRESS (use 0x before each pair)
uint8_t receiver_mac_address[] = {0x7C, 0x4F, 0xAD, 0xB7, 0xBC, 0x64};

typedef struct struct_message {
    int8_t steering;
    int8_t throttle;
} struct_message;

struct_message myData;


void OnDataSent(const wifi_tx_info_t *tx_info, esp_now_send_status_t status) {
    printf("Send Status: %s\n", status == ESP_NOW_SEND_SUCCESS ? "Success" : "Fail");
}

void app_main(void) {
    nvs_flash_init();
    esp_netif_init();
    esp_event_loop_create_default();
    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    esp_wifi_init(&cfg);
    esp_wifi_set_mode(WIFI_MODE_STA);
    esp_wifi_start();

    esp_now_init();
    esp_now_register_send_cb(OnDataSent);

    // Register the receiver as a peer
    esp_now_peer_info_t peerInfo = {};
    memcpy(peerInfo.peer_addr, receiver_mac_address, 6);
    peerInfo.channel = 0;  // 0 means use the current Wi-Fi channel
    peerInfo.encrypt = false;
    esp_now_add_peer(&peerInfo);

    // Starting test values
    myData.steering = 0;
    myData.throttle = -50;

    while (1) {
        // Simulate changing joystick values for the test
        myData.steering++;
        myData.throttle++;
        if(myData.steering > 100) myData.steering = -100;
        if(myData.throttle > 100) myData.throttle = -100;
        
        // Send the data packet
        esp_now_send(receiver_mac_address, (uint8_t *) &myData, sizeof(myData));
        
        // Send a packet 10 times per second (100ms delay)
        vTaskDelay(100 / portTICK_PERIOD_MS); 
    }
}