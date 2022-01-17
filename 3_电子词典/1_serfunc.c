#include "./3_ser.h"
sqlite3 * sqlite_init()
{
	//创建数据库
	char filename[20] = "./sq.db";
	sqlite3 *db = NULL;
	if(sqlite3_open(filename,&db) != SQLITE_OK)
	{
		printf("sqlite open failed:  %s\n",sqlite3_errmsg(db));
		return NULL;
	}
	//创建电子词典表
	do_dict_copy(db);
	//创建user表
	char sql[128] = "create table if not exists user (name char primary key,pwd char,state char)";
	char *errmsg = NULL;
	if(sqlite3_exec(db,sql,NULL,NULL,&errmsg) != SQLITE_OK)
	{
		printf("line = %d\n",__LINE__);
		printf("errmsg: %s\n",errmsg);
		return NULL;
	}
	printf("创建user表成功\n");
	//将用户状态初始化
	bzero(sql,sizeof(sql));
	strcpy(sql,"update user set state=\"0\" where name=name");
	if(sqlite3_exec(db,sql,NULL,NULL,&errmsg) != SQLITE_OK)
	{
		printf("line = %d\n",__LINE__);
		printf("errmsg: %s\n",errmsg);
		return NULL;
	}
	printf("用户状态初始化成功\n");
	//创建history表成功
	bzero(sql,sizeof(sql));
	strcpy(sql,"create table if not exists history (name char,word char,mean char,time char)");
	if(sqlite3_exec(db,sql,NULL,NULL,&errmsg) != SQLITE_OK)
	{
		printf("line = %d\n",__LINE__);
		printf("errmsg: %s\n",errmsg);
		return NULL;
	}
	printf("创建history表成功\n");

	return db;
}
int do_dict_copy(sqlite3 *db)
{
	char *errmsg = NULL;
/*	sqlite3 *db = NULL;
	char filename[128] = "./sq.db";
	if(sqlite3_open(filename,&db) != SQLITE_OK)
	{
		printf("sqlite open failed:  %s\n",sqlite3_errmsg(db));
		return -1;
	}
*/
	//创建表格
	char sql[128] = "create table dict (word char,mean char)";
	int res = sqlite3_exec(db,sql,NULL,NULL,&errmsg);
	if(res != SQLITE_OK)
	{
		if(1 == sqlite3_errcode(db))
		{
			printf("表格已存在\n");
		}
		else
		{
			printf("创建失败\n");
			printf("errmsg: %s\n",errmsg);
			return -1;
		}
	}
	else
	{
		printf("创建成功\n");
		//打开dict
		FILE *fp = fopen("./dict.txt","r");
		if(NULL == fp)
		{
			ERR_MSG("fopen");
			return -1;
		}
		char buf[128] = "";
		char buf1[128] = "";
		char str[128] = "";
		char str1[128] = "";
		while(NULL != fgets(buf,sizeof(buf),fp))
		{
			sscanf(buf,"%[^ ]",str);
			printf("str1 = %s\n",str);
			sscanf(buf,"%*s%[^\n]",str1);
			printf("%s\n",str1);

			sprintf(buf1,"insert into dict values(\"%s\",\"%s\")",str,str1);
			if(sqlite3_exec(db,buf1,NULL,NULL,&errmsg) != SQLITE_OK)
			{
				printf("errmsg: %s\n",errmsg);
				return -1;
			}
			printf("插入成功\n");
			bzero(buf,sizeof(buf));
			bzero(buf1,sizeof(buf1));
			bzero(str,sizeof(str));
			bzero(str1,sizeof(str1));
		}
	
		fclose(fp);
	}
}
int inet_init()
{
	//网络初始化
	int sfd = socket(AF_INET,SOCK_STREAM,0);
	if(sfd < 0)
	{
		ERR_MSG("socket");
		return -1;
	}
	int resuse = 1;
	if(setsockopt(sfd,SOL_SOCKET,SO_REUSEADDR,&resuse,sizeof(resuse)) < 0)
	{
		ERR_MSG("setsockopt");
		return -1;
	}
	struct sockaddr_in sin;
	sin.sin_family 		= AF_INET;
	sin.sin_port 		= htons(PORT);
	sin.sin_addr.s_addr = inet_addr(IP);
	if(bind(sfd,(struct sockaddr *)&sin,sizeof(sin)) < 0)
	{
		ERR_MSG("bind");
		return -1;
	}
	if(listen(sfd,10) < 0)
	{
		ERR_MSG("listen");
		return -1;
	}
	printf("监听成功\n");
	return sfd;
}
int do_register(MSG recv_msg,DB A)
{
	char name[20]  = "";
	strcpy(name,recv_msg.name);
	char pwd[20] = "";
	strcpy(pwd,recv_msg.pwd);
	char *errmsg = NULL;
	sqlite3 *db = A.db;
	int newfd = A.newfd;
	char sql[128] = "";
	char **presult = NULL;
	int row,column;
	//判断用户名是否存在
	sprintf(sql,"select * from user where name=\"%s\"",name);
	if(sqlite3_get_table(db,sql,&presult,&row,&column,&errmsg) != SQLITE_OK)
	{
		printf("line = %d\n",__LINE__);
		printf("errmsg: %s\n",errmsg);
		return -1;
	}
	if(row > 0)
	{
		//用户已存在
		bzero(sql,sizeof(sql));
		strcpy(sql,"注册失败，用户名已存在");
		if(send(newfd,sql,sizeof(sql),0) < 0)
		{
			ERR_MSG("send");
			return -1;
		}
	}
	else
	{
		//用户不存在注册用户
		bzero(sql,sizeof(sql));
		sprintf(sql,"insert into user values(\"%s\",\"%s\",\"0\")",name,pwd);
		if(sqlite3_exec(db,sql,NULL,NULL,&errmsg) != SQLITE_OK)
		{
			printf("line = %d\n",__LINE__);
			printf("errmsg: %s\n",errmsg);
			return -1;
		}
		bzero(sql,sizeof(sql));
		strcpy(sql,"注册成功");
		if(send(newfd,sql,sizeof(sql),0) < 0)
		{
			ERR_MSG("send");
			return -1;
		}
	}
	return 0;
}
int do_login(MSG recv_msg,DB A)
{
	char name[20] = "";
	strcpy(name,recv_msg.name);
	char pwd[20] = "";
	strcpy(pwd,recv_msg.pwd);

	char *errmsg = NULL;
	char **presult = NULL;
	int row = 0;
	int column = 0;
	sqlite3 *db = A.db;
	int newfd = A.newfd;
	char flag = 0;
	char sql[128] = "";
	sprintf(sql,"select * from user where name=\"%s\" and pwd=\"%s\"",name,pwd);
	if(sqlite3_get_table(db,sql,&presult,&row,&column,&errmsg) != SQLITE_OK)
	{
		printf("line = %d\n",__LINE__);
		printf("errmsg: %s\n",errmsg);
		return -1;
	}
	if(row > 0)
	{
		//找到name和pwd了
		//判断该用户是否登录
		int line = 0;
		bzero(sql,sizeof(sql));
		sprintf(sql,"select * from user where name=\"%s\" and state=\"0\"",name);
		if(sqlite3_get_table(db,sql,&presult,&line,&column,&errmsg) != SQLITE_OK)
		{
			printf("line = %d\n",__LINE__);
			printf("errmsg: %s\n",errmsg);
			return -1;
		}
		if(line > 0 )
		{
			flag = 1;
			if(send(newfd,&flag,sizeof(flag),0) < 0)
			{
				ERR_MSG("send");
				return -1;
			}
			bzero(sql,sizeof(sql));
			sprintf(sql,"update user set state=\"1\" where name=\"%s\"",name);

			if(sqlite3_get_table(db,sql,&presult,&line,&column,&errmsg) != SQLITE_OK)
			{
				printf("line = %d\n",__LINE__);
				printf("errmsg: %s\n",errmsg);
				return -1;
			}
			flag =0;
		}
		else
		{
			if(send(newfd,&flag,sizeof(flag),0) < 0)
			{
				ERR_MSG("send");
				return -1;
			}

		}

	}
	else 
	{
		if(send(newfd,&flag,sizeof(flag),0) < 0)
		{
			ERR_MSG("send");
			return -1;
		}
	}
	return 0;
}

int do_search(MSG recv_msg,DB A)
{
	sqlite3 *db = A.db;
	int newfd = A.newfd;
	char word[128] = "";
	strcpy(word,recv_msg.word);
	char name[128] = "";
	strcpy(name,recv_msg.name);
	char **presult = NULL;
	char *errmsg = NULL;
	int row,column;
	char sql[256] = "";
	sprintf(sql,"select * from dict where word=\"%s\"",word);

	if(sqlite3_get_table(db,sql,&presult,&row,&column,&errmsg) != SQLITE_OK)
	{
		printf("line = %d\n",__LINE__);
		printf("errmsg: %s\n",errmsg);
		return -1;
	}
	if(row > 0)
	{
		//如果找到单词发送
		int i = 0;
		for(i=0;i<row;i++)
		{
			bzero(sql,sizeof(sql));
			sprintf(sql,"%s\t%s",presult[(i+1)*column],presult[(i+1)*column+1]);
			if(send(newfd,sql,sizeof(sql),0) < 0)
			{
				ERR_MSG("send");
				return -1;
			}
			//将查询记录存进history表
			//printf("%4d-%02d-%02d %02d:%02d:%02d\r", \
			info->tm_year+1900, info->tm_mon+1, info->tm_mday,\
				info->tm_hour, info->tm_min, info->tm_sec);
			bzero(sql,sizeof(sql));
			time_t t1;
			t1 = time(NULL);
			struct tm* info =NULL;
			info = localtime(&t1);
			char t2[128] = "";
			sprintf(t2,"%4d-%02d-%02d %02d:%02d:%02d",info->tm_year+1900,\
					info->tm_mon+1,info->tm_mday,info->tm_hour,info->tm_min,\
					info->tm_sec);
			sprintf(sql,"insert into history values(\"%s\",\"%s\",\"%s\",\"%s\")",name,presult[(i+1)*column],presult[(i+1)*column+1],t2);
			if(sqlite3_exec(db,sql,NULL,NULL,&errmsg) != SQLITE_OK)
			{
				printf("line = %d\n",__LINE__);
				printf("errmsg: %s\n",errmsg);
				return -1;
			}
		}
		printf("成功将查询记录存入history表\n");
		bzero(sql,sizeof(sql));
		strcpy(sql,"over");
		if(send(newfd,sql,sizeof(sql),0) < 0)
		{
			ERR_MSG("send");
			return -1;
		}
	}
	else
	{
		bzero(sql,sizeof(sql));
		strcpy(sql,"NOT FOUND");
		if(send(newfd,sql,sizeof(sql),0) < 0)
		{
			ERR_MSG("send");
			return -1;
		}
	}
}

int do_searchhistory(MSG recv_msg,DB A)
{
	sqlite3 *db = A.db;
	int newfd = A.newfd;
	char name[128] = "";
	strcpy(name,recv_msg.name);
	char sql[256] = "";
	sprintf(sql,"select * from history where name=\"%s\"",name);
	char **presult = NULL;
	int row,column;
	char *errmsg;
	if(sqlite3_get_table(db,sql,&presult,&row,&column,&errmsg) != SQLITE_OK)
	{
		printf("line = %d\n",__LINE__);
		printf("errmsg: %s\n",errmsg);
		return -1;
	}
	if(row > 0)
	{
		int i,j;
		bzero(sql,sizeof(sql));
		char buf[256] = "";
		for(i=0;i<row;i++)
		{
			bzero(buf,sizeof(buf));
	
		//	printf("%s  %s  %s  %s\n",presult[(i+1)*column],presult[(i+1)*column+1],\
					presult[(i+1)*column+2],presult[(i+1)*column+3]);	
			sprintf(buf,"%s  %s  %s  %s",presult[(i+1)*column],presult[(i+1)*column+1],\
					presult[(i+1)*column+2],presult[(i+1)*column+3]);	
			if(send(newfd,buf,sizeof(buf),0) < 0)
			{
				ERR_MSG("send");
				return -1;
			}
		}
		bzero(buf,sizeof(buf));
		strcpy(buf,"over");
		if(send(newfd,buf,sizeof(buf),0) < 0)
		{
			ERR_MSG("send");
			return -1;
		}
	}
	else
	{
		bzero(sql,sizeof(sql));
		strcpy(sql,"NOT FOUND");
		if(send(newfd,sql,sizeof(sql),0) < 0)
		{
			ERR_MSG("send");
			return -1;
		}
	}
	return 0;
}

int do_logout(MSG recv_msg,DB A)
{
	sqlite3 *db = A.db;
	int newfd = A.newfd;
	char name[128] = "";
	strcpy(name,recv_msg.name);
	char sql[128] = "";
	char *errmsg = NULL;
	char flag = 1;
	sprintf(sql,"update user set state=\"0\" where name=\"%s\"",name);
	if(sqlite3_exec(db,sql,NULL,NULL,&errmsg) != SQLITE_OK)
	{
		printf("line = %d\n",__LINE__);
		printf("errmsg: %s\n",errmsg);
		return -1;
	}
	if(send(newfd,&flag,sizeof(flag),0) < 0)
	{
		ERR_MSG("send");
		return -1;
	}
	return 0;
}
