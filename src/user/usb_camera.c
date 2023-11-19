#include "user/usb_camera.h"

static int init_flag = 0;
int camera_using = 0;
pthread_mutex_t camera_mutex;
/* 用户定义设备标识 */
struct buffer {
    void *start;
    size_t length;
};

int init_camera()
{
    if(1 == init_flag) return  -1; 
    if (pthread_mutex_init(&camera_mutex, NULL) != 0) {
        perror("Mutex initialization failed");
        exit(1);
    }
    init_flag = 1;
    return 0;
}

int deinit_camera()
{
    pthread_mutex_destroy(&camera_mutex);
    init_flag = 0;
}

// 摄像头线程函数
void* camera_thread(void* arg) {
    // 初始化摄像头

    init_camera();

    pthread_mutex_lock(&camera_mutex);  // 锁住互斥锁
    if(camera_using == 0)  {
        pthread_mutex_unlock(&camera_mutex); 
        return NULL;
    }

    int fd;
    int pictures_fd = 0;
    struct v4l2_capability cap;
    struct v4l2_fmtdesc fmt_desc;
    struct v4l2_format format;
    struct v4l2_frmsizeenum frmsize;
    struct v4l2_control ctrl;
    enum v4l2_buf_type type;
    pthread_t ctr_thread;
    struct buffer buffers[10];


    // 打开摄像头设备
    fd = open(CAMERA_DEVICE, O_RDWR);
    if (fd == -1) {
        perror("无法打开摄像头设备");
       return NULL ;
    }

    ioctl(fd, VIDIOC_G_FMT, &format);
    printf("format.type=%d \n", format.type);

    format.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    format.fmt.pix.width = 800;
    format.fmt.pix.height = 600;
    format.fmt.pix.pixelformat = V4L2_PIX_FMT_MJPEG; // 使用Motion-JPEG格式
    if (ioctl(fd, VIDIOC_S_FMT, &format) == -1) {
        perror("无法设置格式");
      //  close(fd);
      //  return NULL ;
    }

 
    
//请求视频缓冲区
    struct v4l2_requestbuffers reqbuf;// 结构体用于请求视频缓冲区的分配
    reqbuf.count = 10;
    reqbuf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;//
    reqbuf.memory = V4L2_MEMORY_MMAP;
    if (ioctl(fd, VIDIOC_REQBUFS, &reqbuf) == 0) {
        printf("支持 mmap \n count = %d\n", reqbuf.count);
    } else {
        printf("不支持 mmap\n");
    }

   // 查询和映射视频缓冲区
    for (int i = 0; i < reqbuf.count; i++) {
        struct v4l2_buffer buffer;
        memset(&buffer, 0, sizeof(buffer));
        buffer.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        buffer.memory = V4L2_MEMORY_MMAP;
        buffer.index = i;

        if (ioctl(fd, VIDIOC_QUERYBUF, &buffer) == -1) {
            perror("无法查询视频缓冲区");
            close(fd);
            return NULL ;
        }

        buffers[i].length = buffer.length;
        buffers[i].start = mmap(NULL, buffer.length, PROT_READ | PROT_WRITE, MAP_SHARED, fd, buffer.m.offset);

        if (buffers[i].start == MAP_FAILED) {
            perror("内存映射失败");
            close(fd);
            return NULL ;
        }

        // 将缓冲区放入空闲链表
        if (ioctl(fd, VIDIOC_QBUF, &buffer) == -1) {
            perror("无法重新放入缓冲区");
            close(fd);
            return NULL ;
        }
    }

 //   pthread_join(ctr_thread, NULL);

    // 设置对比度
    ctrl.id = V4L2_CID_CONTRAST;
    ctrl.value = 60;  // 设置对比度值，可以根据需要进行调整
    if (ioctl(fd, VIDIOC_S_CTRL, &ctrl) == -1) {
        perror("无法设置对比度");
        close(fd);
        return NULL ;
    }
    
    // 启动捕获
    type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    if (ioctl(fd, VIDIOC_STREAMON, &type) == -1) {
        perror("无法启动捕获");
        close(fd);
        return NULL ;
    }

    // 在这里进行图像捕获和处理
    struct pollfd pollfds[5];
    pollfds[0].fd = fd;
    pollfds[0].events = POLLIN;

    printf("camera open success\n");
    while(camera_using)
    {
        poll(pollfds,1,-1);
        struct v4l2_buffer buffer;

        memset(&buffer, 0, sizeof(buffer));
        buffer.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        buffer.memory = V4L2_MEMORY_MMAP;

        if (ioctl(fd, VIDIOC_DQBUF, &buffer) == -1) {
            perror("无法取出缓冲区");
            break;
        }

        char filename[50]= "./image/cap_pictures.jpg";
        static unsigned char cnt =0;
        sprintf(filename, "./image/cap_pictures%d.jpg",cnt);

        
        if(cnt > 5) cnt = 0;        
         
        if( (pictures_fd = open(filename, O_CREAT | O_RDWR , 0666)) < 0) {
            perror("无法创建文件");
            break;
        }
     
        if( write(pictures_fd, buffers[buffer.index].start, buffers[buffer.index].length) <= 0 ){
            perror("无法写入文件");
            break;
        }

        close(pictures_fd);

        //将转换后的数据传入数组 让lvgl显示
        //decode_jpg(filename,&camera_cat_photo);
        decode_jpg2(buffers[buffer.index].start,&camera_cat_photo,buffers[buffer.index].length);
        // 将已处理的缓冲区重新排入空闲队列
        if (ioctl(fd, VIDIOC_QBUF, &buffer) == -1) {
            perror("无法重新排入缓冲区");
            break;
        }
        //usleep(5000);
        cnt++;
   
        
        lv_timer_handler();
        //lv_canvas_draw_img(canvas, 0, 0, &camera_cat_photo, NULL);

        if(camera_cat_photo.data != NULL && img1 != NULL) 
            lv_img_set_src(img1,&camera_cat_photo);
        
    }
    // 解除视频缓冲区的内存映射
    for (int i = 0; i < reqbuf.count; i++) {
        munmap(buffers[i].start, buffers[i].length);
    }


    // 停止捕获
    if (ioctl(fd, VIDIOC_STREAMOFF, &type) == -1) {
        perror("无法停止捕获");
        close(fd);
        return NULL ;
    }
    printf("camera close success\n");
    // 关闭摄像头设备
    //free(camera_cat_photo.data);  // 不再需要数据时，释放内存
    close(fd);

    pthread_mutex_unlock(&camera_mutex);  // 解锁互斥锁
    return NULL;
}

