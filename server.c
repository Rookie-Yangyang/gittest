#include <stdio.h>
#include <errno.h>
#include <string.h>
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


int g_stop=0;
void print_help(char *progname)
{
	printf("%s usage: \n", progname);
	printf("-p(--port): sepcify server listen port.\n");
 	printf("-h(--Help): print this help information.\n");
	return ;
}
void sig_handler(int signum)
{
	if( g_stop=signum )
	{
		g_stop=1;
	}
}

int main(int argc,char **argv)
{
	int                      ch;
	int                      port=0;
	int                      sockfd=-1;
	int                      rv;
	int                      clifd=-1;
	int                      on=1;

	char                     buf[1024];

	pid_t                    pid;
	socklen_t                len;
	struct sockaddr_in       servaddr;
	struct sockaddr_in       cliaddr;

	struct option opt[]={
		{"port", required_argument, NULL, 'p'},
		{"help", no_argument, NULL, 'h'},
		{NULL, 0, NULL, 0}
		};

	while( (ch=getopt_long(argc, argv, "p:h", opt, NULL))!=-1 )
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
	if( !port )
	{
		print_help(argv[0]);
		return 0;
	}

	sockfd=socket(AF_INET,SOCK_STREAM,0);
	if(sockfd<0)
	{
		printf("create socket failure:%s\n",strerror(errno));
		return -1;
	}
	printf("create socket[%d],successfully!\n",sockfd);

	setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &on, sizeof(on));
	memset(&servaddr,0,sizeof(servaddr));
	servaddr.sin_family=AF_INET;
	servaddr.sin_port=htons(port);
	servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
	 
	rv=bind(sockfd,(struct sockaddr *)&servaddr,sizeof(servaddr));
	if( rv<0 )
	{
		printf("Socket[%d] bind on port[%d] failure: %s\n", sockfd, port, strerror(errno));
		return -2;
	}
	printf("socket[%d] bind on port [%d] successfully!\n",sockfd,port,strerror(errno));

	listen(sockfd,13);
	printf("start to listen on port [%d]\n",port);

	while( !g_stop )
	{
		printf("start to accept new client incoming...\n");

		clifd=accept(sockfd,(struct sockaddr *)&cliaddr,&len);
		if( clifd<0 )
		{
			printf("accept new client failure:%s\n",strerror(errno));
			continue;
		}
		printf("Accept new client[%s:%d] successfully\n", inet_ntoa(cliaddr.sin_addr),ntohs(cliaddr.sin_port));

		pid = fork();
		if( pid < 0 )
		{
			printf("fork() create child process failure: %s\n", strerror(errno));
			close(clifd);
			continue;
		}
		else if( pid > 0 )
		{
			/* Parent process close client fd and goes to accept new socket client again */
			close(clifd);
			continue;
		}
		else if ( 0 == pid )
		{
			printf("Child process start to commuicate with socket client...\n");
			/* Child process close the listen socket fd */
			while(1)
			{
				memset(buf, 0, sizeof(buf));
				rv=read(clifd, buf, sizeof(buf));
				if( rv < 0 )
				{

					printf("Read data from client sockfd[%d] failure: %s\n", clifd,strerror(errno));
					continue;
				}

				else if( rv == 0) 
				{    
					printf("Socket[%d] get disconnected\n", clifd);
					continue;

				}
				else if( rv > 0 )
				{
					printf("Read %d bytes data from Server: %s\n", rv, buf);


				}
				//解析数据
				char             *string="/";
				char             *p;
				char             *s;
				char             device[50];
				char             time[50];
				char             wendu[50];

				p=strstr(buf,string);
				printf("%s\n",p);
				if(p!=NULL)
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
				if(s!=NULL)
				{
					s++;
					strncpy(time,p,strlen(p)-strlen(s)-1);
					printf("Time:%s\n",time);
					printf("Wendu:%s\n",s);
				}
				else
				{
					printf("not found!\n");
				}

				//上传到数据库

				char               *zErrMsg = 0;
				int                rc;
				char               *sql;
				sqlite3            *db;
				rc = sqlite3_open("test.db", &db);
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
				       "ID      INT PRIMARY KEY,"\
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
			}
		}

	}
	close(sockfd);
	return 0;
}
