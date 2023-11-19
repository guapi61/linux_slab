#ifndef USER_BLUETOOTH_
#define USER_BLUETOOTH_
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/socket.h>
#include <bluetooth/bluetooth.h> 
#include <bluetooth/hci.h>
#include <bluetooth/hci_lib.h>
#include <pthread.h>
#include <gio/gio.h>
#include <bluetooth/rfcomm.h>
#include <gobject/gtype.h>
#include "user/ui_lvgl.h"

#define BLUEZ_SERVICE "org.bluez" 
#define ADAPTER_INTERFACE BLUEZ_SERVICE ".Adapter1"
#define DEVICE_INTERFACE BLUEZ_SERVICE ".Device1"
#define GATT_CHARACTERISTIC_INTERFACE BLUEZ_SERVICE ".GattCharacteristic1"

struct Bluetooth_Message  {
    char name[248];
    char blue_addr[256];
    gchar* bluetooth_device_path;
    struct Bluetooth_Message  *next;
};


typedef struct {
    bdaddr_t address;
    char name[248];
    char blue_addr[256];
} BluetoothDevice;

void accept_bluetooth(const gchar*  bluetooth_device_path);
void* bluetooth_scan_thread(void* argv);
void disaccept_bluetooth();
void bluetooth_send(/*char* arg*/);
int query_bluetooth_connect_state();
#endif // !