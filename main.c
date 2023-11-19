#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdlib.h>
#include <pthread.h>
#include <time.h>
#include <sys/time.h>
#include <linux/videodev2.h>
#include <sys/mman.h>
#include <poll.h>
#include <string.h>
#include <pthread.h>
#include <sys/ioctl.h>
#include <termios.h>
#include <semaphore.h>

#include "user/blue_tooth.h"
#include "user/voice_control.h"
#include "user/ui_lvgl.h"
#include "user/onenet_mqtt.h"
#include "user/usb_camera.h"
#include "user/main.h"

extern int camera_using ;
extern int bluetooth_scan_using;
extern int present_screen ;

message_handler_parameter* message_handler_parameter_malloc()
{
    message_handler_parameter* message_handler_param;
    message_handler_param = malloc(sizeof(message_handler_parameter));
    message_handler_param->event = CAMERA_START;
    message_handler_param->param = NULL; 

    return(message_handler_param);
}

void message_handler_parameter_free(message_handler_parameter* message_handler_param)
{

    if(message_handler_param != NULL ) free(message_handler_param);
}

void* message_handler_thread(void* param_){
    message_handler_parameter* param = (message_handler_parameter*)param_;
    int event  = -1;
    void* arg = NULL;
    event = param->event;
    if(param->param != NULL) arg = param->param;
    pthread_t thread_id;
   
    switch ( event )
    {
    case CAMERA_CLOSE:    
        camera_using = 0;
        lv_scr_load(screen_menu);
        present_screen = MENU_SCREEN;
        break;
    
    case CAMERA_START:
        camera_using = 1;
        lv_scr_load(screen_camera);
        camera_thread(NULL);
        present_screen = CAMERA_SCREEN;
        break;

    case BLUETOOTH_OPEN:
        lv_scr_load(screen_bluetooth);
        bluetooth_scan_using =1;
        present_screen = BULETOOTH_SCREEN;
        bluetooth_scan_thread(NULL);
        
        break;
    
    case BLUETOOTH_CONNECT:
        if(arg!= NULL) accept_bluetooth(arg);
        break;

    case BLUETOOTH_DISCONNECT:
        disaccept_bluetooth();
        break;
    case BLUETOOTH_SEND:
        if(arg!= NULL) bluetooth_send(arg);
        break;
    case BLUETOOTH_CLOSE:
        bluetooth_scan_using = 0;
        lv_scr_load(screen_menu);
        present_screen = MENU_SCREEN;
        break;
    case CAR_CTR_OPEN:
        lv_scr_load(screen_car_ctr);
        present_screen = CAR_CTR_SCREEN;
        break;
    case CAR_CTR_CLOSE:
        lv_scr_load(screen_menu);
        present_screen = MENU_SCREEN;
        break;
    }

    if(param_ != NULL)message_handler_parameter_free(param_);
} 



int main() 
{
    pthread_t lvgl_thread_id, bluetooth_thread_id, mqtt_thread_id,uart_contorl_thread_id;
    

    // 创建LVGL线程
    pthread_create(&lvgl_thread_id, NULL, lvgl_thread, NULL);

    // 创建摄像头线程
   // pthread_create(&camera_thread_id, NULL, camera_thread, NULL);

    // 创建MQTT线程
    pthread_create(&mqtt_thread_id, NULL, mqtt_thread, NULL);

    // 创建串口线程
    pthread_create(&uart_contorl_thread_id, NULL, uart_contorl_thread, NULL);

    // 主线程等待其他线程结束
    pthread_join(lvgl_thread_id, NULL);
  //  pthread_join(camera_thread_id, NULL);
    pthread_join(mqtt_thread_id, NULL);
    pthread_join(uart_contorl_thread_id, NULL);
    return 0;
}

/*Set in lv_conf.h as `LV_TICK_CUSTOM_SYS_TIME_EXPR`  using to lvgl*/ 
uint32_t custom_tick_get(void)
{
    static uint64_t start_ms = 0;
    if(start_ms == 0) {
        struct timeval tv_start;
        gettimeofday(&tv_start, NULL);
        start_ms = (tv_start.tv_sec * 1000000 + tv_start.tv_usec) / 1000;
    }

    struct timeval tv_now;
    gettimeofday(&tv_now, NULL);
    uint64_t now_ms;
    now_ms = (tv_now.tv_sec * 1000000 + tv_now.tv_usec) / 1000;

    uint32_t time_ms = now_ms - start_ms;
    return time_ms;
}


