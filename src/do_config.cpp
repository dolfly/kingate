/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/
#include <assert.h>
#include <string>
#include "do_config.h"
#include "utils.h"
#include "log.h"
#include "oops.h"
#include "KConfig.h"
#include "KUser.h"
#include "malloc_debug.h"

#ifndef CONFIG_FILE
#ifndef _WIN32
#define CONFIG_FILE "/etc/kingate.conf"
#else
#define CONFIG_FILE "\\etc\\kingate.conf"
#endif
#endif
KConfig m_config;
KConfig m_locale;
using namespace std;
unsigned long get_cache(const char *size)
{
	long radi=1;
	long cache_size=atoi(size);
	char t=size[strlen(size)-1];
	switch(t){
	case 'k':
	case 'K':
		radi=1024;
		break;
	case 'm':
	case 'M':
		radi=1024*1024;
		break;
	case 'g':
	case 'G':
		radi=1024*1024*1024;
	}
	return cache_size*radi;

}
/*
int get_dns_server_from_local(char *m_dns)
{
	KConfig m_config;
	const char *value;
	if(m_config.open("/etc/resolv.conf")<=0){
		fprintf(stderr,"Cann't open /etc/resolv.conf file.\r\n");
		exit(1);
	}	
	value=m_config.GetValue("nameserver");
	if(value==NULL){
		strcpy(m_dns,"211.141.90.68");
		return 1;
	}
	strncpy(m_dns,value,18);
	return 1;

}*/
/*
void create_smtp_server(KConfig &m_config,smtpmap &m_smtpmap)
{
	const char *value;
	SMTP_HOST * m_smtp;
	conf.m_smtp=NULL;
	char *src=NULL;
	char *server=NULL;
	int index=0;
	char *tmp=NULL;
	while((value=m_config.GetValue("smtp_server",index++))){
	//	printf("%s\n",tmp);
		if(tmp){
			free(tmp);
	//		tmp=NULL;
		}
		if(strlen(value)<=0)
			break;	
		tmp=strdup(value);
/*
		m_smtp=new SMTP_HOST;
		m_smtp->next=conf.m_smtp;
		conf.m_smtp=m_smtp;
* /		src=strtok(tmp,"_");
		if(src==NULL){
			m_config.DelName("smtp_server",index-1);
			continue;
		}
		server=strtok(NULL,"_");
		if(server==NULL){
			m_config.DelName("smtp_server",index-1);
			continue;
		}
		host_infomation2 host_info;
		host_info.host=strdup(strtok(server,":"));
		char *port_str=strtok(NULL,":");
		if(port_str==NULL){
			host_info.port=25;
		}else{
			host_info.port=atoi(port_str);
		}
		m_smtpmap.insert(make_pair<string,host_infomation2>(src,host_info));
	//	sscanf(value,"%17[^_]_%20s",m_smtp->src_ip,m_smtp->smtp_server);
		/*
		split(value,'_',0,m_smtp->src_ip);
		split(value,'_',1,m_smtp->smtp_server);
		* /
	}

}
*/
#ifdef CONTENT_FILTER
void create_content_keys(KConfig &m_config,vector<char *> &keys)
{
	const char *value;
	int index=0;
	char *tmp;
	conf.max_key_len=0;
	while((value=m_config.GetValue("key",index++))){
		if(strlen(value)<=0)
			break;
		tmp=strdup(value);
		if(strlen(tmp)>conf.max_key_len){
			conf.max_key_len=strlen(tmp);
		}
		keys.push_back(tmp);
	}
}
#endif
void create_afurls(KConfig &m_config,vector<char *> &afurls)
{
        const char *value;
        int index=0;
        char *tmp;
        while((value=m_config.GetValue("afurls",index++))){
                if(strlen(value)<=0)
                        break;
                tmp=strdup(value);
                afurls.push_back(tmp);
        }
}

void create_http_redirect(KConfig &m_config,vector<http_redirect> &http_redirects)
{
	const char *value;
	int index=0;
	char dst_model[100];
	char file_model[100];
	char hosts_model[256];
//	int proxy_flag=0;
	char flags[100];
	char *tmp;
	char *host; 
	char *ptr;
	short port=0;
	http_redirect m_http_redirect;
	host_infomation m_hosts;
	while((value=m_config.GetValue("http_redirect",index++))){
		if(strlen(value)<=0)
			break;
		memset(&m_http_redirect.dst,0,sizeof(m_http_redirect.dst));
		m_http_redirect.file_ext=NULL;
		m_http_redirect.file_ext_revers=false;
		m_http_redirect.flag=0;
		m_http_redirect.hosts.clear();

		if(sscanf(value,"%100s%100s%256s%100s",dst_model,file_model,hosts_model,flags)<4){
			fprintf(stderr,"http_redirect value %s is error,right format is: dst_model file_model hosts_model proxy_flag.\n",value);
			m_config.DelName("http_redirect",index-1);
			continue;
		}
	//	printf("dst_model=%s.\n",dst_model);
		if(!AddIPModel(dst_model,m_http_redirect.dst)){
			m_config.DelName("http_redirect",index-1);
			continue;
		}
		if(strcmp(file_model,"*")!=0){
			m_http_redirect.file_ext=strdup(file_model);
		}
		/*
		if(proxy_flag==0)
			m_http_redirect.proxy_flag=false;
		else
			m_http_redirect.proxy_flag=true;
		*/
		if(strstr(flags,"proxy"))
			SET(m_http_redirect.flag,USE_PROXY);
		if(strstr(flags,"log"))
			SET(m_http_redirect.flag,USE_LOG);
		if(strstr(flags,"ignore_case")){
			if(m_http_redirect.file_ext){
				for(int i=0;i<strlen(m_http_redirect.file_ext);i++){
					m_http_redirect.file_ext[i]=toupper(m_http_redirect.file_ext[i]);
				}
			}
			SET(m_http_redirect.flag,IGNORE_CASE);
		}
		if(strstr(flags,"no_filter")){
			SET(m_http_redirect.flag,NO_FILTER);
		}	
		tmp=hosts_model;
//		int i=m_http_redirect.hosts.size();

		while((host=strtok_r(tmp,"|",&ptr))!=NULL){
			tmp=NULL;
			memset(&m_hosts,0,sizeof(m_hosts));
			m_hosts.port=split_host_port(host,':',sizeof(hosts_model));
			m_hosts.ip=ConvertIP(host);
			m_http_redirect.hosts.push_back(m_hosts);			
		}
//	i=m_http_redirect.hosts.size();
		http_redirects.push_back(m_http_redirect);

	}	
	/*
	std::vector<http_redirect>::iterator it;
	for(it=conf.m_http_redirects.begin();it!=conf.m_http_redirects.end();it++){
		printf("dst=%x,mask=%x,dst_port=%d\n",(*it).dst.ip,(*it).dst.mask,(*it).dst.port);
		printf("file_ext=%s\n",(*it).file_ext);
		for(int i=0;i<(*it).hosts.size();i++){
			printf("host=%x,host port=%d\n",(*it).hosts[i].ip,(*it).hosts[i].port);
		}
	}
	*/
}
void create_redirect(KConfig &m_config,vector<REDIRECT> &m_redirect)
{
//	char tmp[255],tmp2[255];
	const char *value;
//	char s_file[255];
//	char tmp_port[8];
	//config s_config;
	REDIRECT redirect;
//	conf.redirect=NULL;
	int index=0;
	//printf("create redirect now\n");
	while((value=m_config.GetValue("redirect",index++))){
		if(strlen(value)<=0)
			break;
		memset(&redirect,0,sizeof(redirect));
		sscanf(value,"%d_%[^:]:%d_%d",&redirect.src_port,redirect.dest_addr,&redirect.dest_port,&redirect.time_out);
		m_redirect.push_back(redirect);
	}
}
void LoadDefault()
{
	for(int i=0;i<TOTAL_SERVICE;i++){
		conf.state[i]=0;
		conf.time_out[i]=300;
	}
	conf.max=500;
	conf.max_per_ip=0;
	strcpy(conf.bind_addr,"off");
	conf.user_model[0]=0;
	conf.refresh=REFRESH_AUTO;
	conf.refresh=10;
}
bool saveConfig()
{
		string file_name=conf.path;
		file_name+=CONFIG_FILE;
		//printf("id=%d\n",atoi(getUrlValue("id").c_str()));
		return m_config.SaveFile(file_name.c_str());
}
void do_config()
{
	char tmp[ITEM_WIDTH+1];
	memset(tmp,0,sizeof(tmp));
	const char *value;
	#ifndef DISABLE_USER
	m_user.LoadUser();
	#endif
	sprintf(tmp,"%s%s",conf.path.c_str(),CONFIG_FILE);
	printf("Try to read config file:%s ...\n",tmp);
	LoadDefault();
	if(m_config.open(tmp)!=1){
		printf("Read config file:%s error\n",tmp);
		exit(1);
	}
	printf("success!!\n");
	fflush(stdout);
	/*
	value=m_config.GetValue("locale");
	sprintf(tmp,"%s../etc/kingate_locale.%s",conf.path,value);
	if(m_locale.open(tmp)!=1){
		printf("Read locale file:%s error\n",tmp);
		exit(0);
	}
	printf("Read config file success.\n");
	*/
	value=m_config.GetValue("http");
	if(value!=NULL && strcasecmp(value,"on")==0)
		conf.state[HTTP]=1;
	value=m_config.GetValue("ftp");
	if(value!=NULL && strcasecmp(value,"on")==0)
		conf.state[FTP]=1;
	value=m_config.GetValue("telnet");
	if(value!=NULL && strcasecmp(value,"on")==0)
		conf.state[TELNET]=1;
	value=m_config.GetValue("pop3");
	if(value!=NULL && strcasecmp(value,"on")==0)
		conf.state[POP3]=1;
	value=m_config.GetValue("smtp");
	if(value!=NULL && strcasecmp(value,"on")==0)
		conf.state[SMTP]=1;
	value=m_config.GetValue("socks");
	if(value!=NULL && strcasecmp(value,"on")==0)
		conf.state[SOCKS]=1;
	value=m_config.GetValue("mms");
	if(value!=NULL && strcasecmp(value,"on")==0)
		conf.state[MMS]=1;
	value=m_config.GetValue("rtsp");
	if(value!=NULL && strcasecmp(value,"on")==0)
		conf.state[RTSP]=1;
	value=m_config.GetValue("manage");
	if(value!=NULL && strcasecmp(value,"on")==0)
		conf.state[MANAGE]=1;

	if((value=m_config.GetValue("http_port"))==NULL)
		conf.port[HTTP]=8082;
	else
		conf.port[HTTP]=atoi(value);
	if((value=m_config.GetValue("ftp_port"))==NULL)
		conf.port[FTP]=2121;
	else
		conf.port[FTP]=atoi(value);
	if((value=m_config.GetValue("telnet_port"))==NULL)
		conf.port[TELNET]=2323;
	else
		conf.port[TELNET]=atoi(value);
	if((value=m_config.GetValue("pop3_port"))==NULL)
		conf.port[POP3]=1100;
	else
		conf.port[POP3]=atoi(value);
	if((value=m_config.GetValue("smtp_port"))==NULL)
		conf.port[SMTP]=2525;
	else
		conf.port[SMTP]=atoi(value);
	if((value=m_config.GetValue("socks_port"))==NULL)
		conf.port[SOCKS]=1082;
	else
		conf.port[SOCKS]=atoi(value);
	if((value=m_config.GetValue("mms_port"))==NULL)
		conf.port[MMS]=1755;
	else
		conf.port[MMS]=atoi(value);
	if((value=m_config.GetValue("rtsp_port"))==NULL)
		conf.port[RTSP]=5540;
	else
		conf.port[RTSP]=atoi(value);
	
	if((value=m_config.GetValue("manage_port"))==NULL)
		conf.port[MANAGE]=8880;
	else
		conf.port[MANAGE]=atoi(value);
	
	if((value=m_config.GetValue("max"))!=NULL)
		conf.max=atoi(value);
	if((value=m_config.GetValue("max_per_ip"))!=NULL)
		conf.max_per_ip=atoi(value);
/*	if((value=m_config.GetValue("max_queue_thread"))!=NULL)
		conf.max_queue_thread=atoi(value);
*/	if((value=m_config.GetValue("max_deny_per_ip"))!=NULL)
		conf.max_deny_per_ip=atoi(value);
/*	if(conf.max_queue_thread<=0)
		conf.max_queue_thread=5;
*/	conf.min_free_thread=3;
	if((value=m_config.GetValue("max_per_ip"))!=NULL)
                conf.max_per_ip=atoi(value);

	if((value=m_config.GetValue("total_seconds"))!=NULL)
		conf.total_seconds=atoi(value);

	if((value=m_config.GetValue("max_request"))!=NULL)
		conf.max_request=atoi(value);

	if((value=m_config.GetValue("http_time_out"))!=NULL)
		conf.time_out[HTTP]=atoi(value);
	if((value=m_config.GetValue("ftp_time_out"))!=NULL)
		conf.time_out[FTP]=atoi(value);
	if((value=m_config.GetValue("telnet_time_out"))!=NULL)
		conf.time_out[TELNET]=atoi(value);
	if((value=m_config.GetValue("pop3_time_out"))!=NULL)
		conf.time_out[POP3]=atoi(value);
	if((value=m_config.GetValue("smtp_time_out"))!=NULL)
		conf.time_out[SMTP]=atoi(value);
	if((value=m_config.GetValue("socks_time_out"))!=NULL)
		conf.time_out[SOCKS]=atoi(value);
	if((value=m_config.GetValue("mms_time_out"))!=NULL)
		conf.time_out[MMS]=atoi(value);
	if((value=m_config.GetValue("rtsp_time_out"))!=NULL)
		conf.time_out[RTSP]=atoi(value);	
	if((value=m_config.GetValue("manage_time_out"))!=NULL)
		conf.time_out[MANAGE]=atoi(value);
	
	conf.user_time_out=0;
	if((value=m_config.GetValue("user_time_out"))!=NULL)
		conf.user_time_out=atoi(value);
	/*
	value=m_config.GetValue("dns_server");
	if(value!=NULL && strcasecmp(conf.dns_server,"local")==0)
		get_dns_server_from_local(conf.dns_server);
	else
		strncpy(conf.dns_server,value,sizeof(conf.dns_server));
	*/
	//获取log模式配置
	conf.log_close_msg=true;
	if(m_debug!=1){
		value=m_config.GetValue("log_model");
		if(value==NULL)
			conf.log_model=LOG_SYSTEM_MODEL;
		else if(strcasecmp(value,"user")==0)
			conf.log_model=LOG_USER_MODEL;
		else if(strcasecmp(value,"debug")==0)
			conf.log_model=LOG_DEBUG_MODEL;
		else
			conf.log_model=LOG_NONE_MODEL;
		if(conf.log_level==-1){
			value=m_config.GetValue("log_level");
			if(value==NULL)
				conf.log_level=1;
			else
				conf.log_level=atoi(value);
		}
		value=m_config.GetValue("log_close_msg");
		if(strcasecmp(value,"off")==0)
			conf.log_close_msg=false;
	}
	/*
	value=m_config.GetValue("log_file");
	if(value==NULL)
		strcpy(conf.log_file,"./kingate.log");
	else
		strncpy(conf.log_file,value,sizeof(conf.log_file));
	#ifdef _WIN32
	while(str_replace(conf.log_file,"/","\\",sizeof(conf.log_file)));
	#endif
	*/
	conf.log_rotate=m_config.GetValue("log_rotate");
//	memset(conf.log_rotate,0,sizeof(conf.log_rotate));
//j	if(strlen(value)==0)
/*		strcpy(conf.log_rotate,"0 0 * * *");
	else
		strncpy(conf.log_rotate,value,sizeof(conf.log_rotate)-1);
*/
	value=m_config.GetValue("bind_addr");
	if(strlen(value)==0)
		strcpy(conf.bind_addr,"*");
	else{
		bool allnum=true;
		for(int i=0;i<strlen(value);i++){
			if(value[i]!='.' && (value[i]<'0' || value[i]>'9')){
				allnum=false;
				break;
			}
		}
		memset(conf.bind_addr,0,sizeof(conf.bind_addr));
		if(allnum)
			strncpy(conf.bind_addr,value,sizeof(conf.bind_addr)-1);
		else
			strcpy(conf.bind_addr,"*");
	}
	//////////////////////////////////
	conf.run_user=NULL;
	value=m_config.GetValue("run_user");
	if(strlen(value)>0)
		conf.run_user=strdup(value);		
	//////////////////////////////////
	value=m_config.GetValue("user_model");
	memset(conf.user_model,0,sizeof(conf.user_model));
	if(strlen(value)>0)
		strncpy(conf.user_model,value,sizeof(conf.user_model)-1);


	/*
	value=m_config.GetValue("access");
	if(value[0]==0){
		printf("warning no access file.kingate will deny all connection.\n");
	}else{
*/	string access_file=conf.path;
	access_file+="/etc/access.conf";
//	conf.filter_type=FILTER_FILE;
	conf.m_kfilter.LoadConfig(access_file.c_str());
//		import_filter(conf.m_kfilter,value);
//	}
	
	create_redirect(m_config,conf.redirect);
	create_http_redirect(m_config,conf.m_http_redirects);
	#ifdef CONTENT_FILTER
	create_content_keys(m_config,conf.keys);	
	#endif
	create_afurls(m_config,conf.afurls);
	conf.use_disk_cache=true;
	if(strcmp(m_config.GetValue("use_disk_cache"),"off")==0){
		conf.use_disk_cache=false;
	}
	
//	create_smtp_server(m_config,conf.smtp_host);//创建smtp服务器列表

	/*****************************************************
		cache config
	********************************************************/
	value=m_config.GetValue("mem_cache");
	if(value[0]==0)
		conf.mem_cache=32*1024*1024;
	else
		conf.mem_cache=get_cache(value);
	
	conf.min_limit_speed_size=get_cache(m_config.GetValue("min_limit_speed_size"));
	conf.limit_speed=get_cache(m_config.GetValue("limit_speed"));
//	printf("conf.min_limit_speed_size=%d,conf.limit_speed=%d\n",conf.min_limit_speed_size,conf.limit_speed);
	conf.disk_cache=get_cache(m_config.GetValue("disk_cache"));
	value=m_config.GetValue("refresh");
	if(value[0]!=0){
		if(strcasecmp(value,"auto"))
			conf.refresh=REFRESH_AUTO;
		else if(strcasecmp(value,"never"))
			conf.refresh=REFRESH_NEVER;
		else if(strcasecmp(value,"any"))
			conf.refresh=REFRESH_ANY;
		else
			klog(ERR_LOG,"refresh value (%s) error,it must be auto,never or any.kingate will use auto",value);
	}
	value=m_config.GetValue("refresh_time");
	if(value[0]!=0)
		conf.refresh_time=atoi(value);
	conf.socks5_user=false;
	value=m_config.GetValue("socks5_user");
	if(value[0]!=0){
		if(strcasecmp(value,"on")==0){
			conf.socks5_user=true;
		}
	}
	conf.http_accelerate=false;
	value=m_config.GetValue("http_accelerate");
	if(value[0]!=0){
		if(strcasecmp(value,"on")==0){
			conf.http_accelerate=true;
		}
	}
	conf.x_forwarded_for=false;
	value=m_config.GetValue("x_forwarded_for");
	if(value[0]!=0){
		if(strcasecmp(value,"on")==0){
			conf.x_forwarded_for=true;
		}
	}
	conf.insert_via=true;
	value=m_config.GetValue("insert_via");
	if(value[0]!=0){
		if(strcasecmp(value,"off")==0){
			conf.insert_via=false;
		}
	}
	value=m_config.GetValue("ftp_redirect");
	conf.ftp_redirect=NULL;
	if(value[0]!=0){
		conf.ftp_redirect=strdup(value);
		conf.ftp_redirect_port=split_host_port(conf.ftp_redirect,':');
	}
	/*******************************************************
	remote manage config
	*****************************************************/
//	m_config.SaveFile("./test.conf");

}
void do_config_clean()
{
//	SMTP_HOST * smtp_tmp=conf.m_smtp;
//	REDIRECT * redirect_tmp=conf.redirect;
/*	while(smtp_tmp!=NULL){
		conf.m_smtp=conf.m_smtp->next;
		delete smtp_tmp;
		smtp_tmp=conf.m_smtp;
	}
	/*
	while(redirect_tmp!=NULL){
		conf.redirect=conf.redirect->next;
		delete redirect_tmp;
		redirect_tmp=conf.redirect;
	}
	*/
}
