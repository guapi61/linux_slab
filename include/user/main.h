#ifndef MAIN_H
#define MAIN_H
typedef struct Message_Handler_Parameter{
    int event ;
    void *param;
}message_handler_parameter;

enum message_handler{
CAMERA_CLOSE   ,
CAMERA_START   ,
BLUETOOTH_OPEN ,
BLUETOOTH_CLOSE ,
BLUETOOTH_CONNECT ,
BLUETOOTH_DISCONNECT ,
BLUETOOTH_SEND,
CAR_CTR_OPEN,
CAR_CTR_CLOSE,
};

void* message_handler_thread(void* param);//
message_handler_parameter* message_handler_parameter_malloc();
void message_handler_parameter_free(message_handler_parameter* message_handler_param);

#endif // !1
