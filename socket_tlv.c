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
#include "crc-itu-t.c"
#include "crc-itu-t.h"


#define TLV_MIN_SIZE        6
#define PACK_HEADER         0xfd
#define TLV_MAX_SIZE        8
#define BUFSIZE             128



void print_usage(char *progname)
{       
	printf("%s usage:\n",progname);
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
	int                   bv=0;
	socklen_t             len;
	struct sockaddr_in    servaddr;
	struct sockaddr_in    cliaddr;
	pid_t                 pid;



	struct option opts[]={
		{"port",required_argument,NULL,'p'},
		{"help",no_argument,NULL,'h'},
		{NULL,0,NULL,0}
	};
	while( (ch=getopt_long(argc,argv,"p:h",opts,NULL))!=-1 )
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


		memset(&cliaddr,0,sizeof(cliaddr));
		memset(&len,0,sizeof(len));
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
				rv=read(clifd,&buf[bv],sizeof(buf));
				if(rv<0)
				{

					printf("read data from client socket[%d] failure:%s",clifd,strerror(errno));
					close(clifd);
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
					bv=bv+rv;
					printf("read %d bytes data from server\n",rv);
					for(i=0;i<bv;i++)
					{
						printf("0x%02x ",(unsigned char)buf[i]);

					}
					printf("\n");
					bv=unpacktlv(buf,bv);

				}
			}
		}
	}
	close(sockfd);
	return 0;
}

int unpacktlv(char *buf,int bv)
{
	int                   i;
	int                   tlv_len;
	char                  *ptr=NULL;


	unsigned short        crc16;
	unsigned short        crc;

	if( !buf )
	{
		printf("Invailed input!\n");
		return 0;
	}

	if( buf==NULL )
	{
		printf("buf is NULL!\n");
		return 0;
	}
	printf("buf is not NULL\n");
go_on:
	if( bv<TLV_MIN_SIZE )
	{
		printf("buf 不完整\n");
		return bv;
	}
	printf("buf is good!\n");

	for(i=0;i<bv;i++)
	{

		if( (unsigned char)buf[i]!=PACK_HEADER )
		{
			//printf("can not find header..\n");
			continue ;
		}
		printf("\nFind header!\n");

		if( bv-i<2 )
		{
			printf("can not find length...\n");
			memmove(buf,&buf[i],bv-i);
			bv=bv-i;
			goto go_on;
		}

		ptr=&buf[i];
		tlv_len=ptr[2];

		printf("tlv length:%d\n",tlv_len);

		if( tlv_len<TLV_MIN_SIZE || tlv_len>TLV_MAX_SIZE )
		{
			printf("tlv_len is error!\n");
			memmove(buf,&ptr[2+1],bv-i-2);
			bv=bv-i-2;
			goto go_on;
		}
		
		printf("tlv_len is ture!\n");

		if( tlv_len>bv-i )
		{
			printf("Error!\n");
			memmove(buf,ptr,bv-i);
			return bv-i;
		}

		crc16=crc_itu_t(MAGIC_CRC,(unsigned char*)ptr, tlv_len-2);
		crc=bytes_to_ushort((unsigned char*)&ptr[tlv_len-2],2) ;


		if( crc!=crc16 )
		{
			printf("crc is not ture!\n");
			memmove(buf,&ptr[tlv_len],bv+tlv_len);
			bv=bv+tlv_len;
			goto go_on;
		}

		printf("crc is true!\n");

		printf("data:0x%02x ",(unsigned char)ptr[3]);
		printf("\n");

		memmove(buf,&ptr[tlv_len],bv-i-tlv_len);
		bv=bv-i-tlv_len;
		goto go_on;
	}
	return 0;
}
