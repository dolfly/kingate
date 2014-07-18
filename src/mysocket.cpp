//#include "stdafx.h"
/************************************
一个socket类，工作在linux,和win32平台下(作者：king(king@txsms.com));
版本：v0.12 build 20020718
***********************************/
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include "mysocket.h"
#include "malloc_debug.h"
#ifndef _WIN32
#define BSD_COMP
#include <sys/ioctl.h>
#include<syslog.h>
#else
static WSADATA wsaData;
#endif
#ifdef USEDNSCACHE
KDnsCache m_dns_cache;
#endif
KMutex m_make_ip_lock;
int WaitForReadWrite(SOCKET sockfd,int flag,int timeo)
{
        struct timeval tm;
        fd_set fds;
        if(sockfd<0)
                return 0;
	if(timeo<=0)
		return 1;
        FD_ZERO(&fds);
        FD_SET(sockfd,&fds);
        tm.tv_sec=timeo;
	tm.tv_usec=0;
        int ret=select(sockfd+1,((flag==0)?&fds:NULL),((flag==1)?&fds:NULL),NULL,&tm);
	if(ret<=0)
		return ret;
	if(FD_ISSET(sockfd,&fds))
		return 1;
        return 0;
}
void make_ip(unsigned long ip,char *ips,bool mask)
{

	struct in_addr s;
	memset(ips,0,18);
	ips[0]='/';
	int skip=1;
	if(!mask)
		skip=0;
	s.s_addr=ip;
	if(ip==0)
		strcpy(ips,"*");
	else if(ip==~0)
		ips[0]=0;
	else if(ip==1)
		strcpy(ips,"localhost");
	else{	
		m_make_ip_lock.Lock();
		strncpy(ips+skip,inet_ntoa(s),16);
		m_make_ip_lock.Unlock();
	}
}
void init_socket()
{
	#ifdef _WIN32
	static WORD wVersionRequested;
	int err; 
	wVersionRequested = MAKEWORD( 2, 2 ); 
	err = WSAStartup(wVersionRequested, &wsaData );
	#endif
}
void clean_socket()
{
#ifdef _WIN32
	WSACleanup();
#endif
}
int setnoblock(int sockfd)
{
	int	iMode=1;
#ifdef _WIN32
	ioctlsocket(sockfd, FIONBIO, (u_long FAR*)&iMode);
#else
	if(ioctl(sockfd, FIONBIO, &iMode)!=0){
		syslog(LOG_NOTICE,"ioctl error in %s:%d with errno=%d",__FILE__,__LINE__,errno);
	}
#endif
	return 1;
}
int setblock(int sockfd)
{
	int	iMode=0;
#ifdef _WIN32
	ioctlsocket(sockfd, FIONBIO, (u_long FAR*) &iMode);
#else
	if(ioctl(sockfd, FIONBIO, &iMode)!=0){
		syslog(LOG_NOTICE,"ioctl error in %s:%d with errno=%d",__FILE__,__LINE__,errno);
	}
#endif
	return 1;
}
int connect(int sockfd, const struct sockaddr *serv_addr,socklen_t addrlen,int tmo)
{
//#ifndef _WIN32
	struct timeval	tv;
	fd_set	wset;
	int ret;
	if(tmo==0)
		return connect(sockfd,serv_addr,addrlen);
	return connect(sockfd,serv_addr,addrlen);
/*	state=fcntl(sockfd, F_GETFL, 0);
	fcntl(sockfd, F_SETFL, state|O_NONBLOCK );
*/
	setnoblock(sockfd);
	connect(sockfd,serv_addr,addrlen);
	tv.tv_sec=tmo;
	tv.tv_usec=0;
	FD_ZERO(&wset);
	FD_SET(sockfd,&wset);
	ret = select(sockfd+1,NULL,&wset,NULL,&tv);
//	fcntl(sockfd, F_SETFL, state );
	setblock(sockfd);
	if(ret<=0)
		return -1;
	return 0;
/*#else
	return connect(sockfd,serv_addr,addrlen);
#endif
*/
};
mysocket::mysocket()
{

	new_socket=INVALID_SOCKET;
	old_socket=INVALID_SOCKET;
//	title_msg=NULL;
//	uid=0;
	m_protocol=TCP;
	frag=send_size=recv_size=0;
	time_value=0;
	memset(&addr,0,sizeof(addr));
	#ifdef USE_SSL
	SSL_load_error_strings();
    	SSLeay_add_ssl_algorithms();
	sslContext =SSL_CTX_new(SSLv3_method());
	if(sslContext==NULL)
		fprintf(stderr,"ssl_ctx_new function error\n");
	#endif
#ifdef USE_UDP_E
	last_sequence=1;
#endif
	
}
mysocket::mysocket(int socket_type)
{

	new_socket=INVALID_SOCKET;
	old_socket=INVALID_SOCKET;
//	title_msg=NULL;
//	uid=0;
	time_value=0;
	if(socket_type==UDP)
		m_protocol=UDP;
	else
		m_protocol=TCP;
	frag=send_size=recv_size=0;
	memset(&addr,0,sizeof(addr));
#ifdef USE_UDP_E
	last_sequence=1;
#endif
}
mysocket::~mysocket()
{
//	printf("%d,%d,%d,%s,%s\n",wsaData.iMaxSockets,wsaData.wHighVersion,wsaData.wVersion,wsaData.szDescription,wsaData.szSystemStatus);

	close();
/*
	if(title_msg)
		free((char *)title_msg);
*/
}
#ifdef USETITLEMSG
void mysocket::set_title_msg(const char *msg)
{
	if(msg==NULL)
		return;
	title_msg_lock.Lock();
	title_msg=msg;
	title_msg_lock.Unlock();
}
void mysocket::get_title_msg(std::string &title_msg_result)
{
	title_msg_lock.Lock();
	title_msg_result=title_msg;
	title_msg_lock.Unlock();
}
#endif
/** No descriptions */
bool mysocket::connect(unsigned long ip,int port)
{
	addr.sin_addr.s_addr=ip;
	addr.sin_family=AF_INET;
		int n=1;
	addr.sin_port=htons(port);
	switch(m_protocol){
	case TCP:
		if(new_socket!=INVALID_SOCKET){
			close();
		}
		if((new_socket=socket(AF_INET,SOCK_STREAM,0))==INVALID_SOCKET){
				return false;

		}
#ifdef _WIN32
		setsockopt(old_socket,SOL_SOCKET,SO_REUSEADDR,(const char *)&n,sizeof(int)); 
	//	printf("now try to connect %x:%d.\n",ip,port);
#endif
		if(::connect(new_socket,(struct sockaddr *)(&addr),sizeof(struct sockaddr),time_value)<0){
			//	new_socket=-1;
		//		printf("*************************connect errno=%d\n",WSAGetLastError());
				return false;
		}
		break;
	case UDP:
		if(new_socket<=0){
			if((new_socket=socket(AF_INET,SOCK_DGRAM,0))==INVALID_SOCKET){
			//	new_socket=-1;
				return false;
			}
		}
		break;
	}

	return true;

}
int mysocket::connect(struct sockaddr_in *server_addr)
{
	close();
	 if((new_socket=socket(AF_INET,SOCK_STREAM,0))==INVALID_SOCKET){
	//		new_socket=-1;
			return 0;
	  }
	memcpy(&addr,server_addr,sizeof(addr));
	if(::connect(new_socket,(struct sockaddr *)(&addr),sizeof(struct sockaddr),time_value)<0)
		return 0;
	return 1;
}
int mysocket::connect(const char *host,int port,int host_type,int protocol)
{
	struct hostent	he_b;
	char		he_strb[2048];
	int		he_errno;
	int		rc = 0;
	struct hostent	*he_x;
	int work_type=m_protocol;

	if(protocol!=-1)
		work_type=protocol;
	switch(work_type){
		case TCP_MODEL:
		  close();
		  if((new_socket=socket(AF_INET,SOCK_STREAM,0))==INVALID_SOCKET){
			//	new_socket=-1;
				return 0;

		  }
		  addr.sin_family=AF_INET;
		  addr.sin_port=htons(port);
#ifdef USEDNSCACHE
		addr.sin_addr.s_addr=m_dns_cache.GetName(host);
		if(addr.sin_addr.s_addr==0)
			return 0;
#else
		  if(host_type==ADDR_NAME){
		
#ifdef WIN32
					if((h=gethostbyname(host))==NULL){
						::close2(new_socket);
						new_socket=INVALID_SOCKET;
						return 0;
					}
				//	memcpy(&addr.sin_addr,h->h_addr,MIN(h->h_length,sizeof(addr.sin_addr)));
				addr.sin_addr=*((struct in_addr *)h->h_addr);

#else
				rc = gethostbyname_r(host, &he_b, he_strb, sizeof(he_strb),
				&he_x,
				&he_errno);
				if(rc!=0){
					::close2(new_socket);
					new_socket=-1;
					return 0;
				}
				addr.sin_addr=*((struct in_addr *)he_b.h_addr);
#endif
			}else if(host_type==ADDR_IP)
				addr.sin_addr.s_addr=inet_addr(host);
#endif
			
		  memset(&(addr.sin_zero),0,8);
		  if(::connect(new_socket,(struct sockaddr *)(&addr),sizeof(struct sockaddr),time_value)<0){
				 //::close2(new_socket);
				 //new_socket=-1;
				return 0;
		  }
		
		 // printf("socket=%d\n",new_socket);

		
	
		break;
	case UDP_MODEL:
		if(new_socket<=0)
			if((new_socket=socket(AF_INET,SOCK_DGRAM,0))==INVALID_SOCKET)
				return 0;
		addr.sin_family=AF_INET;
		addr.sin_port=htons(port);
#ifdef USEDNSCACHE
		addr.sin_addr.s_addr=m_dns_cache.GetName(host);
		if(addr.sin_addr.s_addr==0)
			return 0;
#else
#ifdef _WIN32
		if((h=gethostbyname(host))==NULL){
			::close2(new_socket);
			new_socket=-1;
		return 0;
		}
		addr.sin_addr=*((struct in_addr *)h->h_addr);
#else
		rc = gethostbyname_r(host, &he_b, he_strb, sizeof(he_strb),
			&he_x,
			&he_errno);
		if(rc!=0){
			::close2(new_socket);
			new_socket=-1;
			return 0;
		}
		addr.sin_addr=*((struct in_addr *)he_b.h_addr);
#endif
#endif
		//  addr_addr.sin_addr.s_addr=inet_addr(host);
		memset(&(addr.sin_zero),0,8);
		break;
	case SSL_MODEL:
		if(connect(host,port,host_type,TCP)<=0)
			return -1;
		#ifdef USE_SSL
		return sslutil_connect();
		#endif
		break;
	default:
		return -6;//protocol is error
	}
	return 1;
}
void mysocket::create(SOCKET socket_id)
{
	m_protocol=TCP_MODEL;
	new_socket=socket_id;
	socklen_t addr_len=sizeof(addr);
	::getpeername(new_socket,(struct sockaddr *)&addr,&addr_len);
}
int mysocket::close()
{
	int ret=new_socket;
	if(new_socket!=INVALID_SOCKET){
		ret=::close2(new_socket);
		new_socket=INVALID_SOCKET;
	}
	if(old_socket!=INVALID_SOCKET){		
		::close2(old_socket);
		old_socket=INVALID_SOCKET;
	}
	return ret;
}
int mysocket::shutdown(int howto)
{
	int  ret;
	 ret=::shutdown(new_socket,howto);
	// new_socket=-1;
	 return ret;
}

int mysocket::send(const char *str)
{
	if(str==NULL)
		return 0;
	return send(str,strlen(str));
}
int mysocket::send(const char *str,int len,int tmo)
{	
	if(tmo==-1)
		tmo=time_value;
	if(WaitForReadWrite(new_socket,1,tmo)<=0)
	       return -1;
	switch(m_protocol){
		case TCP_MODEL:
			len=::send(new_socket,str,len,0);
			if(len>0)
				send_size+=len;
			break;
		case UDP_MODEL:
			len=::sendto(new_socket,str,len,0,(struct sockaddr *)&addr,sizeof(addr));
			if(len>0)
				send_size+=len;
			break;
		#ifdef USE_SSL
		case SSL_MODEL:
			len = SSL_write(ssl, str, len);
			if(len>0)
				send_size+=len;
			break;
		#endif
		default:
			return -1;
	}
	return len;
}
int mysocket::recv2(char * buf,int len,int tmo)
{
	socklen_t addr_len;
	if(tmo==-1)
		tmo=time_value;
	if(WaitForReadWrite(new_socket,0,tmo)<=0)
		return -1;
	switch(m_protocol){
		case TCP_MODEL:
			len=::recv(new_socket,buf,len,0);
			if(len>0)
				recv_size+=len;
			break;
		case UDP_MODEL:
			addr_len=sizeof(addr);
			len=recvfrom(new_socket,buf,len,0,(struct sockaddr *)&addr,&addr_len);
			if(len>0)
				recv_size+=len;
			break;
		default:
			return -1;
	}
	return len;
}
int mysocket::recv(char *str,int len,const char * end_str)
{
	int length=0;
	char * buffer=str;
	int max_len=len;
	if(end_str!=NULL){
		memset(str,0,len);
		max_len--;
		len=1;
	}
	if(closed())
		return -1;
	int remaining = max_len;
	while(remaining > 0){
		if(WaitForReadWrite(new_socket,0,time_value)<=0)
			return -1;
		length=::recv(new_socket,str,len,0);
		if(length<=0)
			return length;
		remaining -= length;
		recv_size+=length;
		if( (end_str==NULL)|| (strstr(buffer,end_str)!=NULL) )
			break;		
		str += length;		
	}
	length=max_len-remaining;
//	printf("length=%d,max_len=%d,remaining=%d.\n",length,max_len,remaining);
	return length;
}
void mysocket::clear_recvq(int len)
{
	if(len<=0)
		return;
	char *str=(char *)malloc(len);
	setnoblock(new_socket);
	::recv(new_socket,str,len,0);
	setblock(new_socket);
	free(str);
}
int mysocket::open(int port,const char * ip)
{
	int n=1;
	if(m_protocol==TCP){
		if(old_socket==INVALID_SOCKET)
		if((old_socket=socket(AF_INET,SOCK_STREAM,0))==INVALID_SOCKET)
			return -1;
		memset(&addr,0,sizeof(addr));
		addr.sin_family=AF_INET;
		addr.sin_port=htons(port);
		if(ip==NULL)
			addr.sin_addr.s_addr=htonl(0);
		else
			addr.sin_addr.s_addr=inet_addr(ip);
#ifndef _WIN32
		setsockopt(old_socket,SOL_SOCKET,SO_REUSEADDR,(const char *)&n,sizeof(int)); 
#endif
	//	if(port!=0)
			if(::bind(old_socket,(struct sockaddr *) &addr,sizeof(struct sockaddr))<0){
				::close2(old_socket);
				old_socket=-1;
				return -2;
			}
		if(::listen(old_socket,128)<0){
			::close2(old_socket);
	//		old_socket=-1;
			return -3;
		}
		return 1;
	}else if( m_protocol==UDP_MODEL || m_protocol==UDP_MODEL_E ){
		if(port!=0){
			addr.sin_addr.s_addr=INADDR_ANY;
			addr.sin_family=AF_INET;
			addr.sin_port=htons(port);
			if(new_socket<=0)
			if((new_socket=socket(AF_INET,SOCK_DGRAM,0))==INVALID_SOCKET)
				return -1;
			
			if(::bind(new_socket,(struct sockaddr *) &addr,sizeof(struct sockaddr))<0){
				::close2(new_socket);
				new_socket=INVALID_SOCKET;
				return -2;
			}
		}else{
			connect("127.0.0.1",0,ADDR_IP);
			send("test");
		}
		
		return 1;
	}else if(m_protocol==SSL_MODEL){

	}
	return -4;
}
int mysocket::bind(int port)
{
	if(new_socket<=0)
	if((new_socket=socket(AF_INET,SOCK_STREAM,0))<0)
		return -1;
	memset(&addr,0,sizeof(addr));
	addr.sin_family=AF_INET;
	addr.sin_port=htons(port);
	addr.sin_addr.s_addr=htonl(0);
	if(::bind(new_socket,(struct sockaddr *) &addr,sizeof(struct sockaddr))<0){
		::close2(new_socket);
		new_socket=INVALID_SOCKET;
		return -2;
	}
	return 1;
}
SOCKET mysocket::accept()
{
	socklen_t sin_size=sizeof(struct sockaddr);
//	struct linger m_linger;
	new_socket=::accept(old_socket,(struct sockaddr *)&addr,&sin_size);
	/*
	setsockopt(new_socket,SOL_SOCKET,SO_RCVTIMEO,(void *)&tm,sizeof(tm));
	setsockopt(new_socket,SOL_SOCKET,SO_RCVTIMEO,(void *)&tm,sizeof(tm));
	*/
	return new_socket;
}
void mysocket::use(int socket)
{
	if(socket==OLD){
  		::close2(new_socket);
		new_socket=INVALID_SOCKET;
	}else{
		::close2(old_socket);
		old_socket=INVALID_SOCKET;
	}
}
/*
const char * mysocket::get_addr()
{
//	strncpy(client_ip,inet_ntoa(addr.sin_addr),16);
	make_ip(addr.sin_addr.s_addr,client_ip);
	return client_ip;

}
*/
/*
const char * mysocket::getsockname()
{
	struct sockaddr_in s_sockaddr;
	socklen_t addr_len=sizeof(s_sockaddr);
	::getsockname(new_socket,(struct sockaddr *)&s_sockaddr,&addr_len);
	make_ip(s_sockaddr.sin_addr.s_addr,client_ip);
	return client_ip;
//	return inet_ntoa(s_sockaddr.sin_addr);
}*/

int mysocket::get_protocol()
{
	return m_protocol;
}
void mysocket::set_protocol(int protocol)
{
	m_protocol=protocol;

}
int mysocket::closed()
{
	if(new_socket==INVALID_SOCKET)
		return 1;
	else
		return 0;
}

/** No descriptions */
/*
int mysocket::get_port(int sockfd)
{
	struct sockaddr_in s_sockaddr;
	if(sockfd==NEW)
		return ntohs(addr.sin_port);
	
	socklen_t addr_len=sizeof(s_sockaddr);
	::getsockname(old_socket,(struct sockaddr *)&s_sockaddr,&addr_len);
	return ntohs(s_sockaddr.sin_port);
}
*/
SOCKET mysocket::get_socket(int socket)
{
	if(socket==NEW)
		return new_socket;
	else
		return old_socket;
}
/*
int mysocket::get_socket(int socket)
{

		return new_socket;
}*/
mysocket * mysocket::clone()
{
	mysocket * tmp=new mysocket;
	tmp->new_socket=new_socket;
	memcpy(&tmp->addr,&addr,sizeof(addr));
//	memcpy((void *)tmp,(void *)this,sizeof(mysocket));
	tmp->old_socket=-1;
	return tmp;
}
/*
int mysocket::set_opt(int name,const void *val,socklen_t *len,int socket)
{
	if((socket==NEW)||(socket==-1))
		setsockopt(new_socket,SOL_SOCKET,name,val,len);
	if((socket==OLD)||(socket==-1))
		setsockopt(old_socket,SOL_SOCKET,name,val,len);
	return 1;
}
*/
#ifdef USE_SSL
int mysocket::sslutil_accept()
{
	int     err;
   if((ssl = SSL_new(sslContext)) == NULL){
        err = ERR_get_error();
        fprintf(stderr, "SSL: Error allocating handle: %s\n", ERR_error_string(err, NULL));
        return -1;
    }
    SSL_set_fd(ssl, new_socket);
    if(SSL_accept(ssl) <= 0){
        err = ERR_get_error();
        fprintf(stderr, "SSL: Error accepting on socket: %s\n", ERR_error_string(err, NULL));
        return -1;
    }
    fprintf(stderr, "SSL: negotiated cipher: %s\n", SSL_get_cipher(ssl));
	//d->ssl = ssl;
    return 1;
}
int mysocket::sslutil_connect()
{
	int     err;
	if((ssl = SSL_new(sslContext)) == NULL){
        err = ERR_get_error();
        fprintf(stderr, "SSL: Error allocating handle: %s\n", ERR_error_string(err, NULL));
        return -1;
    }
    SSL_set_fd(ssl, new_socket);
    if(SSL_connect(ssl) <= 0){
        err = ERR_get_error();
        fprintf(stderr, "SSL: Error conencting socket: %s\n", ERR_error_string(err, NULL));
        return -1;
    }
    fprintf(stderr, "SSL: negotiated cipher: %s\n", SSL_get_cipher(ssl));
	
    return 1;
}
#endif

int mysocket::get_self_port()
{
	struct sockaddr_in s_sockaddr;
	socklen_t addr_len=sizeof(s_sockaddr);
	::getsockname(new_socket,(struct sockaddr *)&s_sockaddr,&addr_len);
	return ntohs(s_sockaddr.sin_port);

}
int mysocket::get_remote_port()
{
	return ntohs(addr.sin_port);
}

const char * mysocket::get_remote_name()
{
//	strncpy(client_ip,inet_ntoa(addr.sin_addr),16);
	make_ip(addr.sin_addr.s_addr,client_ip);
	return client_ip;

}
unsigned long mysocket::get_self_addr()
{
	struct sockaddr_in s_sockaddr;
	socklen_t addr_len=sizeof(s_sockaddr);
	::getsockname(new_socket,(struct sockaddr *)&s_sockaddr,&addr_len);
	return s_sockaddr.sin_addr.s_addr;

}
unsigned long mysocket::get_remote_addr()
{
	return addr.sin_addr.s_addr;
}
const char * mysocket::get_self_name()
{
	struct sockaddr_in s_sockaddr;
	socklen_t addr_len=sizeof(s_sockaddr);
	::getsockname(new_socket,(struct sockaddr *)&s_sockaddr,&addr_len);
	make_ip(s_sockaddr.sin_addr.s_addr,client_ip);
	return client_ip;
}
int mysocket::get_listen_port()
{
	struct sockaddr_in s_sockaddr;
	socklen_t addr_len=sizeof(s_sockaddr);
	::getsockname(old_socket,(struct sockaddr *)&s_sockaddr,&addr_len);
	return ntohs(s_sockaddr.sin_port);
}
unsigned mysocket::set_time(unsigned second)
{
	unsigned old_time=time_value;
	time_value=second;
	return old_time;
/*
        struct timeval msec;
        msec.tv_sec=second;
        msec.tv_usec=0;
//        printf("set sock=%d timeout=%d\n",new_socket,second);
   	setsockopt(new_socket, SOL_SOCKET, SO_RCVTIMEO,(char *) &msec, sizeof(msec));
//        printf("errno=%d\n",errno);
    	setsockopt(new_socket, SOL_SOCKET, SO_SNDTIMEO,(char *) &msec, sizeof(msec));
	return 0;
*/
}
unsigned mysocket::get_time()
{
	return time_value;
}
struct sockaddr_in * mysocket::get_sockaddr()
{
	return &addr;
}
