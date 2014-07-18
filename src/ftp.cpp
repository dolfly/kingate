#include <string.h>
#include <string>
#include <vector>
#include <sstream>
#include <assert.h>
#include <string.h>
#include <map>
#include "do_config.h"
#include "ftp.h"
#include "utils.h"
//#include "allow_connect.h"
#include "log.h"
#include "forwin32.h"
//#include "KUser.h"
//#include "kmysql.h"
#include "malloc_debug.h"
#define		TYPE_I		0
#define		TYPE_A		1
#define	    THIS_IP		"*"
//#define		BASE_ROOT		"e:\\"
using namespace std;

long file_size(FILE *fp);
int url_unencode(char *url_msg,size_t url_len);
/*
char *my_strtok(char *msg,char split,char **ptrptr)
{
	char *str;
	if(msg)
		str=msg;
	else
		str=*ptrptr;
	if(str==NULL)
		return NULL;
	int len=strlen(str);
	if(len<=0){
		return NULL;
	}
	for(int i=0;i<len;i++){
		if(str[i]==split){
			str[i]=0;
			*ptrptr=str+i+1;
			return str; 
		}
	}
	*ptrptr=NULL;
	return str;

}
/*
struct user_info
{
	string user;
	string homedir;
	int uid;
	int gid;
//	KMysql sql;
	long max_size;
	long cur_size;
	int change_size;
	KMutex user_lock;
	int refs;
	bool sql_connected;
};
*/
struct ftp_session
{
//	user_info *user;
//	int type;
	mysocket *server;
	mysocket data;
	mysocket client;
	mysocket client_data;
//	string pwd;
//	int rest;
//	string rnfr;
	HOST_MESSAGE host_message;
	bool pasv_cmd;
	//	string rnft;
};
//KMutex map_ftp_session_lock;
//typedef map<string,user_info*> map_ftp_session;
//map_ftp_session users;
/*
int get_tmp(char m_char)
{
	if(m_char<='9' && m_char>='0')
		return 0x30;
	if(m_char<='f' && m_char>='a')
		return 0x57;
	if(m_char<='F' && m_char>='A')
		return 0x37;
	return 0;
	
}
int url_unencode(char *url_msg,size_t url_len)
{
	int j=0;
	int i=0;
	// printf("url=%s\n",url_msg);
	if(url_len==0)
		url_len=strlen(url_msg);
	for(;;){
		if(i>=url_len){
			url_msg[j]=0;
			return j;
		}
		if(url_msg[i]=='%' && (i<=url_len-3) && (get_tmp(url_msg[i+1])!=0) && (get_tmp(url_msg[i+2])!=0)){
			
			url_msg[j]=(url_msg[i+1]-get_tmp(url_msg[i+1]))*0x10+url_msg[i+2]-get_tmp(url_msg[i+2]);
			i+=3;
		}else{
			if(url_msg[i]=='+')
				url_msg[j]=' ';
			else
				url_msg[j]=url_msg[i];
			i++;
		}
		j++;
	}
}
*/
bool enter_port_cmd(ftp_session *m_sess,const char *cmd)
{
	char str[100];
	char tmp[21];
	//	char tmp_port[20];
	int port,port_low,port_high;
	//	strcpy(str,"227 Entering Passive Mode (");
	memset(tmp,0,sizeof(tmp));
	strncpy(tmp,((strcmp(THIS_IP,"*")==0)?m_sess->client.get_self_name():THIS_IP),sizeof(tmp)-1);
	//	while(str_replace(tmp,".",",")>0);
	int tmp_len=strlen(tmp);
	for(int i=0;i<tmp_len;i++){
		if(tmp[i]=='.'){
			tmp[i]=',';
		}
	}
	m_sess->pasv_cmd=false;
	m_sess->data.close();
	m_sess->data.open(0);
	port=m_sess->data.get_listen_port();
	port_high=port/0x100;
	port_low=port-port_high*0x100;
	m_sess->server->send("200 PORT command successful\r\n");
	snprintf(str,sizeof(str)-1,"PORT %s,%d,%d\r\n",tmp,port_high,port_low);
	printf("client send str=%s",str);
	if(m_sess->client.send(str)<=0)
		return false;
/*	m_sess->client.send("PASV\r\n");
	memset(str,0,sizeof(str));
	printf("client send PASV\n");
	*/
	if(m_sess->client.recv(str,sizeof(str)-2,"\r\n")<=0){
		return false;
	}
	printf("client recv msg %s\n",str);
	printf("cmd=%s\n",cmd);
	get_host_message_from_ftp_str(cmd,&m_sess->host_message,PORT_CMD);
	return true;


/*
	int tmp[6];
	memset(tmp,0,sizeof(tmp));
	sscanf(cmd,"%d,%d,%d,%d,%d,%d",&tmp[0],&tmp[1],&tmp[2],&tmp[3],&tmp[4],&tmp[5]);
	char host[21];
	int port;
	snprintf(m_sess->host_message.host,sizeof(host)-1,"%d.%d.%d.%d\0",tmp[0],tmp[1],tmp[2],tmp[3]);
	m_sess->host_message.port=tmp[4]*0x100+tmp[5];
	memset(tmp,0,sizeof(tmp));
	strncpy(tmp,((strcmp(THIS_IP,"*")==0)?m_sess->server->get_self_name():THIS_IP),sizeof(tmp)-1);
	//	while(str_replace(tmp,".",",")>0);
	* /
	char tmp[21];
	int tmp_len=strlen(tmp);
	for(int i=0;i<tmp_len;i++){
		if(tmp[i]=='.'){
			tmp[i]=',';
		}
	}
	//	m_sess->pasv_cmd=true;
	m_sess->data.close();
	m_sess->data.open(0);
	port=m_sess->data.get_listen_port();
	port_high=port/0x100;
	port_low=port-port_high*0x100;
	char str[100];
	memset(str,0,sizeof(str));
	snprintf(str,sizeof(str)-1,"\r\n\0",tmp,port_high,port_low);
	if(m_sess->server->send(str)<=0)
		return false;
/*	m_sess->data.close();
//	m_sess->pasv=false;
//	m_sess->data.set_time(20);
	m_sess->pasv_cmd=false;
	if(m_sess->data.connect(host,port)<=0){
		printf("connect error.\r\n");
		m_sess->data.close();
		return false;
	}
//	m_sess->data.set_time(20);
	if(m_sess->server->send("200 Port command successful\r\n")<=0)
		return false;
	return true;
	*/
}
bool accept_data_connect(mysocket *server)
{
	struct timeval tm;
	int ret;
    memset(&tm,0,sizeof(tm));
    fd_set fds;
    FD_ZERO(&fds);
    if(server->old_socket<=0)
            goto clean;
    FD_SET(server->old_socket,&fds);
    tm.tv_sec=30;
    ret=select(server->old_socket+1,&fds,NULL,NULL,&tm);
    if(ret<=0)
            goto clean;
    if(FD_ISSET(server->old_socket,&fds)){
		if(server->accept()<=0){
			printf("accept error\n");
			goto clean;
		}else{
		//	m_sess->data.set_time(20);
	//		m_sess->pasv_cmd=false;
	
			printf("success\n");
			return true;
		}
	}
clean:
//	m_sess->pasv_cmd=false;
//	m_sess->data.close();
	printf("accept error\n");
	server->close();
	return false;
}
bool enter_pasv_cmd(ftp_session *m_sess)
{
	char str[100];
	char tmp[21];
	//	char tmp_port[20];
	int port,port_low,port_high;
	//	strcpy(str,"227 Entering Passive Mode (");
	memset(tmp,0,sizeof(tmp));
	strncpy(tmp,((strcmp(THIS_IP,"*")==0)?m_sess->server->get_self_name():THIS_IP),sizeof(tmp)-1);
	//	while(str_replace(tmp,".",",")>0);
	int tmp_len=strlen(tmp);
	for(int i=0;i<tmp_len;i++){
		if(tmp[i]=='.'){
			tmp[i]=',';
		}
	}
	m_sess->pasv_cmd=true;
	m_sess->data.close();
	m_sess->data.open(0);
	port=m_sess->data.get_listen_port();
	port_high=port/0x100;
	port_low=port-port_high*0x100;
	snprintf(str,sizeof(str)-1,"227 Entering Passive Mode (%s,%d,%d)\r\n\0",tmp,port_high,port_low);
	if(m_sess->server->send(str)<=0)
		return false;
	m_sess->client.send("PASV\r\n");
	memset(str,0,sizeof(str));
	printf("client send PASV\n");
	if(m_sess->client.recv(str,sizeof(str)-2,"\r\n")<=0){
		return false;
	}
	printf("client recv msg %s\n",str);
	get_host_message_from_ftp_str(str,&m_sess->host_message,PASV_CMD);
	return true;
	
}
void init_ftp_session(ftp_session *m_sess)
{
//	m_sess->rest=0;
//	m_sess->user=NULL;
	m_sess->server=NULL;
	m_sess->data.set_time(conf.time_out[FTP]);
	m_sess->client.set_time(conf.time_out[FTP]);
	m_sess->pasv_cmd=false;
//	m_sess->type=TYPE_I;
}
void clean_ftp_session(ftp_session *m_sess)
{
//	user_info *m_user=NULL;
	/*
	if(m_sess->user){
		sync_cur_size(m_sess->user);
		map_ftp_session_lock.Lock();
		m_sess->user->refs--;
		if(m_sess->user->refs<=0){
			m_user=m_sess->user;
			map_ftp_session::iterator it;
			it=users.find(m_user->user);			
			assert((it!=users.end()));
			users.erase(it);
		}
		map_ftp_session_lock.Unlock();
	}
	if(m_user){
		delete m_user;
	}
	*/
}

bool enter_data_cmd(ftp_session *m_sess,char *msg)
{

//	stringstream s;
//	s << msg << "\r\n";
	if(!m_sess->pasv_cmd){
		if(m_sess->client.send(msg)<=0)
			return false;
	}
	if(!accept_data_connect(&m_sess->data)){
		return false;
	}
	printf("connect to %s:%d\n",m_sess->host_message.host,m_sess->host_message.port);
	if(m_sess->client_data.connect(m_sess->host_message.host,m_sess->host_message.port)<=0){
		printf("failed\n");
		return false;
	//	goto clean;
	}
	printf("success\n");
	printf("client send %s\n",msg);
	if(m_sess->pasv_cmd){
	if(m_sess->client.send(msg)<=0)
		return false;
	}
	printf("client send %s...",msg);
	char str[512];
	memset(str,0,sizeof(str));
	if(m_sess->client.recv(str,sizeof(str)-3,"\r\n")<=0){
		return false;
	}

//	s.str("");
//	s << str
	printf("str=%s",str);
	if(m_sess->server->send(str)<=0){
		return false;
	}
	create_select_pipe(&m_sess->data,&m_sess->client_data,conf.time_out[FTP]);
	m_sess->data.close();
	m_sess->client_data.close();
	printf("create select pipe finished.\n");
	return true;
}
void run_ftp(mysocket *server)
{
	int length,port,len;
	ftp_session m_sess;//=NULL;
	char cmd[10];	
	string user;
	stringstream s;
	char *tmp;
	char *pass;
	char msg[512];
	char p[512];
	char *client_msg;
//	int port;
	bool logined=false;
	char *host;
	init_ftp_session(&m_sess);
	m_sess.server=server;	
	int err_login=0;
	s << "220 " << PROGRAM_NAME << "(" << VER_ID << ") ftp server ready.\r\n";
	if(m_sess.server->send(s.str().c_str())<=0){
		printf("send msg %s failed\n",msg);
		goto clean;
	}
	for(;;){	
		if(err_login>5){
			goto clean;
		}
	//	memset(msg,0,sizeof(msg));
		length=m_sess.server->recv(msg,sizeof(msg)-1,"\r\n");
		if(length<=2)
			goto clean;
		msg[length]=0;
	//	cmd[0]=0;p[0]=0;
		memset(cmd,0,sizeof(cmd));
		memset(p,0,sizeof(p));
		int ret=sscanf(msg,"%9[^ ]%*[ ]%510[^\r]",cmd,p);
		if(ret==1){
			int cmd_len=strlen(cmd);
			if(cmd_len<=2)
				goto clean;
			cmd[cmd_len-2]=0;
		}
		printf("cmd=%s.p=%s.\r\n",cmd,p);
		if(strcasecmp(cmd,"user")==0){
			if(logined){
				s << "503 You are already logged in!\r\n";
				if(m_sess.server->send(s.str().c_str())<=0)
					goto clean;
				continue;
			}
			logined=true;
			if(conf.ftp_redirect==NULL){
				tmp=(char *)memchr((void *)p,'@',strlen(p));
				if(tmp==NULL){
					tmp=(char *)memchr((void *)p,'#',strlen(p));
				}
				if(tmp==NULL){
					goto clean;
				}
				int user_len=tmp-p;
				p[user_len]=0;
				user=p;
				tmp++;
				port=split_host_port(tmp,':',strlen(tmp));
				host=tmp;
			}else{
				user=p;
				host=conf.ftp_redirect;
				port=conf.ftp_redirect_port;
			}
			if(port==0)
				port=21;
			if(m_sess.client.connect(host,port)<=0){
				goto clean;
			}
			for(;;){
				if((length=m_sess.client.recv(msg,sizeof(msg)-1,"\r\n"))<=0){
					klog(ERR_LOG,"It is error while recv first message from remote host\n");
					goto clean;
				}
			//	msg[length]=0;
				printf("%s...",msg);
			//	break;
				if(strncmp(msg,"220 ",4)==0){
					printf("yeah........\n");
					break;
				}
			}
			s.str("");
			s << "USER " << p << "\r\n";
			printf("send msg:%s",s.str().c_str());
			if(m_sess.client.send(s.str().c_str())<=0){
				goto clean;
			}
			goto skip_client_send;
		//	client_msg=(char *)s.str().c_str();
		//	printf("client_msg=%s,s.str().c_str()=%s\n",client_msg,s.str().c_str());
		}else if(strcasecmp(cmd,"pasv")==0){
			if(!enter_pasv_cmd(&m_sess))
				goto clean;
			continue;
		//	goto skip_client_send;
		}else if(strcasecmp(cmd,"port")==0){
			if(!enter_port_cmd(&m_sess,p))
				goto clean;
			continue;
		}else if(
			(strcasecmp(cmd,"list")==0)
			|| (strcasecmp(cmd,"NLST")==0)
			|| (strcasecmp(cmd,"retr")==0)
			|| (strcasecmp(cmd,"stor")==0)
			|| (strcasecmp(cmd,"appe")==0)
			){
			if(!enter_data_cmd(&m_sess,msg)){
				goto clean;
			}
			goto skip_client_send;
		//	continue;
		}
			//client_msg=msg;
		if(m_sess.client.send(msg)<=0){
				goto clean;
		}
skip_client_send:
		for(;;){
			memset(msg,0,sizeof(msg));
			if(m_sess.client.recv(msg,sizeof(msg)-1,"\r\n")<=0){
				goto clean;
			}
		//	s.str("");
		//	s << msg << "\r\n";
			printf("client recv msg %s...\n",msg);
			if(m_sess.server->send(msg)<=0)
				goto clean;
			if(msg[3]==' '){
				printf("yeah........\n");
				break;
			}
		}
		if(!logined){
			err_login++;
			if(m_sess.server->send("530 Please login with USER and PASS\r\n")<=0)
				goto clean;
			continue;
		}
	}
clean:
	if(logined){
		klog(RUN_LOG,"user %s closed connection from %s:%d.\n",user.c_str(),m_sess.server->get_remote_name(),m_sess.server->get_remote_port());
	}
	clean_ftp_session(&m_sess);
	return ;
}

int get_host_message_from_ftp_str(const char * str,HOST_MESSAGE * m_host,int ftp_cmd)
{
	int tmp[6];
	switch(ftp_cmd){
		case PASV_CMD:
			sscanf(str,"%*[^(](%d,%d,%d,%d,%d,%d)",&tmp[0],&tmp[1],&tmp[2],&tmp[3],&tmp[4],&tmp[5]);
			break;
		case PORT_CMD:
			sscanf(str,"%d,%d,%d,%d,%d,%d",&tmp[0],&tmp[1],&tmp[2],&tmp[3],&tmp[4],&tmp[5]);
			break;
		default:
			klog(ERR_LOG,"ftp cmd is error\n");
			return -1;//ftp cmd error
	}
	memset(m_host->host,0,sizeof(m_host->host));
	snprintf(m_host->host,sizeof(m_host->host)-1,"%d.%d.%d.%d",tmp[0],tmp[1],tmp[2],tmp[3]);
	m_host->port=tmp[4]*0x100+tmp[5];
	return 1;
}
