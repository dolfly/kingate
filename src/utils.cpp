/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/
#define CHECK_FIRST
#include "kingate.h"
#undef LINUX
#include <stdio.h>
#ifdef _WIN32
#include <afxmt.h>
#else
#define _USE_BSD
#include <pthread.h>
#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>
#include <sys/resource.h>
#include <sys/wait.h>
#include <syslog.h>
#endif
#include "mysocket.h"
#include "utils.h"
#include "log.h"
#include "do_config.h"
#include "allow_connect.h"
#include "ftp.h"
#include "forwin32.h"
#include "oops.h"
#include "http.h"
#include "server.h"
#include "KThreadPool.h"
//#include "KManage.h"
#include "other.h"
#include "KUser.h"
#include "malloc_debug.h"
using namespace std;
filter_time_map_t filter_time_list;
SERVICE *service_head=NULL;
KThreadPool m_thread;
fd_set readfds;	
pthread_t second_tpid;
int maxfd=0;
//各函数用途看utils.h文件
int total_thread=0;//总子线程数
pthread_mutex_t	max_lock, name2ip_lock;
KMutex filter_time_lock;
void PRINT(const char *str,int size)
{
	int i;
	unsigned const char *msg=(unsigned const char *)str;
	if(size==0)
		size=strlen(str);
	printf("************* begin length=%d ********\n",size);
	for(i=0;i<size;i++)
		printf("0x%02x,",msg[i]);
	printf("\n");
	printf("************** end *******************\n");
}

void close_child(int pid)
{
	#ifndef _WIN32

	int child_pid;
	int exit_code=0;
	while((child_pid=wait3((int *)&exit_code,WNOHANG,NULL))>0){
	//	printf("exit_code=%d\n",WEXITSTATUS(exit_code));
	//	exit_code=0;
	}

	#endif
}

int udp_encode(mysocket * tcp_sock,mysocket *udp_sock,const char *src_data,int length_src_data,socks_udp *m_socks_udp)
{
	char *dest_data;//=new char[length_src_data+10];
	unsigned long dst_ip=0;
	if(length_src_data>PACKAGE_SIZE&&length_src_data<=0)
		return 0;
	if(allow_connect(SOCKS,tcp_sock,udp_sock->get_remote_name(),udp_sock->get_remote_port(),dst_ip,false,m_socks_udp->uid)==DENY)
		return 0;
	dest_data=(char *)malloc(length_src_data+11);
	if(dest_data==NULL)
		return 0;
	memset(dest_data,0,2);
	dest_data[2]=tcp_sock->frag;dest_data[3]=1;
	convert_addr(udp_sock->get_remote_name(),dest_data+4);
	dest_data[8]=udp_sock->get_remote_port()/0x100;
	dest_data[9]=udp_sock->get_remote_port()-(src_data[8]&0xff)*0x100;
	memcpy(dest_data+10,src_data,length_src_data);
	if(m_socks_udp->client_ip==0){
		free(dest_data);
		return 0;
	}
	udp_sock->connect(m_socks_udp->client_ip,m_socks_udp->port);
//	printf("udp_encode package now send to %x:%d\n",m_socks_udp->client_ip,m_socks_udp->port);
	udp_sock->send(dest_data,length_src_data+10);
	//tcp_sock->recv_size+=(length_src_data+10)
	free(dest_data);
	return length_src_data+10;
}
int udp_unencode(mysocket *tcp_sock,mysocket * udp_sock,const char *src_data,int length_src_data,socks_udp *m_socks_udp)
{
	char host[128];
	int port;
	int port_offset=8;
	size_t host_len=0;
	int dest_length=0;
//	unsigned d_time;
	unsigned long dst_ip;
	memset(host,0,sizeof(host));
	if((length_src_data<11)&&(length_src_data>PACKAGE_SIZE))
		return 0;
	udp_sock->frag=src_data[2];
	if(src_data[3]==1){
		snprintf(host,16,"%d.%d.%d.%d",0xff&src_data[4],0xff&src_data[5],0xff&src_data[6],0xff&src_data[7]);
	}else{
		host_len=MIN(src_data[4],sizeof(host));
		if(host_len<=0)
			return 0;
		memcpy(host,src_data+5,host_len);
		host[host_len]=0;
		port_offset=5+host_len;
	}
//	printf("srcuid=%d,dstuid=%d.\n",tcp_sock->uid,udp_sock->uid);

	port=(0xff&src_data[port_offset])*0x100+(0xff&src_data[port_offset+1]);
	if(allow_connect(SOCKS,tcp_sock,host,port,dst_ip,false,m_socks_udp->uid)==DENY)
		return 1;
	//d_time=udp_sock->set_time(conf.time_out[SOCKS]);
	if(m_socks_udp->client_ip==0){//it is first data;
		m_socks_udp->client_ip=udp_sock->get_remote_addr();
		m_socks_udp->port=udp_sock->get_remote_port();
	}
	if(udp_sock->connect(host,port)<=0)
		return 0;
//	printf("udp_unencoder now host=%s,port=%d.\n",host,port);
//	udp_sock->set_time(d_time);
	dest_length=length_src_data-port_offset-2;
	if(dest_length<=0)
		return 0;
//	memcpy(dest_data,src_data+port_offset+2,dest_length);
	udp_sock->send(src_data+port_offset+2,dest_length);
	return length_src_data-port_offset-2;
	
}
void  convert_addr(const char *char_addr,char *int_addr)
{
	char tmp[4];
	char *ep;
	char k;
	int i,p=-1,j=0,size=strlen(char_addr);
	for(i=0;i<size;i++){
		if((char_addr[i]=='.')||(i==size-1)){  
			if(i==size-1)
				i++;
			memcpy(tmp,char_addr+p+1,i-p-1);
			tmp[i-p-1]='\0';
			k=strtol(tmp,&ep,10);
			int_addr[j]=k;

			j++;
			p=i;
		}
	}
}
void time_out(int sig)
{
	exit(8);
	//kill_self();
}
int str_replace(char *str,const char *str1,const char *str2,int size)
{
	int i,p;
	char *str3;
	if(size==0)
		size=strlen(str);
	int size1=strlen(str1),size2=strlen(str2);
	if((str3=strstr(str,str1))==NULL)
		return 0;
	p=str3-str;
	if(size1>size2){
		for(i=p+size2;i<size+size2;i++)
			str[i]=str[i+size1-size2];		
	}else if(size1<size2){
		for(i=size+size1;i>p+size1-1;i--)
			str[i+size2-size1]=str[i];
	}
	memcpy(str3,str2,size2);
	str[size-(size1-size2)]=0;
	return size-(size1-size2);
}

int split_host_port(char *host,char separator,size_t host_len)
{	
	int point;
	int size;
	if(host_len==0){
		size=strlen(host);
	}else{
		size=MIN(strlen(host),host_len);
	}
	for(point=size-2;point>0;point--){
		if(host[point]==separator)
			break;
	}
	if(point==0)
		return 0;
	host[point]=0;
	return atoi(host+point+1);

}
int split(const char * str,char separate,int point,char * return_str)
 {
   int start=0,k=0,i=0,len=strlen(str);
   if(point>0)
   for(start=0;start<len;start++){
     if(str[start]==separate)
       k++;
     if(k>=point){
       start++;
       break;
     }
   }
   for(i=start;i<len;i++){
     if(str[i]==separate)
       break;
     return_str[i-start]=str[i];
   }
   return_str[i-start]='\0';
	if((i-start<1)&&(len-start>1))
		return 0;
   if(i-start<1)
     return -1;
   return 1;
 }
int create_select_pipe(mysocket * server,mysocket *client,int tmo,int max_server_len,int max_client_len,bool key_checked)
{
	int maxfd=0;
	char *buf;
	int len;
	int key_state;
	assert(server && client);
	SOCKET s1=server->get_socket(),s2=client->get_socket();
	int server_recv_len=0,client_recv_len=0;
	struct timeval tm;
	fd_set readfds;
	#ifdef CONTENT_FILTER
	string title_msg;
	#endif
	memset(&tm,0,sizeof(tm));
	if(s1<=0||s2<=0)
		return 0;
	int total_len=0;
//	unsigned s_time=server->set_time();
//	unsigned c_time=client->set_time();
	int ret=0;
	int max_recv_len=0;
	buf=(char *)malloc(PACKAGE_SIZE+1);
	if(buf==NULL)
		return 0;
	maxfd=MAX(s2,s1);
//	printf("max_server_len=%d,max_client_len=%d\n",max_server_len,max_client_len);
	for(;;){
		FD_ZERO(&readfds);
		FD_SET(s1,&readfds);
		FD_SET(s2,&readfds);
		tm.tv_sec=tmo;
		ret=select(maxfd+1,&readfds,NULL,NULL,(tmo==0)?NULL:&tm);
		if(ret<=0)
			break;
		max_recv_len=PACKAGE_SIZE;
		if(FD_ISSET(s1,&readfds)){
			if(max_server_len>=0){
				if(server_recv_len>=max_server_len){
					ret=-2;
					break;
				}
				max_recv_len=MIN(max_server_len-server_recv_len,max_recv_len);
			
			}
			if((len=server->recv2(buf,max_recv_len,0))<=0){
				ret=s1;
				break;
			}
	//		buf[len]=0;
	//		printf("***************leave read:\n%s\n",buf);
			server_recv_len+=len;
			/*
			if(conf.limit_speed>0 && server_recv_len>=conf.min_limit_speed_size){//limit speed
				my_msleep(len*1000/conf.limit_speed);
			}*/
			if(client->send(buf,len)<=0){
				ret=s2;
				break;
			}
		}else if(FD_ISSET(s2,&readfds)){
			if(max_client_len>=0){
				if(client_recv_len>=max_client_len){
					ret=-2;
					break;
				}
				max_recv_len=MIN(max_client_len-client_recv_len,max_recv_len);
			}
			if((len=client->recv2(buf,max_recv_len,0))<=0){
				ret=s2;
				break;
			}
	//		buf[len]=0;
	//		printf("***************************************** recv other msg from client:\n%s",buf);
			#ifdef CONTENT_FILTER
			if(!key_checked){
				key_state=check_filter_key(buf,len);
				if(key_state>=0){
					key_checked=true;
					server->get_title_msg(title_msg);
					klog(ERR_LOG,"%s filter_key_matched[%s].\n",title_msg.c_str(),conf.keys[key_state]);
				}
			}
			#endif
			client_recv_len+=len;
			if(conf.limit_speed>0 && client_recv_len>=conf.min_limit_speed_size){//limit speed
				my_msleep(len*1000/conf.limit_speed);
			}
			if(server->send(buf,len)<=0){
				ret=s1;
				break;
			}
			
		}else{
			klog(ERR_LOG,"bug!! in %s:%d,errno=%d.\n",__FILE__,__LINE__,errno);
			break;
		}
	}
//	server->set_time(s_time);
//	client->set_time(c_time);
	free(buf);
	return ret;	
}

void create_select_pipe(SERVER * m_server)
{

	int total_fd;
	char *str=(char *)malloc(PACKAGE_SIZE+1);//,msg[PACKAGE_SIZE+33];
	assert(m_server);
	assert(m_server->server);
	assert(m_server->client);
	mysocket *server=m_server->server,*client=m_server->client;
//	struct timeval * m_timeout=NULL;
	int length=0;
	int i;
	SOCKET s1=server->get_socket(),s2=client->get_socket(),ret,ftp_data_cmd=0,model=m_server->model;
	struct timeval t_timeout;
	fd_set readfds;
	total_fd=MAX(s1,s2)+1;
	memset(&t_timeout,0,sizeof(t_timeout));
	int total_length=0;
	unsigned s_time=server->get_time();
	if(str==NULL)
		return;
	if(s1<=0||s2<=0)
		goto clean;		
	for(;;){
/********************** select的准备工作 ********************************/
		FD_ZERO(&readfds);
		FD_SET(s1,&readfds);
		FD_SET(s2,&readfds);
		
		t_timeout.tv_sec=s_time;
/******************* 看server或client是否有数据读 *********************/
		if((ret=select(total_fd,&readfds,NULL,NULL,(t_timeout.tv_sec==0?NULL:&t_timeout)))<=0){
			klog(RUN_LOG,"select error or time out,error=%d\n",ret);
			break;
		}
/******************** server 有数据可以读 ****************************/
		if(FD_ISSET(s1,&readfds)){
		//		printf("s:\n");
		//		PRINT(str,length);
		//		str[length]=0;
		//		printf("s:\n%s",str);
				/*
				total_length+=length;
				if(conf.limit_speed>0 && total_length>=conf.min_limit_speed_size){//limit speed
					my_msleep(length*1000/conf.limit_speed);
				}*/
				switch (model){
					case MODEL_SOCKS_UDP:
						goto clean;
					/*
					case FTP:
						for(i=0;i<PACKAGE_SIZE-1;i++){
							if(server->recv(str+i,1)<=0)
								goto clean;	
							if(str[i]=='\n')
								break;
						}
						length=i+1;
				 		if( (strncasecmp(str,"PASV",4)==0) ){
							ftp_data_cmd=1;
						}else if( (strncasecmp(str,"PORT",4)==0) ){
							length=rewrite_cmd(m_server,str,2);						
						}
						break;	
						*/
					default:
						if((length=server->recv2(str,PACKAGE_SIZE,0))<=0)
							goto clean;
						break;				
						
				}
				if(client->send(str,length)<=0)
					break;
				continue;
		}
/******************** client 有数据可以读 **********************/
		if(FD_ISSET(s2,&readfds)){
			/*
			if(model==FTP){
				for(i=0;i<PACKAGE_SIZE-1;i++){
					if(client->recv(str+i,1)<=0)
						goto clean;
					if(str[i]=='\n')
						break;
				}
				length=i+1;
			}else */
			if((length=client->recv2(str,PACKAGE_SIZE,0))<=0)
				break;
		//	str[length]=0;
		//	printf("c:\n%s",str);
		//	printf("c:\n");
		//	PRINT(str,length);
			total_length+=length;
			if(conf.limit_speed>0 && total_length>=conf.min_limit_speed_size){//limit speed
				my_msleep(length*1000/conf.limit_speed);
			}
			switch (model){
				case MODEL_SOCKS_UDP:
					if(client->get_remote_addr()==server->get_remote_addr())
					{
						if(udp_unencode(server,client,str,length,(socks_udp *)m_server->ext)<=0){
							goto clean;
						}
						server->send_size+=length;
					}else{
						if(udp_encode(server,client,str,length,(socks_udp *)m_server->ext)<=0){
							goto clean;
						}
						server->recv_size+=length;
					}
					continue;
					/*
				case FTP:
					if(ftp_data_cmd==1){
						str[length]=0;
						length=rewrite_cmd(m_server,str,ftp_data_cmd);
						ftp_data_cmd=0;
					}
				*/
			}
		if(server->send(str,length)<=0)
				break;
		
		continue;
		}
		break;
		
	}
/******************** 关闭退出 *****************************/
clean:
	free(str);
	server->close();
	client->close();
/**********************************************************/
}
void catch_sig(int sig)
{
	#ifdef DEBUG
	printf("#################################################################sig=%d\n",sig);
	signal(sig,NULL);
	#endif
}

FUNC_TYPE FUNC_CALL server_thread(void *m_mysocket)
{
	mysocket *client=NULL;
	SERVER * m_server=(SERVER *)m_mysocket;
	m_server->server=new mysocket;
	m_server->server->create(m_server->accept_fd);
	m_server->server->set_time((m_server->model>10?0:conf.time_out[m_server->model]));
	unsigned server_port=(m_server->model>10?m_server->model:conf.port[m_server->model]);
	unsigned long ip=m_server->server->get_remote_addr();
	int start_time=time(NULL);
//	m_server->client=NULL;
//	if(m_server->model!=SOCKS){
//	m_server->server->send("haha....");
//	goto cleanup;
	if((m_server->model==HTTP) || (m_server->model==MANAGE)){
		run_client(m_server);
		goto cleanup;
	}else if(m_server->model==FTP){
		run_ftp(m_server->server);
		goto cleanup;
	}
	/*else if(m_server->model==MANAGE){		
		KManage m_manage;
		m_manage.Start(m_server->server);
		goto cleanup;
	}*/else if(m_server->model>10){
		if((client=create_redirect_connect(m_server))==NULL)
			goto cleanup;
	}else{
		if((client=create_connect[m_server->model](m_server))==NULL)
			goto cleanup;
	}
	if(client->get_protocol()==UDP)
		m_server->model=MODEL_SOCKS_UDP;
	m_server->client=client;
	create_select_pipe(m_server);
cleanup:
	if(conf.log_close_msg)
		klog(RUN_LOG,"%s:%d close %d,send %db,recv %db,use %ds\n",
	m_server->server->get_remote_name(),
	m_server->server->get_remote_port(),
	server_port,
	m_server->server->recv_size,
	m_server->server->send_size,
	time(NULL)-start_time);
	if(m_server->model==MODEL_SOCKS_UDP){
		if(m_server->ext)
			delete (socks_udp *)m_server->ext;
	}
	if(client)
		delete client;
	del_filter_time(); 
	delete m_server->server;
	delete m_server;
				return
#ifndef _WIN32
			NULL

#endif

			;//	return (FUNC_TYPE)ip;
}
void listen_fd_set()
{
	maxfd=0;
	FD_ZERO(&readfds);
	SERVICE *tmp=service_head;
	while(tmp!=NULL){
		FD_SET(tmp->server.get_socket(OLD),&readfds);
		maxfd=MAX(maxfd,tmp->server.get_socket(OLD));
		tmp=tmp->next;
	}
}
void server_proxy()
{
	SERVER * m_server;
	SERVER * m2_server;
	fd_set fds;
	int ret;
	SERVICE *tmp;
//	filter_time_list=NULL;
	
	pthread_t id;
	#ifndef _WIN32
	pthread_attr_t attr;
	pthread_attr_init(&attr);
	signal(SIGPIPE,SIG_IGN);
	signal(SIGCHLD,&close_child);
//	signal(SIG	
	pthread_attr_setdetachstate(&attr,PTHREAD_CREATE_DETACHED);//设置线程为分离
	#endif
	pthread_mutex_init(&max_lock,NULL);
//	pthread_mutex_init(&filter_time_lock,NULL);
	pthread_create(&second_tpid, &attr, time_thread, NULL) ;
	listen_fd_set();
	for(;;){		
		fds=readfds;
		ret=select(maxfd+1,&fds,NULL,NULL,NULL);
		if(ret<=0){
	//		printf("select error.maybe recv a signal.\n");
			continue;//maybe recv a signal or error
		}
		tmp=service_head;
		if(tmp==NULL){
			klog(ERR_LOG,"all service have closed\n");
			break;
		}
		m_server=NULL;
		while(tmp!=NULL){
			if(FD_ISSET(tmp->server.get_socket(OLD),&fds)){
				m_server=&tmp->m_server;
				m_server->server=&tmp->server;
				break;
			}
			tmp=tmp->next;
		}
		if((tmp->m_server.accept_fd=tmp->server.accept())==INVALID_SOCKET){
			klog(ERR_LOG,"accept error,error=%d\n",errno);
			continue;
		}
		if(total_thread<0){
			klog(ERR_LOG,"bug!!! total_thread=%d,now fix it to zero\n",total_thread);
			total_thread=0;
		}
		/*
		if(total_thread>=conf.max){
			klog(ERR_LOG,"too many user,connect refuse,total_thread=%d\n",total_thread);
			tmp->server.use(OLD);
			continue;
		}
		*/
		klog(DEBUG_LOG,"client %s:%d come in\n",tmp->server.get_remote_name(),tmp->server.get_remote_port());
		m2_server=new SERVER;
		if(m2_server==NULL){
			klog(ERR_LOG,"error,cann't new memory.in %s:%d\n",__FILE__,__LINE__);
			tmp->server.use(OLD);
			continue;
		}
		memcpy(m2_server,m_server,sizeof(SERVER));//克隆m_server至m2_server
		if(m_thread.Start(m2_server)<=0){
			delete m2_server;
			tmp->server.use(OLD);
			continue;
		}
	}
//	assert(0);//never to here
//	exit(3);
}
int split_user_host(const char *str,char *orig_msg,size_t orig_msg_size,char *host,size_t host_size)//从user指令得到host信息,用于pop3代理和ftp代理。
{
	unsigned short i;
	unsigned short size=strlen(str);
	int ret=0;
	for(i=size;i>0;i--)
		if((str[i]=='#')||(str[i]=='@')){
			int orig_msg_len=MIN(i,orig_msg_size);
			if(str[i]=='@')
				ret=1;
			else
				ret=2;
			memcpy(orig_msg,str,orig_msg_len);
			orig_msg[orig_msg_len]=0;
			int host_len=MIN(size-i,host_size);
			memcpy(host,str+i+1,host_len);
			host[host_len]=0;
			return ret;
		}
	return ret;
}
int check_end(char *str,int size)//检查str里是否有\n或\r\n。
{
	int i;
//	printf("%x ",str[size]);
//	fflush(stdout);

	for(i=0;i<size;i++){
		if(str[i]=='\n'){
			str[i]='\0';
			return 1;
		}
		if(str[i]=='\r'){
			if(i==size-1)
				return 0;
			else
				if(str[i+1]=='\n'){
					str[i]='\0';
					return 1;
				}
		}
	}
	return 0;
}
int get_param(int argc,char **argv,const char *param,char *value)
{
	int i;
	for(i=0;i<argc;i++)
		if(strcmp(argv[i],param)==0){
			if(value==NULL)
				return 1;
			if(i<argc-1){
				if(strlen(argv[i+1])<255)
					strcpy(value,argv[i+1]);
				else
					value[0]=0;
				return 1;
			}
			value[0]=0;
			return 1;
		}
	return 0;
}
int get_path(char *argv0,string &path)
{
	#ifndef _WIN32
	char sep='/';
	#else
	char sep='\\';
	#endif
	int size=strlen(argv0);
	int i;
	int p=0;
	for(i=size-1;i>=0;i--){
		if(argv0[i]==sep){
			if(p==1)
				break;
			p=1;
		}
	}
	
#ifndef _WIN32
	if(argv0[0]!='/'){
		char *pwd=getenv("PWD");
		if(pwd)
			path=pwd;
		path+="/";
	}
#endif
	argv0[i+1]=0;
	path+=argv0;
	if(argv0[i]==sep)
		return 1;
	else{
#ifndef _WIN32
		return 0;
#else
		path=".\\";
		return 1;
#endif
	}
}
void add_filter_time(filter_time &m_filter_time)
{
	m_filter_time.start_time=time(NULL);
	pthread_t pid=pthread_self();
	
	filter_time_lock.Lock();
	filter_time_list[pid]=m_filter_time;
	filter_time_lock.Unlock();
	
}
int del_filter_time()
{
	int uid=0;
	unsigned src_ip;
	int recv_size;
	int send_size;
	pthread_t pid=pthread_self();
	filter_time_map_t::iterator it;
	filter_time_lock.Lock();
	it=filter_time_list.find(pid);
	if(it==filter_time_list.end())
		goto done;
	uid=(*it).second.m_state.uid;
	src_ip=(*it).second.m_state.src_ip;
	recv_size=(*it).second.server->recv_size;
	send_size=(*it).second.server->send_size;
	filter_time_list.erase(it);
	filter_time_lock.Unlock();
	#ifndef DISABLE_USER
	m_user.UpdateSendRecvSize(src_ip,recv_size,send_size,uid);
	#endif
	return uid;
done:
	filter_time_lock.Unlock();
	return uid;
	
}
