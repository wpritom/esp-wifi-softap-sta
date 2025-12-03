#include <stdio.h>
#include <string.h>
#include "esp_log.h"
#include "mqtt_client.h"
#include "raven_mqtt_handler.h"


extern const char _binary_hivemq_ca_pem_start[];
extern const char _binary_hivemq_ca_pem_end[];
static const char *TAG = "RAVEN_MQTT";


/* ========== Your HiveMQ Credentials ========== */
#define MQTT_BROKER_URI  "mqtts://173da0488d444eae9c65ebaf494d71f0.s1.eu.hivemq.cloud:8883"
#define MQTT_USERNAME    "aerosense"
#define MQTT_PASSWORD    "Asdf1234#"
// #define MQTT_TOPIC_SUB   "gdmd/CMDFG" // will be taken from NVS (PID/DEVICEID)
char raven_mqtt_topic[128];
char raven_mqtt_buff[256];
uint8_t MQTT_MSG_AVAILABLE = 0;


uint8_t is_mqtt_message_available()
{
    if(MQTT_MSG_AVAILABLE){
        MQTT_MSG_AVAILABLE = 0;
        return 1;
    }
    else{
        return 0;
    }
}


void fetch_raven_mqtt_topic(){
    snprintf(
    raven_mqtt_topic,
    sizeof(raven_mqtt_topic),
    "%s/%s",
    DEVICE_PID,
    device_mac_str
);
}

/* ========== MQTT Event Handler ========== */
static void mqtt_event_handler(void *handler_args,
                               esp_event_base_t base,
                               int32_t event_id,
                               void *event_data)
{
    esp_mqtt_event_handle_t event = event_data;
    esp_mqtt_client_handle_t client = event->client;

    switch (event_id) {

        case MQTT_EVENT_CONNECTED:
            ESP_LOGI(TAG, " ||| <> Connected to broker");
            esp_mqtt_client_subscribe(client, raven_mqtt_topic, 1);
            ESP_LOGI(TAG, " ||| <<>> Subscribed to topic: %s", raven_mqtt_topic);
            break;

        case MQTT_EVENT_DISCONNECTED:
            ESP_LOGW(TAG, " ||| </> Disconnected from broker");
            break;

        case MQTT_EVENT_SUBSCRIBED:
            ESP_LOGI(TAG, " ||| <>Subscription successful");
            break;

        case MQTT_EVENT_DATA:
            // ESP_LOGI(TAG, " ||| - Message received:");
            // printf("Topic: %.*s\n", event->topic_len, event->topic);
            // printf("Data : %.*s\n", event->data_len, event->data);
            MQTT_MSG_AVAILABLE = 1;
            snprintf(raven_mqtt_buff, sizeof(raven_mqtt_buff), "%.*s", event->data_len, event->data);
            break;

        case MQTT_EVENT_ERROR:
            ESP_LOGE(TAG, "⚠️ MQTT Error occurred");
            break;

        default:
            break;
    }
}

/* ========== Start MQTT Client ========== */
void raven_mqtt_start(void)
{   
    
    ESP_LOGI(TAG, "🚀 Starting Raven MQTT Client...");
    fetch_raven_mqtt_topic();
    esp_mqtt_client_config_t mqtt_cfg = {
        .broker.address.uri = MQTT_BROKER_URI,

        .broker.verification.certificate = (const char *)_binary_hivemq_ca_pem_start,

        .credentials.username = MQTT_USERNAME,
        .credentials.authentication.password = MQTT_PASSWORD,

        .session.keepalive = 60,
    };

    esp_mqtt_client_handle_t client = esp_mqtt_client_init(&mqtt_cfg);

    esp_mqtt_client_register_event(client,
                                   ESP_EVENT_ANY_ID,
                                   mqtt_event_handler,
                                   NULL);

    esp_mqtt_client_start(client);
}
