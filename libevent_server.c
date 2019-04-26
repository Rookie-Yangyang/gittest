#include <stdio.h>
#include <string.h>
#include <getopt.h>
#include <errno.h>
#include <stdlib.h>
#include <event2/event.h>
#include <event2/bufferevent.h>

#define MAXSIZE      256

void print_help(char *progname)
{
	printf("The progname:%s\n",progname);
	printf("p(--port):specify listen port!\n");
	printf("h(--help):print help information!\n");
	return ;
}


void do_accept(evutil_socket_t sockfd, short event, void *arg)
{
	struct event_base *base = (struct event_base *)arg;

	evutil_socket_t                clifd;
	struct sockaddr_in             servaddr;
	socklen_t                      len;

	clifd=accept(sockfd,(struct sockaddr *)&servaddr,&len);
	if( clifd<0 )
	{
		printf("Accept new client failure:%s\n",strerror(errno));
		return -1;
	}
	if( clifd>FD_SETSIZE )
	{
		printf("Error！\n");
		return -2;
	}

	printf("ACCEPT: clifd = %u\n", clifd);

	/*使用bufferevent_socket_new创建一个struct bufferevent *bev，关联该sockfd，托管给event_base*/

	struct bufferevent *bev=buferevent_socket_new(base,clifd,BEV_OPT_CLOSE_ON_FREE);

	/*使用bufferevent_setcb(bev, read_cb, write_cb, error_cb, (void *)arg)将EV_READ/EV_WRITE对应的函数*/
	bufferevent_setcb(bev,read_cb,NULL,error_cb,arg);

	/*使用bufferevent_enable(bev, EV_READ|EV_WRITE|EV_PERSIST)来启用read/write事件*/
	bufferevent_enable(bev,EV_READ|EV_WRITE|EV_PERSIST);
}

void read_cb(struct bufferevent *bev, void *arg)
{
	char                  msg[MAX_SIZE+1];
	int                   n;

	evutil_socket_t       fd = bufferevent_getfd(bev);

	n=bufferevent_read(bev,msg,MAX_SIZE);
	while( n>0 )
	{
		msg[n]='\0';
		printf("fd = %u\n",fd);
		bufferevent_write(bev,msg,n);
	}
}
/*客户端write什么都不用做*/
void write_cb(struct bufferevent *bev, void *arg){}

void error_cb(struct bufferevent *bev,short event,void *arg)
{
	evutil_socket_t fd = bufferevent_getfd(bev);

	if (event & BEV_EVENT_TIMEOUT) {
		printf("Timed out\n"); //if bufferevent_set_timeouts() called
	}
	else if (event & BEV_EVENT_EOF) {
		printf("connection closed\n");
	}
	else if (event & BEV_EVENT_ERROR) {
		printf("some other error\n");
	}
	bufferevent_free(bev);
}


int main(int argc,char **argv)
{
	int                         ch;
	int                         port=0;

	evutil_socket_t             sockfd;
	struct sockaddr_in          cliaddr;
	struct sockaddr_in          server_addr;
	socklen_t                   len;

	struct event *listen_event;

	struct option opt[]={
		{"port",required_arguments,NULL,'p'},
		{"help",no_argument,NULL,'h'},
		{NULL,0,NULL,0}
	};

	while( (ch=getopt_long(argc,argv,"p:h",opt,NULL))!=-1 )
	{
		switch(ch)
		{
			case 'p':
				port=atoi(optarg);
			case 'h':
				print_help(aegv[0]);
				return 0;
		}
	}

	sockfd=socket(AF_INET,SOCK_STREAM,0);
	if( sockfd<0 )
	{
		printf("create socket failure:%s\n",strerror(errno));
		return -1;
	}
	printf("create socket[%d] successfully\n",sockfd);

	/*setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &on, sizeof(on));*/
	evutil_make_listen_socket_reuseable(sockfd);/*实现端口重用，返回值为0成功，为-1代表失败*/

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


	struct event_base* base = event_base_new();/*创建一个event_base*/
	assert(base != NULL);


	
	listen_event = event_new(base, listener, EV_READ | EV_PERSIST, sockfd, (void *)base);/*创建一个event，将该socket托管给event_base*/



	event_add(listen_event, NULL);

	/*启用事件循环*/
	event_base_dispatch(base);

	printf("The end!\n");
	return 0;
}
	



