#include <stdio.h>
#include "crc-itu-t.h"
#include "crc-itu-t.c"
#include <errno.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <getopt.h>

#define TLV_MIN_SIZE       (TLV_FIXED_SIZE+1)
#define PACK_HEADER        0xFD
#define DATA0              0
#define DATA1              1
#define BUFSIZE            128
#define TLV_FIXED_SIZE     5
#define LINELEN 81
#define CHARS_PER_LINE 16


enum
{
	Tag_data= 1,
};

void print_help(char *progname)
{
	printf("The prog name:%s\n",progname);
	printf("-i(--ipaddr):specify server ip!\n");
	printf("-p(--port):specify server port!\n");
	printf("-h(--help):print help information\n");
	return ;
}




int packtlv_data(char *buf,int size,int data)
{
	unsigned short            crc16=0;
	int                       packlen=TLV_FIXED_SIZE+1;

	int                      i;

	if( !buf||size<TLV_MIN_SIZE )
	{
		printf("Invalid input arguments\n");
		return 0;
	}

	/*pack header*/
	buf[0]=PACK_HEADER;
	/*Tag*/
	buf[1]=Tag_data;
	/*pack length*/
	buf[2]=packlen;
	/*pack value*/
	buf[3]=(DATA0==data)?0X00:0X01;
	/* Calc CRC16 checksum value from Packet Head(buf[0])~ Packet Value(buf[3]) */
	crc16 = crc_itu_t(MAGIC_CRC, buf, 4);
	/* Append the 2 Bytes CRC16 checksum value into the last two bytes in packet buffer */
	ushort_to_bytes(&buf[4], crc16);
	for(i=0;i<packlen;i++)
	{

		printf("0x%02x ",(unsigned char)buf[i]);
	}
	printf("\n");

	return packlen;
}



int main(int argc,char **argv)
{
	char                     buf[BUFSIZE];
	char                     *server_ip;
	int                      server_port=0;
	int                      bytes;
	int                      ch;

	int           sockfd=-1;
	int           rv=-1;
	struct sockaddr_in           servaddr;
	
	struct option opt[]={
		{"ipaddr",required_argument,NULL,'i'},
		{"port",required_argument,NULL,'p'},
		{"help",no_argument,NULL,'h'},
		{NULL,0,NULL,0}
	};

	while( (ch=getopt_long(argc,argv,"i:p:h",opt,NULL))!=-1 )
	{
		switch(ch)
		{
			case 'i':
				server_ip=optarg;
				break;
			case 'p':
				server_port=atoi(optarg);
				break;
			case 'h':
				print_help(argv[0]);
				return 0;
		}
	}
	if( !server_ip )
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
	printf("create socket[%d] successfully!\n",sockfd);

	memset(&servaddr,0,sizeof(servaddr));
	servaddr.sin_family=AF_INET;
	servaddr.sin_port=htons(server_port);
	inet_aton(server_ip,&servaddr.sin_addr);

	while(1)
	{
		if( (connect(sockfd,(struct sockaddr*)&servaddr,sizeof(servaddr)))<0 )
		{
			printf("connect to server failure:%s\n",strerror(errno));
			return 0;
		}
		bytes=packtlv_data(buf,sizeof(buf),DATA1);





		if( (write(sockfd,buf,sizeof(buf)))<0 )
		{
			printf("write data to server failure:%s\n",strerror(errno));
			return 0;
		}
		printf("write successfully!\n");
	}



	return 0;
}
