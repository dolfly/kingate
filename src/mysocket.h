/************************************
一个socket类，工作在linux,和win32平台下(作者：king(king@txsms.com));
版本：v0.1 build 20020718
***********************************/

#ifndef MYSOCKET1_H__6249DFB7_FF97_4C74_8833_8DFE68599955__INCLUDED_
#define MYSOCKET1_H__6249DFB7_FF97_4C74_8833_8DFE68599955__INCLUDED_ 
//#define USE_SSL
#define USEDNSCACHE
#define USETITLEMSG
#ifdef HAVE_CONFIG_H
#include "config.h"
#endif
#if     !defined(HAVE_SOCKLEN_T)
#if     defined(_AIX41)
typedef         size_t          socklen_t;
#else
typedef         int             socklen_t;
#endif
#endif  /* !HAVE_SOCKLEN_T */

#ifndef MIN
#define MIN(a,b)        (((a)<(b))?(a):(b))
#endif
#ifdef _WIN32 //for win32
	#define		FD_SETSIZE		8192 
	#include <Winsock2.h>
	#ifndef bzero
	#define bzero(X,Y)	memset(X,0,Y)
	#endif
	#define close2(X)	closesocket(X)
#else	//for linux
#define SOCKET	int
#define INVALID_SOCKET	-1
	#include<sys/wait.h>
	#include <netinet/in.h>
	#include<sys/socket.h>
	#include<string.h>
	#include<netdb.h>
	#include<arpa/inet.h>
	#include<unistd.h>
	#define close2(X)	close(X)
	#ifdef USE_SSL
	#include <openssl/ssl.h>
	#include <openssl/err.h>
	#endif
	#include <sys/select.h>
	#include <sys/time.h>
	#include <sys/types.h>
#endif
#ifdef USETITLEMSG
#include<string>
#endif
#ifdef USEDNSCACHE
#include "KDnsCache.h"
#endif
#include "KMutex.h"
#define OLD					0
#define NEW					1
#define CHILD				NEW
#define MAIN				OLD

#define TCP					1
#define UDP					2
#define TCP_MODEL			1
#define UDP_MODEL			2
#define UDP_MODEL_E			3		//保证的UDP协议连接
#define SSL_MODEL			4


#define ADDR_IP				0
#define ADDR_NAME			1


class mysocket  
{
	public: 
		mysocket();
		mysocket(int socket_type);
		~mysocket();
		void create(SOCKET socket_id);
		int connect(struct sockaddr_in *server_addr);
		bool connect(unsigned long ip,int port);
		int connect(const char * host,int port,int host_type=ADDR_NAME,int protocol=-1);
	//	int connect(const char * host,int port,int socket_type);
		int send(const char *str);
		int send(const char *str,int len,int tmo=-1);
		int recv(char *str,int len,const char * end_str=NULL);
		void clear_recvq(int len);
		int open(int port=0,const char * ip=NULL);
		SOCKET accept();
		int bind(int port);
		void use(int socket);//select to use socket,socket=0 use old socket,socket=1 use new socket.
		unsigned set_time(unsigned second=0);
		unsigned get_time();
		int close();
		int shutdown(int howto);
		int closed();//socket是否关闭,如果是返回1,否则返回0；
		int recv2(char * buf,int len,int timo=-1);
		#ifdef USETITLEMSG
		void set_title_msg(const char *msg);
		void get_title_msg(std::string &title_msg_result);
		#endif
	//	int get_client_port();
//		int get_server_port();
		int get_listen_port();
		int get_self_port();
		int get_remote_port();

		const char * get_remote_name();		
		const char * get_self_name();

	//	unsigned long get_client_addr();
		unsigned long get_self_addr();
		unsigned long get_remote_addr();

		struct sockaddr_in * get_sockaddr();
		int get_protocol();
		SOCKET get_socket(int socket=NEW);
	//	int get_socket();
		void set_protocol(int protocol);
		mysocket * clone();//
		#ifdef USE_SSL
		int sslutil_accept();
		int sslutil_connect();
		#endif
		//旧函数
		
	//	int get_port(int sockfd=NEW);
		int operator<<(const char *str);
		int operator<<(int value);
	public:
		unsigned long frag;
		unsigned long send_size;
		unsigned long recv_size;
		SOCKET new_socket,old_socket;
	private:
		#ifdef USETITLEMSG
		std::string title_msg;
		KMutex title_msg_lock;
		#endif
		void create();
		int m_protocol;
		struct hostent *h;
		struct sockaddr_in addr;
		char client_ip[18];
		unsigned time_value;	
		#ifdef USE_SSL
		SSL_CTX  *sslContext;
		SSL	*ssl;
		#endif
};
void init_socket();
void clean_socket();
int connect(int sockfd, const struct sockaddr *serv_addr,socklen_t addrlen,int tmo);
void make_ip(unsigned long ip,char *ips,bool mask=false);
extern KMutex m_make_ip_lock;
#endif 
