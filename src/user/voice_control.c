#include "user/voice_control.h"
#include "user/main.h"
extern int camera_using;

/* set_opt(fd,115200,8,'N',1) */
int set_opt(int fd)
{
	struct termios newtio,oldtio;
	
	if ( tcgetattr( fd,&oldtio) != 0) { 
		perror("SetupSerial 1");
		return -1;
	}
	
	bzero( &newtio, sizeof( newtio ) );
	newtio.c_cflag |= CLOCAL | CREAD; 
	newtio.c_cflag &= ~CSIZE; 
	newtio.c_lflag  &= ~(ICANON | ECHO | ECHOE | ISIG);  /*Input*/
	newtio.c_oflag  &= ~OPOST;   /*Output*/
    newtio.c_cflag |= CS8;
    newtio.c_cflag &= ~PARENB;

	cfsetispeed(&newtio, B115200);
	cfsetospeed(&newtio, B115200);

	newtio.c_cflag &= ~CSTOPB;
	newtio.c_cc[VMIN]  = 1;  /* 读数据时的最小字节数: 没读到这些数据我就不返回! */
	newtio.c_cc[VTIME] = 0; /* 等待第1个数据的时间: 
	                         * 比如VMIN设为10表示至少读到10个数据才返回,
	                         * 但是没有数据总不能一直等吧? 可以设置VTIME(单位是10秒)
	                         * 假设VTIME=1，表示: 
	                         *    10秒内一个数据都没有的话就返回
	                         *    如果10秒内至少读到了1个字节，那就继续等待，完全读到VMIN个数据再返回
	                         */
	tcflush(fd,TCIFLUSH);
	
	if((tcsetattr(fd,TCSANOW,&newtio))!=0)
	{
		perror("com set error");
		return -1;
	}
	//printf("set done!\n");
	return 0;
}

void* uart_contorl_thread(void* arg)
{
    int fd;
	int iRet;
	char c[1024];
    memset(c, 0, sizeof(c));
    pthread_t uart_message_handler_thread_id;
	/* 1. open */

	/* 2. setup 
	 * 115200,8N1
	 * RAW mode
	 * return data immediately
	 */

	/* 3. write and read */
	fd = open("/dev/ttySTM3", O_RDWR|O_NOCTTY); 
    if (-1 == fd){
        printf("open /dev/ttySTM3 failed\n");
		return NULL;
    }

    if(fcntl(fd, F_SETFL, 0)<0) /* 设置串口为阻塞状态*/
	{
		printf("fcntl failed!\n");
		return NULL;
	}


	iRet = set_opt(fd);
	if (iRet)
	{
		printf("set port err!\n");
		return NULL;
	}

	printf("Enter string: ");
	while (1)
	{
		//scanf("%s", &c);

		//iRet = write(fd, &c,strlen(c));
        memset(c, 0, sizeof(c));
        iRet = 0;
		iRet = read(fd, &c, 20);

        if(strcmp(c,"close camera") == 0){
            if(camera_using == 1){
                pthread_create(&uart_message_handler_thread_id, NULL, message_handler_thread, (void*)CAMERA_CLOSE);
                pthread_detach(uart_message_handler_thread_id);
            }
        } else if(strcmp(c,"open camera") == 0){
            if(camera_using == 0){      
                pthread_create(&uart_message_handler_thread_id, NULL, message_handler_thread, (void*)CAMERA_START);
                pthread_detach(uart_message_handler_thread_id);
            }
        } 


		if (iRet > 0)
			printf("get:  %s\n",c);
		else
			printf("can not get data\n");
	}

	return NULL;
}