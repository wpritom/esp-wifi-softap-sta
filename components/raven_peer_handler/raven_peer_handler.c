#include "raven_peer_handler.h"

uint8_t RAVEN_AP_MODE = 0;
uint8_t RAVEN_STA_MODE = 1;

uint8_t connected_mode = 0;
uint8_t wifi_mode = 0;


void erase_wifi_config(void){
    nvs_memory_erase("SSID");
    nvs_memory_erase("PASS");
    nvs_memory_erase("CHECK");
    nvs_memory_erase("PAIRED");
    sta_ssid[0] = '\0';
    sta_pass[0] = '\0';
}


void device_softap_mode_init(void){
    wifi_set_ap();
    start_webserver();
}

void device_station_mode_init(void){
    stop_webserver();
    wifi_set_sta();
}


uint8_t device_wifi_provision(void){
      /*
    CONFIGURED_STATE == 0/1 : not configured
    CONFIGURED_STATE == 2 : first time checking
    CONFIGURED_STATE == 3 : already configured

    returns 
    CONNECTED_MODE = 0 = AP Mode
    CONNECTED_MODE = 1 = STA Mode
    CONNECTED_MODE = 3 = ELSE
    */

    uint8_t CONFIGURED_STATE = 0;
    uint8_t CONNECTED_MODE = 0;
    
    if(!WIFI_GLOBAL_INIT){
        wifi_init_all();
    }
    

    // check wifi configuration data in nvs
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

    if(CONFIGURED_STATE==2){
        printf("--- Configured state %d\n", CONFIGURED_STATE);
        // first time peer attempt (peering)
        if(!nvs_memory_read("CHECK")){
            printf("--- peering attempt...");
            device_station_mode_init();
            
            if(!is_wifi_sta_connected()){
                printf("--- initial peering attempt failed!");
                erase_wifi_config();
                CONNECTED_MODE = RAVEN_AP_MODE;
                device_softap_mode_init();
                
            }
            // after successful peering
            else{
                CONNECTED_MODE = RAVEN_STA_MODE;
                nvs_memory_store("CHECK","1");

            } 
        }
        // device already connected once
        else{
            device_station_mode_init();
            CONNECTED_MODE = RAVEN_STA_MODE;
        }
    }

    else{
        // no saved peering data
        device_softap_mode_init();
        CONNECTED_MODE=RAVEN_AP_MODE;;
    }

    return CONNECTED_MODE;
}


uint8_t isPaired(void){
    uint8_t success = nvs_memory_read("PAIRED");
    uint8_t val = 0;

    if(success){
        val = (uint8_t)read_nvs_buffer[0];
    }
    return val;
}

void set_device_paired(void){
    nvs_memory_store("PAIRED","1");
}