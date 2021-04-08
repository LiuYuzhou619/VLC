#include <cv.h>  
#include <highgui.h>  
#include <cxcore.h> 
#include <highgui.h>
#include <iostream>
#include <opencv2/opencv.hpp>
#include <fstream>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <unistd.h>
#include <fcntl.h>


//definaton of Xil_Out32()
#define PAGE_SIZE ((size_t)getpagesize())
#define PAGE_MASK ((uint64_t) (long)~(PAGE_SIZE-1))

//device baseaddr
#define VGA_BASEADDR  0x43C00000
#define DDR_BASEADDR  0x00000000
#define VDMA_BASEADDR 0x43000000

//vga resolving power
#define H_STRIDE 640
#define H_ACTIVE 640
#define V_ACTIVE 480
#define VIDEO_LENGTH (H_STRIDE*V_ACTIVE)

//ddr writen address
#define VIDEO_BASEADDR0  DDR_BASEADDR+0X2000000
#define VIDEO_BASEADDR1  DDR_BASEADDR+0X3000000
#define VIDEO_BASEADDR2  DDR_BASEADDR+0X4000000

#define MY_AXI_VGA_IP_S00_AXI_SLV_REG0_OFFSET 0
#define MY_AXI_VGA_IP_S00_AXI_SLV_REG1_OFFSET 4
#define MY_AXI_VGA_IP_S00_AXI_SLV_REG2_OFFSET 8
#define MY_AXI_VGA_IP_S00_AXI_SLV_REG3_OFFSET 12
#define MY_AXI_VGA_IP_S00_AXI_SLV_REG4_OFFSET 16
#define MY_AXI_VGA_IP_S00_AXI_SLV_REG5_OFFSET 20
#define MY_AXI_VGA_IP_S00_AXI_SLV_REG6_OFFSET 24
#define MY_AXI_VGA_IP_S00_AXI_SLV_REG7_OFFSET 28

using namespace std;
using namespace cv;

void show_img(uint32_t disp_base_addr, unsigned char *addr);
void Xil_Out32(uint64_t io_addr,uint32_t val);

int key;
int delay=30; //ms

CvCapture * cap;
IplImage* img;

int main()
{

//VDMA config
   Xil_Out32((VDMA_BASEADDR + 0x000),0x00000003);

   Xil_Out32((VDMA_BASEADDR + 0x05c),VIDEO_BASEADDR0);
   Xil_Out32((VDMA_BASEADDR + 0x060),VIDEO_BASEADDR0);
   Xil_Out32((VDMA_BASEADDR + 0x064),VIDEO_BASEADDR0);

   Xil_Out32((VDMA_BASEADDR + 0x058),(H_STRIDE*4));
   Xil_Out32((VDMA_BASEADDR + 0x054),(H_ACTIVE*4));

   Xil_Out32((VDMA_BASEADDR + 0x050),V_ACTIVE);

//vga config
   Xil_Out32((VGA_BASEADDR + MY_AXI_VGA_IP_S00_AXI_SLV_REG1_OFFSET),(uint32_t)((803<<16)|525));   
   Xil_Out32((VGA_BASEADDR + MY_AXI_VGA_IP_S00_AXI_SLV_REG2_OFFSET),(uint32_t)((96<<16)|2));
   Xil_Out32((VGA_BASEADDR + MY_AXI_VGA_IP_S00_AXI_SLV_REG3_OFFSET),(uint32_t)((48<<16)|33));
   Xil_Out32((VGA_BASEADDR + MY_AXI_VGA_IP_S00_AXI_SLV_REG4_OFFSET),(uint32_t)((640<<16)|480));

   Xil_Out32((VGA_BASEADDR + MY_AXI_VGA_IP_S00_AXI_SLV_REG0_OFFSET),(uint32_t)(0X04));
  
   
   cap=cvCaptureFromCAM(-1);
   if(!cap)
    {
        cout<<"create camera capture error"<<endl;
        system("pause");
        cvReleaseImage(&img);
        exit(-1);
    }
    while(1)
    {
        img=cvQueryFrame(cap);
        if(!img)
            cout << "fail to get picture" <<endl;
        else
        {
            cout << "got a picture" <<endl;
            img->origin=1; 
            show_img(VIDEO_BASEADDR0,(unsigned char*)img->imageData);
            cout<<"show the picture"<<endl;
            //system("pause");
        }
        key = waitKey(delay);  
        if(27 == key || 'Q' == key || 'q' == key)  
        break;  
    }
    cvReleaseImage(&img);
   return 0;
 }

 
void show_img(uint32_t disp_base_addr, unsigned char *addr)
{
    uint32_t i=0;
    uint32_t j=0;
    uint32_t r,g,b;
    int fd;
    volatile uint8_t *map_base; 
    uint64_t base = disp_base_addr & PAGE_MASK;
    uint64_t pgoffset = disp_base_addr & (~PAGE_MASK);
    if((fd = open("/dev/mem",O_RDWR|O_SYNC)) == -1)
    {
        perror(" open failed");
        exit(-1);
    }
    map_base = (volatile uint8_t *)mmap(NULL,300*PAGE_SIZE,PROT_READ|PROT_WRITE,MAP_SHARED,fd,base);
    if(map_base == MAP_FAILED)
    {
        perror(" mmap failed");
        exit(-1);
    }
    for(j=0;j<480;j++)
    {
    for(i=0;i<640;i++)
    {
        b = *(addr+(i+j*640)*3+0);
        g = *(addr+(i+j*640)*3+1);
        r = *(addr+(i+j*640)*3+2);
        *(volatile uint32_t *)(map_base + pgoffset+(i+j*H_STRIDE)*4)=((r<<24)|(g<<16)|(b<<8)|0x0 );
    }
   }
   close(fd);
   munmap((void *)map_base,PAGE_SIZE);
}

void Xil_Out32(uint64_t io_addr,uint32_t val)
{
   int fd;
   volatile uint8_t *map_base;  
   uint64_t base = io_addr & PAGE_MASK;
   uint64_t pgoffset = io_addr & (~PAGE_MASK);

   if((fd = open("/dev/mem",O_RDWR|O_SYNC)) == -1)
   {
    perror(" open failed");
   }

   map_base = (volatile uint8_t *)mmap(NULL,PAGE_SIZE,PROT_READ|PROT_WRITE,MAP_SHARED,fd,base);

   if(map_base == MAP_FAILED)
   {
    perror(" mmap failed");
   }

   *(volatile uint32_t *)(map_base + pgoffset) = val;
   close(fd);
   
   munmap((void *)map_base,PAGE_SIZE);
}