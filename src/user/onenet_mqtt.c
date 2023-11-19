#include "onenet_mqtt.h"


//MQTT
static void interceptor_handler(void* client, message_data_t* msg)
{
    (void) client;
    MQTT_LOG_I("-----------------------------------------------------------------------------------");
    MQTT_LOG_I("%s:%d %s()...\ntopic: %s\nmessage:%s", __FILE__, __LINE__, __FUNCTION__, msg->topic_name, (char*)msg->message->payload);
    MQTT_LOG_I("-----------------------------------------------------------------------------------");
}

// MQTT线程函数
void* mqtt_thread(void* arg) {
    // 连接到MQTT服务器
    int res;
    pthread_t thread1;
    mqtt_client_t *client = NULL;
    char buf[100] = { 0 };
    mqtt_message_t msg;
    
    printf("\nwelcome to mqttclient test...\n");

    mqtt_log_init();

    client = mqtt_lease();

    mqtt_set_port(client, "1883");
    mqtt_set_host(client, "mqtts.heclouds.com");
    mqtt_set_client_id(client, "stm32mp157_control");
    mqtt_set_user_name(client, "4U86I1hTQq");
    mqtt_set_password(client, "version=2018-10-31&res=products%2F4U86I1hTQq%2Fdevices%2Fstm32mp157_control&et=9991537255523&method=md5&sign=Ao2LVitvxQUYuxOYETtlSQ%3D%3D");
    mqtt_set_clean_session(client, 1);
    
    while(MQTT_SUCCESS_ERROR  != mqtt_connect(client))
    {
        printf("mqtt connect failed\n");
        sleep(3);
    };

    mqtt_subscribe(client, "$sys/4U86I1hTQq/stm32mp157_control/thing/property/post/reply", QOS0, NULL);
    mqtt_set_interceptor_handler(client, interceptor_handler);     // set interceptor handler


    memset(&msg, 0, sizeof(msg));
    sprintf(buf, "welcome to mqttclient, this is a publish test...");
    
    msg.qos = 0;
    msg.payload = (void *) buf;

        // 创建 JSON 对象
    cJSON *root = cJSON_CreateObject();

    // 添加 "id" 和 "version" 到 JSON 对象
    cJSON_AddStringToObject(root, "id", "123");
    cJSON_AddStringToObject(root, "version", "1.0");
    sprintf(buf, "welcome to mqttclient, this is a publish test, a rand number: %d ...", random_number());

    // 创建 "params" 对象
    cJSON *params = cJSON_CreateObject();

    cJSON *pow = cJSON_CreateObject();
    cJSON_AddItemToObject(params, "pow", pow);
    cJSON_AddStringToObject(pow, "value", "123");
    cJSON_AddNumberToObject(pow, "time", 1555);

    // 添加 "params" 到 JSON 对象
    cJSON_AddItemToObject(root, "params", params);

    char *json_str = cJSON_Print(root);
    msg.qos = 0;
    msg.payload = (void *) json_str;
    printf("%s\n", json_str);

    while(1) {
        mqtt_publish(client, "$sys/4U86I1hTQq/stm32mp157_control/thing/property/post", &msg);
        sleep(4);
    }

    return NULL;
}
