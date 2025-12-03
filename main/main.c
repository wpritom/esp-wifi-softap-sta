#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_system.h"
#include "esp_event.h"
#include "esp_log.h"
#include "lwip/err.h"
#include "lwip/sys.h"
#include "driver/gpio.h"
// raven projects includes
#include "raven_peer_handler.h"
#include "api_request_handler.h"
#include "raven_mqtt_handler.h"
#include <string.h>

#define INDICATOR_LED 40 //GPIO_NUM_6


void app_main(void)
{
    esp_reset_reason();
    uint8_t COUNT_CONFIG_BUTTON_PRESSED = 0;
    uint8_t INDICATOR_STATE = 0;
    // Initialize NVS
    esp_err_t ret = nvs_flash_init();
    // nvs_flash_erase();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND)
    {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);

    
    
    // GPIO config
    gpio_config_t boot_en_pin_conf= {
        .pin_bit_mask = (1ULL << GPIO_NUM_0), // Bit mask of the pins that you want to set,e.g.GPIO18/19
        .intr_type = GPIO_INTR_DISABLE, //Interrupt type
        .mode = GPIO_MODE_INPUT, //Set pin direction to output
        .pull_down_en = GPIO_PULLDOWN_DISABLE, //Disable pull-down mode
        .pull_up_en = GPIO_PULLUP_DISABLE, //Disable pull-up mode
        .intr_type = GPIO_INTR_DISABLE, //Interrupt type
    };

    gpio_config_t status_io_conf = {
        .pin_bit_mask = (1ULL << INDICATOR_LED),
        .mode = GPIO_MODE_OUTPUT,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE
    };

    // gpio_reset_pin(GPIO_NUM_9);

    gpio_config(&boot_en_pin_conf);
    gpio_config(&status_io_conf);
    int RETRY_CTD = 10;
    ///////////////////////////////////////////////////////////////////////


    uint8_t CONNECTED_MODE = device_wifi_provision();
    // http_result_t http_result = {0};
    uint8_t PAIRED = isPaired();
    
    uint8_t RAVEN_MQTT_CALLED = 0;
    ////////////////////////////////////////////////////////////////////////
    while (1)
    {
        // __NOP(); // <-  Prevent WDT Reset
        printf("%s\n", device_mac_str);
        vTaskDelay(500 / portTICK_PERIOD_MS); // <- 1 Second
        printf("CONNECTED MODE %d WIFI CONNECTED %d\n", CONNECTED_MODE, is_wifi_sta_connected());

        if (is_wifi_sta_connected() && PAIRED && !RAVEN_MQTT_CALLED){
            raven_mqtt_start();
            RAVEN_MQTT_CALLED = 1;
        }

        if(CONNECTED_MODE==RAVEN_STA_MODE && is_wifi_sta_connected()){
            INDICATOR_STATE=1;
            
        }
        else if(CONNECTED_MODE==RAVEN_AP_MODE && !is_wifi_sta_connected()){
            INDICATOR_STATE=!INDICATOR_STATE;
        }
       
        if(!is_wifi_sta_connected() && http_read_connect_command_and_reset()){
            printf(" --- Device Connection Command Received --- \n");
            CONNECTED_MODE = device_wifi_provision();
        }

        if (gpio_get_level(GPIO_NUM_0) == 0)
        {
            printf("Button Pressed\n");
            COUNT_CONFIG_BUTTON_PRESSED++;

            if(COUNT_CONFIG_BUTTON_PRESSED>=5){
                // esp_wifi_disconnect()
                erase_wifi_config();
                COUNT_CONFIG_BUTTON_PRESSED=0;
                CONNECTED_MODE = device_wifi_provision();
                PAIRED = isPaired();
            }
        }
        gpio_set_level(INDICATOR_LED, INDICATOR_STATE);

        // device pairing section
        if (is_wifi_sta_connected() &!PAIRED){
            if(!raven_http_result.in_progress && !raven_http_result.done){
                async_api_get_device_pairing(device_mac_str,
                                              DEVICE_PID,
                                              DEVICE_SECRET);
              
            }
            else if (raven_http_result.done){
                // optional printout
                printf("[%d] RESPONSE %s\n", raven_http_result.status_code, raven_http_result.response);
                raven_http_result.done = false;

                if (raven_http_result.status_code == 200)
                {
                    bool success = set_device_paired(raven_http_result.response);
                    if (success)
                    {
                        PAIRED = isPaired();
                    }
                    else
                    {
                        printf("Pairing failed!\n");
                    }
                }
            }

            else if (raven_http_result.in_progress){
                // adding a minimal delay for http request to process
                vTaskDelay(10 / portTICK_PERIOD_MS); // 10ms
                printf(" ... HTTP REQUEST RUNNING ... \n");
            }     
            
        }

        if(PAIRED && is_wifi_sta_connected()){
            printf(" >>> http request: in_progress %d | done %d \n", raven_http_result.in_progress, raven_http_result.done);
            
            if(!raven_http_result.in_progress && !raven_http_result.done){
                async_api_post_device_data(device_mac_str,
                                       DEVICE_SECRET,   
                                       USER_UUID,
                                       1,
                                       1);
            }
            else if (raven_http_result.done){
                printf("[%d] RESPONSE %s\n", raven_http_result.status_code, raven_http_result.response);
                raven_http_result.done = false;
            }       
        }

        if(CONNECTED_MODE==RAVEN_STA_MODE && !is_wifi_sta_connected()){
            if(RETRY_CTD==0){
                RETRY_CTD=10;
                check_and_retry_wifi(1);
            }
            else{
                RETRY_CTD--;
            }
            printf("WIFI DISCONNECTED | RETRY CTD %d\n", RETRY_CTD);
        }

    }
}

