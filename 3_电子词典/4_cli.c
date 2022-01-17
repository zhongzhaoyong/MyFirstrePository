#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <string.h>
#include <arpa/inet.h>
#include <stdlib.h>
#define ERR_MSG(msg)do{\
	printf("line = %d\n",__LINE__);\
	perror(msg);\
}while(0);
#define PORT 8888
#define IP "192.168.1.8"
typedef struct{
	char type;
	char name[20];
	char pwd[20];
	char word[128];
}__attribute__((packed)) MSG;
int do_login_page(int sfd);
int do_register(int sfd);
int do_login(int sfd);
int do_searchword(int sfd,char *name);
int do_searchhistory(int sfd,char *name);
int do_logout(int sfd,char *name);
int main(int argc, const char *argv[])
{
	int sfd = socket(AF_INET,SOCK_STREAM,0);
	if(sfd < 0)
	{
		ERR_MSG("scoket");
		return -1;
	}
	struct sockaddr_in sin;
	sin.sin_family 		= AF_INET;
	sin.sin_port 		= htons(PORT);
	sin.sin_addr.s_addr = inet_addr(IP);

	if(connect(sfd,(struct sockaddr *)&sin,sizeof(sin)) < 0)
	{
		ERR_MSG("connect");
		return -1;
	}
	printf("连接成功\n");

	do_login_page(sfd);
	return 0;
}
int do_login_page(int sfd)
{
	char choose = 0;
	while(1)
	{
		printf("-----------------------\n");
		printf("--------1.注册---------\n");
		printf("--------2.登录---------\n");
		printf("--------3.退出---------\n");
		printf("-----------------------\n");
		printf("请输入>>>\n");
		scanf("%c",&choose);
		while(getchar()!=10);
		switch(choose)
		{
		case '1':
			//注册
			do_register(sfd);
			break;
		case '2':
			//登录
			do_login(sfd);
			break;
		case '3':
			exit(0);
			break;
		default :
			printf("输入错误,请重新输入\n");
			continue;
		}
	}
}
int do_register(int sfd)
{
	printf("请输入注册名>>>");
	char name[20] = "";
	scanf("%s",name);
	while(getchar()!=10);

	printf("请输入密码>>>");
	char pwd[20] = "";
	scanf("%s",pwd);
	while(getchar()!=10);
	MSG send_msg;
	send_msg.type = 'R';
	strcpy(send_msg.name,name);
	strcpy(send_msg.pwd,pwd);
	
	if(send(sfd,&send_msg,sizeof(send_msg),0) < 0)
	{
		ERR_MSG("send");
		return -1;
	}
	char sql[128] = "";
	if(recv(sfd,sql,sizeof(sql),0) < 0)
	{
		ERR_MSG("recv");
		return -1;
	}
	printf("%s\n",sql);
	bzero(sql,sizeof(sql));
	printf("输入任意字符清屏>>>");
	if(getchar())
	{
		system("clear");
	}
}
int do_login(int sfd)
{
	printf("请输入登录名>>>");
	char name[20] = "";
	scanf("%s",name);
	while(getchar()!=10);

	printf("请输入密码>>>");
	char pwd[20] = "";
	scanf("%s",pwd);
	while(getchar()!=10);
	MSG send_msg;
	send_msg.type = 'L';
	strcpy(send_msg.name,name);
	strcpy(send_msg.pwd,pwd);

	if(send(sfd,&send_msg,sizeof(send_msg),0) < 0)
	{
		ERR_MSG("send");
		return -1;
	}
	printf("发送登录请求成功\n");
	char flag = 0;
	if(recv(sfd,&flag,sizeof(flag),0) < 0)
	{
		ERR_MSG("recv");
		return -1;
	}
	printf("flag =%d\n",flag);
	while(1)
	{
		if(flag == 1)
		{
			printf("登录成功\n\n");
			printf("---------------------------------\n");
			printf("-----------1.查找单词------------\n");
			printf("-----------2.查看历史记录--------\n");
			printf("-----------3.退出登录------------\n");
			printf("---------------------------------\n");

			char choose = 0;
			printf("请输入选项>>>");
			scanf("%c",&choose);
			while(getchar()!=10);
			switch(choose)
			{
			case '1':
				//查找单词
				do_searchword(sfd,name);
				break;
			case '2':
				//查看历史记录
				do_searchhistory(sfd,name);
				break;
			case '3':
				//返回上一层
				do_logout(sfd,name);
				goto END;
				break;
			}
		}

		else
		{
			printf("登录失败\n");
			return -1;
		}
	}
END:
	return 0;
}
int do_searchword(int sfd,char *name)
{
	MSG send_msg;
	send_msg.type = 'S';
	strcpy(send_msg.name,name);
	printf("请输入要查找的单词>>>");
	char word[128] = "";
	scanf("%s",word);
	while(getchar()!=10);
	strcpy(send_msg.word,word);
	if(send(sfd,&send_msg,sizeof(send_msg),0) < 0)
	{
		ERR_MSG("send");
		return -1;
	}
	char buf[256] = "";
	while(1)
	{
		bzero(buf,sizeof(buf));
		if(recv(sfd,buf,sizeof(buf),0) < 0)
		{
			ERR_MSG("recv");
			return -1;
		}
		if(strncasecmp(buf,"over",4) == 0)
		{
			break;
		}
		printf("查询结果>>>   %s\n",buf);
	}
	return 0;
}
int do_searchhistory(int sfd,char *name)
{
	MSG send_msg;
	send_msg.type = 'H';
	strcpy(send_msg.name,name);
//	printf("请问你要查找谁的历史记录，请输入姓名>>> \n");
//	scanf("%s",send_msg.name);
//	while(getchar()!=10);
	if(send(sfd,&send_msg,sizeof(send_msg),0) < 0)
	{
		ERR_MSG("send");
		return -1;
	}
	char buf[256] ="";
	while(1)
	{
		bzero(buf,sizeof(buf));
		if(recv(sfd,buf,sizeof(buf),0) < 0)
		{
			ERR_MSG("recv");
			return -1;
		}
		if(strncasecmp(buf,"over",4) == 0)
		{
			break;
		}
		printf("历史记录>>>  %s\n",buf);
		if(strncasecmp(buf,"not found",9) == 0)
		{
			break;
		}

	}
	return 0;

}

int do_logout(int sfd,char *name)
{
	MSG send_msg;
	send_msg.type = 'Q';
	strcpy(send_msg.name,name);
	if(send(sfd,&send_msg,sizeof(send_msg),0) < 0)
	{
		ERR_MSG("send");
		return -1;
	}
	char flag = 0;
	if(recv(sfd,&flag,sizeof(flag),0) < 0)
	{
		ERR_MSG("recv");
		return -1;
	}
	if(1 == flag)
	{
		printf("退出登录成功\n");
	}
	return 0;
}
