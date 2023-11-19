#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include "../include/turbojpeg.h"

#define THROW(action, message) { \
  printf("ERROR in line %d while %s:\n%s\n", __LINE__, action, message); \
  retval = -1;  goto bailout; \
}

#define THROW_TJ(action)  THROW(action, tj3GetErrorStr(tjInstance))
#define THROW_UNIX(action)  THROW(action, strerror(errno))
#define DEFAULT_SUBSAMP  TJSAMP_444
#define DEFAULT_QUALITY  95


const char *subsampName[TJ_NUMSAMP] = {
  "4:4:4", "4:2:2", "4:2:0", "Grayscale", "4:4:0", "4:1:1", "4:4:1"
};

const char *colorspaceName[TJ_NUMCS] = {
  "RGB", "YCbCr", "GRAY", "CMYK", "YCCK"
};

tjscalingfactor *scalingFactors = NULL;
int numScalingFactors = 0;


static void usage(char *programName)
{
  int i;

  printf("\nUSAGE: %s <Input image> <Output image> [options]\n\n",
         programName);

  printf("Input and output images can be in Windows BMP or PBMPLUS (PPM/PGM) format.  If\n");
  printf("either filename ends in a .jpg extension, then the TurboJPEG API will be used\n");
  printf("to compress or decompress the image.\n\n");

  printf("Compression Options (used if the output image is a JPEG image)\n");
  printf("--------------------------------------------------------------\n\n");

  printf("-subsamp <444|422|420|gray> = Apply this level of chrominance subsampling when\n");
  printf("     compressing the output image.  The default is to use the same level of\n");
  printf("     subsampling as in the input image, if the input image is also a JPEG\n");
  printf("     image, or to use grayscale if the input image is a grayscale non-JPEG\n");
  printf("     image, or to use %s subsampling otherwise.\n\n",
         subsampName[DEFAULT_SUBSAMP]);

  printf("-q <1-100> = Compress the output image with this JPEG quality level\n");
  printf("     (default = %d).\n\n", DEFAULT_QUALITY);

  printf("Decompression Options (used if the input image is a JPEG image)\n");
  printf("---------------------------------------------------------------\n\n");

  printf("-scale M/N = Scale the input image by a factor of M/N when decompressing it.\n");
  printf("(M/N = ");
  for (i = 0; i < numScalingFactors; i++) {
    printf("%d/%d", scalingFactors[i].num, scalingFactors[i].denom);
    if (numScalingFactors == 2 && i != numScalingFactors - 1)
      printf(" or ");
    else if (numScalingFactors > 2) {
      if (i != numScalingFactors - 1)
        printf(", ");
      if (i == numScalingFactors - 2)
        printf("or ");
    }
  }
  printf(")\n\n");

  printf("-hflip, -vflip, -transpose, -transverse, -rot90, -rot180, -rot270 =\n");
  printf("     Perform one of these lossless transform operations on the input image\n");
  printf("     prior to decompressing it (these options are mutually exclusive.)\n\n");

  printf("-grayscale = Perform lossless grayscale conversion on the input image prior\n");
  printf("     to decompressing it (can be combined with the other transform operations\n");
  printf("     above.)\n\n");

  printf("-crop WxH+X+Y = Perform lossless cropping on the input image prior to\n");
  printf("     decompressing it.  X and Y specify the upper left corner of the cropping\n");
  printf("     region, and W and H specify the width and height of the cropping region.\n");
  printf("     X and Y must be evenly divible by the MCU block size (8x8 if the input\n");
  printf("     image was compressed using no subsampling or grayscale, 16x8 if it was\n");
  printf("     compressed using 4:2:2 subsampling, or 16x16 if it was compressed using\n");
  printf("     4:2:0 subsampling.)\n\n");

  printf("General Options\n");
  printf("---------------\n\n");

  printf("-fastupsample = Use the fastest chrominance upsampling algorithm available\n\n");

  printf("-fastdct = Use the fastest DCT/IDCT algorithm available\n\n");

  exit(1);
}


static int customFilter(short *coeffs, tjregion arrayRegion,
                        tjregion planeRegion, int componentIndex,
                        int transformIndex, tjtransform *transform)
{
  int i;

  for (i = 0; i < arrayRegion.w * arrayRegion.h; i++)
    coeffs[i] = -coeffs[i];

  return 0;
}

uint16_t rgb888_to_rgb565(uint8_t r, uint8_t g, uint8_t b) {
    // 使用位操作来构建 RGB565 颜色
    uint16_t r5 = (r >> 3) & 0x1F;   // 取高 5 位
    uint16_t g6 = (g >> 2) & 0x3F;   // 取高 6 位
    uint16_t b5 = (b >> 3) & 0x1F;   // 取高 5 位
    // 合并为 RGB565
    uint16_t rgb565 = (r5 << 11) | (g6 << 5) | b5;
    return rgb565;
}

int decode_jpg(char *argv, lv_img_dsc_t* img_lvgl)
{
  tjscalingfactor scalingFactor = TJUNSCALED;
  int outSubsamp = -1, outQual = -1;
  tjtransform xform;
  int fastUpsample = 0, fastDCT = 0;
  int width, height;
  char *inFormat, *outFormat;
  FILE *jpegFile = NULL;
  unsigned char *imgBuf = NULL, *jpegBuf = NULL;
  int retval = 0, i, pixelFormat = TJPF_UNKNOWN;
  tjhandle tjInstance = NULL;

  if ((scalingFactors = tj3GetScalingFactors(&numScalingFactors)) == NULL)
    THROW_TJ("getting scaling factors");
  memset(&xform, 0, sizeof(tjtransform));


  /* Parse arguments. */
  // for (i = 3; i < argc; i++) {
  //   if (!strncasecmp(argv[i], "-sc", 3) && i < argc - 1) {
  //     int match = 0, temp1 = 0, temp2 = 0, j;

  //     if (sscanf(argv[++i], "%d/%d", &temp1, &temp2) < 2)
  //       usage(argv[0]);
  //     for (j = 0; j < numScalingFactors; j++) {
  //       if ((double)temp1 / (double)temp2 == (double)scalingFactors[j].num /
  //                                            (double)scalingFactors[j].denom) {
  //         scalingFactor = scalingFactors[j];
  //         match = 1;
  //         break;
  //       }
  //     }
  //     if (match != 1)
  //       usage(argv[0]);
  //   } else if (!strncasecmp(argv[i], "-su", 3) && i < argc - 1) {
  //     i++;
  //     if (!strncasecmp(argv[i], "g", 1))
  //       outSubsamp = TJSAMP_GRAY;
  //     else if (!strcasecmp(argv[i], "444"))
  //       outSubsamp = TJSAMP_444;
  //     else if (!strcasecmp(argv[i], "422"))
  //       outSubsamp = TJSAMP_422;
  //     else if (!strcasecmp(argv[i], "420"))
  //       outSubsamp = TJSAMP_420;
  //     else
  //       usage(argv[0]);
  //   } else if (!strncasecmp(argv[i], "-q", 2) && i < argc - 1) {
  //     outQual = atoi(argv[++i]);
  //     if (outQual < 1 || outQual > 100)
  //       usage(argv[0]);
  //   } else if (!strncasecmp(argv[i], "-g", 2))
  //     xform.options |= TJXOPT_GRAY;
  //   else if (!strcasecmp(argv[i], "-hflip"))
  //     xform.op = TJXOP_HFLIP;
  //   else if (!strcasecmp(argv[i], "-vflip"))
  //     xform.op = TJXOP_VFLIP;
  //   else if (!strcasecmp(argv[i], "-transpose"))
  //     xform.op = TJXOP_TRANSPOSE;
  //   else if (!strcasecmp(argv[i], "-transverse"))
  //     xform.op = TJXOP_TRANSVERSE;
  //   else if (!strcasecmp(argv[i], "-rot90"))
  //     xform.op = TJXOP_ROT90;
  //   else if (!strcasecmp(argv[i], "-rot180"))
  //     xform.op = TJXOP_ROT180;
  //   else if (!strcasecmp(argv[i], "-rot270"))
  //     xform.op = TJXOP_ROT270;
  //   else if (!strcasecmp(argv[i], "-custom"))
  //     xform.customFilter = customFilter;
  //   else if (!strncasecmp(argv[i], "-c", 2) && i < argc - 1) {
  //     if (sscanf(argv[++i], "%dx%d+%d+%d", &xform.r.w, &xform.r.h, &xform.r.x,
  //                &xform.r.y) < 4 ||
  //         xform.r.x < 0 || xform.r.y < 0 || xform.r.w < 1 || xform.r.h < 1)
  //       usage(argv[0]);
  //     xform.options |= TJXOPT_CROP;
  //   } else if (!strcasecmp(argv[i], "-fastupsample")) {
  //     printf("Using fast upsampling code\n");
  //     fastUpsample = 1;
  //   } else if (!strcasecmp(argv[i], "-fastdct")) {
  //     printf("Using fastest DCT/IDCT algorithm\n");
  //     fastDCT = 1;
  //   } else usage(argv[0]);
  // }

  /* Determine input and output image formats based on file extensions. */
  inFormat = strrchr(argv, '.');
 
  inFormat = &inFormat[1];
  
  if (!strcasecmp(inFormat, "jpg")) {
       /* Input image is a JPEG image.  Decompress and/or transform it. */
    long size;
    int inSubsamp, inColorspace;
    int doTransform = (xform.op != TJXOP_NONE || xform.options != 0 ||
                       xform.customFilter != NULL);
    size_t jpegSize;

    /* Read the JPEG file into memory. */
    if ((jpegFile = fopen(argv, "rb")) == NULL)
      THROW_UNIX("opening input file");
    if (fseek(jpegFile, 0, SEEK_END) < 0 || ((size = ftell(jpegFile)) < 0) ||
        fseek(jpegFile, 0, SEEK_SET) < 0)
      THROW_UNIX("determining input file size");
    if (size == 0)
      THROW("determining input file size", "Input file contains no data");
    jpegSize = size;
    if ((jpegBuf = tj3Alloc(jpegSize)) == NULL)
      THROW_UNIX("allocating JPEG buffer");
    if (fread(jpegBuf, jpegSize, 1, jpegFile) < 1)
      THROW_UNIX("reading input file");
    fclose(jpegFile);  jpegFile = NULL;

    if (doTransform) {
      /* Transform it. */
      
      unsigned char *dstBuf = NULL;  /* Dynamically allocate the JPEG buffer */
      size_t dstSize = 0;

      if ((tjInstance = tj3Init(TJINIT_TRANSFORM)) == NULL)
        THROW_TJ("initializing transformer");
      xform.options |= TJXOPT_TRIM;
      if (tj3Transform(tjInstance, jpegBuf, jpegSize, 1, &dstBuf, &dstSize,&xform) < 0) {
        tj3Free(dstBuf);
        THROW_TJ("transforming input image");
      }
      tj3Free(jpegBuf);
      jpegBuf = dstBuf;
      jpegSize = dstSize;
    } else {
      if ((tjInstance = tj3Init(TJINIT_DECOMPRESS)) == NULL)
        THROW_TJ("initializing decompressor");
    }
    if (tj3Set(tjInstance, TJPARAM_FASTUPSAMPLE, fastUpsample) < 0)
      THROW_TJ("setting TJPARAM_FASTUPSAMPLE");
    if (tj3Set(tjInstance, TJPARAM_FASTDCT, fastDCT) < 0)
      THROW_TJ("setting TJPARAM_FASTDCT");

    if (tj3DecompressHeader(tjInstance, jpegBuf, jpegSize) < 0)
      THROW_TJ("reading JPEG header");
    width = tj3Get(tjInstance, TJPARAM_JPEGWIDTH);
    height = tj3Get(tjInstance, TJPARAM_JPEGHEIGHT);
    inSubsamp = tj3Get(tjInstance, TJPARAM_SUBSAMP);
    inColorspace = tj3Get(tjInstance, TJPARAM_COLORSPACE);

    if (tj3Get(tjInstance, TJPARAM_LOSSLESS))
      scalingFactor = TJUNSCALED;

    // printf("%s Image:%s  %d x %d pixels, %s subsampling, %s colorspace\n",
    //        (doTransform ? "Transformed" : "Input"), argv,width, height,
    //        subsampName[inSubsamp], colorspaceName[inColorspace]);
    img_lvgl->header.h = height;
    img_lvgl->header.w = width;



    
    //Decoded picture
    uint8_t* img_data_888 = NULL;
    if(NULL == img_data_888)//888
      img_data_888 = (uint8_t*)malloc(width * height * 4);  // BGR565 image
    else
      img_data_888 = (uint8_t*)realloc((void*)img_data_888,width * height * 4);

    static uint8_t* img_data_565 = NULL;
    if(NULL == img_data_565)//565
      img_data_565 = (uint8_t*)malloc(width * height * 3);  // BGR565 + A
    else
      img_data_565 = (uint8_t*)realloc((void*)img_data_565,width * height * 3);  

    if( 0 != tjDecompress2(tjInstance, jpegBuf, jpegSize, (void*)img_data_888, width, 0, height, TJPF_RGBA, 0) )
    {
      printf("tjDecompress2 failed\n");
    }
   
    for(int i=0,j =0; i  < width * height * 4 ; i+=4 , j+=3)
    {
      uint16_t color_565 = rgb888_to_rgb565(img_data_888[i],img_data_888[i+1],img_data_888[i+2]);
      
      img_data_565[j] = (color_565) & 0xFF;
      img_data_565[j+1] = (color_565 >> 8) & 0xFF;
      img_data_565[j+2] = img_data_888[i+3];

    
    }
 
    
    img_lvgl->data = img_data_565;   
    img_lvgl->data_size = width * height * LV_IMG_PX_SIZE_ALPHA_BYTE;
    img_lvgl->header.cf = LV_IMG_CF_TRUE_COLOR_ALPHA;
    img_lvgl->header.always_zero = 0,
    img_lvgl->header.reserved = 0,
 
  
    // 释放 TurboJPEG 资源
    //free(show_buf);
    free(img_data_888);
  }

  bailout:
  
  tj3Free(imgBuf);
  tj3Destroy(tjInstance);
  tj3Free(jpegBuf);
  if (jpegFile) fclose(jpegFile);

  
  return retval;
}


int decode_jpg2(unsigned char * jpegBuf, lv_img_dsc_t* img_lvgl,size_t jpegSize)
{
  tjscalingfactor scalingFactor = TJUNSCALED;
  int outSubsamp = -1, outQual = -1;
  tjtransform xform;
  int fastUpsample = 0, fastDCT = 0;
  int width, height;
  char *inFormat, *outFormat;
  FILE *jpegFile = NULL;
  int retval = 0, i, pixelFormat = TJPF_UNKNOWN;
  tjhandle tjInstance = NULL;

  int inSubsamp, inColorspace;

  /* Determine input and output image formats based on file extensions. */ 
    if ((tjInstance = tj3Init(TJINIT_DECOMPRESS)) == NULL);


    if (tj3Set(tjInstance, TJPARAM_FASTUPSAMPLE, fastUpsample) < 0);
 
    if (tj3Set(tjInstance, TJPARAM_FASTDCT, fastDCT) < 0);
      

    if (tj3DecompressHeader(tjInstance, jpegBuf, jpegSize) < 0);

    width = tj3Get(tjInstance, TJPARAM_JPEGWIDTH);
    height = tj3Get(tjInstance, TJPARAM_JPEGHEIGHT);
    inSubsamp = tj3Get(tjInstance, TJPARAM_SUBSAMP);
    inColorspace = tj3Get(tjInstance, TJPARAM_COLORSPACE);

    if (tj3Get(tjInstance, TJPARAM_LOSSLESS))
      scalingFactor = TJUNSCALED;

    // printf("%s Image:%s  %d x %d pixels, %s subsampling, %s colorspace\n",
    //        (doTransform ? "Transformed" : "Input"), argv,width, height,
    //        subsampName[inSubsamp], colorspaceName[inColorspace]);
    img_lvgl->header.h = height;
    img_lvgl->header.w = width;

    //Decoded picture
    uint8_t* img_data_888 = NULL;
    if(NULL == img_data_888)//888
      img_data_888 = (uint8_t*)malloc(width * height * 4);  // BGR565 image
    else
      img_data_888 = (uint8_t*)realloc((void*)img_data_888,width * height * 4);

    static uint8_t* img_data_565 = NULL;
    if(NULL == img_data_565)//565
      img_data_565 = (uint8_t*)malloc(width * height * 3);  // BGR565 + A
    else
      img_data_565 = (uint8_t*)realloc((void*)img_data_565,width * height * 3);  

    if( 0 != tjDecompress2(tjInstance, jpegBuf, jpegSize, (void*)img_data_888, width, 0, height, TJPF_RGBA, 0) )
    {
      printf("tjDecompress2 failed\n");
    }
   
    for(int i=0,j =0; i  < width * height * 4 ; i+=4 , j+=3)
    {
      uint16_t color_565 = rgb888_to_rgb565(img_data_888[i],img_data_888[i+1],img_data_888[i+2]);
      
      img_data_565[j] = (color_565) & 0xFF;
      img_data_565[j+1] = (color_565 >> 8) & 0xFF;
      img_data_565[j+2] = img_data_888[i+3];
    
    }
 
    
    img_lvgl->data = img_data_565;   
    img_lvgl->data_size = width * height * LV_IMG_PX_SIZE_ALPHA_BYTE;
    img_lvgl->header.cf = LV_IMG_CF_TRUE_COLOR_ALPHA;
    img_lvgl->header.always_zero = 0,
    img_lvgl->header.reserved = 0,

    // 释放 TurboJPEG 资源
    //free(show_buf);
    free(img_data_888);

  return 0;
}