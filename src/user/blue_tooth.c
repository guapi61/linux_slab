#include "blue_tooth.h"

#define MAX_DEVICES 50

int bluetooth_scan_using = 0;

struct Bluetooth_Message* bluetooth_message_head = NULL;//头插入
pthread_mutex_t bluetooth_message_mutex;
pthread_mutex_t bluetooth_mutex_scan;//防止多次开启蓝牙
pthread_mutex_t bluetooth_accepting;
pthread_mutex_t bluetooth_send_message;

static int init_flag = 0;
static uint16_t accept_handle = -1;

static GDBusConnection *conn = NULL;
static GDBusProxy *adapter_proxy = NULL;
static GDBusObjectManager *bluetooth_manager;

static gchar now_accept_device_path[40];
static GDBusProxy *now_accept_device_proxy;

static void device_found(GDBusObjectManager *bluetooth_manager, GDBusObject *object, gpointer user_data) {
    // 获取设备路径
    const gchar *path = g_dbus_object_get_object_path(object);

    printf("device_found: %s\n", path);
    // 获取设备属性
    GDBusProxy *device_proxy = g_dbus_proxy_new_sync(
        conn,
        G_DBUS_PROXY_FLAGS_NONE,
        NULL,
        BLUEZ_SERVICE,
        path,
        DEVICE_INTERFACE,
        NULL,
        NULL
    );

    if (device_proxy == NULL) {
        printf("创建设备代理时出错\n");
        return;
    }

    GVariant *properties = g_dbus_proxy_get_cached_property(device_proxy, "Address");
    if (properties == NULL) {
        printf("获取设备属性时出错\n");
        g_object_unref(device_proxy);
        return;
    }

    const gchar *address;
    g_variant_get(properties, "s", &address);

    properties = g_dbus_proxy_get_cached_property(device_proxy, "Name");
    const gchar *name;
    g_variant_get(properties, "s", &name);
    printf(" 111Address: %s, Name: %s\n", address, name);

    g_variant_unref(properties);
    g_object_unref(device_proxy);
}

static void device_removed(GDBusObjectManager *bluetooth_manager, GDBusObject *object, gpointer user_data)
{
    const gchar *path = g_dbus_object_get_object_path(object);
    printf("remove:%s,,,,,,,,,,,\n",path);
}


void clean_bluetooth_message(struct Bluetooth_Message *Bluetooth_Message)//递归清除先前蓝牙信息
{
    if(Bluetooth_Message == NULL) return;
    if(Bluetooth_Message->next == NULL)
    {
        if(Bluetooth_Message->bluetooth_device_path != NULL) free(Bluetooth_Message->bluetooth_device_path);
        free(Bluetooth_Message);
        return;
    }

    clean_bluetooth_message(Bluetooth_Message->next);
    if(Bluetooth_Message->bluetooth_device_path != NULL) free(Bluetooth_Message->bluetooth_device_path);
    Bluetooth_Message->bluetooth_device_path = NULL;
    if(Bluetooth_Message != NULL) free(Bluetooth_Message);
    Bluetooth_Message = NULL;
    return;
}

int query_bluetooth_connect_state()
{
    return 1;

}



void add_bluetooth_message(char* name,char* blue_addr,const gchar* bluetooth_device_path)//头插入
{
    struct Bluetooth_Message* tmp = malloc(sizeof(struct Bluetooth_Message) );
    tmp->bluetooth_device_path = malloc(strlen(bluetooth_device_path) + 1);
    strcpy(tmp->name,name);
    strcpy(tmp->blue_addr,blue_addr);
    strcpy(tmp->bluetooth_device_path,bluetooth_device_path);

    tmp->next = bluetooth_message_head;
    bluetooth_message_head = tmp;
}

void bluetooth_send(char* data)
{
    pthread_mutex_lock(&bluetooth_send_message);
    GError *error = NULL;

    // 获取 GATT 特征代理
    GDBusProxy *characteristic_proxy = g_dbus_proxy_new_sync(
        conn,
        G_DBUS_PROXY_FLAGS_NONE,
        NULL,
        BLUEZ_SERVICE,
        "/org/bluez/hci0/dev_00_15_A6_01_10_CD/service0028/char0029",
        GATT_CHARACTERISTIC_INTERFACE,
        NULL,
        &error
    );

    if (error != NULL) {
        printf("创建 GATT 特征代理时出错: %s\n", error->message);
        g_error_free(error);
        goto bluetooth_end ;
    }
    
    char user_data = *data;
     printf("user_dataaaaaaaaaaaa:%d,%c,\n",user_data,user_data);

    GVariantBuilder *builder;
    builder = g_variant_builder_new (G_VARIANT_TYPE ("ay"));
    for(int i = 0;i < strlen(data); i++){
        g_variant_builder_add (builder, "y", data[i]);
        printf("data: %d\n", data[i]);
    }
    //value = g_variant_new ("ay", builder);

    GVariantBuilder *b;
    b = g_variant_builder_new (G_VARIANT_TYPE ("a{sv}"));
    g_variant_builder_add (b, "{sv}", "name", g_variant_new_string ("foo"));
    g_variant_builder_add (b, "{sv}", "timeout", g_variant_new_int32 (10));
 
    GVariant* ans = g_variant_new("(aya{sv})",builder,b);
  
    error = NULL;
    g_dbus_proxy_call_sync(
        characteristic_proxy,
        "WriteValue",
        ans,
        G_DBUS_CALL_FLAGS_NONE,
        -1,
        NULL,
        &error
    );

    if (error != NULL) {
        printf("写入数据时出错: %s code:%d\n", error->message,error->code);
        g_error_free(error);
    } else {
        printf("数据写入成功\n");
    }

bluetooth_end:
    if (ans != NULL) {
        g_variant_unref(ans);
    }
    if(characteristic_proxy != NULL) {
        g_object_unref(characteristic_proxy);
    }
    if(builder != NULL){
        g_variant_builder_unref(builder);
    }
    if(b != NULL){
        g_variant_builder_unref(b);
    }
    pthread_mutex_unlock(&bluetooth_send_message);
}


static void pairing_complete(GDBusProxy *device_proxy, GAsyncResult *res, gpointer user_data) {
    GError *error = NULL;

    // 完成配对
    g_dbus_proxy_call_finish(device_proxy, res, &error); 

    if (error != NULL) {
        printf("配对时出错: %s\n", error->message);
        g_error_free(error);
    } else {
        g_print("设备配对成功\n");
    }

    // 退出主循环，结束程序
    g_main_loop_quit(user_data);
}


void accept_bluetooth(const gchar*  bluetooth_device_path)
{
    pthread_mutex_lock(&bluetooth_accepting);
    
    GError *error = NULL;
    printf("%s\n", bluetooth_device_path);
    // 创建设备代理
    GDBusProxy *device_proxy = g_dbus_proxy_new_sync(
        conn,
        G_DBUS_PROXY_FLAGS_NONE,
        NULL,
        BLUEZ_SERVICE,
        bluetooth_device_path,
        DEVICE_INTERFACE,
        NULL,
        &error
    );

    if (device_proxy == NULL) {
        printf("创建设备代理时出错: %s\n", error->message);
        goto accept_bluetooth_end;
    }

    if(error != NULL)
    {
        printf("创建设备代理时出错: %s\n", error->message);
        goto accept_bluetooth_end;
    }
    error = NULL;

    GVariant *passkey_variant = g_variant_new_uint32(123456);  // 1234是PIN码
    g_dbus_proxy_set_cached_property(device_proxy, "Passkey", passkey_variant);

    GVariant *result_pair = g_dbus_proxy_call_sync(
        device_proxy,
        "Pair",
        NULL,
        G_DBUS_CALL_FLAGS_NONE,
        -1,
        NULL,
        &error
    );

    if (error != NULL) {
        printf("配对时出错: %s\n", error->message);
        g_error_free(error);
    }

    error = NULL;

    //Connect to the device
    GVariant *result = g_dbus_proxy_call_sync(
        device_proxy,
        "Connect",
        NULL,
        G_DBUS_CALL_FLAGS_NONE,
        -1,
        NULL,
        &error
    );

    if (error != NULL) {
        printf("连接设备时出错: %s\n", error->message);
        goto accept_bluetooth_end;
    } else {
        printf("连接成功\n");
        now_accept_device_proxy = device_proxy;
        strcpy(now_accept_device_path,bluetooth_device_path);

        //bluetooth_send("ffffxxxxxxxxxxxxx\n");
    }

    // Cleanup
accept_bluetooth_end:
    if(result != NULL )g_variant_unref(result);
    if(device_proxy !=NULL) g_object_unref(device_proxy);
    if(error !=NULL) g_error_free(error);
    pthread_mutex_unlock(&bluetooth_accepting);
}

void disaccept_bluetooth()
{
    GError *call_error = NULL;
    GVariant *result = g_dbus_proxy_call_sync(
        now_accept_device_proxy,
        "Disconnect",
        NULL,
        G_DBUS_CALL_FLAGS_NONE,
        -1,
        NULL,
        &call_error
    );

    if (call_error != NULL) {
        printf("断开连接时出错: %s\n", call_error->message);
        g_error_free(call_error);
    } else {
        g_object_unref(now_accept_device_proxy);
    }
    return;
}

gboolean matches_pattern(const gchar *path, const gchar *pattern) {
    GRegex *regex = g_regex_new(pattern, 0, 0, NULL);
    gboolean match = g_regex_match(regex, path, 0, NULL);
    if(regex != NULL) g_regex_unref(regex);
    return match;
}

int query_device_have_rssi(const char* path)
{
     bool have_rssi =0;
    GError* error = NULL;
 // 需要的路径
    
    if (!matches_pattern(path, ".*dev.*")) {
        return 0;
    }

    GDBusProxy *device_proxy_tmp = g_dbus_proxy_new_sync(
            conn,
            G_DBUS_PROXY_FLAGS_NONE,
            NULL,
            BLUEZ_SERVICE,
            path,
            DEVICE_INTERFACE,
            NULL,
            &error
    );

    if(error !=NULL)
    {
        printf("removedevice eroor : %s \n",error->message);
        g_error_free(error);
        return 0;
    }

    GVariant *rssi_variant = g_dbus_proxy_get_cached_property(device_proxy_tmp, "RSSI");
    
    gint16 rssi = 1;

    if (rssi_variant != NULL) {
        g_variant_get(rssi_variant, "n", &rssi);
        printf("Device at path %s has RSSI: %d dBm\n", path, rssi);
        g_variant_unref(rssi_variant);
        have_rssi =1;
    }

    g_object_unref(device_proxy_tmp);
    //g_variant_unref(result);
    return have_rssi;
}

void scan_devices() {
    GError *call_error = NULL;
     // 启动设备扫描

    GVariant *result;

    result = g_dbus_proxy_call_sync(
        adapter_proxy,
        "StartDiscovery",
        NULL,
        G_DBUS_CALL_FLAGS_NONE,
        -1,
        NULL,
        &call_error
    );

    if (call_error != NULL) {
        printf("启动设备扫描时出错: %s\n", call_error->message);
        g_error_free(call_error);
        return ;
    }
    if(result!= NULL) g_variant_unref(result);
  
}

void unscan_devices() {
    // 停止设备扫描
    GVariant *result;
    GError *call_error = NULL;
    result = g_dbus_proxy_call_sync(
        adapter_proxy,
        "StopDiscovery",
        NULL,
        G_DBUS_CALL_FLAGS_NONE,
        -1,
        NULL,
        &call_error
    );

    if (call_error != NULL) {
        printf("停止设备扫描时出错: %s\n", call_error->message);
        g_error_free(call_error);
    }
}


void updata_bluetooth_scandevices() {
    GList *objects = g_dbus_object_manager_get_objects(bluetooth_manager);
    if(objects != NULL) printf("objects != NULL\n");
    for (GList *l = objects; l != NULL; l = l->next) {
        const gchar *path = g_dbus_object_get_object_path(l->data);
        
        if(!query_device_have_rssi(path))
        {
            continue;
        }

        GDBusProxy *device_proxy_tmp = g_dbus_proxy_new_sync(
            conn,
            G_DBUS_PROXY_FLAGS_NONE,
            NULL,
            BLUEZ_SERVICE,
            path,
            DEVICE_INTERFACE,
            NULL,
            NULL
        );

        if (device_proxy_tmp == NULL) {
            printf("创建设备代理时出错\n");
            return;
        }

        gchar* address;
        gchar* name;
        GVariant* properties[2] ;
        properties[0] = g_dbus_proxy_get_cached_property(device_proxy_tmp, "Address");
        if(properties[0] != NULL)  g_variant_get(properties[0], "s", &address);
    

        properties[1] = g_dbus_proxy_get_cached_property(device_proxy_tmp, "Name");
        if(properties[1] != NULL) g_variant_get(properties[1], "s", &name);
        if(properties[1] != NULL)  printf(" 2:Address: %s, Name: %s\n", address, name);
        
        if(properties[1] != NULL) 
            add_bluetooth_message(name,address,path);
    
        if(properties[0] != NULL)  g_variant_unref(properties[0]);
        if(properties[1] != NULL)  g_variant_unref(properties[1]);

        if(device_proxy_tmp != NULL) g_object_unref(device_proxy_tmp);
        
    }

    if(objects != NULL) g_list_free_full(objects,g_object_unref);
    
}

int init_bluetooth()
{
    if(1 == init_flag) return  -1; 
    if (pthread_mutex_init(&bluetooth_message_mutex, NULL) != 0) {
        perror("Mutex initialization failed");
        exit(1);
    }
    if (pthread_mutex_init(&bluetooth_mutex_scan, NULL) != 0) {
        perror("Mutex initialization failed");
        exit(1);
    }
    if (pthread_mutex_init(&bluetooth_accepting, NULL) != 0) {
        perror("Mutex initialization failed");
        exit(1);
    }

    if (pthread_mutex_init(&bluetooth_send_message, NULL) != 0) {
        perror("Mutex initialization failed");
        exit(1);
    }
    
    FILE *fp;
    char path[1035];

    // 打开命令的输出流
    fp = popen("hciconfig hci0 up && hciconfig hci0 lm master && hciconfig hci0 piscan", "r");

    if (fp == NULL) {
        printf("Failed to run command\n");
        exit(1);
    }

    // 读取输出流并打印到控制台
    while (fgets(path, sizeof(path) - 1, fp) != NULL) {
        printf("%s", path);
    }

    // 关闭输出流
    pclose(fp);



    GError *error = NULL;
    //g_type_init();
    // 连接到系统总线
    conn = g_bus_get_sync(G_BUS_TYPE_SYSTEM, NULL, &error);

    if (error != NULL) {
        printf("连接到 D-Bus 时出错: %s\n", error->message);
        g_error_free(error);
        return 1;
    }

    // 为 BlueZ 适配器创建代理
    adapter_proxy = g_dbus_proxy_new_sync(
        conn,
        G_DBUS_PROXY_FLAGS_NONE,
        NULL,
        BLUEZ_SERVICE,
        "/org/bluez/hci0", // 适配器路径，注意替换为你的实际适配器路径
        ADAPTER_INTERFACE,
        NULL,
        &error
    );

    if (error != NULL) {
        printf("创建适配器代理时出错: %s\n", error->message);
        g_error_free(error);
        g_object_unref(conn);
        return 1;
    }

    // 获取设备管理器
    bluetooth_manager = g_dbus_object_manager_client_new_sync(
        conn,
        G_DBUS_OBJECT_MANAGER_CLIENT_FLAGS_NONE,
        "org.bluez",
        "/", // BlueZ 服务路径
        NULL,
        NULL,
        NULL,
        NULL,
        &error
    );
    
    if (error != NULL) {
        printf("获取设备管理器时出错: %s\n", error->message);
        g_error_free(error);
        g_object_unref(adapter_proxy);
        g_object_unref(conn);
        return 1;
    }
    g_signal_connect(bluetooth_manager, "object-removed", G_CALLBACK(device_removed), NULL);
    g_signal_connect(bluetooth_manager, "object-added", G_CALLBACK(device_found), NULL);
    init_flag = 1;
    return 0;
}

int deinit_bluetooth()
{
    pthread_mutex_destroy(&bluetooth_message_mutex);
    pthread_mutex_destroy(&bluetooth_mutex_scan);
    pthread_mutex_destroy(&bluetooth_accepting);
    pthread_mutex_destroy(&bluetooth_send_message);
    init_flag = 0;

    g_object_unref(bluetooth_manager);
    g_object_unref(adapter_proxy);
    g_object_unref(conn);
}


void* bluetooth_scan_thread(void* argv) {

    pthread_mutex_lock(&bluetooth_mutex_scan);
    init_bluetooth();
    BluetoothDevice devices[MAX_DEVICES];
    int num_devices = 0;
    printf("Initializing Bluetooth...\n");


    // 扫描蓝牙设备
    printf("Scanning for devices...\n");
    scan_devices();
    while (bluetooth_scan_using)
    {
        //pthread_mutex_lock(&bluetooth_message_mutex); 
        pthread_mutex_lock(&bluetooth_accepting);
        pthread_mutex_lock(&bluetooth_message_mutex); 
        clean_bluetooth_message(bluetooth_message_head);
        pthread_mutex_unlock(&bluetooth_accepting);

        bluetooth_message_head = NULL;
        memset(&devices, 0, sizeof(devices));       

        
        sleep(3); 
        updata_bluetooth_scandevices();

        pthread_mutex_unlock(&bluetooth_message_mutex); 
        sleep(5);
    }
    unscan_devices();

    pthread_mutex_lock(&bluetooth_accepting);
    pthread_mutex_lock(&bluetooth_message_mutex); 
    clean_bluetooth_message(bluetooth_message_head);
    bluetooth_message_head = NULL;
    printf("ntnnnnnn\n");
    pthread_mutex_unlock(&bluetooth_accepting);
    pthread_mutex_unlock(&bluetooth_message_mutex);     
    
    pthread_mutex_unlock(&bluetooth_mutex_scan);
    return 0;
}