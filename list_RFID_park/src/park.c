#include "myhead.h"

typedef struct list_node{
	int id;//数据域
	int time;
	struct list_node *next;//指针域
}Node,*PLNode;

int car = 0;
//用于存放数据库命令
char cmd[100] = "create table if not exists park (id int,name char[10],time int);";

char flag[3] = {0,0,0};
int  flag_i;
//尾插
int list_add_tail(PLNode head,int num,int time)
{	
	//0.判断头节点是否为NULL
	if(head == NULL)
	{
		printf("list_add_tail head is NULL!\n");
		return -1;
	}

	//1.为尾插的节点申请空间
	PLNode node = NULL;
	node = (PLNode)malloc(sizeof(Node));
	if(node == NULL)
		printf("list_add_tail node is NULL!\n");

	//2.为尾插的节点赋值
	node->id = num;
	node->time = time;
	node->next = NULL;
	
	//3.首先找到最后一个节点
	//  从循环出来后，p就是指向最后一个节点的
	PLNode p = NULL;
	for(p=head;p->next!=NULL;p=p->next);
	
	//4.把新的节点接入到链表尾部
	p->next = node;
	
	return 0;
}

//删除节点
int delete_list_node(PLNode head,int num)
{
	PLNode q = NULL;//保证断开p后，p前面的节点不会跟后面的链表掉队
	PLNode p = NULL;
	for(q=head,p=head->next;p!=NULL;q=p,p=p->next)
	{
		if(p->id == num)
		{
			q -> next = p->next;
			free(p);
			goto OK;
		}
	
	}
	printf("not found delete node!\n");
	return -1;
OK:
	return 0;
	
}
//创建头结点
PLNode init_head(PLNode head)
{
	//1.为头节点的指针指向的节点申请堆空间
	head = (PLNode)malloc(sizeof(Node));
	
	//2.为头节点赋值
	head->next = NULL;

	return head;
}


void *serial_port(void *arg)
{
	int fd_d = (int)arg;
	int ret;

	PLNode head = NULL;
	head = init_head(head);

	if(sqlite3_open("./data.db",&mysq) != SQLITE_OK)
	{
		printf("open databank error\n");
		exit(-1);
	}
	//新建一个表格
	if(sqlite3_exec(mysq,cmd,NULL,NULL,NULL) != SQLITE_OK)
	{
		printf("create sqlite3 error\n");
		exit(-1);
	}

	while(1) //多次请求
	{
		AA:
		bzero(cmd,100);
		/*请求天线范围的卡*/
		if (PiccRequest(fd_d)==-1  )
		{
			//close(fd);
			//return -1;
		}
		usleep(100000);
		/*进行防碰撞，获取天线范围内最大的ID*/
		if( PiccAnticoll(fd_d)==-1)
		{
			//close(fd);
			//return -1;
		}
		else
		{
			printf("card ID = %x\n",cardid);

			//查找停车场(数据库)中有没有这个id,有的话就是退出,没有的话就是进入
			PLNode pose = NULL;
			//遍历链表
			for(pose=head;pose != NULL;pose=pose->next)
			{
				//如果停车场有这辆车则表明是离开
				if(pose->id == cardid)
				{
					printf("Welcome to back!\n");
					//system("madplay bye.mp3 &");
					int i;
					bzero(cmd,100);
					//计算停了多久的车和收费
					int car_out,finaltime,pay;
					car_out =gettime();
					pay = (car_out - pose->time);
					printf("you should pay for %d ¥\n", pay);
					chdir("0_9");
					char *buf_bmp1[10] = {"0.bmp","1.bmp","2.bmp","3.bmp","4.bmp","5.bmp","6.bmp","7.bmp","8.bmp","9.bmp",};
					//打印百位的图片
					for(i=0;i<10;i++)
					{
						if( i == pay/100)
						{
							if(i == 0)
							{
								break;
							}
							show_anybmp(150,0,100,100,buf_bmp1[i]);
						}
					}
					//打印十位的图片
					for(i=0;i<10;i++)
					{
						if( i == (pay/10)%10)
						{
							if(i == 0)
							{
								break;
							}
							show_anybmp(350,0,100,100,buf_bmp1[i]);
						}
					
					}
					//打印个位的图片
					for(i=0;i<10;i++)
					{
						if( i == pay%10)
						{
							show_anybmp(550,0,100,100,buf_bmp1[i]);
						}
					
					}
					chdir("/");
					sprintf(cmd,"delete from park where id=%d;",cardid);
					//从数据库删除数据
					ret = sqlite3_exec(mysq,cmd,NULL,NULL,NULL);
					if(ret != SQLITE_OK)
					{
						printf("delete error\n");
						exit(-1);
					}
					//从链表中删除
					delete_list_node(head,cardid);
					car--;
					for(flag_i = 0;flag_i<3;flag_i++)
					{
						if(flag[flag_i] == 1)
						{
							flag[flag_i] = 0;

							if(flag_i==0)
							{
								show_anybmp(150,380,100,100,"/0_9/green.bmp");				
							}
							else if(flag_i==1)
							{
								show_anybmp(350,380,100,100,"/0_9/green.bmp");				
							}
							else if(flag_i==2)
							{
								show_anybmp(550,380,100,100,"/0_9/green.bmp");				
							}
							break;
						}
					}
					goto AA;
				}
			}

			if(car<3)
			{
				printf("Welcome!!\n");
				//system("madplay welcome.mp3 &");
				bzero(cmd,100);
				//自定义车主姓名与年龄
				char car_name[20] = {0}; 
				int car_time;
				printf("plz input car_name\n");
				scanf("%s",car_name);  
				//获取停车的时间
				car_time = gettime();

				sprintf(cmd,"insert into park values(%d,\"%s\",%d);",cardid,car_name,car_time);
				//停入停车场(将信息存入数据库中)
				if(sqlite3_exec(mysq,cmd,NULL,NULL,NULL) != SQLITE_OK)
				{
					printf("insert error!\n");
					exit(-1);
				}
				//插入链表
				list_add_tail(head,cardid,car_time);	
				car++;
				for(flag_i = 0;flag_i<3;flag_i++)
				{
					if(flag[flag_i] == 0)
					{
						flag[flag_i] = 1;
						if(flag_i==0)
						{
							show_anybmp(150,380,100,100,"/0_9/red.bmp");				
						}
						else if(flag_i==1)
						{
							show_anybmp(350,380,100,100,"/0_9/red.bmp");				
						}
						else if(flag_i==2)
						{
							show_anybmp(550,380,100,100,"/0_9/red.bmp");				
						}
						break;
					}
				}
			}
			else
			{
				printf("Parking failure\n");
			}
									
		}
		usleep(100000);
	}
	pthread_exit(NULL);
}

int main(int argc, char const *argv[])
{
	int ret, i,fd,n;
	void *wait_pthreadid = NULL;

	//打开串口
	fd = open(DEV_PATH, O_RDWR | O_NOCTTY | O_NONBLOCK);
	if (fd < 0)
	{
		fprintf(stderr, "Open Gec210_ttySAC1 fail!\n");
		return -1;
	}
	/*初始化串口*/
	init_tty(fd);
	timeout.tv_sec = 1;
	timeout.tv_usec = 0;

	show_fullbmp("/0_9/park.bmp");

	//创建线程来处理串口函数	
	pthread_t pthreadid;
	ret = pthread_create(&pthreadid,NULL,serial_port,(void *)fd);
	if(ret == -1)
	{
		printf("pthread_create error!\n");
		return -1;
	}
	

	pthread_join(pthreadid,&wait_pthreadid);
	close(fd);
	return 0;	
}
