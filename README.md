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