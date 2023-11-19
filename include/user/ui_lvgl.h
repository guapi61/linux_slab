#ifndef  USER_LVGL_UI
#define  USER_LVGL_UI 1

#include "lvgl/lvgl.h"
#include "lv_drivers/display/fbdev.h"
#include "lv_drivers/indev/evdev.h"
#include "user/blue_tooth.h"
#include "user/main.h"
#include <pthread.h>
#include <semaphore.h>
#include <bluetooth/bluetooth.h> 
#include <bluetooth/hci.h>
#include <bluetooth/hci_lib.h>

#define CAMERA_DEVICE "/dev/video0"
#define DISP_BUF_SIZE (128 * 1024)

#define BLUETOOTH_MESSAGE_NAME_STATE_X 20
#define BLUETOOTH_MESSAGE_NAME_STATE_Y 20
#define BLUETOOTH_MESSAGE_ADDR_STATE_X BLUETOOTH_MESSAGE_NAME_STATE_X + 380
#define BLUETOOTH_MESSAGE_ADDR_STATE_Y BLUETOOTH_MESSAGE_NAME_STATE_Y 
#define BLUETOOTH_MESSAGE_X_OFFSET 60
#define BLUETOOTH_MESSAGE_Y_OFFSET 60

#define MAX_SHOW_BLUETOOTH_DEVICE 20

lv_obj_t *img1 ;
lv_obj_t *screen_menu ;
lv_obj_t* screen_car_ctr;
lv_obj_t *screen_camera ;
lv_obj_t *screen_bluetooth;
lv_obj_t *btn_open_camera;
lv_obj_t *btn_close_camera;
lv_obj_t *btn_open_bluetooth;
lv_obj_t *btn_close_bluetooth;
lv_obj_t *btn_open_car_ctr;
lv_img_dsc_t camera_cat_photo;

static const lv_font_t * font_normal;


enum SCREEN_STATE{
    MENU_SCREEN,
    BULETOOTH_SCREEN ,
    CAMERA_SCREEN,
    CAR_CTR_SCREEN ,
};


typedef struct UI_MESSAGE 
{
    enum SCREEN_STATE what_screen;
    lv_obj_t *screen;
}ui_message;


void* lvgl_thread(void* arg);
#endif // ! USER_LVGL_UI