#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
// #include "esp_mac.h"
#include "esp_system.h"
// #include "esp_wifi.h"
#include "esp_event.h"
#include "esp_log.h"
// #include "nvs_flash.h"

#include "lwip/err.h"
#include "lwip/sys.h"

// #include "esp_netif.h"

#include "driver/gpio.h"
// #include <event_groups.h>
#include "raven_nvs_handler.h"
#include "raven_wifi_config.h"
#include "server_handler.h"

#define INDICATOR_LED 40 //GPIO_NUM_6

const char *CLIENT_TAG_STA = "wifi station";
static const char *CLIENT_TAG = "wifi softAP";


void erase_wifi_config(void){
    nvs_memory_erase("SSID");
    nvs_memory_erase("PASS");
    nvs_memory_erase("CHECK");
}

uint8_t config_and_provisioning(void){
      /*
    CONFIGURED_STATE == 0/1 : not configured
    CONFIGURED_STATE == 2 : first time checking
    CONFIGURED_STATE == 3 : already configured
    */
    uint8_t CONFIGURED_STATE = 0;
    uint8_t CONNECTED_MODE = 0;
     
    // check memory for connection state
    if (nvs_memory_read("SSID"))
    {
        strcpy(sta_ssid, read_nvs_buffer);
        CONFIGURED_STATE++;
    }
    if (nvs_memory_read("PASS"))
    {
        strcpy(sta_pass, read_nvs_buffer);
        CONFIGURED_STATE++;
    }
    
    // connected once check then connected mode otherwise will trigger ap mode (nvs cleared)
    
    if (nvs_memory_read("CHECK")){
        CONFIGURED_STATE++;
    }
  
   
    if (CONFIGURED_STATE == 2 || CONFIGURED_STATE ==3)// need to check tomorrow
    {
        wifi_init_sta();
    }
    else
    {
        ESP_LOGI(CLIENT_TAG, "ESP_WIFI_MODE_STA IMPOSSIBLE");
        wifi_init_softap();
        start_webserver();
        ESP_LOGI(CLIENT_TAG, "ESP_WIFI_MODE_AP");
    }

    // connection check and feedback
    if (CONFIGURED_STATE == 2)
    {
        if(!WIFI_CONNECTED){
            erase_wifi_config();
            printf("FIRST TIME CONNECTION FAILED. Restarting...\n");
            vTaskDelay(1000 / portTICK_PERIOD_MS);
            esp_restart(); // return to bootloader
        }
        else{
            nvs_memory_store("CHECK", "1");
        }  

       CONNECTED_MODE = 1;
   }
   else if (CONFIGURED_STATE == 3)
   {
       CONNECTED_MODE = 1;
   }
 
    return CONNECTED_MODE;
}


void app_main(void)
{
    
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

   uint8_t CONNECTED_MODE = config_and_provisioning();
    
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
    
    ///////////////////////////////////////////////////////////////////////
    while (1)
    {
        // __NOP(); // <-  Prevent WDT Reset
        vTaskDelay(500 / portTICK_PERIOD_MS); // <- 1 Second
    
        if (gpio_get_level(GPIO_NUM_0) == 0)
        {
            printf("Button Pressed\n");
            COUNT_CONFIG_BUTTON_PRESSED++;

            if(COUNT_CONFIG_BUTTON_PRESSED>=5){
                erase_wifi_config();
                esp_restart();
            }
            
        }

        if (CONNECTED_MODE & WIFI_CONNECTED){
            printf("Connected Mode\n");
            INDICATOR_STATE = 1;
        }
        else if (CONNECTED_MODE & !WIFI_CONNECTED){
            printf("Disconnected Mode\n");
            INDICATOR_STATE = 0;
        }
        else
        {
            INDICATOR_STATE = !INDICATOR_STATE;
            printf("AP Mode\n");
          
        }
        gpio_set_level(INDICATOR_LED, INDICATOR_STATE);
        
    }
}