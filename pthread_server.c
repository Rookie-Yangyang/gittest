#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <stdlib.h>
#include <getopt.h>
#include <pthread.h>
#include <ctype.h>
#include <signal.h>
#include <sqlite3.h>

typedef void *(THREAD_BODY) (void *thread_arg);
void *thread_worker(void *ctx);
int thread_create(pthread_t * thread_id, THREAD_BODY * thread_workbody, void *thread_arg);



void print_help(char *progname)
{
	printf("%s user\n",progname);
	printf("-p(--port):specify listen port\n");
	printf("-h(--help):print help information\n");
	return ;
}

int g_stop=0;
void sig_handler(int signum)
{
	if( g_stop==signum )
	{
		g_stop=1;
	}
}


int main(int argc, char **argv)
{
	int      		sockfd=-1;
	int      		clifd;
	int      		ch;
	int                     port=0;
	int                     on=1;
	int                     rv=-1;


	socklen_t                len;
	pthread_t                tid;


	 pthread_mutex_t mutex;
	 pthread_mutex_init(&mutex, NULL);

	struct sockaddr_in      servaddr;
	struct sockaddr_in      cliaddr;

	struct option opt[]={
		{"port",required_argument,NULL,'p'},
		{"help",no_argument,NULL,'h'},
		{NULL,0,NULL,0}
	};

	while( (ch=getopt_long(argc,argv,"p:h",opt,NULL))!=-1 )
	{
		switch(ch)
		{
			case 'p':
				port=atoi(optarg);
				break;
			case 'h':
				print_help(argv[0]);
				return 0;
		}
	}
	//printf("%d\n",ch);

	if( !port )
	{
		print_help(argv[0]);
		return 0;
	}

	sockfd=socket(AF_INET,SOCK_STREAM,0);
	if( sockfd<0 )
	{
		printf("create socket failure:%s\n",strerror(errno));
		return -1;
	}
	printf("create socket [%d] successfully!\n",sockfd);


	setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &on, sizeof(on));
	memset(&servaddr,0,sizeof(servaddr));
	servaddr.sin_family=AF_INET;
	servaddr.sin_port=htons(port);//将整型变量从主机字节顺序转变成网络字节顺序
	servaddr.sin_addr.s_addr=htonl(INADDR_ANY);//本机字节顺序转化为网络字节顺序

	rv=bind(sockfd,(struct sockaddr *)&servaddr,sizeof(servaddr));
	if( rv<0 )
	{
		printf("socket [%d] bind on port[%d] failure:%s\n",sockfd,port,strerror(errno));
		return -2;
	}
	listen(sockfd,13);

	signal(SIGUSR1,sig_handler);
	while( !g_stop )
	{
		clifd=accept(sockfd,(struct sockaddr *)&cliaddr,&len);
		if( clifd<0 )
		{
			printf("accept new client failure:%s\n",strerror(errno));
			return -1;
		}
		thread_create(&tid,thread_worker,  (void *)clifd);

		pthread_mutex_destroy(&mutex);

	}
	close(sockfd);
	return 0;
}

int thread_create(pthread_t * thread_id, THREAD_BODY * thread_workbody, void *thread_arg)
{
	int                        rv;
	pthread_attr_t             thread_attr;

	if( pthread_attr_init(&thread_attr) )
	{
		printf("pthread_attr_init() failure: %s\n", strerror(errno));
		goto CleanUp;
	}
	if( pthread_attr_setstacksize(&thread_attr, 120*1024) )
	{
		printf("pthread_attr_setstacksize() failure: %s\n", strerror(errno));
		goto CleanUp;
	}
	if( pthread_attr_setdetachstate(&thread_attr, PTHREAD_CREATE_DETACHED) )
	{
		printf("pthread_attr_setdetachstate() failure: %s\n", strerror(errno));
		goto CleanUp;

	}
	if( pthread_create(thread_id, &thread_attr, thread_workbody, thread_arg) )
	{
		printf("Create thread failure: %s\n", strerror(errno));
		goto CleanUp;
	}
	rv = 0;

CleanUp:

	/* Destroy the attributes of thread */
	pthread_attr_destroy(&thread_attr);
	return rv;
}

void *thread_worker(void *ctx)
{
	int clifd;
	int rv;
	char buf[1024];
	int i;


	 pthread_mutex_t mutex;
	 pthread_mutex_init(&mutex, NULL);

	if( !ctx )
	{
		printf("Invalid input arguments in %s()\n", __FUNCTION__);
		pthread_exit(NULL);
	}
	clifd = (int)ctx;
	printf("Child thread start to commuicate with socket client...\n");
	while(1)
	{
		pthread_mutex_lock(&mutex);

		memset(buf, 0, sizeof(buf));
		rv=read(clifd, buf, sizeof(buf));
		if( rv < 0)
		{
			printf("Read data from client sockfd[%d] failure: %s and thread will exit\n", clifd,
					strerror(errno));
			close(clifd);
			pthread_exit(NULL);
		}
		else if( rv == 0)
		{
			printf("Socket[%d] get disconnected and thread will exit.\n", clifd);
			close(clifd);
			pthread_exit(NULL);
		}
		else if( rv > 0 )
		{
			printf("Read %d bytes data from Server: %s\n", rv, buf);
			char         *p;
			char         *s;
			char         *string="/";
			char         device[50];
			char         time[50];


			printf("%s\n",buf);
			
			p=strstr(buf,string);
			if( p!=NULL )
			{
				
				p++;
				strncpy(device,buf,strlen(buf)-strlen(p)-1);
				printf("Device:%s\n",device);
			}
			else
			{
				printf("not found!\n");
			}
			s=strstr(p,string);
			if( s!=NULL )
			{
				s++;
				strncpy(time,p,strlen(p)-strlen(s)-1);
				printf("Time:%s\n",time);
				printf("wendu:%s\n",s);
			}
			else
			{
				printf("not found!\n");
			}


			/*上传数据库*/

			sqlite3            *db;
			char               *zErrMsg = 0;
			int                rc;
			char               *sql;



			rc = sqlite3_open("test.db",&db);
			if( rc )
			{
				fprintf(stderr, "Can't open database: %s\n", sqlite3_errmsg(db));
				exit(0);
			}
			else
			{
				fprintf(stdout, "Opened database successfully\n");    
			}
			/* Create SQL statement */
			sql = "CREATE TABLE if not exists Wangjiayang("  \
			       "Device char(30)     NOT NULL," \
			       "Time   char(30)     NOT NULL," \
			       "Wendu  char(30)     NOT NULL);";
			/* Execute SQL statement */
			rc = sqlite3_exec(db, sql, 0, 0, &zErrMsg);
			if( rc != SQLITE_OK )
			{
				fprintf(stderr, "SQL error: %s\n", zErrMsg);
				sqlite3_free(zErrMsg);
			}
			else
			{
				fprintf(stdout, "Table created successfully\n");
			}
	snprintf(buf,sizeof(buf),"INSERT INTO Wangjiayang (Device,Time,Wendu) VALUES('%s','%s','%s');",device,time,&s);
			sql = buf;
			/* Execute SQL statement */

			printf("char *sql=%s\n",sql);
			rc = sqlite3_exec(db, sql, 0, 0, &zErrMsg);
			if( rc != SQLITE_OK )
			{
				fprintf(stderr, "SQL error: %s\n", zErrMsg);
				sqlite3_free(zErrMsg);
			}
			else
			{
				fprintf(stdout, "Records created successfully\n");
			}

			 pthread_mutex_unlock(&mutex);



		}




	//	close(clifd);
	//	pthread_exit(NULL);
	}
}
		  

