/*******************************************************************
dns proxy use multithread ,write by king(king@txsms.com)
*******************************************************************/
#include <stdio.h>
#include <stdlib.h>
#ifndef _WIN32
#include <pthread.h>
#include <pwd.h>
#include <sys/types.h>
#include <signal.h>
#endif
#include "mysocket.h"
#include "KConfig.h"
#include "forwin32.h"
typedef struct 
{
	char *buf;
	int len;
	mysocket *server;
} DNSPARAM;
char host[20];
char bindaddr[20];
int maxthread=0;
int total_thread=0;
FUNC_TYPE FUNC_CALL  dns_proxy(void *param);
int get_param(int argc,char **argv,const char *param,char *value,int len);
pthread_mutex_t max_lock;
#define INCTHREAD		{pthread_mutex_lock(&max_lock);total_thread++;pthread_mutex_unlock(&max_lock);}
#define DECTHREAD		{pthread_mutex_lock(&max_lock);total_thread--;pthread_mutex_unlock(&max_lock);}
int get_dns_server_from_local(char *m_dns)
{
#ifndef _WIN32
	KConfig m_config;
	m_config.open("/etc/resolv.conf");
	const char *value=m_config.GetValue("nameserver");
	if(value==NULL)
		strcpy(m_dns,"211.141.90.68");
	else
		strncpy(m_dns,value,17);
#else
	printf("use default dns server:211.141.90.68\n");
	strcpy(m_dns,"211.141.90.68");
#endif
	return 1;
}
void set_user(const char *user)
{
#if	!defined(_WIN32)
int		rc;
struct passwd	*pwd = NULL;
         
    if ( (pwd = getpwnam(user)) != 0 ) {
		rc = setgid(pwd->pw_gid);
		if ( rc == -1 )
			printf("set_user(): Can't setgid(): %m\n");

	#if	defined(LINUX)
		/* due to linuxthreads design you can not call setuid even
		   when you call setuid before any thread creation
		 */
		rc = seteuid(pwd->pw_uid);
	#else
		rc = setuid(pwd->pw_uid);
	#endif
		if ( rc == -1 )
			printf("set_user(): Can't setuid(): %m\n");
    } else
		printf("set_user(): Can't getpwnam() `%s'.\n", user);
#endif	/* !_WIN32 */
}
void init_daemon()
{
#ifndef _WIN32
	int i,max_fd;
	//printf("umask=%d",umask(0));
	umask(0);
	if(fork()!=0)
		exit(0);
	if(setsid()<0)
		exit(1);
	if(fork()!=0)
		exit(1);
	//chdir("/");
	umask(0);
//	setpgrp();
	max_fd=sysconf(_SC_OPEN_MAX);
	for(i=0;i<max_fd;i++)
		close(i);
	open("/dev/null",O_RDWR);
	dup(1);dup(2);
#endif
}
int main(int argc,char **argv)
{

	char max_t[20];
	char buf[4068];
	char user[64];
	int len;

	mysocket server(UDP);
	mysocket client(UDP);
	DNSPARAM *param=NULL;
	pthread_t id;
	printf("Author:king\r\nUsage:%s -h host -b bindaddr -m max -u run_user -q\n",argv[0]);
#ifndef _WIN32
	pthread_attr_t attr;

	if(getuid()!=0){
		printf("you must be root user\n");
		exit(0);
	}
#endif
	init_socket();
	if(get_param(argc,argv,"-q",host,20)){
		sprintf(buf,"killall %s",argv[0]);
	//	system(buf);
		printf("use \"killall %s\" command  to kill the %s\n",argv[0],argv[0]);
		exit(0);
	}
	if(get_param(argc,argv,"-h",host,20)<=0)
		if( get_dns_server_from_local(host)<=0){
			printf("your dns server is not found in you /etc/resolv.conf file\n");
			exit(1);
	}
	get_param(argc,argv,"-b",bindaddr,20);
	get_param(argc,argv,"-m",max_t,20);
	if(get_param(argc,argv,"-u",user,64)<=0)
		strcpy(user,"nobody");
	if(get_param(argc,argv,"-d",max_t,20)<=0)
		init_daemon();
#ifndef _WIN32
	signal(SIGPIPE,SIG_IGN);
	pthread_attr_init(&attr);
	pthread_attr_setdetachstate(&attr,PTHREAD_CREATE_DETACHED);//设置线程为分离
#endif
	maxthread=atoi(max_t);
	if(maxthread==0)
		maxthread=10;
	pthread_mutex_init(&max_lock,NULL);
	if(server.open(53)<=0){
		printf("Unable to open 53 port,you must be root user or another program have open 53 port\n");
		exit(1);
	}
	printf("%s start success.\n",argv[0]);

	set_user(user);
	
	for(;;){
		len=server.recv(buf,4068);
		if(len<=0){
		//	printf("recv error\n");
			break;
		}
		if(total_thread>maxthread){
		//	printf("max user is now\n");
			continue;
		}
/*		client.connect(host,53);
		client.send(buf,len);
		len=client.recv(buf,1024);
		server.send(buf,len);
		continue;
		printf("len=%d\n",len);
*/
		param=new DNSPARAM;
		param->buf=new char[len+1];
		memcpy(param->buf,buf,len);
		param->len=len;
		param->server=server.clone();
		param->server->set_protocol(UDP);
	
		pthread_create(&id,&attr,dns_proxy,(void *)param);
	}
	clean_socket();
}
int get_param(int argc,char **argv,const char *param,char *value,int len)
{
	int i;
	for(i=0;i<argc;i++)
		if(strcmp(argv[i],param)==0){
			if(value==NULL)
				return 1;
			if(i<argc-1){
				if(strlen(argv[i+1])<255)
					strncpy(value,argv[i+1],len);
				else
					value[0]=0;
				return 1;
			}
			value[0]=0;
			return 1;
		}
	return 0;
}
FUNC_TYPE FUNC_CALL  dns_proxy(void *param)
{
	//mysocket *server=m_server->server;

	char buf[4068];
	int len;
	DNSPARAM * p=(DNSPARAM *)param;
	mysocket *server=p->server;
	mysocket client(UDP);
	INCTHREAD;
	client.connect(host,53);
	client.set_time(5);
	server->set_time(5);
//	int len=server->recv(buf,4068);
/*	if(len<=0)
		goto clean;
*/	if(client.send(p->buf,p->len)<=0)
		goto clean;
	if((len=client.recv(buf,4068))<=0)
		goto clean;
	server->send(buf,len);
	//buf[len]=0;
	//printf("len=%d\n",len);
clean:
	p->server->new_socket=-1;//don't close it
	delete p->server;
	delete[] p->buf;
	delete p;
	DECTHREAD;
	return NULL;
		
}

