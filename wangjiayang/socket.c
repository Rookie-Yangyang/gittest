#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <stdlib.h>
#include <getopt.h>
#include <ctype.h>



void print_usage(char *progname)
{
	ptintf("%s usage:\n",progname);
	printf("-p(--port):sepcify server listen port.\n");
	printf("-h(--help):print this help information\n");
	return ;
}
int main(int argc,char **argv)
{
	int                   ch;
	int                   sockfd=-1;
	int                   clifd;
        int                   on=1;
	int                   rv;
	int                   port=0;
	socklen_t             len;
        struct sockaddr_in    servaddr;
	struct sockaddr_in    cliaddr;
        pid_t                 pid;



	struct option opts[]={
		{"port",required_argument,NULL,'p'},
		{"help",no_argument,NULL,'h'},
		{NULL,0,NULL,0}
	};
	while( (ch=getopt_long(argc,argv,"p:h",opts,NULL))!=0 )
	{
	        switch(ch)
		{
        	case 'p':
			port=atoi(optarg);
			break;
		case 'h':
			print_usage(argv[0]);
			return 0;
		}
	}
	if(!port)
	{
		print_usage(argv[0]);
		return;
	}
	sockfd=socket(AF_INET,SOCK_STREAM,0);
	if(sockfd<0)
	{
		printf("create socket failure:%s\n",strerror(errno));
		return -1;
	}
	printf("create sockfd[%d] successfully!\n",sockfd);
        
	setsockopt(sockfd,SOL_SOCKET,SO_REUSEADDR,&on,sizeof(on));

        memset(&servaddr,0,sizeof(servaddr));
        servaddr.sin_family=AF_INET;
	servaddr.sin_port=htons(port);
	servaddr.sin_addr.s_addr=htonl(INADDR_ANY);
	rv=bind(sockfd,(struct sockaddr *)&servaddr,sizeof(servaddr));
	if(rv<0)
	{
		printf("socket[%d] bind on port[%d] failure:%s\n",sockfd,port,strerror(errno));
		return -2;
	}
	listen(sockfd,13);
	printf("start to listen on port[%d]\n",port);
        
	while(1)
	{
		printf("start accept new client incoming...\n");

		clifd=accept(sockfd,(struct sockaddr *)&clifd,&len);
                if(clifd<0)
		{
			printf("accept new client failure:%s\n",strerror(errno));
			continue;
		}
		printf("accept new client[%s:%d] successfully!\n",inet_ntoa(cliaddr.sin_addr),ntohs(cliaddr.sin_port));
		pid=fork();
		if(pid<0)
		{
		    printf("fork() create child process failure:%s\n",strerror(errno));
		    close(clifd);
		    continue;
		}
		else if(pid>0)
		{
		    close(clifd);
		    continue;
		}
		else if(pid==0)
		{
		
		    char       buf[1024];
		    int        i;
		    printf("child process start to commuicate with socket client...\n");
		    close(sockfd);
		   while(1)
		   {
			  memset(buf,0,sizeof(buf));
			  rv=read(clifd,buf,sizeof(buf));
			  if(rv<0)
			  {

			  printf("read data from client socket[%d] failure:%s",clifd,strerror(errno));                          close(clifd);
			  exit(0);
			  }
			  else if(rv==0)
			  {
			      printf("socket[%d] get disconnected...\n",clifd);
			      close(clifd);
			      exit(0);
			  } 
			  else if(rv>0)
			  {
			       printf("read %d bytes data from server;%s\n",rv,buf);
			  }
			  for(i=0;i<rv;i++)
	                  {
	            	           buf[i]=toupper(buf[i]);
	        	  }
		          rv=write(clifd,buf,rv);
		          if(rv<0)
			  {
		               printf("write to client by socket[%d] failure:%s\n",clifd,strerror(errno));
			       close(clifd);
			       exit(0);
			  }
		   }
		}
	}
	close(sockfd);
	return 0;
}
		    
		   

                


