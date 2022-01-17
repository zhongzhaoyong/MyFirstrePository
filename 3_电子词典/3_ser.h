#ifndef __SERH__
#define __SERH__

#include <stdio.h>
#include <sqlite3.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <pthread.h>
#include <arpa/inet.h>
#include <unistd.h>
#define ERR_MSG(msg)do{\
	printf("line = %d\n",__LINE__);\
	perror(msg);\
}while(0);
#define PORT 8888
#define IP "192.168.1.8"
typedef struct {
	char type;
	char name[20];
	char pwd[20];
	char word[128];
}__attribute__((packed)) MSG;
typedef struct{
	sqlite3 *db;
	int newfd;
}DB;
sqlite3 * sqlite_init();
int do_dict_copy(sqlite3 *db);
int inet_init();
int do_register(MSG recv_msg,DB A);
int do_login(MSG recv_msg,DB A);
int do_search(MSG recv_msg,DB A);
int do_searchhistory(MSG recv_msg,DB A);
int do_logout(MSG recv_msg,DB A);
void *hanger(void*arg);
#endif
