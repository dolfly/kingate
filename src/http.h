#ifndef FTP_H_SADFLKJASDLFKJASDLFKJ234234234
#define FTP_H_SADFLKJASDLFKJASDLFKJ234234234
#include "kingate.h"
#include "oops.h"

#define GET_METHOD		1
#define POST_METHOD		2
#define OPTIONS_METHOD		3
#define PUT_METHOD		4
#define DELETE_METHOD		5
#define TRACE_METHOD		6
#define CONNECT_METHOD		7

#define 	HEAD_OK   			0
#define		HEAD_OK_NO_STORE		1
#define		CONNECT_ERR			2
#define		HEAD_UNKNOW			3
#define 	HEAD_NOT_OK			4
#define		HEAD_NOT_MODIFIED		5
#define		SEND_FROM_MEM_OK		6
//#define 	CLIENT_HAVE_DATA		7
//#define		CLOSE_CLIENT			(1<<20)
//#define		SEND_DATA			(1<<21)
typedef struct 
{
	int method;
	int protocol;
	char user[MAX_URL];
	char pass[MAX_URL];
	char host[MAX_URL];
	int	 port;
	char file[MAX_URL];
} URL_MESSAGE;
int split_url_message(const char * str,URL_MESSAGE * url);
void http_ftp_start(request * rq);
int ftp_split_host(URL_MESSAGE  * url);
bool build_data_connect(mysocket * control_connect,mysocket * data_connect,char *err_msg);
void file_not_found(mysocket * server,url * m_url,const char *err_msg);
const char * get_content_type(const char * file_name);
bool ftp_is_dir(mysocket *server,mysocket *client,url * m_url,char *err_msg);
bool ftp_set_type(mysocket * control_connect,const char * type,char *err_msg);
bool ftp_login(mysocket * control_connect,url * m_url,char *err_msg);
int		oops_http_init();
int		send_from_mem(struct request *rq, struct mem_obj *obj);
int		fill_mem_obj(struct request *rq,struct mem_obj *obj,struct mem_obj *old_obj=NULL);
int		send_data_from_obj(struct request*, struct mem_obj *);
int		send_not_cached(SOCKET, struct request*, char*);
int parse_url(char *src, struct url *url, char *httpv=NULL);
bool CheckStringModel(const char *str,const char *model,int flag);
bool CheckFileModel(const url  *m_url,const char *model,int flag);
int url_unencode(char *url_msg,size_t url_len=0);
int stored_obj(struct mem_obj *obj);
void dead_obj(mem_obj *obj);
int parse_raw_url(char *src, struct url *url);
#ifdef CONTENT_FILTER
int check_filter_key(const char *msg,int msg_len);
#endif
#endif
