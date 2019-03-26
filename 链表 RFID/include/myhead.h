//把系统中常用的头文件全部都包含，然后丢到系统头文件的环境变量中/usr/include，那么以后就可以将myhead.h当成系统提供的一个头文件
#ifndef _MYHEAD_H
#define _MYHEAD_H
#include <stdio.h>
#include <time.h>
#include <errno.h>
#include <stdbool.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <strings.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <dirent.h>
#include <linux/input.h>//跟输入子系统模型有关的头文件
#include <sys/mman.h>
#include <sys/wait.h>//跟wait有关
#include <signal.h>//跟信号有关的头文件
#include <sys/shm.h>//跟共享内存有关的头文件
#include <sys/ipc.h>
#include <sys/msg.h>
#include <sys/sem.h>
#include <pthread.h>
#include <semaphore.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include <sys/select.h>
#include <poll.h>
#include <termios.h>

#include <sys/select.h>
#include <stropts.h>
#include <poll.h>

#include "sqlite3.h"

int cardid;
static struct timeval timeout;
sqlite3 *mysq;



#define DEV_PATH   "/dev/ttySAC1"


/* 设置窗口参数:9600速率 */
void init_tty(int fd);
/*计算校验和*/
unsigned char CalBCC(unsigned char *buf, int n);
//封装发送A命令的函数
int PiccRequest(int fd);
/*防碰撞，获取范围内最大ID*/
int PiccAnticoll(int fd);
// 自定义函数，专门用于显示800*480bmp格式的图片
int show_fullbmp(char *bmpname);
//自定义时间函数
int gettime();
// 自定义函数，任意位置显示任意大小的bmp
int show_shapebmp(int x,int y,int w,int h,char *bmpname);
/*将任意大小的图片缩小成某种比例
	参数介绍：x ，y --》左上角x,y坐标
	          w ---》图片的实际宽
			  h ---》图片的实际高
			  scale---》缩小的倍数
			  bmpname---》图片的路径
	代码重点提示：m相当于是缩小之后的图片的宽，n相当于是缩小之后的图片的高
*/           
int show_shortbmp(int x,int y,int w,int h,int scale,char *bmpname);
// 自定义任意位置显示任意大小的bmp
int show_anybmp(int x,int y,int w,int h,char *bmpname);

int show_fullbmp(char *bmpname);


#endif
