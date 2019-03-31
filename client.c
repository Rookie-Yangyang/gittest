#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <time.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <dirent.h>
#include <stdlib.h>
#include <getopt.h>



int g_stop=0;
void sig_handler(int signum)
{
	if( SIGUSR1==signum )
	{
		g_stop=-1;
	}
}
/*参数解析*/
void print_help(char *progname)
{
	printf("%s usage: \n", progname);
	printf("-i(--ipaddr): sepcify server IP.\n");
	printf("-p(--port): sepcify server port.\n");
	printf("-h(--Help): print this help information.\n");
	return ;
}
/*获取温度函数*/
int get_temperature(float *temp)
{
	char w1_path[50] = "/sys/bus/w1/devices/";
	char chip[20];
	char buf[128];
	DIR *dirp;
	struct dirent *direntp;
	int fd =-1;
	char *ptr;
	float value;
	int found = 0;

	if( !temp )
	{
		return -1;
	}
	if((dirp = opendir(w1_path)) == NULL)
	{
		printf("opendir error: %s\n", strerror(errno));
		return -2;
	}
	while((direntp = readdir(dirp)) != NULL)
	{
		if(strstr(direntp->d_name,"28-"))
		{
			strcpy(chip,direntp->d_name);
			found = 1;
		}
	}
	closedir(dirp);
	if( !found )
	{
		printf("Can not find ds18b20 in %s\n", w1_path);
		return -3;
	}
	strncat(w1_path, chip, sizeof(w1_path));
	strncat(w1_path, "/w1_slave", sizeof(w1_path));
	if( (fd=open(w1_path, O_RDONLY)) < 0 )
	{
		printf("open %s error: %s\n", w1_path, strerror(errno));
		return -4;
	}

	if(read(fd, buf, sizeof(buf)) < 0)
	{
		printf("read %s error: %s\n", w1_path, strerror(errno));
		return -5;
	}

	ptr = strstr(buf, "t=")+2;
	if( !ptr )
	{
		printf("ERROR: Can not get temperature\n");
		return -6;
	}
	*temp = atof(ptr)/1000;

	return 0;
}

int main(int argc,char **argv)
{
	int                           sockfd=-1;
	int                           rv = -1;
	int                           ch;
	float                         temp;
	char                          *SERVER_IP;
	int                           SERVER_PORT=0;
	char                          buf[1024];
	char                          name[20]="RPIwangjiayang";
	struct sockaddr_in            servaddr;
	time_t                        timep;
	struct                        tm* p;

	struct option opts[] = {
		{"ipadrr", required_argument,NULL,'i'},
		{"port", required_argument, NULL, 'p'},
		{"help", no_argument, NULL, 'h'},
		{NULL, 0, NULL, 0}
	};

	while( (ch=getopt_long(argc, argv, "i:p:h", opts, NULL)) != -1 )
	{
		switch(ch)
		{
			case 'i':
				SERVER_IP=optarg;
				break;
			case 'p':
				SERVER_PORT=atoi(optarg);
				break;
			case 'h':
				print_help(argv[0]);
				return 0;
		}
	}

	printf("[%s:%d]\n",SERVER_IP,SERVER_PORT);
	signal(SIGUSR1,sig_handler);

	while( !g_stop )
	{
		/*获取温度*/
		if( (get_temperature(&temp)) < 0 )
		{
			printf("ERROR: get temprature failure\n");
			return 1;
		}
		printf("get temperature: %f ℃\n", temp);

		/*获取时间*/
		time(&timep);
		p=localtime(&timep);
		snprintf(buf,sizeof(buf),"%s/%d-%d-%d %d:%d:%d/%f",name,(1900 + p->tm_year),(1 + p->tm_mon),p->tm_mday,
				(p->tm_hour+12),p->tm_min,p->tm_sec,temp);
		printf("%s\n",buf);
		
		/*socket初始化*/
		if( (sockfd=socket(AF_INET,SOCK_STREAM,0))<0 )
		{
			printf("create socket failure:%s\n",strerror(errno));
			goto cleanup;
		}
		printf("create socket[%d] successfully!\n",sockfd);

		memset(&servaddr,0,sizeof(servaddr));
		servaddr.sin_family = AF_INET;
		servaddr.sin_port = htons(SERVER_PORT);
		inet_aton( SERVER_IP, &servaddr.sin_addr );

		if( connect(sockfd, (struct sockaddr *)&servaddr, sizeof(servaddr))<0 )
		{
			printf("connect to server [%s:%d] failure: %s\n", SERVER_IP, SERVER_PORT, strerror(errno));
			goto cleanup;
		}

		if( write(sockfd, buf, strlen(buf)) < 0 )
		{
			printf("Write data to server [%s:%d] failure: %s\n", SERVER_IP, SERVER_PORT, strerror(errno));
			goto cleanup;
		}
cleanup:
		close(sockfd);
		sleep(30);

		/*
		   memset(buf, 0, sizeof(buf));
		   rv = read(sockfd, buf, sizeof(buf));
		   if(rv < 0)
		   {
		   printf("Read data from server failure: %s\n", strerror(errno));
		   goto cleanup;
		   }
		   else if( 0 == rv )
		   {
		   printf("Client connect to server get disconnected\n");
		   goto cleanup;
		   }
		   printf("Read %d bytes data from server: '%s'\n", rv, buf);
		 */



	}
}
