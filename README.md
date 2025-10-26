# ESP WiFi Easy Provisioning (RAVEN)
In this project we upload Wi-Fi credentials when ESP32 is in SoftAP mode. After submit with `http` api request, ESP32 will connect to the network and restart itself.


# How to Use
This program assumes you did not yet integrate following feature in your ESP-IDF program.

* ESP WiFi
* ESP HTTP Server
* ESP NVS Flash

Add all the components from the `components` directory in your program. 

Load the following header file in your program:

```
#include "raven_peer_handler.h"
```

In your program main function, call `device_wifi_provision();` to start the process.

```c
CONNECTED_MODE = device_wifi_provision();
```

* CONNECTED_MODE = 0 = AP Mode
* CONNECTED_MODE = 1 = STA Mode
* CONNECTED_MODE = 3 = ELSE

# Access Point Mode (AP Mode)
In this mode your ESP32 will be a WiFi hotspot.

At the beginning of your program, there is no peering configeration. At that stage `CONNECTED_MODE = 0`. 

If you search the wifi hotspot from your phone, you will see the hotspot name defined as  `EXAMPLE_ESP_WIFI_PASS` in the `components\raven_wifi_config\include\raven_wifi_config.h` file. You can use that hotspot name and password to connect the ESP32. 


# Station Mode (STA Mode)
In this mode your ESP32 will be a WiFi client. It will try to connect to the peering network. If it cannot, it will be in AP mode.

# API
### GET: Heartbeat
```
http://192.168.4.1/hello
```
This also reports the existing data in the `nvs`.

### POST : ssid | pass
```
http://192.168.4.1/ssid
http://192.168.4.1/pass
```
These endpoints will update SSID and password in the `nvs`.

### GET : connect command
```
http://192.168.4.1/connect
```
This endpoint sends the connect command to ESP.


# Device Pairing

Following Steps to pair the device with the app. 

From the device perspective: 
* Initially device not paired with the app `isPaired()` will give 0. 
* Connect the device with a WiFi that provides internet access.
* After it successfully connects with the WiFi call `async_api_get_device_pairing()`
* When `async_api_get_device_pairing()` returns response 200
* Success response is replied with the follwoing data
```
{
  "device_id": "xx",
  "device_secret": "xx",
  "uuid": "xxxuuixxx",
  "pid": "xxx"
}
```
* Among the above data the `uuid` is the one that you need to save in your program. This will be required while reporting device data.

## Device Pairing Example 

Declate a global variable as a flag
```c
// declare global variables
uint8_t PAIRED = isPaired(); // primarily it will be 0

```
Add this code in your main loop. 
This will work when WiFi is connected and device is not paired.
```c
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

        if (raven_http_result.status_code==200){
            
            bool success = set_device_paired(raven_http_result.response);
            
            if (success){
                PAIRED=isPaired();
            }
            else{
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

```
Notes:
* Signal should be received within 1 minute. Otherwise you can consider the pairing as failed.
* The `isPaired()` function is defined in `raven_peer_handler.h`