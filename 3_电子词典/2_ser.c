#include "./3_ser.h"

int main(int argc, const char *argv[])
{
	//数据库初始化
	sqlite3 *db = sqlite_init();	
	//网络初始化
	int sfd = inet_init();
	int newfd;
	DB A;
	A.db = db;
	struct sockaddr_in cin;
	socklen_t addrlen = sizeof(cin);
	pthread_t tid;
	while(1)
	{
		newfd = accept(sfd,(struct sockaddr *)&cin,&addrlen);
		if(newfd < 0)
		{
			ERR_MSG("accept");
			return -1;
		}
		A.newfd = newfd;
		if(pthread_create(&tid,NULL,hanger,&A)!=0)
		{
			ERR_MSG("pthread_create");
			return -1;
		}
	}
	pthread_join(tid,NULL);
	close(sfd);
	return 0;
}
void *hanger(void *arg)
{
	DB A = *(DB *)arg;
	sqlite3 *db = A.db;
	int newfd = A.newfd;
	pthread_detach(pthread_self());
	MSG recv_msg;
	ssize_t res;
	while(1)
	{
		res = recv(newfd,&recv_msg,sizeof(recv_msg),0);
		if(res < 0)
		{
			ERR_MSG("recv");
			return NULL;
		}
		else if(res == 0)
		{
			printf("对方客户端关闭\n");
			char sql[128] = "";
			char *errmsg = NULL;
			sprintf(sql,"update user set state=\"0\" where name=\"%s\"",recv_msg.name);
			if(sqlite3_exec(db,sql,NULL,NULL,&errmsg) != SQLITE_OK)
			{
				printf("line = %d\n",__LINE__);
				printf("errmsg: %s\n",errmsg);
				return NULL;
			}
			printf("%s用户状态已修改\n",recv_msg.name);
			close(newfd);
			break;
		}
		switch(recv_msg.type)
		{
		case 'R':
			//注册
			do_register(recv_msg,A);
			break;
		case 'L':
			//登录
			do_login(recv_msg,A);
			break;
		case 'S':
			//查找单词
			do_search(recv_msg,A);
			break;
		case 'H':
			//查看历史记录
			do_searchhistory(recv_msg,A);
			break;
		case 'Q':
			do_logout(recv_msg,A);
			break;
		}
	}
	pthread_exit(NULL);
}
