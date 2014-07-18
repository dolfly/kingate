#ifdef HAVE_CONFIG_H
#include <config.h>
#endif
#ifndef DISABLE_HTTP
#include "KHttpManage.h"
#include "KThreadPool.h"
#include "cache.h"
#include "log.h"
#include <map>
#include <vector>
#ifdef HAVE_SSTREAM
#include <sstream>
#else
#include "mysstream"
#endif
#include <ctype.h>
#include <time.h>
#include "malloc_debug.h"

using namespace std;
string getConnectionInfo(int sortby,bool desc);
string get_connect_per_ip();
static const char *refresh_model[3]={"auto","any","never"};
string chanagePasswordForm()
{
	return "<html><body><form action=chanage_password method=get>old password:<input type=password name=old_password><br>new password:<input type=password name=new_password><br>retype new password:<input type=password name=re_new_password><br><input type=submit value=submit></form></body></html>";
}
bool KHttpManage::config()
{
	const int max_config=7;
	int i=0;
	const char *config_header[max_config]={"service","http_redirect","tcp_port_redirect","cache","log","resource_limit","other_config"};
	stringstream s;
	string item=getUrlValue("item");
	string file_name=conf.path;
	bool canWrite=true;
//	stringstream s;
	file_name+="/etc/kingate.conf";
	FILE *fp=fopen(file_name.c_str(),"a+");
	if(fp==NULL)
		canWrite=false;
	else
		fclose(fp);
	s << "<html><body>";
	if(!canWrite){
		s << "<font color=red>Warning:The user running by kingate cann't write config file: " << file_name << "</font>";
	}
	s << "<form action=/configsubmit?item=" << item << " method=post>";
	s << "<table border=1 cellpadding=0 cellspacing=0 width=100% bordercolor=\"#FFFFFF\"><tr>";
	for(i=0;i<max_config;i++){
		s << "<td width=12% align=\"center\" bgcolor=\"";
		if(item==config_header[i]){
			s << "#CCFF99\">" << config_header[i];
		}else{
			s << "#99FF99\"><a href=/config?item=" << config_header[i] << ">" << config_header[i] << "</a>";
		}
		s << "</td>";
	}
	s << "</tr></table>";

	if(item=="service"){
		s << "\n<table>";
		for(i=TOTAL_SERVICE-1;i>=0;i--){
			string port=service_name[i];
			string time_out=service_name[i];
			string state=m_config.GetValue(service_name[i]);
			port+="_port";
			time_out+="_time_out";
			
			s << "<tr><td>";
			if(state=="off")
				s << "<font color=red>";
			s << service_name[i] << ":";
			if(state=="off")
				s << "</font>";
			s << "</td><td> <select name=" << service_name[i] << "><option value=on ";
			if(state=="on"){
				s << "selected";
			}
			s << ">on</option><option value=off ";
			if(state=="off"){
				s << "selected";
			}
			s << ">off</option></select>\n</td><td>";
			s << service_name[i] << " port:</td><td>" << "<input type=text name=" << port << " value=" << m_config.GetValue(port.c_str()) << "></td><td>\n";
			s << "<B><i>" << service_name[i] << " time out:</i></B></td><td>" << "<input type=text name=" << time_out << " value=" << m_config.GetValue(time_out.c_str()) << "></td></tr>\n";
		}
		s << "</table>";
	}else if(item=="http_redirect"){
		std::vector<http_redirect> http_redirects;
		create_http_redirect(m_config,http_redirects);
		s << "<table><tr><td>operator</td><td>destination ip(s)</td><td>file</td><td>redirect to</td><td>flag</td><td>";
		for(i=0;i<http_redirects.size();i++){
			s << "<tr><td><a href=\"javascript:if(confirm('Are you sure to delete the http redirect index=" << i << "')){window.location='/configsubmit?item=http_redirect&action=delete&id="<< i << "';}\">delete</a></td><td>";
			s<< (http_redirects[i].dst.ip_revers?"!":"") << make_ip(http_redirects[i].dst.ip) ;
			s<< make_ip(http_redirects[i].dst.mask,true);
			
			s << "</td><td>";
			if(http_redirects[i].file_ext_revers)
				s << "!" ;
			if(http_redirects[i].file_ext)
				s << http_redirects[i].file_ext;
			else
				s << "*";
			s << "</td><td>";
			for(int p=0;p<http_redirects[i].hosts.size();p++){
				s << make_ip(http_redirects[i].hosts[p].ip) << ":" << http_redirects[i].hosts[p].port << "<br>";
			}
			if(http_redirects[i].file_ext)
				free(http_redirects[i].file_ext);
			s << "</td><td>" ;
			if(TEST(http_redirects[i].flag,USE_PROXY)){
				s << "proxy";
			}
			if(TEST(http_redirects[i].flag,IGNORE_CASE)){
				s << "|ignore_case";
			}
			if(TEST(http_redirects[i].flag,NO_FILTER)){
				s << "|no_filter";
			}
			s << "</td></tr>";
		}
		s << "</table><hr>Add new http redirect config:<br>";
		s << "destination ip(net):<input type=text name=dst_ip value=*><br>\n";
		s << "destination port:<input type=text name=dst_port value=*><br>\n";
		s << "destination netmask:<input type=text name=dst_mask value=*><br>\n";
		s << "url file model:<input type=text name=file value=*><br>\n";
		s << "flag:<input type=checkbox checked name=flag_ignore_case value=ignore_case>ignore_case<input type=checkbox checked name=flag_no_filter value=no_filter>no_filter<input type=checkbox checked name=flag_proxy value=proxy>proxy<br>\n";
		s << "redirect to host:port <input type=text name=redirect value=*><br>\n";
	}else if(item=="tcp_port_redirect"){
		vector<REDIRECT> redirect;
		create_redirect(m_config,redirect);

		s << "<table><tr><td>operator</td><td>listen port</td><td>redirect to</td></tr>";
		for(i=0;i<redirect.size();i++){
	//	do{

	//			if(tmp_redirect==NULL)
	//				break;
				s << "<tr><td><a href=\"javascript:if(confirm('Are you sure to delete redirect port " << redirect[i].src_port << "')){window.location='/configsubmit?item=tcp_port_redirect&action=delete&id="<< i << "';}\">delete</a><td>" << redirect[i].src_port << "</td><td>" << redirect[i].dest_addr << ":" << redirect[i].dest_port << "</td></tr>\n";

			//	redirect_proxy(tmp_redirect);
	//			i++;
	//		}while( (tmp_redirect=tmp_redirect->next)!=NULL );
		}
			s << "</table>\n";
			s << "<hr>Add a new tcp port redirect config:<br>";
			s << "listen port:<input type=text name=src_port><br>";
			s << "redirect to :(format is ip:port)<input type=text name=dst>";
	}/*else if(item=="smtp_server"){
		smtpmap m_smtpmap;
		create_smtp_server(m_config,m_smtpmap);
		s << "<table><tr><td>operator</td><td>source ip</td><td>use smtp server</td></tr>";
		smtpmap::iterator it;
		i=0;
		for(it=m_smtpmap.begin();it!=m_smtpmap.end();it++,i++){
			s << "<tr><td><a href=\"javascript:if(confirm('Are you sure to delete this smtp config')){window.location='/configsubmit?item=smtp_server&action=delete&id=" << i << "';}\">delete</a><td>" << (*it).first << "</td><td>" << (*it).second.host ;
			if((*it).second.port!=25){
				s << ":" << (*it).second.port;
			}
			if((*it).second.host)
				free((*it).second.host);
			s << "</td></tr>\n";
		}
		s << "</table><hr>add a new smtp config:<br>source ip:<input type=text name=src_ip><br>use smtp server(domain or ip):<input type=text name=host><br>smtp host port:<input type=text name=port value=25>";
		
	}*/else if(item=="cache"){
		s << "<b><i>mem_cache:</i></b>" << "<input type=text name=mem_cache value=" << m_config.GetValue("mem_cache") <<"><br>";
		s << "<b><i>disk_cache:</i></b>" << "<input type=text name=disk_cache value=" << m_config.GetValue("disk_cache") <<"><br>";
		s << "<b><i>min refresh time:</i></b><input type=text name=refresh_time value=" << conf.refresh_time << "><br>";
		s << "refresh:" << "<select name=refresh>";
		for(i=0;i<3;i++){
			s << "<option value=" << refresh_model[i] ;
			if(strcasecmp(m_config.GetValue("refresh"),refresh_model[i])==0){
				s << " selected";
			}
			s << ">" << refresh_model[i] << "</option>";
		}
		s << "</select><br>";
		s << "use disk cache :<select name=use_disk_cache><option value=on ";
                if(strcasecmp(m_config.GetValue("use_disk_cache"),"off")!=0)
                        s << "selected";
                s << " >on</option><option value=off ";
                if(strcasecmp(m_config.GetValue("use_disk_cache"),"off")==0)
                        s << "selected";
                s << " >off</option></select><br>";
		
	}else if(item=="log"){
		const int max_log_model=2;
		const char *log_model_str[max_log_model]={
		"user","nolog"
		};

		s << "rotate log time:<input type=text name=log_rotate value=\"" << m_config.GetValue("log_rotate") << "\">(format as crontab: min hour mday month wday)<br>\n";
		s << "log level:<input type=text name=log_level value=" << m_config.GetValue("log_level") << "><br>\n";
		s << "log model:<select name=log_model>";
		for(i=0;i<max_log_model;i++){
			s << "<option ";
			if(strcasecmp(m_config.GetValue("log_model"),log_model_str[i])==0)
				s << "selected";
			s << " value=" << log_model_str[i] << ">" << log_model_str[i] << "</option>\n";
		}
		s << "</select><br>";
		s << "<b><i>log the connection close msg:</i></b><select name=log_close_msg><option value=on ";
		if(conf.log_close_msg)
			s << "selected";
		s << " >on</option><option value=off ";
		if(!conf.log_close_msg)
			s << "selected";
		s << " >off</option></select>";
	}else if(item=="resource_limit"){
		s << "<b><i>total max threads(0 for unlimited): </i></b><input type=text size=5 name=max value=" << m_config.GetValue("max") <<"><br>";
		s << "<b><i>total max threads for each ip(0 for unlimited):</i></b><input type=text size=3 name=max_per_ip value=" << conf.max_per_ip << "><br>";
		s << "<b><i>total max threads will be deny for each ip(0 for unlimited):</i></b><input type=text size=3 name=max_deny_per_ip value=" << conf.max_deny_per_ip << "><br>";
//		s << "<b><i>total max threads for run the queue work:</i></b><input type=text name=max_queue_thread value=" << conf.max_queue_thread << "><br>";
		s << "<b><i>min free thread for close the free thread: </i></b><input type=text name=min_free_thread size=3 value=" << conf.min_free_thread << "><br>";
		s << "<b><i>min obj size to limit speed: </i></b><input type=text name=min_limit_speed_size size=5 value=" << m_config.GetValue("min_limit_speed_size") << "><br>";
		s << "<b><i>limit speed(0 for unlimited): </i></b><input type=text name=limit_speed size=5 value=" << m_config.GetValue("limit_speed") << ">/s<br>";
#ifndef DISABLE_USER
		s << "<b><i>user active time out:</i></b><input type=text name=user_time_out size=3 value=" << conf.user_time_out << "><br>";
#endif
		s << "<b><i>anti fatboy ddos:</i></b>max_request:<input type=text size=3 name=max_request value=" << conf.max_request << ">,total_seconds:<input type=text size=3 name=total_seconds value=" << conf.total_seconds << "><br>";
	}else if(item=="other_config"){
		s << "bind_addr: <input type=text name=bind_addr value=\"" << m_config.GetValue("bind_addr") << "\"><br>";
#ifndef _WIN32
		s << "run user for kingate:<input type=text name=run_user value=" << m_config.GetValue("run_user") << "><br>";
#endif
		s << "<b><i>http accelerate :</i></b><select name=http_accelerate><option value=on ";
		if(conf.http_accelerate)
			s << "selected";
		s << " >on</option><option value=off ";
		if(!conf.http_accelerate)
			s << "selected";
		s << " >off</option></select><br>";

		s << "<b><i>insert x_forwarded_for in http proxy :</i></b><select name=x_forwarded_for><option value=on ";
		if(conf.x_forwarded_for)
			s << "selected";
		s << " >on</option><option value=off ";
		if(!conf.x_forwarded_for)
			s << "selected";
		s << " >off</option></select><br>";
	
		s << "<b><i>insert via in http proxy :</i></b><select name=insert_via><option value=on ";
		if(conf.insert_via)
			s << "selected";
		s << " >on</option><option value=off ";
		if(!conf.insert_via)
			s << "selected";
		s << " >off</option></select><br>";
		
	
		s << "<b><i>socks5 support user auth:</i></b><select name=socks5_user><option value=on ";
		if(conf.socks5_user)
			s << "selected";
		s << " >on</option><option value=off ";
		if(!conf.socks5_user)
			s << "selected";
		s << " >off</option></select>";
		#ifdef CONTENT_FILTER
		s << "<br>filter content keys: ";
		for(i=0;i<conf.keys.size();i++){
			s << conf.keys[i] << ",";
		}
		#endif
	}
	s << "<br><input type=submit value=submit></form></body></html>";
	return	sendHttp(s.str());
}
bool KHttpManage::configsubmit()
{
	string item=getUrlValue("item");
	if(item=="service"){
		m_config.SetValue("http",getUrlValue("http").c_str());
		m_config.SetValue("ftp",getUrlValue("ftp").c_str());
		m_config.SetValue("socks",getUrlValue("socks").c_str());
		m_config.SetValue("mms",getUrlValue("mms").c_str());
		m_config.SetValue("rtsp",getUrlValue("rtsp").c_str());
		m_config.SetValue("telnet",getUrlValue("telnet").c_str());
		m_config.SetValue("pop3",getUrlValue("pop3").c_str());
		m_config.SetValue("smtp",getUrlValue("smtp").c_str());
		m_config.SetValue("manage",getUrlValue("manage").c_str());

		m_config.SetValue("http_time_out",getUrlValue("http_time_out").c_str());
		m_config.SetValue("ftp_time_out",getUrlValue("ftp_time_out").c_str());
		m_config.SetValue("socks_time_out",getUrlValue("socks_time_out").c_str());
		m_config.SetValue("mms_time_out",getUrlValue("mms_time_out").c_str());
		m_config.SetValue("rtsp_time_out",getUrlValue("rtsp_time_out").c_str());
		m_config.SetValue("telnet_time_out",getUrlValue("telnet_time_out").c_str());
		m_config.SetValue("pop3_time_out",getUrlValue("pop3_time_out").c_str());
		m_config.SetValue("smtp_time_out",getUrlValue("smtp_time_out").c_str());
		m_config.SetValue("manage_time_out",getUrlValue("manage_time_out").c_str());
		
		conf.time_out[HTTP]=atoi(getUrlValue("http_time_out").c_str());
		conf.time_out[FTP]=atoi(getUrlValue("ftp_time_out").c_str());
		conf.time_out[SOCKS]=atoi(getUrlValue("socks_time_out").c_str());
		conf.time_out[MMS]=atoi(getUrlValue("mms_time_out").c_str());
		conf.time_out[RTSP]=atoi(getUrlValue("rtsp_time_out").c_str());
		conf.time_out[TELNET]=atoi(getUrlValue("telnet_time_out").c_str());
		conf.time_out[POP3]=atoi(getUrlValue("pop3_time_out").c_str());
		conf.time_out[SMTP]=atoi(getUrlValue("smtp_time_out").c_str());
		conf.time_out[MANAGE]=atoi(getUrlValue("manage_time_out").c_str());


		m_config.SetValue("http_port",getUrlValue("http_port").c_str());
		m_config.SetValue("ftp_port",getUrlValue("ftp_port").c_str());
		m_config.SetValue("socks_port",getUrlValue("socks_port").c_str());
		m_config.SetValue("mms_port",getUrlValue("mms_port").c_str());
		m_config.SetValue("rtsp_port",getUrlValue("rtsp_port").c_str());
		m_config.SetValue("telnet_port",getUrlValue("telnet_port").c_str());
		m_config.SetValue("pop3_port",getUrlValue("pop3_port").c_str());
		m_config.SetValue("smtp_port",getUrlValue("smtp_port").c_str());
		m_config.SetValue("manage_port",getUrlValue("manage_port").c_str());


	}else if(item=="http_redirect"){
		string action=getUrlValue("action");
		if(action=="delete"){
			int id=atoi(getUrlValue("id").c_str());
			m_config.DelName("http_redirect",id);
		}else{
			stringstream http_redirect_value;
			http_redirect_value << getUrlValue("dst_ip");
			if(getUrlValue("dst_port")!="*"){
				http_redirect_value << ":" << getUrlValue("dst_port");
			}
			if(getUrlValue("dst_mask")!="*"){
				http_redirect_value << "/" << getUrlValue("dst_mask");
			}
			http_redirect_value << " " ;
			if(getUrlValue("file").size()<=0){
				http_redirect_value << "*";
			}else{
				http_redirect_value << getUrlValue("file");
			}
			http_redirect_value << " ";
			if(getUrlValue("redirect").size()<=0){
				http_redirect_value << "*";
			}else{
				http_redirect_value << getUrlValue("redirect");
			}
			http_redirect_value << " " << getUrlValue("flag_ignore_case") << "|" << getUrlValue("flag_proxy") << "|" << getUrlValue("flag_no_filter");
			m_config.AddValue("http_redirect",http_redirect_value.str().c_str(),false,true,true);
			
		}
	}else if(item=="cache"){
		m_config.SetValue("mem_cache",getUrlValue("mem_cache").c_str());
		m_config.SetValue("disk_cache",getUrlValue("disk_cache").c_str());
		m_config.SetValue("refresh",getUrlValue("refresh").c_str());
		m_config.SetValue("refresh_time",getUrlValue("refresh_time").c_str());
		m_config.SetValue("use_disk_cache",getUrlValue("use_disk_cache").c_str());
				
		conf.mem_cache=get_cache(getUrlValue("mem_cache").c_str());
		conf.disk_cache=get_cache(getUrlValue("disk_cache").c_str());
		conf.refresh_time=atoi(getUrlValue("refresh_time").c_str());
	}else if(item=="log"){
		m_config.SetValue("log_rotate",getUrlValue("log_rotate").c_str(),0,true);
		m_config.SetValue("log_model",getUrlValue("log_model").c_str());
		m_config.SetValue("log_level",getUrlValue("log_level").c_str());
		m_config.SetValue("log_close_msg",getUrlValue("log_close_msg").c_str());
		if(getUrlValue("log_close_msg")=="on")
			conf.log_close_msg=true;
		else
			conf.log_close_msg=false;
	}else if(item=="resource_limit"){
		int max_per_ip=atoi(getUrlValue("max_per_ip").c_str());
		m_config.SetValue("max",getUrlValue("max").c_str());
		m_config.SetValue("max_per_ip",getUrlValue("max_per_ip").c_str());
		m_config.SetValue("max_deny_per_ip",getUrlValue("max_deny_per_ip").c_str());
		m_config.SetValue("min_free_thread",getUrlValue("min_free_thread").c_str());
		m_config.SetValue("user_time_out",getUrlValue("user_time_out").c_str());
		m_config.SetValue("max_queue_thread",getUrlValue("max_queue_thread").c_str());
		m_config.SetValue("min_limit_speed_size",getUrlValue("min_limit_speed_size").c_str());
		m_config.SetValue("limit_speed",getUrlValue("limit_speed").c_str());
		m_config.SetValue("max_request",getUrlValue("max_request").c_str());
		m_config.SetValue("total_seconds",getUrlValue("total_seconds").c_str());

		conf.user_time_out=atoi(getUrlValue("user_time_out").c_str());
		conf.max=atoi(getUrlValue("max").c_str());
//		conf.max_queue_thread=atoi(getUrlValue("max_queue_thread").c_str());
		conf.max_deny_per_ip=atoi(getUrlValue("max_deny_per_ip").c_str());
		conf.min_free_thread=atoi(getUrlValue("min_free_thread").c_str());
		conf.min_limit_speed_size=get_cache(getUrlValue("min_limit_speed_size").c_str());
		conf.limit_speed=get_cache(getUrlValue("limit_speed").c_str());
		conf.max_request=atoi(getUrlValue("max_request").c_str());
		conf.total_seconds=atoi(getUrlValue("total_seconds").c_str());

		if(conf.max_per_ip!=max_per_ip)
			set_max_per_ip(max_per_ip);
	}else if(item=="other_config"){
		m_config.SetValue("bind_addr",getUrlValue("bind_addr").c_str());
		m_config.SetValue("run_user",getUrlValue("run_user").c_str());
		m_config.SetValue("socks5_user",getUrlValue("socks5_user").c_str());
		m_config.SetValue("http_accelerate",getUrlValue("http_accelerate").c_str());
		m_config.SetValue("x_forwarded_for",getUrlValue("x_forwarded_for").c_str());
		m_config.SetValue("insert_via",getUrlValue("insert_via").c_str());
				
		if(getUrlValue("http_accelerate")=="on")
			conf.http_accelerate=true;
		else
			conf.http_accelerate=false;
		if(getUrlValue("x_forwarded_for")=="on")
			conf.x_forwarded_for=true;
		else
			conf.x_forwarded_for=false;

		if(getUrlValue("insert_via")=="on")
			conf.insert_via=true;
		else
			conf.insert_via=false;
		if(getUrlValue("socks5_user")=="on")
			conf.socks5_user=true;
		else
			conf.socks5_user=false;
		
	}/*else if(item=="smtp_server"){
		string action=getUrlValue("action");
		if(action=="delete"){
			int id=atoi(getUrlValue("id").c_str());
			m_config.DelName("smtp_server",id);
		}else{
			stringstream smtp_server_value;
			smtp_server_value << getUrlValue("src_ip") << "_" << getUrlValue("host") << ":" << getUrlValue("port");
			m_config.AddValue("smtp_server",smtp_server_value.str().c_str());
		}
	}*/else if(item=="tcp_port_redirect"){
		string action=getUrlValue("action");
		if(action=="delete"){
			int id=atoi(getUrlValue("id").c_str());
			m_config.DelName("redirect",id);
		}else{
			stringstream redirect_value;
			redirect_value << getUrlValue("src_port") << "_" << getUrlValue("dst");
			m_config.AddValue("redirect",redirect_value.str().c_str());
		}
	}
	if(!saveConfig()){
		return sendErrPage("Cann't save the config file!");
	}
	string url="/config?item=";
	url+=item;
	return sendRedirect(url.c_str());
	//return true;
}
KHttpManage::KHttpManage()
{
	rq=NULL;
}
KHttpManage::~KHttpManage()
{

}
string KHttpManage::getUrlValue(string name)
{
	map<string,string>::iterator it;
	it=urlParam.find(name);
	if(it==urlParam.end())
		return "";
	return (*it).second;
	
}
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
bool KHttpManage::parseUrlParam(char *param)
{
	char *name;
	char *value;
	char *tmp;
	char *msg;
	char *ptr;
	char *ptr2;
	for(int i=0;i<strlen(param);i++){
		if(param[i]=='\r' || param[i]=='\n'){
			param[i]=0;
			break;
		}
	}
	url_unencode(param);
	tmp=param;
	char split='=';
//	strcpy(split,"=");
	while((msg=my_strtok(tmp,split,&ptr))!=NULL){
		tmp=NULL;
		if(split=='='){
			name=msg;
			split='&';
		}else{//strtok_r(msg,"=",&ptr2);
			
			value=msg;//strtok_r(NULL,"=",&ptr2);
			/*
			if(value==NULL)
				continue;
			*/
			split='=';
			for(int i=0;i<strlen(name);i++){
				name[i]=tolower(name[i]);
			}
			urlParam.insert(make_pair<string,string>(name,value));				
		}
								
	}
	return true;
}
bool KHttpManage::parseUrl(char *url)
{

	char *ptr;
	char *param;	
	if(strtok_r(url,"?",&ptr)==NULL)
		return false;
	param=strtok_r(NULL,"?",&ptr);
	if(param==NULL)
		return false;
	return parseUrlParam(param);

}
bool KHttpManage::sendHttp(const string &msg)
{	
	bool return_result=true;
	stringstream s;
        s << "HTTP/1.1 200 OK\r\nServer: kingate(" << VER_ID << ")\r\nCache-control: no-cache,no-store\r\nConnection: ";
        if(TEST(rq->flags,RQ_HAS_KEEP_CONNECTION) && (rq->meth==METH_GET) && (!TEST(rq->flags,RESULT_CLOSE)))
                s << "keep-alive\r\n";
        else{
		return_result=false;
                s << "close\r\n";
	}
        s << "Content-length: ";
	s << msg.size() << "\r\nContent-type: text/html\r\n\r\n";
	int len=0;
	if((len=rq->server->send(s.str().c_str()))<=0){
		return false;
	}
	if((len=rq->server->send(msg.c_str()))<=0){
		return false;
	}
/*	*/
//	rq->server->send("haha...");
//	return false;
//	sleep(5);
	return return_result;
	
}
void KHttpManage::sendTest()
{
	//sendHeader(200);
	map<string,string>::iterator it;
	for(it=urlParam.begin();it!=urlParam.end();it++){
		rq->server->send("name:");
		rq->server->send((*it).first.c_str());
		rq->server->send(" value:");
		rq->server->send((*it).second.c_str());
		rq->server->send("<br>");
	}
}
bool KHttpManage::sendErrPage(const char *err_msg,int close_flag)
{	
	stringstream s;
	s << "<html><body><h1><font color=red>" << err_msg ;
	if(close_flag==1){
		s << "</font></h1><br><a href=\"javascript:window.close();\">close</a></body></html>";
	}else{
		s << "</font></h1><br><a href=\"javascript:history.go(-1);\">back</a></body></html>";
	}
	return sendHttp(s.str());
}
#ifndef DISABLE_USER
bool KHttpManage::sendLogin()
{
	//sendHeader(200);
	return sendHttp("<html><body><form action=/login method=post>User:<input name=user><br>Password:<input type=password name=password><br><input type=submit value=submit></form></body></html>");
	
}
bool KHttpManage::userList()
{
	stringstream s;
	s << "<html><body><script language=javascript>function show(url) { window.open(url,'','height=200,width=450,resize=no,scrollbars=no,toolsbar=no,top=200,left=200');}</script><table><tr><td><a href=/>home</a></td><td><a href=\"javascript:show('adduserform');\">add user</a></td></tr></table>";
	s << m_user.ListUser();
	s << "</body></html>";
	return sendHttp(s.str());
}
bool KHttpManage::loginList()
{
	//sendHeader(200);
	stringstream s;
	s << "<html><body>" << m_user.ListLoginUser() << "</body></html>";
	return sendHttp(s.str());
}
#endif
bool KHttpManage::sendLeftMenu()
{
	stringstream s;
	//sendHeader(200);
	s << "<html><head><title>kingate(" << VER_ID << ") manager</title></head><body>";
	s << "<a href=/main target=mainFrame>main</a><br>";
	#ifndef DISABLE_USER
/*
	s << user_info.user->user.c_str() << " login from " << rq->server->get_remote_name() << " total time(sec) " << user_info.user->total_time+(time(NULL)-user_info.login_time) << " total send size:" << user_info.user->send_size/1024 << "kb total recv size:" << user_info.user->recv_size/1024 << "kb";
*/

	s << "<a href=logout target=_top>logout</a><br>";
	s << "<a href=chanage_password_form target=mainFrame>chanage password</a>\n";
	if(m_user.CheckGroup(user_info.user->uid,m_user.getRootGid())){
//	if(user_info.user->user=="root"){//it is the root user
	#endif

//		s << "<h3>You are the root user. You can manage kingate now!</h3>";
	#ifndef DISABLE_USER
		s << "<a href=userlist target=mainFrame>users</a><br>";
		s << "<a href=loginlist target=mainFrame>login users</a><br>";
	#endif
		s << "<a href=accesslist target=mainFrame>access</a><br>\n";
		s << "<a href=info target=mainFrame>info</a><br>\n";
		if(conf.max_per_ip>0)
			s << "<a href=/connect_per_ip target=mainFrame>connect_per_ip</a><br>\n";
		s << "<a href=/connection target=mainFrame>connection</a><br>\n";
		s << "<a href=config?item=service target=mainFrame>config</a><br>\n";
		//rq->server->send("add a user:<form action=adduser method=get target=_blank><input name=user><input type=submit></form>");
		//rq->server->send("del a user:<form action=deluser method=get target=_blank><input name=user><input type=submit></form>");
		//rq->server->send("add a group:<form action=addgroup method=get target=_blank><input name=group><input type=submit></form>");
		//rq->server->send("del a group:<form action=delgroup method=get target=_blank><input name=group><input type=submit></form>");
#ifndef DISABLE_USER
	}
#endif
	s << "</body></html>";
	return sendHttp(s.str().c_str());

}
bool KHttpManage::sendMainFrame()
{
	stringstream s;
	s << "<p>   kingate(" << VER_ID << ") writed by king(khj99@tom.com), kingate homepage is <a href=\"http://sourceforge.net/projects/kingate/\" target=\"_blank\">http://sourceforge.net/projects/kingate/</a></p>";
 	#ifndef DISABLE_USER
	s << user_info.user->user.c_str() << " login from " << rq->server->get_remote_name() << "<br>total time(sec) " << user_info.user->total_time+(time(NULL)-user_info.login_time) << "<br>total send size:" << user_info.user->send_size/1024 << "kb <br>total recv size:" << user_info.user->recv_size/1024 << "kb";
	#endif
	return sendHttp(s.str().c_str());
}
bool KHttpManage::sendMainPage()
{
	stringstream s;
	/*	
	//sendHeader(200);
	s << "<html><head><title>kingate(" << VER_ID << ") manager</title></head><body>";
	#ifndef DISABLE_USER
	s << user_info.user->user.c_str() << " login from " << rq->server->get_remote_name() << " total time(sec) " << user_info.user->total_time+(time(NULL)-user_info.login_time) << " total send size:" << user_info.user->send_size/1024 << "kb total recv size:" << user_info.user->recv_size/1024 << "kb";
	s << "<br><a href=logout>logout</a><br>";
	s << "<a href=chanage_password_form>chanage password</a>\n";
	if(user_info.user->user=="root"){//it is the root user
		s << "<hr>";
	#endif
		s << "<h3>You are the root user. You can manage kingate now!</h3>";
	#ifndef DISABLE_USER
		s << "<a href=userlist target=_blank>users</a><br>";
		s << "<a href=loginlist target=_blank>login users</a><br>";
	#endif
		s << "<a href=accesslist target=_blank>access</a><br>\n";
		s << "<a href=info target=_blank>info</a><br>\n";
		s << "<a href=config?item=service target=_blank>config</a><br>\n";
		//rq->server->send("add a user:<form action=adduser method=get target=_blank><input name=user><input type=submit></form>");
		//rq->server->send("del a user:<form action=deluser method=get target=_blank><input name=user><input type=submit></form>");
		//rq->server->send("add a group:<form action=addgroup method=get target=_blank><input name=group><input type=submit></form>");
		//rq->server->send("del a group:<form action=delgroup method=get target=_blank><input name=group><input type=submit></form>");
#ifndef DISABLE_USER
	}
#endif
	s << "</body></html>";
	return sendHttp(s.str());
*/
	s << "<html><head><title>kingate manage</title></head><frameset rows=\"*\" cols=\"169,*\" framespacing=\"0\" frameborder=\"NO\" border=\"0\">";
	s << "  <frame src=\"/left_menu\" scrolling=\"NO\" noresize>";
	s << "  <frame src=\"/main\" name=\"mainFrame\"></frameset><noframes><body></body></noframes></html>";
	return sendHttp(s.str());
	//rq->server->send(s.str().c_str());
}
bool KHttpManage::sendRedirect(const char *newUrl)
{
	stringstream s;
	s << "HTTP/1.1 302 Found\r\nServer: kingate(" << VER_ID << ")\r\nLocation: " << newUrl << "\r\nConnection: ";
  	if(TEST(rq->flags,RQ_HAS_KEEP_CONNECTION))
                s << "keep-alive\r\n";
        else
                s << "close\r\n";
	s << "\r\n";
	if(rq->server->send(s.str().c_str())<=0)
		return false;
	return true;
}
bool KHttpManage::start(request *rq)
{

	this->rq=rq;
	int i=0;
//	this->rq->server->send("haha....");
//	return false;
	parseUrl((char *)rq->url.path);
	if(rq->meth==METH_POST){
		#define MAX_POST_SIZE	1024
		if(!rq->data){
			return false;
		}
		char *buffer=(char *)malloc(MAX_POST_SIZE+1);
		int buff_len=0;
		char *str=buffer;
		if(buffer==NULL)
			return false;		
		memset(buffer,0,MAX_POST_SIZE+1);
		buff_len=MIN(rq->data->used,MAX_POST_SIZE);	
		memcpy(buffer,rq->data->data,buff_len);
		str+=buff_len;
		//printf("buffer=%s\n",buffer);
		int remaining=MIN(rq->leave_to_read,MAX_POST_SIZE-buff_len);
		int length=0;		
		while(remaining > 0){
			length=rq->server->recv(str,remaining);
			if(length<=0){
				free(buffer);
				return false;
			}
			remaining -= length;
			str += length;		
		}
		rq->server->clear_recvq(4);
		str[length]=0;
		//printf("buffer=%s\n",buffer);
		parseUrlParam(buffer);
    		free(buffer);
	}
	//printf("url.path=%s\n",rq->url.path);	
	#ifndef DISABLE_USER
	if(strcmp(rq->url.path,"/login")==0){		
		if(m_user.CheckPasswd(getUrlValue("user").c_str(),getUrlValue("password").c_str())>0){
			m_user.Login(rq->server->get_remote_addr(),getUrlValue("user").c_str());
			goto continue_check;	
		}else{
	//		sendTest();
			return sendErrPage("username or password error");
	
		}
	}
	#endif
	if(strcmp(rq->url.path,"/test")==0){
		sendTest();
		return false;
	}
continue_check:
	#ifndef DISABLE_USER
	if(m_user.CheckLogin(rq->server->get_remote_addr(),&user_info)==0){
		return sendLogin();
		
	}
	if(strcmp(rq->url.path,"/logout")==0){
		m_user.Logout(rq->server->get_remote_addr());
		return sendErrPage("Lougout success\n",1);
		
	}
	if(strcmp(rq->url.path,"/chanage_password_form")==0){
		//sendHeader(200);
		return sendHttp("<html><body><form action=chanage_password method=post>old password:<input type=password name=old_password><br>new password:<input type=password name=new_password><br>retype new password:<input type=password name=re_new_password><br><input type=submit value=submit></form></body></html>");
		
	}
	if(strcmp(rq->url.path,"/chanage_password")==0){		
		if(m_user.CheckPasswd(user_info.user->user.c_str(),getUrlValue("old_password").c_str())<=0){
			return sendErrPage("your old password is wrong!");
			
		}
		string new_password=getUrlValue("new_password");
		if(new_password!=getUrlValue("re_new_password")){
			klog(ERR_LOG,"passwd=%s,repasswd=%s is not equale.\n",new_password.c_str(),getUrlValue("re_new_password").c_str());
			return sendErrPage("Two new password is not equale!");
			
		}
		if(new_password.length()<6||new_password.length()>32){
			return sendErrPage("new password length is too short or too long,password length 6-32");
			
		}
		if(!m_user.ChangePasswd(user_info.user->user.c_str(),new_password.c_str())){
			return sendErrPage("Chanage password failed, kingate happen unknow error!");
			
		}
		return sendErrPage("Chanage password success!");
		
	}
	#endif
	if(strcmp(rq->url.path,"/left_menu")==0){
		return sendLeftMenu();
	}
	if(strcmp(rq->url.path,"/main")==0){
		return sendMainFrame();
	}
root_operator:
	#ifndef DISABLE_USER
	if(!m_user.CheckGroup(user_info.user->uid,m_user.getRootGid()))
		goto done;
/*
	if(user_info.user->user!="root")
		goto done;
*/
	if(strcmp(rq->url.path,"/adduser")==0){
		string user=getUrlValue("user");
		if(user.length()>=MAXUSER){
			return sendErrPage("Add user error. User name length is too long.max length=64");
			
		}
		if(user.length()==0){
			return sendErrPage("Add user error. User name length is zero");
			
		}
		if(user=="all"){
			return sendErrPage("Add user error. User cann't named all");
			
		}
		if(m_user.NewUser(user.c_str())){
			return sendErrPage("Add user success.",1);
			
		}
		
		return sendErrPage("Add user unknow error,the user maybe exist.");
		
	}
	if(strcmp(rq->url.path,"/addgroup")==0){
		string err_msg;
		stringstream s;
		if(!m_user.AddGroup(getUrlValue("user").c_str(),getUrlValue("group").c_str(),err_msg)){
			return sendErrPage(err_msg.c_str(),1);
			
		}
		s << "Add user " << getUrlValue("user") << " to group " << getUrlValue("group") << " success!";
		return sendErrPage(s.str().c_str(),1);
		
	}
	if(strcmp(rq->url.path,"/deluser")==0){
		string user=getUrlValue("user");
		if(user=="root"){
			return sendErrPage("Your cann't delete the root user");
			
		}
		if(user.length()>=MAXUSER){
			return sendErrPage("Del user error. User name length is too long.max length=64");
			
		}
		if(user.length()==0){
			return sendErrPage("Del user error. User name length is zero");
			
		}
		if(m_user.DelUser(user.c_str())){
			return sendRedirect("/userlist");
			//sendErrPage("Del user success.");
			
		}
		return sendErrPage("Del user unknow error,the user maybe not exist.");
		
	}
	if(strcmp(rq->url.path,"/delgroup")==0){
		string user=getUrlValue("user");
		string err_msg;
		if(m_user.DelGroup(user.c_str(),getUrlValue("group").c_str(),err_msg)){
			return sendRedirect("/userlist");
			//sendErrPage("Del user success.");
			
		}
		return sendErrPage(err_msg.c_str());
		
	}
	if(strcmp(rq->url.path,"/userlist")==0){
		return userList();
		
	}
	if(strcmp(rq->url.path,"/loginlist")==0){
		return loginList();
		
	}
	if(strcmp(rq->url.path,"/setpasswordform")==0){
		stringstream s;
		string user=getUrlValue("user");
//		sendHeader(200);
		s << "<html><body>set user:" << user << " password:<form action=setpassword method=post><input type=hidden name=user value=" << user ;
		s << ">new password:<input type=password name=password><br>\nretype password:<input name=re_password type=password><br>\n<input type=submit value=submit>\n</form>\n</body>\n</html>";
		return sendHttp(s.str());	
	}
	if(strcmp(rq->url.path,"/setpassword")==0){
		string user=getUrlValue("user");
		string password=getUrlValue("password");
		if(password!=getUrlValue("re_password")){
			return sendErrPage("Two password is not equale!");
			
		}
		if(password.length()<6||password.length()>32){
			return sendErrPage("Password length is too short or too long,password length 6-32");
			
		}
		if(!m_user.ChangePasswd(user.c_str(),password.c_str())){
			return sendErrPage("Set password failed, kingate happen unknow error!");
			
		}
//		sendHeader(200);
		return sendHttp("<html><body><h1><font color=red>Set password success!</font></h1><a href='javascript:window.close();'>close</a></body></html>");
		
	}
	if(strcmp(rq->url.path,"/adduserform")==0){
//		sendHeader(200);
		return sendHttp("<html><title>Add new user</title><body>Input user name you want to add<br><form action=adduser method=get>user:<input name=user><input type=submit></form></body></html>");
		
	}
	if(strcmp(rq->url.path,"/addgroupform")==0){
		stringstream s;
		string user=getUrlValue("user");
		vector<string> allUser;
		vector<string> allGroup;		
		vector<string> otherGroup;
		bool have_one_group=false;
		int i,j;
		(m_user.getAllUserName()).swap(allUser);
		(m_user.getAllGroupName(user)).swap(allGroup);
		/*
		for(i=0;i<allUser.size();i++){
			cout << allUser[i] << endl;
		}
		cout << "group list:" << endl;
		for(j=0;j<allGroup.size();j++){
			cout << allGroup[j] << endl;
		}
		*/
		for(i=0;i<allUser.size();i++){
			have_one_group=false;
			if(allUser[i]==user)
				continue;
			for(j=0;j<allGroup.size();j++){
				if(allUser[i]==allGroup[j]){
					have_one_group=true;
					break;		
				}
				
			}
			if(!have_one_group){
				otherGroup.push_back(allUser[i]);
			}
		}

//		sendHeader(200);
		s << "<html><title>Add " << user << " to group</title><body>Add " << user << " to new group<form action=addgroup method=get>group:<input type=hidden name=user value=" << user << ">";
		if(otherGroup.size()==0){
			s << " <font color=red>" << user << " haven't other group to add</font>";
		}else{
			s << "<select name=group>\n";
			for(i=0;i<otherGroup.size();i++){
				s << "<option value='" << otherGroup[i] << "'>" << otherGroup[i] << "</option>\n";
				
			}
			s << "</select>&nbsp;&nbsp;<input type=submit value=submit>";
		}
		s << "</form></body></html>";
		return sendHttp(s.str());		
		
	}
	#endif
	if(strcmp(rq->url.path,"/accesslist")==0){
//		sendHeader(200);
		return sendHttp(conf.m_kfilter.ChainList());
		
	}
	if(strcmp(rq->url.path,"/accessaddform")==0){
		//sendHeader(200);
		stringstream s;
		string model=getUrlValue("model");
		s << "<html><head><title>add " << model << " access</title></head><body><form action=accessadd method=get><input type=hidden name=model value=" << model << "><table  border=1 cellspacing=0><tr><td>service|port</td><td><select name=service><option value='*' selected>*</option>";
		for(i=TOTAL_SERVICE-1;i>=0;i--){
			s << "<option value='" << service_name[i] << "'>" << service_name[i] << "</option>\n";
		}
		for(i=0;i<conf.redirect.size();i++){
	//	REDIRECT *tmp_redirect=conf.redirect;
	//	do{
	//		if(tmp_redirect==NULL)
	//			break;
			s << "<option value='" << conf.redirect[i].src_port << "'>" << conf.redirect[i].src_port << "</option>\n";
	//	}while( (tmp_redirect=tmp_redirect->next)!=NULL );
		}
		s << "</select>\n<input type=checkbox name=service_revers value=1>revers</td></tr>\n";
		#ifndef DISABLE_USER
		s << "<tr><td>group</td><td><input name=group value='";
		if(model=="allow")
			s << "all";
		else
			s << "*";
		s << "'></td></tr>\n";
		#endif
		s << "<tr><td>src_ip/src_mask</td><td><input name=src value='*'></td></tr>\n<tr><td>dst_ip/dst_mask:dst_port</td><td><input name=dst value='*'></td></tr>\n<tr><td>time(min hour mday month wday)</td><td><input name=time value='all'></td></tr></table><input type=submit value=submit></form></body><html>";
		return sendHttp(s.str());
		
	}
	if(strcmp(rq->url.path,"/accessadd")==0){
		stringstream base_rule;
		int model=DENY;
		string err_msg;
		if(getUrlValue("model")=="allow"){
			model=ALLOW;
		}
		if(getUrlValue("service_revers")=="1")
			base_rule << "!";
		base_rule << getUrlValue("service");
		#ifndef DISABLE_USER
		base_rule << " " << getUrlValue("group");
		#endif
		base_rule << " " << getUrlValue("src") << " " << getUrlValue("dst");
		string time_model=getUrlValue("time");
		err_msg=conf.m_kfilter.MakeChain(base_rule.str().c_str(),(time_model=="all"?NULL:time_model.c_str()),model);
		if(err_msg.size()>0){
			err_msg="Add access failed,"+err_msg;
			return sendErrPage(err_msg.c_str());
			
		}
		conf.m_kfilter.SaveConfig();
		return sendErrPage("Add access success",1);
				
		
	}
	if(strcmp(rq->url.path,"/accessdel")==0){
		int model=DENY;
		if(getUrlValue("model")=="allow")
			model=ALLOW;
		if(!conf.m_kfilter.DelChain(atoi(getUrlValue("id").c_str()),model)){
			return sendErrPage("Del access failed");
			
		}
		conf.m_kfilter.SaveConfig();
		return sendRedirect("/accesslist");
		
	}
	if(strcmp(rq->url.path,"/accesschangefirst")==0){
		conf.m_kfilter.changeFirst();
		conf.m_kfilter.SaveConfig();
		return sendRedirect("/accesslist");
		
	}
	if(strcmp(rq->url.path,"/config")==0){
		return config();
		
	}
	if(strcmp(rq->url.path,"/configsubmit")==0){
		return configsubmit();
	}
	if(strcmp(rq->url.path,"/configadd")==0){
		string file_name=conf.path;
		if(!m_config.AddValue(getUrlValue("name").c_str(),getUrlValue("value").c_str(),false,(strstr(getUrlValue("value").c_str()," ")==NULL?false:true),true)){
			return sendErrPage("Error while add config value");
			
		}

		file_name+="/etc/kingate.conf";
		//printf("id=%d\n",atoi(getUrlValue("id").c_str()));
		m_config.SaveFile(file_name.c_str());
		return sendRedirect("/config");
		

	}
	if(strcmp(rq->url.path,"/configeditform")==0){
		stringstream s;
		int id=atoi(getUrlValue("id").c_str());
		if(id<0 || id>m_config.getSize()){
			s << "edit config error!no this id,id=" << id;
			return sendErrPage(s.str().c_str());
			
		}
		s << "<html><head><title>edit " << m_config.GetName(id) << " value</title></head><body><form action=configedit method=get>Please input ";
		s << m_config.GetName(id) << " new value:<input type=hidden name=id value=" << id << "><input name=value value=\"";
		s << m_config.GetValue(id) << "\"><input type=submit value=submit></form></body></html>";
//		sendHeader(200);
		return sendHttp(s.str());
		
		
	}
	if(strcmp(rq->url.path,"/configedit")==0){
		string file_name=conf.path;
		if(!m_config.SetValue(atoi(getUrlValue("id").c_str()),getUrlValue("value").c_str())){
			return sendErrPage("Error while edit config value");
			
		}

		file_name+="/etc/kingate.conf";
		//printf("id=%d\n",atoi(getUrlValue("id").c_str()));
		m_config.SaveFile(file_name.c_str());
		return sendRedirect("/config");
		
	}
	if(strcmp(rq->url.path,"/configdel")==0){
		string file_name=conf.path;
		if(!m_config.delItem(atoi(getUrlValue("id").c_str()))){
			return sendErrPage("Error while delete config item");
			
		}
		file_name+="/etc/kingate.conf";
		//printf("id=%d\n",atoi(getUrlValue("id").c_str()));
		m_config.SaveFile(file_name.c_str());
		return sendRedirect("/config");
		
	}
	#ifndef DISABLE_USER
	if(strcmp(rq->url.path,"/killloginuser")==0){
		m_user.ForceLogout(getUrlValue("user").c_str());
		return sendRedirect("/loginlist");
	}
	#endif
	if(strcmp(rq->url.path,"/connect_per_ip")==0){
		return sendHttp(get_connect_per_ip().c_str());
	}
	if(strcasecmp(rq->url.path,"/connection")==0){
		int orderby=1;
		int desc=0;
		stringstream s;
		if(getUrlValue("orderby").size()>0){
			orderby=atoi(getUrlValue("orderby").c_str());
		}
		if(getUrlValue("desc")=="1")
			desc=1;
		s << "<html><body><h3>Connection Infomation</h3><table border=1><tr><td><a href=/connection?orderby=0";
		if(orderby==0){
			s << "&desc=" << (1-desc) ;
		}
		s << ">src_ip</a></td><td>service|port</td><td>dst_ip</td><td>dst_port</td><td><a href=/connection?orderby=1";
		if(orderby==1){
			s << "&desc=" << (1-desc) ;
		}
		s << ">connect time</a></td><td><a href=/connection?orderby=2";
		if(orderby==2){
                        s << "&desc=" << (1-desc) ;
                }
		s << ">title</a></td></tr>";
		s << getConnectionInfo(orderby,desc) ;
		s << "</table></body></html>";
		s << "<hr><center>Generated by kingate(" << VER_ID << ")</center>";
		return sendHttp(s.str().c_str());
		
	}
	if(strcmp(rq->url.path,"/info")==0){
		stringstream s;
//		sendHeader(200);
		unsigned long total_mem_size=0,total_disk_size=0;
		unsigned long total_run_time=time(NULL)-kingate_start_time;
		get_cache_size(&total_mem_size,&total_disk_size);
		s << "<html><head><title>kingate(" << VER_ID << ") infomation</title></head><body>";
		s << "<h3>Obj cache info</h3>";
		s << "total obj count:" << total_obj_count << "<br>";
		s << "total mem cache size:";
	        if(total_mem_size>1048576){
			s << total_mem_size/1048576 << "M";
		}else if(total_mem_size>1024){
			s << total_mem_size/1024 << "K";
		}else{
			s << total_mem_size;
		}
		s << "<br>total disk cache size:";
	        if(total_disk_size>1048576){
			s << total_disk_size/1048576 << "M";
		}else if(total_disk_size>1024){
			s << total_disk_size/1024 << "K";
		}else{
			s << total_disk_size;
		}
		s << "<h3>Kingate uptime</h3>kingate total running " ;
		if(total_run_time>=86400){
			s << total_run_time/86400 << " day,";
			total_run_time%=86400;
		}
		if(total_run_time>=3600){
			s << total_run_time/3600 << " hour,";
			total_run_time%=3600;
		}
		if(total_run_time>=60){
			s << total_run_time/60 << " min,";
			total_run_time%=60;
		} 
		s << total_run_time << " second.";
		s << "<h3>Thread Infomation</h3>";
		s << "<table><tr><td>working thread</td><td>" << total_thread << "</td></tr>\n";
		s << "<tr><td>free thread</td><td>" << m_thread.getFreeThread() << "</td></tr>\n";
		//s << "<tr><td>mem cache size</td><td>" << 
	//	s << "<tr><td>request queue thread</td><td>" << requestQThreadCount << "(qs:" << requestQ.size() << ")</td></tr>\n";
		s << "</table>";
	//	s << "<h3>Connection Infomation</h3><table border=1><tr><td>src_ip</td><td>service|port</td><td>dst_ip</td><td>dst_port</td><td>connect time</td><td>title</td></tr>";
		s << "<hr><center>Generated by kingate(" << VER_ID << ")</center>";
		s << "</body></html>";
		return sendHttp(s.str());
		
	}
done:
	return sendMainPage();
	
}
#endif
