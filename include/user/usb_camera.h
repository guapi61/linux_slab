#ifndef  USB_CAMERA
#define  USB_CAMERA

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

#include "user/ui_lvgl.h"
#include "turbojpeg.h"


void* camera_thread(void* arg);
#endif // ! USB_CAMERA