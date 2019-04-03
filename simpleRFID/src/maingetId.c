#include "myhead.h"
#include "sqlite3.h"
#include "jpeglib.h"
#include "jconfig.h"
#include "jerror.h"
volatile unsigned long cardid ,number;
static struct timeval timeout;
int get_time,sum_time,money;
int num[3],state;//0��ʾ�г�λ
int ret,fd,flag,j;
sqlite3 *fp;
int a,b,k,l;
char cmd[100] = {"create table  student (name char[10],id int unique,num int,time int)"};
#define DEV_PATH   "/dev/ttySAC1"

/* ���ô��ڲ���:9600���� */
void init_tty(int fd)
{    
	//�������ô��ڵĽṹ��
	struct termios termios_new;
	//����ոýṹ��
	bzero( &termios_new, sizeof(termios_new));
	//	cfmakeraw()�����ն����ԣ���������termios�ṹ�еĸ���������
	cfmakeraw(&termios_new);
	//���ò�����
	//termios_new.c_cflag=(B9600);
	cfsetispeed(&termios_new, B9600);
	cfsetospeed(&termios_new, B9600);
	//CLOCAL��CREAD�ֱ����ڱ������Ӻͽ���ʹ�ܣ���ˣ�����Ҫͨ��λ����ķ�ʽ����������ѡ�    
	termios_new.c_cflag |= CLOCAL | CREAD;
	//ͨ��������������λΪ8λ
	termios_new.c_cflag &= ~CSIZE;
	termios_new.c_cflag |= CS8; 
	//��������żУ��
	termios_new.c_cflag &= ~PARENB;
	//һλֹͣλ
	termios_new.c_cflag &= ~CSTOPB;
	tcflush(fd,TCIFLUSH);
	// �����ý����ַ��͵ȴ�ʱ�䣬������Ҫ����Խ�������Ϊ0
	termios_new.c_cc[VTIME] = 10;
	termios_new.c_cc[VMIN] = 1;
	// �����������/���������
	tcflush (fd, TCIFLUSH);
	//������ú󣬿���ʹ�����º������������
	if(tcsetattr(fd,TCSANOW,&termios_new) )
		printf("Setting the serial1 failed!\n");
}

/*����У���*/
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

//��װ����A����ĺ���
int PiccRequest(int fd)
{
	unsigned char WBuf[128], RBuf[128];
	int  i;
	fd_set rdfd;
	
	memset(WBuf, 0, 128);
	memset(RBuf,1,128);//  ????
	WBuf[0] = 0x07;	//֡��= 7 Byte
	WBuf[1] = 0x02;	//����= 0 , ��������= 0x01
	WBuf[2] = 0x41;	//����= 'A'
	WBuf[3] = 0x01;	//��Ϣ����= 1
	WBuf[4] = 0x52;	//����ģʽ:  ALL=0x52
	WBuf[5] = CalBCC(WBuf, WBuf[0]-2);		//У���
	WBuf[6] = 0x03; 	//������־

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
			if (RBuf[2] == 0x00)	 	//Ӧ��֡״̬����Ϊ0 ������ɹ�
			{
				return 0;
			}
			break;
	}
	return -1;
}

/*����ײ����ȡ��Χ�����ID*/
int PiccAnticoll(int fd)
{
	unsigned char WBuf[128], RBuf[128];
	int  i;
	fd_set rdfd;;
	memset(WBuf, 0, 128);
	memset(RBuf,0,128);
	WBuf[0] = 0x08;	//֡��= 8 Byte
	WBuf[1] = 0x02;	//����= 0 , ��������= 0x01
	WBuf[2] = 0x42;	//����= 'B'
	WBuf[3] = 0x02;	//��Ϣ����= 2
	WBuf[4] = 0x93;	//����ײ0x93 --һ������ײ
	WBuf[5] = 0x00;	//λ����0
	WBuf[6] = CalBCC(WBuf, WBuf[0]-2);		//У���
	WBuf[7] = 0x03; 	//������־
	
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
			if (RBuf[2] == 0x00) //Ӧ��֡״̬����Ϊ0 ���ȡID �ɹ�
			{
				cardid = (RBuf[7]<<24) | (RBuf[6]<<16) | (RBuf[5]<<8) | RBuf[4];//�ֲ�˵�ˣ����ֽ���ǰ��
				return 0;
			}
	}
	return -1;
}

//���ݿⴴ��
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

//�̲߳��ϵĶ�ȡ����
void *fun(void *arg)
{
	while(1) //�������
	{
		/*�������߷�Χ�Ŀ�*/
		PiccRequest(fd);
		usleep(100000);
		/*���з���ײ����ȡ���߷�Χ������ID*/
		PiccAnticoll(fd);
		usleep(100000);
	}
}

//��ʾbmp�����СͼƬ
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
	char buf[54];	//��bmp��54�ֽ���Ϣ
	bmpfd = open(bmpname,O_RDONLY);
	read(bmpfd,buf,54);	//��bmp��54�ֽ���Ϣ
	bmpW = buf[18]|buf[19]<<8|buf[20]<<16|buf[21]<<24;	//ͼƬ�Ŀ�
	bmpH = buf[22]|buf[23]<<8|buf[24]<<16|buf[25]<<24;	//ͼƬ�ĸ�
	
	lseek(bmpfd, 0, SEEK_SET);	//����0
	lseek(bmpfd, 54, SEEK_SET);	//�����ƶ�54���ֽ�
	
	char bmpbuf[bmpW*bmpH*3];	//����bmp�����ص�
	int LCDbuf[bmpW*bmpH];		//����LCD��ʾ���ֽ�
	ret = read(bmpfd,bmpbuf,sizeof bmpbuf);	//��bmp����

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
//��ʾ0-9������
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

//���ҿ����Ƿ����
int call_back(void *arg,int id,char **str,char **name)
{
	int i,k;
	int x ,y ;
	for(i=0;i<id;i++)
	{
		if(cardid == atoi(str[i]))//�����Ƿ����
		{
			get_time = atoi(str[3]);//��ȡͣ��ʱ��
			flag = 1;			
		}	
	}
	for(i = 0;i < 3;i++)
    {
		if(num[i] == atoi(str[2]))//��λ���Ƿ�һ��
		{
			x = 200*num[i];
			y = 240;

			bzero(cmd,100);
			sprintf(cmd,"g%d.jpg",num[i]);
			show_anyjpg(x,y,cmd);//ˢͼ����ʾ�г�λ
			usleep(100000);
			num[i] = 0;//��λ�ͷ�
		}
	}	
	return 0;	
}

//���복������Ϣ
int add_cart()
{
	int i;
	int x ,y ;
	char name[20]={0};	
	j++;//��λ��һ
	for(i = 0;i < 3;i++)//������λ
	{
		if(num[i] == 0)
		{
			state = 1;
			break;
		}	
	}	
	if(j > 2)//�����λ���ˣ��˳�
	{
		printf("not pace\n");
		j = 2;
		return;
	}
	if(state == 1)//һ���г�λ ����ͣ��
	{
		num[i] = j;
		x = 200*num[i]  ;
		y = 240;

		bzero(cmd,100);
		sprintf(cmd,"r%d.jpg",num[i]);
		show_anyjpg(x,y,cmd);//ˢͼ
		time_t timer;
		struct tm *tblock;	
		timer = time(NULL);
		tblock = localtime(&timer);//��ȡͣ����ʱ��
		printf("��ʼͣ��ʱ��: %d:%d:%d\n",tblock->tm_hour,tblock->tm_min, tblock->tm_sec);
				
		bzero(cmd,100);
		printf("�����복�����֣�\n");
		scanf("%s",name);//���복������
		//������Ϣ���루���������ţ���ţ�ʱ�䣨�֣���
		sprintf(cmd,"insert into student values(\"%s\",%d,%d,%d)",name,cardid,num[i],tblock->tm_sec);
		printf("��λ�ţ�%d\n",num[i]);
		ret = sqlite3_exec(fp,cmd,NULL,NULL,NULL);
		if(ret != SQLITE_OK)
		{
			perror("sqlite3_exec error!\n");
		}	
	}
	state = 0;	
	return 0;
}

//ɾ��������Ϣ������ͣ����
int delete_cart()
{
	j--;//��λ��һ
	time_t timer;
	int number;
	struct tm *tblock;	
	timer = time(NULL);
	tblock = localtime(&timer);//��ȡʱ��
	
	bzero(cmd,100);
	sprintf(cmd,"select * from student where id=%d",cardid);
	ret = sqlite3_exec(fp,cmd,call_back,NULL,NULL);//��ѯ�ÿ�����ȡͣ��ʱ��
	if(ret != SQLITE_OK)
	{
		perror("sqlite3_exec error!\n");
	}
	sum_time = tblock->tm_sec - get_time;//�õ�ͣ����ʱ��
	money = sun_money();//�õ�ͣ������
	printf("ͣ��ʱ��:%d �շѣ�%d\n",sum_time,money);
	a = 300;
	b = 175;
	number = money;
	show_lcd_numer(number);//LCD��ʾ����
	
	a = 250;
	b = 120;
	number = sum_time;
	show_lcd_numer(number);//LCD��ʾͣ��ʱ��
	bzero(cmd,100);
	sprintf(cmd,"delete from student where id=%d",cardid);
	ret = sqlite3_exec(fp,cmd,NULL,NULL,NULL);//ɾ��������Ϣ������ͣ����
	if(ret != SQLITE_OK)
	{
		perror("sqlite3_exec error!\n");
	}
	printf("ͣ������\n");	//�뿪
	return 0;
}

//����ͣ����
int sun_money()
{
	money = sum_time*100;
	return money;
}

//��ʾjpeg�����СͼƬ
int show_anyjpg(int x,int y,char *jpgpath)
{
	int i=0;
	int lcdfd;
	//��lcd
	lcdfd=open("/dev/fb0",O_RDWR);
	//ӳ��LCD
	int *lcdmem=mmap(NULL,800*480*4,PROT_READ|PROT_WRITE,MAP_SHARED,lcdfd,0);
	struct jpeg_decompress_struct myinfo;//�������ڴ��jpeg��ѹ�����Ľṹ��
	struct jpeg_error_mgr myerr;//�����Ŵ�����Ϣ�Ľṹ��
	//�Խṹ���ʼ��
	jpeg_create_decompress(&myinfo);
	//ע�������Ϣ
	myinfo.err=jpeg_std_error(&myerr);
	//��ȡjpeg����Դ
	FILE *myjpeg=fopen(jpgpath,"r+");
	jpeg_stdio_src(&myinfo,myjpeg);
	//��ȡjpeg��ͷ��Ϣ
	jpeg_read_header(&myinfo,true);
	//��ʼ��ѹ��
	jpeg_start_decompress(&myinfo);
	//����һ���������ڴ��һ��RGB��ֵ
	char *buf=malloc(myinfo.output_width*3);
	/*
		&buf --->char **
		char buf[10]  &buf--> char(*p)[10]
	*/
	
	int newbuf[myinfo.output_width];
	while(myinfo.output_scanline < myinfo.output_height) //  450*200
	{
		jpeg_read_scanlines(&myinfo,(JSAMPARRAY)&buf,1);//ÿ��ѭ����ȡһ��RGB
		//����ȡ����RGBת��֮�����LCD
		for(i=0;i<myinfo.output_width;i++)
		{
			newbuf[i]=buf[3*i+2]|buf[3*i+1]<<8|buf[3*i]<<16|0x00<<24;
		}
		memcpy(lcdmem+(y+myinfo.output_scanline-1)*800+x,newbuf,myinfo.output_width*4);
		/*��һ��ѭ��  �׵�ַ  lcdmem+y*800+x
		  �ڶ���ѭ��          lcdmem+(y+1)*800+x    
		*/
	}
	//��β
	jpeg_finish_decompress(&myinfo);
	jpeg_destroy_decompress(&myinfo);
	close(lcdfd);
	fclose(myjpeg);
	munmap(lcdmem,800*480*4);
	return 0;
}

//��LCD����ʾҪ��ʾ������
int show_lcd_numer(int mun)
{
	for(l=0;l<mun*10;l++)//����
	{
		k = mun%10;
		a = a - 20;
		mun = mun /10;
		show_num(a,b,k);	
	}
	return 0;
}

//������
int main(int argc,char **argv)
{
	int i;
	//�򿪴���
	fd = open(DEV_PATH,O_RDWR);
	if (fd < 0)
	{
		fprintf(stderr, "Open 6818_ttySAC1 fail!\n");
		return -1;
	}
	/*��ʼ������*/
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
	show_anyjpg(0,0,"m.jpg");//��ʾ����
	while(1)
	{
		if(cardid != 0 && j < 3)//�ж��Ƿ��п����Ƿ��г�λ
		{
			printf("���ţ�%ld\n",cardid);
			
			a = 400;
			b = 65;
			number = cardid;
			show_lcd_numer(number);//lcd ��ʾ����
			
			bzero(cmd,100);
			sprintf(cmd,"select * from student where id=%ld",cardid);
			ret = sqlite3_exec(fp,cmd,call_back,NULL,NULL);//��ѯ�Ƿ�ÿ��Ѿ�����
			if(flag == 0)//��������������ͣ��
				add_cart(cardid);
			if(flag == 1)
			{
				delete_cart();//������ɾ�������㳵��
				flag = 0;
			}
			printf("��ͣ������%d\n",j);
			printf("ʣ�೵λ��%d\n",2-j);
			cardid = 0;	//����ʼ��
		}
	}
	close(fd);
	return 0;	
}
