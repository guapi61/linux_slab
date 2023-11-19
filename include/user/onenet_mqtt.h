#ifndef  USER_ONENET
#define  USER_ONENET
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

#include "test/mqtt_config.h"
#include "common/mqtt_log.h"
#include "mqttclient/mqttclient.h"
#include "cJSON.h"

void* mqtt_thread(void* arg);
#endif // ! USER_ONENET