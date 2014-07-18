#ifndef UTILS_H_93427598324987234kjh234k
#define UTILS_H_93427598324987234kjh234k
#include <stdio.h>
#include <stdlib.h>
#ifndef _WIN32
#include <pthread.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <time.h>
//#include <net/if.h>
//#include <net/if_arp.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <signal.h>
#endif
#include "mysocket.h"
#include "do_config.h"
#include "kingate.h"
#include "forwin32.h"
#include "KDnsCache.h"
#pragma warning (disable : 4786)
struct _SERVER{

	SOCKET	accept_fd;
	short	model;				//����ģʽ��HTTP,FTP,...
	mysocket * server;			//�������ӵĿͻ�mysocket
	mysocket * client;			//�������ӵķ���mysocket
	volatile void * ext;		//���������ַ
};

typedef struct _SERVER SERVER;

typedef struct{
	int protocol;
	int port;
	char host[MAX_URL];
}HOST_MESSAGE;
/*
struct _HTTP_FILTER_STATE
{
	SERVER * m_server;			//������Ϣ
	char * buf;					//�յ��ͻ�http���ʵĵ�һԭʼ��Ϣ,
	int length;					//ԭʼ��Ϣ����
	short closed;				//�Ƿ�ر���socket
	short new_socketd;			//�Ƿ������socket
};
* /
typedef _HTTP_FILTER_STATE HTTP_FILTER_STATE;
*/
struct filter_time
{
	mysocket * server;
	STATE m_state;
	unsigned start_time;
//	pthread_t pid;
};
typedef std::map<pthread_t,filter_time> filter_time_map_t;
/*
struct LOCAL_IP{
  char ip[17];
  struct LOCAL_IP * next;
};
typedef struct LOCAL_IP LOCAL_IP;
*/
typedef struct __SERVICE
{
	SERVER m_server;
	mysocket server;
	struct __SERVICE *next;
} SERVICE;
struct socks_udp
{
	unsigned uid;
	unsigned client_ip;
	int port;
};
extern SERVICE *service_head;
extern filter_time_map_t filter_time_list;
extern	fd_set readfds;	
extern	int maxfd;
//extern pthread_t second_tpid;
extern KMutex filter_time_lock;//pthread_mutex_t	filter_time_lock;
//extern pthread_mutex_t	name2ip_lock;
void PRINT(const char *str,int size=0);//�����ã���ӡstr�и����ַ���16����
void close_child(int pid);//���չرյ��ӽ�����Դ
//int udp_encode(mysocket * src_sock,mysocket *dest_sock,const char *src_data,int length_src_data,char * dest_data);//socks udp ������udp���뺯��
//int udp_unencode(mysocket *src_sock,mysocket * dest_sock,const char *src_data,int length_src_data,char *dest_data);//socks udp ������udp���뺯��
void  convert_addr(const char *char_addr,char *int_addr);//���ַ�����ip��ַת���������ͣ����浽int_addr�С���socks������Ҫ�õ�
void kill_self();
void time_out(int sig);
int str_replace(char *str,const char *str1,const char *str2,int size=0);//�ַ����滻��������str�У��ѵ�һ��str1���滻��str2��
int split_host_port(char *host,char separator,size_t host_len=0);//����host��port���ɹ�����port��separator��host��port�ķָ���
int split(const char * str,char separate,int point,char * return_str);//�ָ��ַ���,
void create_select_pipe(SERVER *);
int create_select_pipe(mysocket * server,mysocket *client,int tmo=0,int max_server_len=-1,int max_client_len=-1,bool key_checked=false);
//void create_select_pipe(mysocket *,mysocket *,int);
void catch_sig(int sig);
void server_proxy();
FUNC_TYPE FUNC_CALL server_thread(void *);
//void init_utils(CONFIG * );
//void http_filter_client(HTTP_FILTER_STATE * m_http_filter_state);
int get_http_head_value(const char *,const char *,char *,int );
//LOCAL_IP * get_local_ip();
int split_user_host(const char *str,char *orig_msg,size_t orig_msg_size,char *host,size_t host_size);//��userָ��õ�host��Ϣ,����pop3�����ftp����
int check_end(char *str,int size);
int rewrite_pasv(char *str);//ftp��������д������pasv��ַ�Ͷ˿ڡ�
int get_param(int argc,char **argv,const char *param,char *value);//�õ��������
int get_path(char *argv,std::string &path);
void add_filter_time(filter_time &m_filter_time);//����filter_time,ʹ�����̼߳������ѵĹ����Ƿ���Ч.
void check_time();//���Ӻ��е��̵߳Ĺ����Ƿ���Ч.
FUNC_TYPE FUNC_CALL time_thread(void *arg);
int del_filter_time();//ɾ�����Ѽ����filter_time
int time_sync();//ʱ��ͬ��,�ó�Ҫ�������,����ALARM�ź�.ʹ�ò����ź�ʱ��պ�������.
void listen_fd_set();
int	run_client(SERVER *);
void closeAllConnection();
mysocket * create_socks5_connect(SERVER *m_server);
mysocket * create_rtsp_connect(SERVER *m_server);
mysocket * create_pop3_connect(SERVER *m_server);
mysocket * create_smtp_connect(SERVER *m_server);
mysocket * create_telnet_connect(SERVER *m_server);
mysocket * create_mms_connect(SERVER *m_server);
//mysocket * create_ftp_connect(SERVER *m_server);
static mysocket * (*create_connect[])(SERVER *)={		
			NULL,
			create_rtsp_connect,
			NULL,
		//	create_ftp_connect,
			create_pop3_connect,
			create_smtp_connect,
			create_telnet_connect,
			create_mms_connect,
			create_socks5_connect
	
};
void run_ftp(mysocket *server);
#endif
