#include "myhead.h"
#include "sqlite3.h"
#include "jpeglib.h"
#include "jconfig.h"
#include "jerror.h"
volatile unsigned long cardid ,number;
static struct timeval timeout;
int get_time,sum_time,money;
int num[3],state;//0表示有车位
int ret,fd,flag,j;
sqlite3 *fp;
int a,b,k,l;
char cmd[100] = {"create table  student (name char[10],id int unique,num int,time int)"};
#define DEV_PATH   "/dev/ttySAC1"

/* 设置窗口参数:9600速率 */
void init_tty(int fd)
{    
	//声明设置串口的结构体
	struct termios termios_new;
	//先清空该结构体
	bzero( &termios_new, sizeof(termios_new));
	//	cfmakeraw()设置终端属性，就是设置termios结构中的各个参数。
	cfmakeraw(&termios_new);
	//设置波特率
	//termios_new.c_cflag=(B9600);
	cfsetispeed(&termios_new, B9600);
	cfsetospeed(&termios_new, B9600);
	//CLOCAL和CREAD分别用于本地连接和接受使能，因此，首先要通过位掩码的方式激活这两个选项。    
	termios_new.c_cflag |= CLOCAL | CREAD;
	//通过掩码设置数据位为8位
	termios_new.c_cflag &= ~CSIZE;
	termios_new.c_cflag |= CS8; 
	//设置无奇偶校验
	termios_new.c_cflag &= ~PARENB;
	//一位停止位
	termios_new.c_cflag &= ~CSTOPB;
	tcflush(fd,TCIFLUSH);
	// 可设置接收字符和等待时间，无特殊要求可以将其设置为0
	termios_new.c_cc[VTIME] = 10;
	termios_new.c_cc[VMIN] = 1;
	// 用于清空输入/输出缓冲区
	tcflush (fd, TCIFLUSH);
	//完成配置后，可以使用以下函数激活串口设置
	if(tcsetattr(fd,TCSANOW,&termios_new) )
		printf("Setting the serial1 failed!\n");
}

/*计算校验和*/
unsigned char CalBCC(unsigned char *buf, int n)
{
	int i;
	unsigned char bcc=0;
	for(i = 0; i < n; i++)
	{
		bcc ^= *(buf+i);
	}
	return (~bcc);
}

//封装发送A命令的函数
int PiccRequest(int fd)
{
	unsigned char WBuf[128], RBuf[128];
	int  i;
	fd_set rdfd;
	
	memset(WBuf, 0, 128);
	memset(RBuf,1,128);//  ????
	WBuf[0] = 0x07;	//帧长= 7 Byte
	WBuf[1] = 0x02;	//包号= 0 , 命令类型= 0x01
	WBuf[2] = 0x41;	//命令= 'A'
	WBuf[3] = 0x01;	//信息长度= 1
	WBuf[4] = 0x52;	//请求模式:  ALL=0x52
	WBuf[5] = CalBCC(WBuf, WBuf[0]-2);		//校验和
	WBuf[6] = 0x03; 	//结束标志

	FD_ZERO(&rdfd);
	FD_SET(fd,&rdfd);

	write(fd, WBuf, 7);;
	ret = select(fd + 1,&rdfd, NULL,NULL,NULL);
	switch(ret)
	{
		case -1:
			perror("select error\n");
			break;
		case  0:
			printf("Request timed out.\n");
			break;
		default:
			ret = read(fd, RBuf, 8);
			if (ret < 0)
			{
				printf("ret = %d, %m\n", ret, errno);
				break;
			}
			if (RBuf[2] == 0x00)	 	//应答帧状态部分为0 则请求成功
			{
				return 0;
			}
			break;
	}
	return -1;
}

/*防碰撞，获取范围内最大ID*/
int PiccAnticoll(int fd)
{
	unsigned char WBuf[128], RBuf[128];
	int  i;
	fd_set rdfd;;
	memset(WBuf, 0, 128);
	memset(RBuf,0,128);
	WBuf[0] = 0x08;	//帧长= 8 Byte
	WBuf[1] = 0x02;	//包号= 0 , 命令类型= 0x01
	WBuf[2] = 0x42;	//命令= 'B'
	WBuf[3] = 0x02;	//信息长度= 2
	WBuf[4] = 0x93;	//防碰撞0x93 --一级防碰撞
	WBuf[5] = 0x00;	//位计数0
	WBuf[6] = CalBCC(WBuf, WBuf[0]-2);		//校验和
	WBuf[7] = 0x03; 	//结束标志
	
	FD_ZERO(&rdfd);
	FD_SET(fd,&rdfd);
	write(fd, WBuf, 8);

	ret = select(fd + 1,&rdfd, NULL,NULL,NULL);
	switch(ret)
	{
		case -1:
			perror("select error\n");
			break;
		case  0:
			perror("Timeout:");
			break;
		default:
			ret = read(fd, RBuf, 10);
			if (ret < 0)
			{
				printf("ret = %d, %m\n", ret, errno);
				break;
			}
			if (RBuf[2] == 0x00) //应答帧状态部分为0 则获取ID 成功
			{
				cardid = (RBuf[7]<<24) | (RBuf[6]<<16) | (RBuf[5]<<8) | RBuf[4];//手册说了，低字节在前面
				return 0;
			}
	}
	return -1;
}

//数据库创建
int sqlite3_create(char *arg)
{
	ret = sqlite3_open(arg,&fp);
	if(ret != SQLITE_OK)
	{
		perror("sqlite3_open error!\n");
		return -1;
	}
	ret = sqlite3_exec(fp,cmd,NULL,NULL,NULL);
	if(ret != SQLITE_OK)
	{
		perror("sqlite3_exec error!\n");
		return -1;
	}
	return 0;
	
}

//线程不断的读取卡号
void *fun(void *arg)
{
	while(1) //多次请求
	{
		/*请求天线范围的卡*/
		PiccRequest(fd);
		usleep(100000);
		/*进行防碰撞，获取天线范围内最大的ID*/
		PiccAnticoll(fd);
		usleep(100000);
	}
}

//显示bmp任意大小图片
void show_shapebmp(int x,int y,char *bmpname)
{
	int ret = 0;
	int LCDfd = 0;
	LCDfd = open("/dev/fb0",O_RDWR);
	int *memoryadd = mmap(NULL, 480*800*4, PROT_READ|PROT_WRITE, MAP_SHARED, LCDfd, 0);
	//--------------------------------------------------------
	
	int j,i,k;
	
	int bmpfd = 0;
	int bmpW = 0;
	int bmpH = 0;
	char buf[54];	//读bmp的54字节信息
	bmpfd = open(bmpname,O_RDONLY);
	read(bmpfd,buf,54);	//读bmp的54字节信息
	bmpW = buf[18]|buf[19]<<8|buf[20]<<16|buf[21]<<24;	//图片的宽
	bmpH = buf[22]|buf[23]<<8|buf[24]<<16|buf[25]<<24;	//图片的高
	
	lseek(bmpfd, 0, SEEK_SET);	//光标归0
	lseek(bmpfd, 54, SEEK_SET);	//向右移动54个字节
	
	char bmpbuf[bmpW*bmpH*3];	//定义bmp的像素点
	int LCDbuf[bmpW*bmpH];		//定义LCD显示的字节
	ret = read(bmpfd,bmpbuf,sizeof bmpbuf);	//读bmp像素

	if(ret < 0)
	{
		printf("read bmp error\n");	
	}
	for(i=0; i<bmpW*bmpH; i++)
		LCDbuf[i]=bmpbuf[3*i]|bmpbuf[3*i+1]<<8|bmpbuf[3*i+2]<<16|0x00<<24;
	int xx = x;
	int yy = y;
	int bre=0;
	for(i=0; i<bmpW; i++)
	{
		for(j=0; j<bmpH; j++)
		{
			if((memoryadd+(j+y)*800+i+x) < (memoryadd+480*800))
				*(memoryadd+(j+y)*800+i+x) = LCDbuf[(bmpH-1-j)*bmpW+i];
		}
	}
	close(LCDfd);
	close(bmpfd);
}
//显示0-9的数字
void show_num(int x,int y,int num)
{
//    0-9 10  11  12  13  14  15         16          17
//num:0-9  A   B   C   D   E   F   	G --> :	    H --> -
	char buf[20];
	bzero(buf,sizeof buf);
	
	if(num>17)	return;
	
	switch(num)
	{
		case 0:		strcpy(buf,"./0.bmp");	break;
		case 1:		strcpy(buf,"./1.bmp");	break;
		case 2:		strcpy(buf,"./2.bmp");	break;
		case 3:		strcpy(buf,"./3.bmp");	break;
		case 4:		strcpy(buf,"./4.bmp");	break;
		case 5:		strcpy(buf,"./5.bmp");	break;
		case 6:		strcpy(buf,"./6.bmp");	break;
		case 7:		strcpy(buf,"./7.bmp");	break;
		case 8:		strcpy(buf,"./8.bmp");	break;
		case 9:		strcpy(buf,"./9.bmp");	break;
		case 10:	strcpy(buf,"./A.bmp");	break;
		case 11:	strcpy(buf,"./B.bmp");	break;
		case 12:	strcpy(buf,"./C.bmp");	break;
		case 13:	strcpy(buf,"./D.bmp");	break;
		case 14:	strcpy(buf,"./E.bmp");	break;
		case 15:	strcpy(buf,"./F.bmp");	break;
		case 16:	strcpy(buf,"./G.bmp");	break;
		case 17:	strcpy(buf,"./H.bmp");	break;
		default :	break;
	}
	
	show_shapebmp(x,y,buf);
}

//查找卡号是否存在
int call_back(void *arg,int id,char **str,char **name)
{
	int i,k;
	int x ,y ;
	for(i=0;i<id;i++)
	{
		if(cardid == atoi(str[i]))//卡号是否存在
		{
			get_time = atoi(str[3]);//获取停车时间
			flag = 1;			
		}	
	}
	for(i = 0;i < 3;i++)
    {
		if(num[i] == atoi(str[2]))//车位号是否一致
		{
			x = 200*num[i];
			y = 240;

			bzero(cmd,100);
			sprintf(cmd,"g%d.jpg",num[i]);
			show_anyjpg(x,y,cmd);//刷图，表示有车位
			usleep(100000);
			num[i] = 0;//车位释放
		}
	}	
	return 0;	
}

//插入车主的信息
int add_cart()
{
	int i;
	int x ,y ;
	char name[20]={0};	
	j++;//车位加一
	for(i = 0;i < 3;i++)//遍历车位
	{
		if(num[i] == 0)
		{
			state = 1;
			break;
		}	
	}	
	if(j > 2)//如果车位满了，退出
	{
		printf("not pace\n");
		j = 2;
		return;
	}
	if(state == 1)//一旦有车位 马上停车
	{
		num[i] = j;
		x = 200*num[i]  ;
		y = 240;

		bzero(cmd,100);
		sprintf(cmd,"r%d.jpg",num[i]);
		show_anyjpg(x,y,cmd);//刷图
		time_t timer;
		struct tm *tblock;	
		timer = time(NULL);
		tblock = localtime(&timer);//读取停车的时间
		printf("开始停车时间: %d:%d:%d\n",tblock->tm_hour,tblock->tm_min, tblock->tm_sec);
				
		bzero(cmd,100);
		printf("请输入车主名字：\n");
		scanf("%s",name);//输入车主姓名
		//车主信息存入（姓名，卡号，编号，时间（分））
		sprintf(cmd,"insert into student values(\"%s\",%d,%d,%d)",name,cardid,num[i],tblock->tm_sec);
		printf("车位号：%d\n",num[i]);
		ret = sqlite3_exec(fp,cmd,NULL,NULL,NULL);
		if(ret != SQLITE_OK)
		{
			perror("sqlite3_exec error!\n");
		}	
	}
	state = 0;	
	return 0;
}

//删除车主信息，结算停车费
int delete_cart()
{
	j--;//车位减一
	time_t timer;
	int number;
	struct tm *tblock;	
	timer = time(NULL);
	tblock = localtime(&timer);//获取时间
	
	bzero(cmd,100);
	sprintf(cmd,"select * from student where id=%d",cardid);
	ret = sqlite3_exec(fp,cmd,call_back,NULL,NULL);//查询该卡，获取停车时间
	if(ret != SQLITE_OK)
	{
		perror("sqlite3_exec error!\n");
	}
	sum_time = tblock->tm_sec - get_time;//得到停车总时间
	money = sun_money();//得到停车费用
	printf("停车时间:%d 收费：%d\n",sum_time,money);
	a = 300;
	b = 175;
	number = money;
	show_lcd_numer(number);//LCD显示费用
	
	a = 250;
	b = 120;
	number = sum_time;
	show_lcd_numer(number);//LCD显示停车时间
	bzero(cmd,100);
	sprintf(cmd,"delete from student where id=%d",cardid);
	ret = sqlite3_exec(fp,cmd,NULL,NULL,NULL);//删除车主信息，结算停车费
	if(ret != SQLITE_OK)
	{
		perror("sqlite3_exec error!\n");
	}
	printf("停车结束\n");	//离开
	return 0;
}

//结算停车费
int sun_money()
{
	money = sum_time*100;
	return money;
}

//显示jpeg任意大小图片
int show_anyjpg(int x,int y,char *jpgpath)
{
	int i=0;
	int lcdfd;
	//打开lcd
	lcdfd=open("/dev/fb0",O_RDWR);
	//映射LCD
	int *lcdmem=mmap(NULL,800*480*4,PROT_READ|PROT_WRITE,MAP_SHARED,lcdfd,0);
	struct jpeg_decompress_struct myinfo;//定义用于存放jpeg解压参数的结构体
	struct jpeg_error_mgr myerr;//定义存放错误信息的结构体
	//对结构体初始化
	jpeg_create_decompress(&myinfo);
	//注册错误信息
	myinfo.err=jpeg_std_error(&myerr);
	//获取jpeg数据源
	FILE *myjpeg=fopen(jpgpath,"r+");
	jpeg_stdio_src(&myinfo,myjpeg);
	//读取jpeg的头信息
	jpeg_read_header(&myinfo,true);
	//开始解压缩
	jpeg_start_decompress(&myinfo);
	//定义一个数组用于存放一行RGB数值
	char *buf=malloc(myinfo.output_width*3);
	/*
		&buf --->char **
		char buf[10]  &buf--> char(*p)[10]
	*/
	
	int newbuf[myinfo.output_width];
	while(myinfo.output_scanline < myinfo.output_height) //  450*200
	{
		jpeg_read_scanlines(&myinfo,(JSAMPARRAY)&buf,1);//每次循环读取一行RGB
		//将读取到的RGB转换之后填充LCD
		for(i=0;i<myinfo.output_width;i++)
		{
			newbuf[i]=buf[3*i+2]|buf[3*i+1]<<8|buf[3*i]<<16|0x00<<24;
		}
		memcpy(lcdmem+(y+myinfo.output_scanline-1)*800+x,newbuf,myinfo.output_width*4);
		/*第一次循环  首地址  lcdmem+y*800+x
		  第二次循环          lcdmem+(y+1)*800+x    
		*/
	}
	//收尾
	jpeg_finish_decompress(&myinfo);
	jpeg_destroy_decompress(&myinfo);
	close(lcdfd);
	fclose(myjpeg);
	munmap(lcdmem,800*480*4);
	return 0;
}

//在LCD上显示要显示的数字
int show_lcd_numer(int mun)
{
	for(l=0;l<mun*10;l++)//卡号
	{
		k = mun%10;
		a = a - 20;
		mun = mun /10;
		show_num(a,b,k);	
	}
	return 0;
}

//主函数
int main(int argc,char **argv)
{
	int i;
	//打开串口
	fd = open(DEV_PATH,O_RDWR);
	if (fd < 0)
	{
		fprintf(stderr, "Open 6818_ttySAC1 fail!\n");
		return -1;
	}
	/*初始化串口*/
	init_tty(fd);
	timeout.tv_sec = 1;
	timeout.tv_usec = 0;
	sqlite3_create(argv[1]);
	pthread_t tid;
	ret = pthread_create(&tid,NULL,fun,NULL);
	if(ret != 0)
	{
		printf("pthread_create error!\n");
	}
	show_anyjpg(0,0,"m.jpg");//显示界面
	while(1)
	{
		if(cardid != 0 && j < 3)//判断是否有卡，是否有车位
		{
			printf("卡号：%ld\n",cardid);
			
			a = 400;
			b = 65;
			number = cardid;
			show_lcd_numer(number);//lcd 显示卡号
			
			bzero(cmd,100);
			sprintf(cmd,"select * from student where id=%ld",cardid);
			ret = sqlite3_exec(fp,cmd,call_back,NULL,NULL);//查询是否该卡已经存在
			if(flag == 0)//如果卡不存在则存停车
				add_cart(cardid);
			if(flag == 1)
			{
				delete_cart();//存在则删除，结算车费
				flag = 0;
			}
			printf("已停车辆：%d\n",j);
			printf("剩余车位：%d\n",2-j);
			cardid = 0;	//卡初始化
		}
	}
	close(fd);
	return 0;	
}
