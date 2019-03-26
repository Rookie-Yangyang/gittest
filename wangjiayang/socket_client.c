#include <stdio.h>
#include <string.h>



void print_usage(char *progname)
{
	printf("%s usage: \n",progname);
	printf("-p(--prot):sepcify server listen prot.\n");
	printf("-h(--help):print this help information.\n");
	return 0;
}
int main(int argc,char **agrv)
{
	int ch;
	struct option opts[]={
		{"prot",required_argument,NULL,'p'},
		{"help",no_argument,NULL,'h'},
		{NULL,0,NULL,0}
	};
	while( (ch=getopt_long(argc,argv,"p:h",opts,NULL))!=-1 )
	{
		switch(ch)
		{
		case 'p':
			prot=atoi(optarg);
			break;
		case 'h':
			print_usage(argv[0]);
			break;
		}
	}
	if(!prot)
	{
		print_usage(argv[0]);
		return 0;
	}
