#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <ctype.h>
#include <time.h>
#include <pthread.h>
#include <getopt.h>
#include <libgen.h>
#include <sys/types.h> 
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/epoll.h>
#include <sys/resource.h>


#define MAX_EVENTS 512
#define ARRAY_SIZE(x) (sizeof(x)/sizeof(x[0]))


void print_help(char *progname)
{
	printf("%s\n",progname);
	printf("-p(--port):specify listen port\n");
	printf("-h(--help):print help information\n");
}

/*define scoket init()*/
int socket_server_init(char *listen_ip,int listen_port)
{
	int                     sockfd=-1;
	int                     on=1;
	
	

	struct sockaddr_in       servaddr;
	struct sockaddr_in       cliaddr;

	sockfd=socket(AF_INET,SOCK_STREAM,0);
	if( sockfd<0 )
	{
		printf("create socket failure:%s\n",strerror(errno));
		return -1;
	}
	printf("create socket[%d] successfully\n",sockfd);

	setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &on, sizeof(on));

	memset(&servaddr,0,sizeof(servaddr));
	servaddr.sin_family=AF_INET;
	servaddr.sin_port=htons(listen_port);
	servaddr.sin_addr.s_addr=htonl(INADDR_ANY);

	if( (bind(sockfd,(struct sockaddr*)&servaddr,sizeof(servaddr)))<0 )
	{
		printf("bind on listen port failure:%s\n",strerror(errno));
		return -2;
	}

	listen(sockfd,64);

	close(sockfd);

	return 0;
}

/* Set open file description count to max */
void set_socket_rlimit(void)
{
	struct rlimit limit={0};

	getrlimit(RLIMIT_NOFILE,&limit);
	limit.rlim_cur=limit.rlim_max;
	setrlimit(RLIMIT_NOFILE,&limit);

	printf("set socket open fd max count to %ld\n", limit.rlim_max);
}





int main(int argc,char **argv)
{   
	int                       ch;
	int                       listenfd;
	int                       serv_port=0;
	int                       epollfd;
	int                       connfd;
	int                       i;
	int                       rv;

	char                      buf[1024];

	int                       events;
	struct epoll_event        event;
	struct epoll_event        event_array[MAX_EVENTS];

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
				serv_port=atoi(optarg);
				break;
			case 'h':
				print_help(argv[0]);
				return 0;
		}
	}

	if( !serv_port )
	{
		print_help(argv[0]);
		return 0;
	}


	set_socket_rlimit();
	listenfd=socket_server_init(NULL,serv_port);
	if( listenfd<0 )
	{
		printf("EEROR:%s server listen on port[%d] failure\n",argv[0],serv_port);
		return -2;
	}
	printf("%s server start to listen on port[%d]\n",argv[0],serv_port);


	/* 创建 epoll 句柄，把监听 socket 加入到 epoll 集合里 */
	if( (epollfd=epoll_create(MAX_EVENTS))<0 )
	{
		printf("epoll_create() failure: %s\n", strerror(errno));
		return -3;
	}
	event.events = EPOLLIN;
	event.data.fd = listenfd;

	if( epoll_ctl(epollfd, EPOLL_CTL_ADD, listenfd, &event) < 0)
	{
		printf("epoll add listen socket failure: %s\n", strerror(errno));
		return -4;
	}

	
	for( ; ;) 
	{
		events=epoll_wait(epollfd,event_array,MAX_EVENTS,-1);
		if( events<0 )
		{
			printf("epoll failure:%s\n",strerror(errno));
			break;
		}
		else if( events==0 )
		{
			printf("epoll get timeout\n");
			continue;
		}
		for(i=0;i<events;i++)
		{
			if ( (event_array[i].events&EPOLLERR) || (event_array[i].events&EPOLLHUP) )/*错误退出*/
			{
				printf("epoll_wait get error on fd[%d]:%s\n",event_array[i].data.fd,strerror(errno));
				
				epoll_ctl(epollfd, EPOLL_CTL_DEL,event_array[i].data.fd,NULL);;
				close(event_array[i].data.fd);
			}
			if( event_array[i].data.fd==listenfd )
			{
				connfd=accept(listenfd,(struct sockaddr *)NULL,NULL);
				if( connfd<0 )
				{
					printf("accept new client failure:%s\n",strerror(errno));
					continue;
				}
				event.data.fd=connfd;
				event.events=EPOLLIN;

				if( epoll_ctl(epollfd, EPOLL_CTL_ADD, connfd, &event) < 0 )
				{
					printf("epoll add client socket failure:%s\n",strerror(errno));
					close(event_array[i].data.fd);
					continue;
				}
				printf("epoll add client socket[%d] ok!\n",connfd);
			}
			else
			{
				if( (rv=read(event_array[i].data.fd,buf,sizeof(buf)))<0 )
				{
					printf("socket[%d] read failure or get disconncet and will be removed.\n",event_array[i].data.fd);
					epoll_ctl(epollfd, EPOLL_CTL_DEL, event_array[i].data.fd, NULL);
					close(event_array[i].data.fd);
					continue;
				}
				else
				{
					printf("socket[%d] read get %d bytes data\n", event_array[i].data.fd, rv);
				}
			}
		}
	}
cleanup:
	close(listenfd);
	return 0;
}



	

