#ifndef FTP_H_asdfkajsdhfeiyr98yw9e8yrwerjhsdfasdf
#define FTP_H_asdfkajsdhfeiyr98yw9e8yrwerjhsdfasdf
#include "do_config.h"
#include "mysocket.h"
#include "utils.h"
#include "kingate.h"
typedef struct{
	int ftp_data_cmd;
	int port;
	HOST_MESSAGE remote;
	char client_ip[20];
	mysocket server;
	SERVER *m_server;
} FTP_DATA;
int get_host_message_from_ftp_str(const char * str,HOST_MESSAGE * m_host,int ftp_cmd);
int rewrite_cmd(SERVER * m_server,char *str,int ftp_data_cmd);
void ftp_proxy();
mysocket * create_ftp_connect(SERVER * m_server);
void create_pasv_str(SERVER * m_server,mysocket *server,char * str);
void create_port_str(SERVER * m_server,mysocket *server,char * str);
void ftp_data_proxy(SERVER * m_server,FTP_DATA * m_ftp,char *str);
#endif
