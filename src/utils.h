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
	short	model;				//工作模式。HTTP,FTP,...
	mysocket * server;			//正在连接的客户mysocket
	mysocket * client;			//正在连接的服务mysocket
	volatile void * ext;		//额外参数地址
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
	SERVER * m_server;			//连接信息
	char * buf;					//收到客户http访问的第一原始信息,
	int length;					//原始信息长度
	short closed;				//是否关闭了socket
	short new_socketd;			//是否更新了socket
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
void PRINT(const char *str,int size=0);//调试用，打印str中各个字符的16进制
void close_child(int pid);//回收关闭的子进程资源
//int udp_encode(mysocket * src_sock,mysocket *dest_sock,const char *src_data,int length_src_data,char * dest_data);//socks udp 代理中udp编码函数
//int udp_unencode(mysocket *src_sock,mysocket * dest_sock,const char *src_data,int length_src_data,char *dest_data);//socks udp 代理中udp解码函数
void  convert_addr(const char *char_addr,char *int_addr);//把字符串的ip地址转换成数字型，并存到int_addr中。在socks代理中要用到
void kill_self();
void time_out(int sig);
int str_replace(char *str,const char *str1,const char *str2,int size=0);//字符串替换函数，在str中，把第一个str1，替换成str2。
int split_host_port(char *host,char separator,size_t host_len=0);//分离host和port，成功返回port。separator是host和port的分隔符
int split(const char * str,char separate,int point,char * return_str);//分割字符串,
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
int split_user_host(const char *str,char *orig_msg,size_t orig_msg_size,char *host,size_t host_size);//从user指令得到host信息,用于pop3代理和ftp代理。
int check_end(char *str,int size);
int rewrite_pasv(char *str);//ftp代理中重写服务器pasv地址和端口。
int get_param(int argc,char **argv,const char *param,char *value);//得到程序参数
int get_path(char *argv,std::string &path);
void add_filter_time(filter_time &m_filter_time);//加入filter_time,使主线线程监视自已的规则是否还有效.
void check_time();//监视后有的线程的规则看是否有效.
FUNC_TYPE FUNC_CALL time_thread(void *arg);
int del_filter_time();//删除自已加入的filter_time
int time_sync();//时间同步,得出要多少秒后,产生ALARM信号.使得产生信号时秒刚好是整数.
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
