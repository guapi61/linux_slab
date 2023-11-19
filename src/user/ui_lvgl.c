#include "user/ui_lvgl.h"
// LVGL线程函数
lv_img_dsc_t camera_cat_photo = {
  .header.always_zero = 0,
  .header.w = 42,
  .header.h = 43,
  .data_size = 1806 * LV_IMG_PX_SIZE_ALPHA_BYTE,
  .header.cf = LV_IMG_CF_TRUE_COLOR_ALPHA,
  .data = NULL,
};

extern struct Bluetooth_Message* bluetooth_message_head;//头插入

int present_screen = MENU_SCREEN;

static void btn_switch_ui_event(lv_event_t * e);
static void bluetooth_list_btn_event(lv_event_t *e);
static void car_ctr_event(lv_event_t *e);
//static struct 

//enum SCREEN_STATE present_screen = MENU_SCREEN;
static lv_obj_t* bluetooth_message_list;
static lv_obj_t* bluetooth_state;
extern pthread_mutex_t bluetooth_message_mutex;
pthread_mutex_t lvgl_mutex;

static ui_message ui_mesg[] = {
    {MENU_SCREEN,NULL},
    {BULETOOTH_SCREEN,NULL},
    {CAMERA_SCREEN,NULL},
    {CAR_CTR_SCREEN,NULL},
};

typedef struct CAR_BUTTON{
    char* label_text;
    char  ctrl_message;
}CAR_BUTTON;



//static car_ctr_labelto;

static void* refresh_ui(void* argv)//需要动态改变的屏幕界面
{
    while (1)
    {
        pthread_mutex_lock(&lvgl_mutex);
        if(0 == query_bluetooth_connect_state())
        {
            printf("Bluetooth connect state is yes\n");
            lv_obj_set_parent(bluetooth_state,ui_mesg[present_screen].screen);
            lv_label_set_text(bluetooth_state, LV_SYMBOL_BLUETOOTH);
        }else{
            lv_label_set_text(bluetooth_state, "   ");
           // printf("Bluetooth connect state is no\n");
        }
        pthread_mutex_unlock(&lvgl_mutex);


        switch (present_screen)
        {
            case BULETOOTH_SCREEN:
            /* code */    
            pthread_mutex_lock(&bluetooth_message_mutex);
            pthread_mutex_lock(&lvgl_mutex);
            lv_obj_clean(bluetooth_message_list);
            printf("present_screen BULETOOTH_SCREEN\n");
            if(bluetooth_message_head == NULL   ) {
                lv_list_add_btn(bluetooth_message_list,LV_SYMBOL_PAUSE,"Finding..."); 
 
            } else if(bluetooth_message_head != NULL){
                //printf("essssssssss\n");
                int j = 0;
                for(struct Bluetooth_Message* i = bluetooth_message_head;i != NULL; i = i->next)
                {
                    if(j >= 20) break;
                    static char bluetooth_device_path[MAX_SHOW_BLUETOOTH_DEVICE][40];
                    strcpy(bluetooth_device_path[j],i->bluetooth_device_path);
                   
                    char message[256];
                    sprintf(message,"%s    %s\n",i->name,i->blue_addr);
                    //lv_obj_set_width(700);
                    lv_obj_t* bluetooth_button = lv_list_add_btn(bluetooth_message_list,LV_SYMBOL_BLUETOOTH,message);
                    lv_obj_add_event_cb(bluetooth_button,  bluetooth_list_btn_event,LV_EVENT_CLICKED,NULL);
                    lv_obj_add_event_cb(bluetooth_button,  bluetooth_list_btn_event,LV_EVENT_RELEASED,bluetooth_device_path[j]);
                   // printf("Bluetooth %s\n", bluetooth_device_path[j]);
                    j++;
                }

            }
            pthread_mutex_unlock(&lvgl_mutex);
            pthread_mutex_unlock(&bluetooth_message_mutex); 
            break;
        
        case MENU_SCREEN:

            break;
        }
        usleep(3000);
    }
}


void create_car_button(lv_obj_t *parent,const CAR_BUTTON* car_btn_message, lv_coord_t x, lv_coord_t y) {
    lv_obj_t *btn = lv_btn_create(parent);
    lv_obj_set_pos(btn, x, y);
    lv_obj_set_size(btn, LV_DPI_DEF *1.25 , LV_DPI_DEF);
    
    lv_obj_t *label_obj = lv_label_create(btn);
    lv_label_set_text(label_obj, car_btn_message->label_text);

    printf("%d,%c,\n",car_btn_message->ctrl_message,car_btn_message->ctrl_message);


    lv_obj_add_event_cb(btn,  car_ctr_event,LV_EVENT_PRESSED,(void*)&(car_btn_message->ctrl_message));
    lv_obj_add_event_cb(btn,  car_ctr_event,LV_EVENT_RELEASED,NULL);
}

void create_car_buttons(lv_obj_t *parent) {
    const static CAR_BUTTON car_button_labels[] = {
        {"Left Front", 'q'},
        {"Forward",'w'}, 
        {"Right Front",'e'},
        {"Left", 'a'},
        {"Stop", 'p'},
        {"Right", 'd'},
        {"Left Rear", ',' },
        {"Backward", 's'},
        {"Right Rear",'z'},
        {"Start", (char)0x31},
        {"Rotate Left", 'j'},
        {"Rotate Right",'l'}
    };

    int button_count = sizeof(car_button_labels) / sizeof(car_button_labels[0]);
    int buttons_per_row = 3;
    int button_width = LV_HOR_RES / buttons_per_row;
    int button_height = LV_DPI_DEF;

    for (int i = 0; i < button_count; ++i) {
        int row = i / buttons_per_row;
        int col = i % buttons_per_row;
        int x = col * button_width;
        int y = row * button_height;

        printf("...........................\n");
        create_car_button(parent, &car_button_labels[i], x, y);
    }
}

void* lvgl_thread(void* arg) {
    font_normal = LV_FONT_DEFAULT;
    
    pthread_mutex_init(&lvgl_mutex,NULL);

    lv_init();
    fbdev_init();
    // 初始化LVGL界面
    static lv_color_t buf[DISP_BUF_SIZE];
    static lv_disp_draw_buf_t disp_buf;
    lv_disp_draw_buf_init(&disp_buf, buf, NULL, DISP_BUF_SIZE);

    static lv_disp_drv_t disp_drv;
    lv_disp_drv_init(&disp_drv);
    disp_drv.draw_buf   = &disp_buf;
    disp_drv.flush_cb   = fbdev_flush;
    disp_drv.hor_res    = 1024;
    disp_drv.ver_res    = 600;
    lv_disp_drv_register(&disp_drv);
  
    evdev_init();
    lv_indev_drv_t indev_drv_1;
    lv_indev_drv_init(&indev_drv_1); /*Basic initialization*/
    indev_drv_1.type = LV_INDEV_TYPE_POINTER;
    indev_drv_1.read_cb = evdev_read;
    lv_indev_t *mouse_indev = lv_indev_drv_register(&indev_drv_1);
    
    lv_theme_default_init(NULL, lv_palette_main(LV_PALETTE_BLUE), lv_palette_main(LV_PALETTE_RED), LV_THEME_DEFAULT_DARK, font_normal);

    lv_obj_t * name = lv_label_create(lv_scr_act());
    lv_label_set_text(name, "Elena Smith");
    
printf("....111111111111111.......................\n");
    screen_menu = lv_obj_create(NULL);
    ui_mesg[MENU_SCREEN].screen = screen_menu;
    screen_camera = lv_obj_create(NULL);
    ui_mesg[CAMERA_SCREEN].screen = screen_menu;
    screen_bluetooth = lv_obj_create(NULL);
    ui_mesg[BULETOOTH_SCREEN].screen = screen_menu;
    screen_car_ctr = lv_obj_create(NULL);
    ui_mesg[BULETOOTH_SCREEN].screen = screen_menu;

    //general object
    bluetooth_state = lv_label_create(screen_menu);
    lv_obj_set_x(bluetooth_state, 20);
    lv_obj_set_y(bluetooth_state, 20);
    
    //meun UI
    btn_open_camera = lv_btn_create(screen_menu);
    lv_obj_set_size(btn_open_camera, 200, 100);
    lv_obj_set_x(btn_open_camera, 70);
    lv_obj_set_y(btn_open_camera, 70);
    
    btn_open_bluetooth = lv_btn_create(screen_menu);
    lv_obj_set_size(btn_open_bluetooth, 200, 100);
    lv_obj_set_x(btn_open_bluetooth, 290);
    lv_obj_set_y(btn_open_bluetooth, 70);

    btn_open_car_ctr = lv_btn_create(screen_menu);
    lv_obj_set_size(btn_open_car_ctr, 200, 100);
    lv_obj_set_x(btn_open_car_ctr, 510);
    lv_obj_set_y(btn_open_car_ctr, 70);
       
   // lv_obj_set_pos(img1, 70, 70);
    lv_obj_add_event_cb(btn_open_camera,  btn_switch_ui_event,LV_EVENT_CLICKED,NULL);
    lv_obj_add_event_cb(btn_open_camera,  btn_switch_ui_event,LV_EVENT_RELEASED,(void *)CAMERA_START);

   // lv_obj_set_pos(img1, 70, 70);
    lv_obj_add_event_cb(btn_open_bluetooth,  btn_switch_ui_event,LV_EVENT_CLICKED,NULL);
    lv_obj_add_event_cb(btn_open_bluetooth,  btn_switch_ui_event,LV_EVENT_RELEASED,(void *)BLUETOOTH_OPEN);

    lv_obj_add_event_cb(btn_open_car_ctr,  btn_switch_ui_event,LV_EVENT_CLICKED,NULL);
    lv_obj_add_event_cb(btn_open_car_ctr,  btn_switch_ui_event,LV_EVENT_RELEASED,(void *)CAR_CTR_OPEN);

    img1 = lv_img_create(screen_camera);
    lv_img_set_src(img1,&camera_cat_photo);
    lv_obj_set_pos(img1, 0, 0);
    lv_obj_set_size(img1, 800, 400);

    lv_obj_t *bluetooth_open_label = lv_label_create(btn_open_bluetooth);
    lv_label_set_text(bluetooth_open_label, "open bluetooth");    

    lv_obj_t *camera_open_label = lv_label_create(btn_open_camera);
    lv_label_set_text(camera_open_label, "open camera");  

    lv_obj_t *car_ctr_open_label = lv_label_create(btn_open_car_ctr);
    lv_label_set_text(car_ctr_open_label, "open car_ctr"); 
   //camera_ui  

    btn_close_camera = lv_btn_create(screen_camera);
    lv_obj_set_size(btn_close_camera, 200, 100);
    lv_obj_set_x(btn_close_camera, 805);
    lv_obj_set_y(btn_close_camera, 70);
    lv_obj_add_event_cb(btn_close_camera,  btn_switch_ui_event,LV_EVENT_CLICKED,NULL);
    lv_obj_add_event_cb(btn_close_camera,  btn_switch_ui_event,LV_EVENT_RELEASED,(void *)CAMERA_CLOSE);

    lv_obj_t *camera_close_label = lv_label_create(btn_close_camera);
    lv_label_set_text(camera_close_label, "close camera"); 

  // bluetooth ui

    btn_close_bluetooth = lv_btn_create(screen_bluetooth);
    lv_obj_set_size(btn_close_bluetooth, 200, 100);
    lv_obj_set_x(btn_close_bluetooth, 805);
    lv_obj_set_y(btn_close_bluetooth, 70);
    lv_obj_add_event_cb(btn_close_bluetooth,  btn_switch_ui_event,LV_EVENT_CLICKED,NULL);
    lv_obj_add_event_cb(btn_close_bluetooth,  btn_switch_ui_event,LV_EVENT_RELEASED,(void *)BLUETOOTH_CLOSE);

    lv_obj_t *bluetooth_close_label = lv_label_create(btn_close_bluetooth);
    lv_label_set_text(bluetooth_close_label, "close bluetooth"); 

    bluetooth_message_list = lv_list_create(screen_bluetooth);
    lv_obj_set_x(bluetooth_message_list, BLUETOOTH_MESSAGE_NAME_STATE_X);
    lv_obj_set_y(bluetooth_message_list, BLUETOOTH_MESSAGE_NAME_STATE_Y);
    lv_obj_set_width(bluetooth_message_list,256+256/2);
    lv_list_add_btn(bluetooth_message_list,LV_SYMBOL_PAUSE,"Finding..."); 

//car_ctr_ui
    // Create a semi-transparent canvas
    lv_obj_t *canvas = lv_canvas_create(screen_car_ctr);
    lv_obj_set_size(canvas, LV_HOR_RES, LV_VER_RES);
    lv_obj_set_style_bg_color(canvas,lv_color_make(0,0,0),LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(canvas,  LV_PART_MAIN, LV_OPA_50);

    printf("....111111111111111.......................\n");
    create_car_buttons(canvas);

    // 创建按钮
    lv_obj_t * btn_close_car_ctr= lv_btn_create(canvas);
    //lv_obj_align(btn_close_car_ctr, LV_ALIGN_CENTER, 0, 0);
    // 创建按钮标签
    lv_obj_t *label = lv_label_create(btn_close_car_ctr);
    lv_label_set_text(label, "close car_ctrl!");

    lv_obj_set_size(btn_close_car_ctr, 200, 100);
    lv_obj_set_x(btn_close_car_ctr, 800);
    lv_obj_set_y(btn_close_car_ctr, 30);
    lv_obj_set_style_bg_color(btn_close_car_ctr,lv_color_make(128,128,128),LV_PART_MAIN | LV_STATE_DEFAULT);

    lv_obj_add_event_cb(btn_close_car_ctr,  btn_switch_ui_event,LV_EVENT_CLICKED,NULL);
    lv_obj_add_event_cb(btn_close_car_ctr,  btn_switch_ui_event,LV_EVENT_RELEASED,(void *)CAR_CTR_CLOSE);


    pthread_t thread_id;
    pthread_create(&thread_id, NULL, refresh_ui, NULL);
    pthread_detach(thread_id);

    lv_scr_load(screen_menu);
    
    while(1){

        pthread_mutex_lock(&lvgl_mutex);   
        lv_timer_handler();
        //usleep(500);
        pthread_mutex_unlock(&lvgl_mutex);   
       // cnt ++;
    }
    return NULL;
}

static void btn_switch_ui_event(lv_event_t * e){
        if(e->code == LV_EVENT_CLICKED){
        pthread_t uart_message_handler_thread_id;
        lv_anim_t anim;
        //int w = lv_obj_get_width ( btn_open_camera);

        lv_anim_init(&anim);
        lv_anim_set_time(&anim, 100);  // 动画持续时间
        lv_anim_set_var(&anim, btn_open_camera);  // 动画目标对象
      //  lv_anim_set_exec_cb(&anim, (lv_anim_exec_xcb_t)lv_obj_set_width);  // 设置宽度属性
        lv_anim_set_values(&anim, lv_obj_get_width(btn_open_camera), lv_obj_get_width(btn_open_camera) * 1.1);  // 从当前大小到放大 1.1 倍
        lv_anim_set_ready_cb(&anim, NULL);  // 动画结束时不执行回调
        lv_anim_start(&anim);
        //sleep(100);
    }
    if(e->code == LV_EVENT_RELEASED){
        int  user_data = (int)e->user_data;
        pthread_t uart_message_handler_thread_id;
        message_handler_parameter* message_handler_param;
        message_handler_param = message_handler_parameter_malloc();
        message_handler_param->event = user_data;
        message_handler_param->param = NULL; 
        
        pthread_create(&uart_message_handler_thread_id, NULL, message_handler_thread, (void*)message_handler_param);
        pthread_detach(uart_message_handler_thread_id);


    } 
}


static void bluetooth_list_btn_event(lv_event_t *e) 
{
    if(e->code == LV_EVENT_CLICKED){
        lv_anim_t anim;
     //   int w = lv_obj_get_width ( btn_close_camera);
        lv_anim_init(&anim);
        lv_anim_set_time(&anim, 100);  // 动画持续时间
        lv_anim_set_var(&anim, btn_close_camera);  // 动画目标对象
      //  lv_anim_set_exec_cb(&anim, (lv_anim_exec_xcb_t)lv_obj_set_width);  // 设置宽度属性
        lv_anim_set_values(&anim, lv_obj_get_width(btn_close_camera), lv_obj_get_width(btn_close_camera) * 1.1);  // 从当前大小到放大 1.1 倍
        lv_anim_set_ready_cb(&anim, NULL);  // 动画结束时不执行回调
        lv_anim_start(&anim);
       // sleep(100);
    }
    if(e->code == LV_EVENT_RELEASED){     
        message_handler_parameter* message_handler_param;
        message_handler_param = message_handler_parameter_malloc();
        message_handler_param->event = BLUETOOTH_CONNECT;
        message_handler_param->param = e->user_data; 

        pthread_t thread_id;
        pthread_create(&thread_id, NULL, message_handler_thread, (void*)message_handler_param);
        pthread_detach(thread_id);

        //message_handler_parameter_free(message_handler_param);
    
    } 
}


static void car_ctr_event(lv_event_t *e)
{
        
    if(e->code == LV_EVENT_PRESSED){
        char user_data = *(char*)(e->user_data);

        lv_anim_t anim;
     //   int w = lv_obj_get_width ( btn_close_camera);
        lv_anim_init(&anim);
        lv_anim_set_time(&anim, 100);  // 动画持续时间
        lv_anim_set_var(&anim, btn_close_camera);  // 动画目标对象
      //  lv_anim_set_exec_cb(&anim, (lv_anim_exec_xcb_t)lv_obj_set_width);  // 设置宽度属性
        lv_anim_set_values(&anim, lv_obj_get_width(btn_close_camera), lv_obj_get_width(btn_close_camera) * 1.1);  // 从当前大小到放大 1.1 倍
        lv_anim_set_ready_cb(&anim, NULL);  // 动画结束时不执行回调
        lv_anim_start(&anim);
       
        message_handler_parameter* message_handler_param;
        message_handler_param = message_handler_parameter_malloc();
        message_handler_param->event = BLUETOOTH_SEND;
        message_handler_param->param = e->user_data; 


        pthread_t thread_id;
        pthread_create(&thread_id, NULL, message_handler_thread, (void*)message_handler_param);
        pthread_detach(thread_id);

        
    }
    if(e->code == LV_EVENT_RELEASED){

        static char auto_stop = 'p';
        message_handler_parameter* message_handler_param;
        message_handler_param = message_handler_parameter_malloc();
        message_handler_param->event = BLUETOOTH_SEND;
        message_handler_param->param = &auto_stop; 

        pthread_t thread_id;
        pthread_create(&thread_id, NULL, message_handler_thread, (void*)message_handler_param);
        pthread_detach(thread_id);
    } 
}
