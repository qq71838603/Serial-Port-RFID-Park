#include "myhead.h"
#include "thread_pool.h"
#include "sqlite3.h"
#include "jpeglib.h"
#include "jconfig.h"
#include "jerror.h"
//串口3
#define DEV_PATH   "/dev/ttySAC3"

unsigned char comm[128];//命令数组
unsigned char date[128];//读取数组
char str_commd[1024];

volatile unsigned int RFID_ID;//保存读取到的卡号
int rfid_flg;//读到卡号的标志位
struct tm *tblock;//时间结构体

int int_bit=12;
//声明sqlite
sqlite3 *mydate;

typedef struct info{
	unsigned int	id;
	char 			name[20];
	char 			car[20];
	char 			phone[20];
	unsigned int 	strattime;
	unsigned int 	closetime;
	char 			stopbit[20];
	int 			stopflg;
}car_info,*Pcar_info;

//保存读取到的数据的结构体
car_info read_db;
//保存输入的数据的结构体
car_info carinfo;


//获取系统时间
int ReadTimer()
{
	time_t timer;
    timer = time(NULL);
    tblock = localtime(&timer);
	return (tblock->tm_sec+tblock->tm_min*60+tblock->tm_hour*120);

}

//显示图片
void ShowJPG(char *path,int x,int y)
{
	int i=0;
	int lcdfd;
	//打开lcd
	lcdfd=open("/dev/fb0",O_RDWR);
	struct jpeg_decompress_struct myinfo;//定义用于存放jpeg解压参数的结构体
	struct jpeg_error_mgr myerr;//定义存放错误信息的结构体
	//对结构体初始化
	jpeg_create_decompress(&myinfo);
	//注册错误信息
	myinfo.err=jpeg_std_error(&myerr);
	//获取jpeg数据源
	FILE *myjpeg=fopen(path,"r+");
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
	//内存映射
	int *FB = mmap(NULL,800*480*4,PROT_READ|PROT_WRITE,MAP_SHARED,lcdfd,0);
	
	int newbuf[myinfo.output_width];
	//printf("图片宽是:%d 高是:%d\n",myinfo.output_width,myinfo.output_height);
	while(myinfo.output_scanline < myinfo.output_height)
	{
		jpeg_read_scanlines(&myinfo,(JSAMPARRAY)&buf,1);//每次循环读取一行RGB
		//将读取到的RGB拷贝到映射的内存
		for(i=0;i<myinfo.output_width;i++)
		{
			newbuf[i]=buf[i*3+2] | buf[i*3+1] << 8 | buf[i*3] << 16;
		}
		memcpy(FB+(y+myinfo.output_scanline-1)*800 + x,newbuf,myinfo.output_width*4);
	}
	//撤销内存映射和关闭文件
	munmap(FB,800*480*4);
	jpeg_finish_decompress(&myinfo);
	jpeg_destroy_decompress(&myinfo);
	close(lcdfd);
	fclose(myjpeg);
}

//显示停车界面
void ShowCarBit(char *temp,int bit)
{
	if(strcmp(temp,"A0") ==0 )
	{
		if(bit == 0)
		{
			ShowJPG("./p.jpg",36,8);
		}
		else
		{
			ShowJPG("./A0.jpg",36,8);
		}
	}
	else if(strcmp(temp,"A1") ==0 )
	{
		if(bit == 0)
		{
			ShowJPG("./p.jpg",167,7);
		}
		else
		{
			ShowJPG("./A1.jpg",167,7);
		}
	}
	else if(strcmp(temp,"A2") ==0 )
	{
		if(bit == 0)
		{
			ShowJPG("./p.jpg",248,7);
		}
		else
		{
			ShowJPG("./A2.jpg",248,7);
		}
	}
	else if(strcmp(temp,"A3") ==0 )
	{
		if(bit == 0)
		{
			ShowJPG("./p.jpg",403,8);
		}
		else
		{
			ShowJPG("./A3.jpg",403,8);
		}
	}
	else if(strcmp(temp,"A4") ==0 )
	{
		if(bit == 0)
		{
			ShowJPG("./p.jpg",546,12);
		}
		else
		{
			ShowJPG("./A4.jpg",546,12);
		}
	}
	else if(strcmp(temp,"A5") ==0 )
	{
		if(bit == 0)
		{
			ShowJPG("./p.jpg",679,13);
		}
		else
		{
			ShowJPG("./A5.jpg",678,13);
		}
	}
	else if(strcmp(temp,"A6") ==0 )
	{
		if(bit == 0)
		{
			ShowJPG("./p.jpg",677,309);
		}
		else
		{
			ShowJPG("./A6.jpg",677,309);
		}
	}
	else if(strcmp(temp,"A7") ==0 )
	{
		if(bit == 0)
		{
			ShowJPG("./p.jpg",548,311);
		}
		else
		{
			ShowJPG("./A7.jpg",548,311);
		}
	}
	else if(strcmp(temp,"A8") ==0 )
	{
		if(bit == 0)
		{
			ShowJPG("./p.jpg",427,310);
		}
		else
		{
			ShowJPG("./A8.jpg",427,310);
		}
	}
	else if(strcmp(temp,"A9") ==0 )
	{
		if(bit == 0)
		{
			ShowJPG("./p.jpg",285,315);
		}
		else
		{
			ShowJPG("./A9.jpg",285,315);
		}
	}
	else if(strcmp(temp,"A10") ==0 )
	{
		if(bit == 0)
		{
			ShowJPG("./p.jpg",153,316);
		}
		else
		{
			ShowJPG("./A10.jpg",153,316);
		}
	}
	else if(strcmp(temp,"A11") ==0 )
	{
		if(bit == 0)
		{
			ShowJPG("./p.jpg",28,315);
		}
		else
		{
			ShowJPG("./A11.jpg",28,315);
		}
	}
	else 
	{
		
	}
}

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

//初始化请求命令的
void Request_RFID(void)
{
	memset(comm,0,128);
	comm[0]=0x07;
	comm[1]=0x02;
	comm[2]=0x41;
	comm[3]=0x01;
	comm[4]=0x52;
	comm[5]=CalBCC(comm, comm[0]-2);;
	comm[6]=0x03;
}

//初始化读取卡号的命令
void Read_RFID_Carid(void)
{
	memset(comm,0,128);
	comm[0]=0x08;
	comm[1]=0x02;
	comm[2]=0x42;
	comm[3]=0x02;
	comm[4]=0x93;
	comm[5]=0x00;
	comm[6]=CalBCC(comm, comm[0]-2);;
	comm[7]=0x03;
}

//读取串口数据
void read_buf(int port_fd)
{
	unsigned char read_4data[4];
	int i=0,j=0;
	int ret;
	while(1)
	{
		bzero(read_4data,4);
		ret = read(port_fd,read_4data,4);
		if(ret < 0) break;
		for(i=0;i<ret;i++)
		{
			date[i+j]=read_4data[i];
		}
		j+=4;
	}
	j=0;
}

//不断读取RFID的任务线程
void *myReadRFID(void *arg)
{
	int port_fd = open(DEV_PATH, O_RDWR | O_NOCTTY | O_NONBLOCK);
	fcntl(port_fd,F_SETFL,FNDELAY);//取消串口阻塞
	if (port_fd < 0)
	{
		fprintf(stderr, "Open ttySAC3 fail!\n");
		return NULL;
	}
	/*初始化串口*/
	init_tty(port_fd);
	
	while(1)
	{
		memset(date,1,128);
		//printf("Query RFID card!\n");
		Request_RFID();
		write(port_fd,comm,comm[0]);
		usleep(100000);
		read_buf(port_fd);
		
		//查询到ID卡
		if(date[0]==0x08 && date[1]==0x02 && date[2]==0x00)
		{
			//printf("Read card number ID!\n");
			Read_RFID_Carid();
			write(port_fd,comm,comm[0]);
			usleep(100000);
			read_buf(port_fd);
		}
		else
		{
			continue;
		}
		//读取ID卡号
		if(date[0]==0x0A && date[1]==0x02 && date[2]==0x00)
		{
			RFID_ID = (date[7]<<24) | (date[6]<<16) | (date[5]<<8) | date[4];
			printf("RFID卡号:%x\n", RFID_ID);
			rfid_flg=1;
			//等待 主函数置0继续扫描RFID卡
			while(rfid_flg == 1);
		}
	}
}

//创建数据库
int mycreate_db(char *mytable,char *mypro)
{
	char commd[1024];
	bzero(commd,1024);
	sprintf(commd,"create table %s (%s);",mytable,mypro);
	int ret = sqlite3_exec(mydate,commd,NULL,NULL,NULL);
	if(ret != SQLITE_OK)
	{
		printf("create table erro!\n");
		return -1;
	}
	return 0;
}
//插入数据
int myinsert_db(char *mytable,char *mypro)
{
	char commd[1024];
	bzero(commd,1024);
	sprintf(commd,"insert into %s values (%s);",mytable,mypro);
	int ret = sqlite3_exec(mydate,commd,NULL,NULL,NULL);
	if(ret != SQLITE_OK)
	{
		printf("insert into table values erro!\n");
		return -1;
	}
	return 0;
}

//删除数据
int mydelete_db(char *mytable,char *mypro)
{
	char commd[1024];
	bzero(commd,1024);
	sprintf(commd,"delete from %s %s;",mytable,mypro);
	int ret = sqlite3_exec(mydate,commd,NULL,NULL,NULL);
	if(ret != SQLITE_OK)
	{
		printf("delete from table values erro!\n");
		return -1;
	}
	return 0;
}

//显示条件查询得到的数据
int show_result(void *arg,int col,char **str,char **name)
{
	int i;
	//printf("查询到的数据:	%d\n",col);
	/*
	for(i=0; i<col; i++)//打印数据
	{
		printf("%s:%s\n",name[i],str[i]);
	}*/
	read_db.id=atoi(str[0]);
	strcpy(read_db.name,str[1]);
	strcpy(read_db.car,str[2]);
	strcpy(read_db.phone,str[3]);
	read_db.strattime=atoi(str[4]);
	read_db.closetime=atoi(str[5]);
	strcpy(read_db.stopbit,str[6]);
	read_db.stopflg=atoi(str[7]);

	return 0; //很关键，一定要写，不写的话后面查询会出错
}

//条件查询查询数据库
int myselect_db(char *mytable,char *mypro)
{
	char commd[1024];
	bzero(commd,1024);
	
	sprintf(commd,"select * from %s %s;",mytable,mypro);
	int ret = sqlite3_exec(mydate,commd,show_result,NULL,NULL);
	if(ret != SQLITE_OK)
	{
		printf("select * from table erro!\n");
		return -1;
	}
	return 0;
}

//读到数据表中的所有数据并且实时更新显示
int showdbALL(void *arg,int col,char **str,char **name)
{
	car_info showimg;
	strcpy(showimg.stopbit,str[6]);
	showimg.stopflg=atoi(str[7]);
	ShowCarBit(showimg.stopbit,showimg.stopflg);
	return 0;
}

//查询所有数据
int myselectALL()
{
	int ret = sqlite3_exec(mydate,"select * from pnrkingInfo;",showdbALL,NULL,NULL);
	if(ret != SQLITE_OK)
	{
		printf("select * from table erro!\n");
		return -1;
	}
	return 0;
}

//创建数据库的任务线程
void *mydatetable(void *arg)
{
	system("rm CarInfo.db");
	int ret = sqlite3_open("./CarInfo.db",&mydate);
	if(ret != SQLITE_OK)
	{
		printf("open sqlite3 erro!\n");
		return NULL;
	}

	mycreate_db("pnrkingInfo",	"id 		long unique,\
								name 		char[20],	\
								carid 		char[20],	\
								phone 		char[20],	\
								strartime 	int,		\
								closetime 	int,		\
								stopbit 	char[20],	\
								stopflg 	int");
	//不断查询数据库中的数据 如果停车位发生变化马上更新显示
	while(1)
	{
		myselectALL();
		usleep(500000);
	}
}



//主函数
int main(int argc,char **argv)
{
	int ret;	
	//初始化线程池
	thread_pool *pool = malloc(sizeof(thread_pool));
	init_pool(pool,2);
	
	
	
	//读取RFID卡号
	add_task(pool,myReadRFID,NULL);
	//数据库操作
	add_task(pool,mydatetable,NULL);
	
	ShowJPG("./m.jpg",0,0);
	sleep(2);
	ShowJPG("./a.jpg",0,0);
	
	while(1)
	{
		//读取到卡号
		while(rfid_flg == 1)//等到读到卡号了之后的就往下执行
		{
			carinfo.id=RFID_ID;
			bzero(str_commd,1024);
			sprintf(str_commd,"where id=%d",carinfo.id);
			ret = myselect_db("pnrkingInfo",str_commd);
			if(carinfo.id != read_db.id && int_bit>0)
			{
				printf("小区有临时停车进入:\n");
				printf("请输入名字:");
				scanf("%s",carinfo.name);
				printf("请输入车牌号:");
				scanf("%s",carinfo.car);
				printf("请输入电话:");
				scanf("%s",carinfo.phone);
				carinfo.strattime=ReadTimer();
				carinfo.closetime=0;
				printf("请输入停车位号:");
				scanf("%s",carinfo.stopbit);
				carinfo.stopflg=0;
				//拼接数据
				bzero(str_commd,1024);
				sprintf(str_commd,	"%d,\"%s\",\"%s\",\"%s\",%d,%d,\"%s\",%d",
									carinfo.id,
									carinfo.name,
									carinfo.car,
									carinfo.phone,
									carinfo.strattime,
									carinfo.closetime,
									carinfo.stopbit,
									carinfo.stopflg);
				//把数据插入到数据库中
				myinsert_db("pnrkingInfo",str_commd);
				int_bit--;
			}
			else
			{
				printf("小区的临时停车出小区:\n");
				read_db.closetime=ReadTimer();
				int num=read_db.closetime-read_db.strattime;
				printf("小区临时停车收费 1￥/s\n");
				printf("车子进入的时间:%d 车子出去的时间:%d\n",read_db.strattime,read_db.closetime);
				printf("临时停车车主:%s	车牌为:%s	一共停车:%ds	总收费为:%d￥\n",read_db.name,read_db.car,num,num);
				bzero(str_commd,1024);
				sprintf(str_commd,"where id=%d",read_db.id);
				mydelete_db("pnrkingInfo",str_commd);
				int_bit++;
				ShowCarBit(read_db.stopbit,1);
				memset(&read_db,0,sizeof(car_info));
			}
			printf("剩余停车位:%d\n",int_bit);
			rfid_flg=0;
		}
	}
	
	//销毁线程池
	destroy_pool(pool);
	return 0;
}









