#include "KRWLock.h"
#include <stdio.h>
#include <string>
#include <time.h>
#ifndef _WIN32
#ifndef HAVE_ULONG
typedef unsigned long u_long;
#endif
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#endif
#ifdef HAVE_CONFIG_H
#include "config.h"
#endif
#include "kingate.h"
#ifdef HAVE_SSTREAM
#include <sstream>
#else
#include "mysstream"
#endif
#include "KFilter.h"
#include "do_config.h"
#include "KUser.h"
#include "malloc_debug.h"
#include "log.h"
using namespace std;
static KRWLock m_lock;
#define DENY_EXPIRE_TIME 86400
unsigned ConvertIP(const char *ip)
{
	if(strcasecmp(ip,"localhost")==0)
		return 1;
	return inet_addr(ip);
}
string make_ip(unsigned long ip,bool mask)
{
	struct in_addr s;
	char ips[18];
	string ips_ret;
	memset(ips,0,sizeof(ips));
	ips[0]='/';
	s.s_addr=ip;
	if(ip==0)
		return "*";
	if(ip==~0) 
		return "";
	if(ip==1)
		return "localhost";
	m_make_ip_lock.Lock();
	strncpy(ips+1,inet_ntoa(s),16);
	m_make_ip_lock.Unlock();
	if(mask)
		ips_ret=ips;
	else
		ips_ret=ips+1;
	return ips_ret;
}

string get_service_to_name(int port)
{

	if(port==0)
		return "*";
	if(port==conf.port[HTTP])
		return "http";
	if(port==conf.port[FTP])
		return "ftp";
	if(port==conf.port[SOCKS])
		return "socks";
	if(port==conf.port[MMS])
		return "mms";
	if(port==conf.port[RTSP])
		return "rtsp";
	if(port==conf.port[TELNET])
		return "telnet";
	if(port==conf.port[POP3])
		return "pop3";
	if(port==conf.port[SMTP])
		return "smtp";
	if(port==conf.port[MANAGE])
		return "manage";
	stringstream port_str;
	port_str << port;
	return port_str.str();
/*
	memset(port_str,0,sizeof(port_str));
	snprintf(port_str,sizeof(port_str)-1,"%d",port);
	return port_str;
*/
}
string chain2str(CHAIN &it,bool useHtml=true)
{
	//static char name[512];
	string user;
	stringstream s;
	bool ret;
	#ifndef DISABLE_USER
	if(it.user_id!=0)
		ret=m_user.GetUserName(it.user_id,user);
	#endif
	if(useHtml)
		s<< "<td>";
	s << (it.service_revers?"!":"") << get_service_to_name(it.service_port) << (useHtml?"</td><td>":" ");
	#ifndef DISABLE_USER
	s<< (it.user_revers?"!":"") << (it.user_id==ALLIP?"*":(it.user_id==ALLUSER?"all":(ret?user.c_str():"erruser"))) << (useHtml?"</td><td>":" ");
	#endif
	s<< (it.src.ip_revers?"!":"") << make_ip(it.src.ip);
	s<< make_ip(it.src.mask,true) << (useHtml?"</td><td>":" ");
	s<< (it.dst.ip_revers?"!":"") << make_ip(it.dst.ip) ;
	s<< make_ip(it.dst.mask,true);
	if(it.dst.port!=0){
		s << ":" << it.dst.port;
	}
	if(useHtml)
		s << "</td>";
	return s.str();
}
unsigned char_count(const char *str,char m_char)
{
	unsigned ret=0;
	for(int i=0;i<strlen(str);i++)
		if(str[i]==m_char)
			ret++;
	return ret;
}
bool MatchIPModel(IP_MODEL &m_ip,unsigned ip,short port,int model)
{
	return	(
	        	( 
				 (m_ip.ip==0)  ||
			       	 (ip==0) || 
		     		 ( (m_ip.ip==(ip & m_ip.mask)) !=  m_ip.ip_revers)
		       	)
	       	&& 
	     		(
			 	(m_ip.port==0) || 
				( port==0) ||
			 	((m_ip.port==port) != m_ip.port_revers)
	       		)
	       	)  ;
//	return	(((m_ip.ip==0)||((model==DENY)?(ip!=0):true) && ((m_ip.ip==(ip & m_ip.mask))!=m_ip.ip_revers)) && ((m_ip.port==0) || ( (model==DENY)?(port!=0):true) && (m_ip.port==port!=m_ip.port_revers) ) )  ;
}
bool AddIPModel(const char *ip_model,IP_MODEL &m_ip)
{
	char tmp[18];
	char mask[18];
	unsigned intmask;
	int point=0;
	unsigned default_mask=0xffffffff;
	unsigned dot_mask[4]={0xff000000,0xffff0000,0xffffff00,0xffffffff};
	if(sscanf(ip_model,"%17[^/]/%17[^:]:%d",tmp,mask,&m_ip.port)>1){
//		printf("have mask mask=%s.\n",mask);
		if(strlen(mask)>4){
			m_ip.mask=ConvertIP(mask);
		}else{
			m_ip.mask=ntohl(((default_mask>>(32-atoi(mask)))<<(32-atoi(mask))));
		}
	}else{
		sscanf(ip_model,"%17[^:]:%d",tmp,&m_ip.port);
		if(tmp[strlen(tmp)-1]=='.'){//it is a network
			unsigned dot_count=char_count(tmp,'.');
			if(dot_count<1 || dot_count>4){
				klog(ERR_LOG,"ip=%s is error.\n",tmp);
				return false;
			}
			snprintf(tmp+strlen(tmp),sizeof(tmp),"0");
			for(int i=1;i<4-dot_count;i++){
				snprintf(tmp+strlen(tmp),sizeof(tmp),".0");
			}
			m_ip.mask=ntohl(dot_mask[dot_count-1]);
				
		}else{
			m_ip.mask=default_mask;
		}
	}
	if(tmp[0]=='!'){
		m_ip.ip_revers=true;
		point=1;
	}
	if(strncmp(tmp+point,"*",1)==0)
		m_ip.ip=0;
	else
		m_ip.ip=ConvertIP(tmp+point);
	m_ip.ip=(m_ip.ip & m_ip.mask);
	return true;
//	*/
}
KFilter::KFilter()
{
//	memset(access_file,0,sizeof(access_file));
}
KFilter::~KFilter()
{
}
int KFilter::Check(STATE &m_state)
{
	int second=1-first;
	if(CheckChain(m_chains[first],m_state,first)){
//		printf("It in first chain.\n");
		return first;
	}
	if(CheckChain(m_chains[second],m_state,second)){
//		printf("It in second chain.\n");
		return second;
	}
//	printf("It in none chain.\n");
	return first;
	

}
bool KFilter::CheckChain(CHAINLIST_T &m_chain,STATE &m_state,int model)
{
	CHAINLIST_T::iterator it;
	unsigned uid=0;
	int i=0;
	m_lock.RLock();
	for(it=m_chain.begin();it!=m_chain.end();it++,i++){
		if(
		(((*it).service_port==m_state.service_port||(*it).service_port==0)!=(*it).service_revers )  &&							//check service match
		 MatchIPModel((*it).src,m_state.src_ip,0,model) &&	//check src match
      		 MatchIPModel((*it).dst,m_state.dst_ip,m_state.dst_port,model) &&	//check dst match
      		(*it).m_time.Check() //check the time match
		 ){
			#ifndef DISABLE_USER
			if((*it).user_id==ALLIP)
				goto match;
			if(m_state.uid==DELAYUSER){
				if(model==DENY){
					if(((*it).user_id==ALLUSER) && !(*it).user_revers)
						goto match;
					continue;
				}
				goto match;
			}
			if(m_state.uid>STARTUID){
				if(m_user.CheckGroup(m_state.uid,(*it).user_id)!=(*it).user_revers)
					goto match;
			}else{
			 	if(m_user.CheckUser(m_state.src_ip,(*it).user_id)!=(*it).user_revers)
					goto match;
			}
			#else
			goto match;
			#endif
		}

	}
	m_lock.Unlock();
	return false;
match:
	m_lock.Unlock();
	return true;
}
bool KFilter::AddChain(int model,CHAIN &m_chain)
{
	if(model<0 || model>1)
		return false;
//	m_chains[model].push_back(m_chain);
	return true;
}
bool KFilter::DelChain(int index,int model)
{
	if(model!=DENY && model!=ALLOW)
		return false;
	if(index<0 || index>=m_chains[model].size())
		return false;
	m_chains[model].erase(m_chains[model].begin()+index);
	return true;
}
string KFilter::MakeChain(const char *str,const char *time_str,int model,int expire_time)
{
	
	char src_ip[35];
	char dst_ip[35];
	char user[17];
	int point=0;
	unsigned default_mask=0xffffffff;
	unsigned dst_port;
	char service[8];
	if(model!=DENY && model!=ALLOW){
		return "model is error.";
	}
	CHAIN m_chain;
	m_chain.expire_time=expire_time;
//	memset(&m_chain,0,sizeof(m_chain));
//	m_chain.allip=false;
	if(!m_chain.m_time.Set(time_str)){
	//	fprintf(stderr,"time format is error!");
		return "time format is error.";		
	}
//	printf("time_str=%s\n",m_chain.m_time.GetTime());
//	m_chain.m_time.Show();
	//printf("time_str=%s.\n",time_str);
	#ifndef DISABLE_USER
	sscanf(str,"%7s%16s%35s%35s",service,user,src_ip,dst_ip); 
	#else
	if(sscanf(str,"%7s%16s%35s%35s",service,user,src_ip,dst_ip)!=4)
		sscanf(str,"%7s%35s%35s",service,src_ip,dst_ip);
	#endif
	AddIPModel(src_ip,m_chain.src);
	AddIPModel(dst_ip,m_chain.dst);
	if(service[0]=='!'){
		m_chain.service_revers=true;
		point=1;
	}
	if(strncmp(service+point,"*",1)==0){
		m_chain.service_port=0;
	}else if(strncmp(service+point,"http",4)==0){
		m_chain.service_port=conf.port[HTTP];
//		printf("service_port=%d\n",conf.port[HTTP]);
	}else if(strncmp(service+point,"ftp",3)==0){
		m_chain.service_port=conf.port[FTP];
	}else if(strncmp(service+point,"rtsp",4)==0){
		m_chain.service_port=conf.port[RTSP];
	}else if(strncmp(service+point,"socks",4)==0){
		m_chain.service_port=conf.port[SOCKS];
	}else if(strncmp(service+point,"pop3",4)==0){
		m_chain.service_port=conf.port[POP3];
	}else if(strncmp(service+point,"smtp",4)==0){
		m_chain.service_port=conf.port[SMTP];
	}else if(strncmp(service+point,"telnet",5)==0){
		m_chain.service_port=conf.port[TELNET];
	}else if(strncmp(service+point,"mms",3)==0){
		m_chain.service_port=conf.port[MMS];
	}else if(strncmp(service+point,"manage",6)==0){
		m_chain.service_port=conf.port[MANAGE];
	}else{
		m_chain.service_port=atoi(service+point);
		if(m_chain.service_port==0){
			return "service name is error.";
		}
	}
	point=0;
#ifndef DISABLE_USER
	if(user[0]=='*'){
		m_chain.user_id=0;
	}else{
		if(user[0]=='!'){
			m_chain.user_revers=true;
			point=1;
		}
		if(strcmp(user+point,"all")==0){
			m_chain.user_id=1;
		}else{
			m_chain.user_id=m_user.GetUserID(user+point);
			if(m_chain.user_id<=STARTUID){
				return "group is not error.";
			}
		}
	}
#endif
	m_lock.WLock();
	m_chains[model].push_back(m_chain);
	m_lock.Unlock();
	return "";
}
void KFilter::changeFirst()
{
	first=1-first;
}
void KFilter::PrintFilter()
{
	if(first==DENY)
		printf("first chain is deny.\n");
	else
		printf("first chain is allow.\n");
	for(int i=0;i<2;i++){
		if(i==DENY)
			printf("deny chain.\n");
		else
			printf("allow chains.\n");
		CHAINLIST_T::iterator it;
		for(it=m_chains[i].begin();it!=m_chains[i].end();it++){
			printf("src_ip=%x,src_ip_mask=%x,service_port=%d,dst_ip=%x,dst_mask=%x,dst_port=%d.\n",
				(*it).src.ip,(*it).src.mask,(*it).service_port,(*it).dst.ip,(*it).dst.mask,(*it).dst.port);
		}
	}
}
bool KFilter::LoadConfig(const char *file_name)
{
	const char *name,*value;
//	char tmp[255];
	int model=DENY;
	int index=0;
	int model_index=0;
	
	KConfig m_config;
//	snprintf(access_file,sizeof(access_file),"%s%s",conf.path,file_name);
	access_file=file_name;
	if(m_config.open(access_file.c_str())<=0){
		klog(ERR_LOG,"warning kingate cannot open access file %s.\n",file_name);
		return 0;
	}
//	strncpy(access_file,tmp,sizeof(access_file));
//	printf("tmp=%s.\n",tmp);
//	m_config.print_all_item();
	while((name=m_config.GetName(index))!=NULL){
		if(strlen(name)<=0)
			break;
	//	printf("name=%s.\n",name);
		if(strcmp(name,"model")==0){
			value=m_config.GetValue(index);
	//		printf("value=%s.\n",value);
			if(strlen(value)==0){
				klog(ERR_LOG,"warning model have no name in access file %s.\n",file_name);
				model=DENY;
			} else if(strcmp(value,"allow")==0){
				model=ALLOW;
			} else {
				model=DENY;
			}
			goto next_index;
		}
		if(strcmp(name,"first")==0){
			value=m_config.GetValue(name);
			if(strlen(value)==0){
				first=DENY;
			}else if(strcmp(value,"allow")==0){
				first=ALLOW;
			}else{
				first=DENY;
			}
			goto next_index;
		}
		value=m_config.GetValue(index);
		MakeChain(name,value,model);
	next_index:
		index++;
	}
//	PrintFilter();
	return 1;
}
bool KFilter::SaveConfig()
{
	KConfig m_config;
//	char name[255];
	if(first==DENY){
		m_config.AddValue("first","deny");
	}else{
		m_config.AddValue("first","allow");
	}
	CHAINLIST_T::iterator it;
	m_lock.RLock();
	for(int i=0;i<2;i++){
		if(i==DENY)
			m_config.AddValue("model","deny");
		else
			m_config.AddValue("model","allow");
		for(it=m_chains[i].begin();it!=m_chains[i].end();it++){
			m_config.AddValue(chain2str((*it),false).c_str(),(*it).m_time.GetTime(),true,strlen((*it).m_time.GetTime())==0?false:true);
		}
	}
	m_lock.Unlock();
	m_config.SaveFile(access_file.c_str());
	return true;
}
std::string KFilter::ChainList()
{

	stringstream s;
	std::vector<std::string> m_chain_list;
	char name[255];
	s << "<html><body><script language=javascript>function show(url) { window.open(url,'','height=210,width=450,resize=no,scrollbars=no,toolsbar=no,top=200,left=200');}</script>first access is:";

	if(first==DENY)
		s << "deny";
	else
		s << "allow";
		
	s << " <input type=button Onclick=\"if(confirm('are your sure to change the first access')){window.location='accesschangefirst';}\" value='change the first access'><hr><br>";
	int j=0;
	CHAINLIST_T::iterator it;
	const char *time_str;
	m_lock.RLock();
	string model;
	int f=first;
	for(int i=0;i<2;i++){	
		if(i==0)
			f=first;
		else 
			f=1-first;
		if(f==DENY)
			model="deny";
		else
			model="allow";
		s << model << " access list:<a href=\"javascript:show('accessaddform?model=" << model << "');\">Add " << model << " access</a><br><br>";
		s << "<table border=1 cellspacing=0><tr><td>operator</td><td>index</td><td>service|port</td>";
		#ifndef DISABLE_USER
		s << "<td>group</td>";
		#endif
		s << "<td>src_ip/src_mask</td><td>dst_ip/dst_mask:dst_port</td> <td>time(min hour mday month wday)</td></tr>\n";		
		for(j=0,it=m_chains[f].begin();it!=m_chains[f].end();j++,it++){
			s << "<tr><td><a href=\"javascript:if(confirm('Are you sure to del " << model;
			s << " access index=" << j << "')){ window.location='accessdel?id=" << j << "&model=" << model << "';}\">del</a></td><td>" << j << "</td>" << chain2str((*it)) << "<td>";
		      	time_str=(*it).m_time.GetTime();
			if(strlen(time_str)!=0)
				s << time_str;
			else
				s << "all";
			s << "</td></tr>\n";
		}
		s << "</table><hr><br>\n";
	}
	s << "</body></html>";
	m_lock.Unlock();
	return s.str();
	

}
void KFilter::checkExpireChain()
{
	long now_time=time(NULL);
	CHAINLIST_T::iterator it;
	m_lock.WLock();
	for(int i=0;i<2;i++){
		for(it=m_chains[i].begin();it!=m_chains[i].end();){
			if((*it).expire_time>0 && (*it).expire_time<now_time){
				it=m_chains[i].erase(it);				
			}else{
				it++;
			}	
		}
	}
	m_lock.Unlock();
}
