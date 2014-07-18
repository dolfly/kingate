/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/
 #ifndef KINGATE_CONFIG_H
#define KINGATE_CONFIG_H
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#ifndef _WIN32
#include <unistd.h>
#endif
#include <vector>
#include <string>
#include <map>
//#include "filter.h"
//#dinclude "config2.h"
#include "kingate.h"
#include "KConfig.h"
#include "KFilter.h"
#include "malloc_debug.h"
#define REFRESH_AUTO	0
#define	REFRESH_ANY		1	
#define REFRESH_NEVER	2

#define USE_PROXY	1
#define USE_LOG		(1<<1)
#define IGNORE_CASE	(1<<2)
#define NO_FILTER	(1<<2)
//�ض���
struct REDIRECT {
	int src_port;//Դ�˿�
	char dest_addr[18];//Ŀ��IP������
	int dest_port;	//Ŀ�Ķ˿�
	int time_out;
//	int max;	//����û���
//	filter m_filter;
//	struct REDIRECT * next;//��һ���ض����ַ
};
typedef	struct REDIRECT REDIRECT;
/*
struct SMTP_HOST{//smtp�������б�
	char src_ip[17];
	char smtp_server[HOST_LEN];
	struct SMTP_HOST *next;
};

typedef struct SMTP_HOST SMTP_HOST;
*/
typedef struct 
{
	char state[5];//�����Ƿ��
	int port;
	char user[PACKAGE_SIZE];
	char pass[PACKAGE_SIZE];
} REMOTE_MANAGE;

struct host_infomation
{
	long ip;
	short port;
};
/*
class host_infomation2
{
public:
	char *host;
	int port;
	host_infomation2(){host=NULL;}
//	~host_infomation2(){if(host) free(host);}
};
typedef std::map<std::string,host_infomation2> smtpmap;
*/
class http_redirect 
{
	/*
	long dst_ip;
	long dst_mask;
	bool dst_ip_revers;
	short dst_port;
	bool dst_port_revers;
	*/
public:
	IP_MODEL dst;
	char *file_ext;	
	bool file_ext_revers;
	int flag;
	std::vector<host_infomation>hosts;
	http_redirect(){file_ext=NULL;}
//	~http_redirect(){if(file_ext) free(file_ext);}
};
//�����������
typedef struct{
//	char * service[TOTAL_SERVICE];		//����򿪻��ǹر�ָ��
	int state[TOTAL_SERVICE];			//����״̬,��/�ر�
	unsigned int port[TOTAL_SERVICE];	//����˿�
	unsigned int max;			//����߳�
	unsigned int max_per_ip;		//ÿ��IP���������
	unsigned int max_deny_per_ip;		
	int	     min_free_thread;
	unsigned int time_out[TOTAL_SERVICE];
//	filter m_filter;
	
//	unsigned short filter_type;//����ģʽ,FILTER_FILE,FILTER_OPEN,FILTER_CLOSE

	//��������
//	char dns_server[32];//dns������

	char *run_user;
	//��־����
	int log_level;
	char log_model;
	//char log_rotate[65];
	std::string log_rotate;
	bool log_close_msg;
	char bind_addr[20];//�������󶨵�ַ
	//smtp�������б���ڵ�ַ
//	SMTP_HOST * m_smtp;
	//�ض������
//	REDIRECT * redirect;
	std::vector<REDIRECT> redirect;
	
	//��ȫ����ϵͳ
//	int security_model;
	
//	REMOTE_MANAGE remote_manage;//Զ�̹���

	std::string path;//kingate·��
	char user_model[64];//�û���֤ģ��
//	short proxy_type;
	bool socks5_user;
	unsigned long mem_cache;
	unsigned long disk_cache;
	bool use_disk_cache;
	int  refresh;
	int refresh_time;
	int min_limit_speed_size;
	int limit_speed;
	int user_time_out;
	bool http_accelerate;
	bool x_forwarded_for;
	bool insert_via;
	char *ftp_redirect;
	int ftp_redirect_port;
	KFilter m_kfilter;
	std::vector<http_redirect> m_http_redirects;
	#ifdef CONTENT_FILTER
	std::vector<char *> keys;
	int max_key_len;
	#endif
	std::vector<char *> afurls;//anti fatboy url
	int total_seconds;
	int max_request;
//	smtpmap	smtp_host;	
} CONFIG;
extern CONFIG conf;
extern int m_debug;
extern KConfig m_config;
extern KConfig m_locale;
//�����ļ����溯��
int get_dns_server_from_local(char *m_dns);
void do_config();
//void create_filter(int protocol,config * m_config);
//bool set_rule(RULE * rule,const char *src,const char *dest);
//void create_redirect(KConfig &m_config);
void create_redirect(KConfig &m_config,std::vector<REDIRECT> &m_redirect);
//void create_smtp_server(KConfig &m_config,smtpmap &m_smtpmap);
void create_http_redirect(KConfig &m_config,std::vector<http_redirect> &http_redirects);
unsigned long get_cache(const char *size);
bool saveConfig();
//void import_filter(filter *m_filter,config *m_config);
//int import_filter(filter * m_filter,const char *file_name);

//void socket5_proxy(CONFIG * m_conf);
#endif
