#ifndef  USER_VOICE_CONTROL
#define  USER_VOICE_CONTROL
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
#include <sys/ioctl.h>
#include <termios.h>

#include "user/main.h"
void* uart_contorl_thread(void* arg);

#endif // ! USER_VOICE_CONTROL