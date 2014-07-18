#ifdef HAVE_CONFIG_H
#include "config.h"
#endif
#include "kingate.h"
#ifndef DISABLE_USER
#ifdef HAVE_SSTREAM
#include <sstream>
#else
#include "mysstream"
#endif
#include <time.h>
#include <assert.h>
#include<stdlib.h>


#include "do_config.h"
#include "log.h"
#include "md5.h"
#include "KUser.h"
#include "malloc_debug.h"

#ifndef _WIN32
#define USER_FILE	"/etc/kingate.user"
#define USER_TMP	"/etc/kingate.user.tmp"
#else
#define USER_FILE	"\\etc\\kingate.user"
#define USER_TMP	"\\etc\\kingate.user.tmp"
#endif
KUser m_user;
using namespace std;
bool split_user_info(const char *name,const char *value,UserInfo &m_user)
{
	m_user.uid=0;
	m_user.last_ip=0;
	m_user.last_time=0;
	m_user.total_time=0;
	m_user.send_size=0;
	m_user.recv_size=0;
	m_user.state=0;
	char gids[255];
	unsigned i_gid=0;
	char passwd[64];
	memset(gids,0,sizeof(gids));
	memset(passwd,0,sizeof(passwd));
	
	sscanf(value,"%64s%u%u%u%u%u%u%d%255s",passwd,&m_user.uid,&m_user.last_ip,&m_user.last_time,&m_user.total_time,&m_user.send_size,&m_user.recv_size,&m_user.state,gids);
	if(m_user.uid<=STARTUID)
		m_user.uid+=STARTUID;
	m_user.user=name;
	m_user.passwd=passwd;
	for(int i=0;i<strlen(gids);i++){
		if(gids[i]=='|'){
			i_gid=atoi(gids+i+1);
			if(i_gid<=STARTUID)
				i_gid+=STARTUID;
			m_user.gids.push_back(i_gid);
		}
	}					
	return true;
}
string make_user_info(UserInfo &m_user)
{
	stringstream s;
	s << m_user.passwd << " " << m_user.uid << " " << m_user.last_ip << " " << m_user.last_time << " " << m_user.total_time << " " << m_user.send_size << " " << m_user.recv_size << " " << m_user.state << " ";
	for(int i=0;i<m_user.gids.size();i++){
		s << "|" << m_user.gids[i];
	}
	//cout << s.str() << endl;
	return s.str();
}
KUser::KUser()
{
	handle=NULL;
	maxuid=0;
	root_gid=-1;
}
KUser::~KUser()
{
#ifdef HAVE_DLFCN_H
	if(handle){
		user_model.user_model_finit();
		dlclose(handle);
	}
#endif
//	Save();
}
bool KUser::LoadLoginUser()
{
		stringmap::iterator it;
		for(it=m_all_user.begin();it!=m_all_user.end();it++){
			if((*it).second->state==1){//set the user login
				Login((*it).second->last_ip,(*it).first.c_str());
			}
		}
		return true;
}
int KUser::getRootGid()
{
	return root_gid;
}
bool KUser::LoadUser()
{
	char tmp[255];
	char md5_passwd[100];
	memset(md5_passwd,0,sizeof(md5_passwd));
	if(strlen(conf.user_model)==0){
		UserInfo *m_user_info;
		KConfig m_user_file;
		memset(tmp,0,sizeof(tmp));
		snprintf(tmp,sizeof(tmp)-1,"%s%s",conf.path.c_str(),USER_FILE);
		if(!m_user_file.open(tmp)){
			maxuid=STARTUID+1;
			m_user_info=new UserInfo;
			//memset(m_user_info,0,sizeof(UserInfo));
			m_user_info->last_ip=0;
			m_user_info->last_time=0;
			m_user_info->uid=STARTUID+1;
			m_user_info->user="root";
			root_gid=m_user_info->uid;
			m_user_info->total_time=0;
			m_user_info->recv_size=0;
			m_user_info->send_size=0;
			m_user_info->state=0;
			MD5("kingate",md5_passwd);
		//	printf("kingate md5 =%s.\n",md5_passwd);
			m_user_info->passwd=md5_passwd;
			m_all_user.insert(pair<string,UserInfo *>(m_user_info->user,m_user_info));
			m_all_user2.insert(pair<unsigned,UserInfo *>(m_user_info->uid,m_user_info));
			return false;
		}
		const char *value;
		const char *name;
		int index=0;
		while((name=m_user_file.GetName(index++))!=NULL){
			if(strlen(name)==0)
				break;
			value=m_user_file.GetValue(name);
			if(strlen(value)>0){
				if(strcmp(name,"maxuid")==0){
					maxuid=atoi(value);
					if(maxuid<=STARTUID)
						maxuid+=STARTUID;
				}else{
					m_user_info=new UserInfo;
					split_user_info(name,value,*m_user_info);
					if(m_user_info->user=="root"){
						root_gid=m_user_info->uid;
					}
					m_all_user.insert(pair<string,UserInfo *>(m_user_info->user,m_user_info));
					m_all_user2.insert(pair<unsigned,UserInfo *>(m_user_info->uid,m_user_info));
				}
			}
		}
		LoadLoginUser();
		return true;
	}else{
#ifdef HAVE_DLFCN_H
		memset(tmp,0,sizeof(tmp));
		snprintf(tmp,sizeof(tmp)-1,"%s%s",conf.path.c_str(),conf.user_model);
		handle=dlopen(tmp,RTLD_NOW);
		if(handle==NULL){
			klog(ERR_LOG,"Error:kingate cann't load %s,errmsg=%s\n",conf.user_model,dlerror());
			exit(6);
		}
		if((user_model.new_user=(new_user_f)dlsym(handle,"new_user"))==NULL)
			goto err;
		if((user_model.del_user=(del_user_f)dlsym(handle,"del_user"))==NULL)
			goto err;
		if((user_model.change_passwd=(change_passwd_f)dlsym(handle,"change_passwd"))==NULL)
			goto err;
		if((user_model.check_passwd=(check_passwd_f)dlsym(handle,"check_passwd"))==NULL)
			goto err;
		if((user_model.get_user_id=(get_user_id_f)dlsym(handle,"get_user_id"))==NULL)
			goto err;
		if((user_model.get_user_name=(get_user_name_f)dlsym(handle,"get_user_name"))==NULL)
			goto err;
		if((user_model.user_model_init=(user_model_init_f)dlsym(handle,"user_model_init"))==NULL)
			goto err;
		if((user_model.user_model_finit=(user_model_finit_f)dlsym(handle,"user_model_finit"))==NULL)
			goto err;
		if(user_model.user_model_init())
			return true;
#endif
		return false;
	}
#ifdef HAVE_DLFCN_H
err:
	klog(ERR_LOG,"Error:kingate cann't load %s,errmsg=%s\n",conf.user_model,dlerror());
	dlclose(handle);
	handle=NULL;
	exit(6);
#endif
}
bool KUser::Login(unsigned long ip,const char *user)
{
	UserInfo m_userinfo;
	if(!LoadUserInfo(user,m_userinfo))
		return false;
	LoginUserInfo tmpuser;
	stringmap::iterator it2;
	KUser_int_map::iterator it;
	tmpuser.login_time=time(NULL);
	tmpuser.last_active_time=tmpuser.login_time;
	//tmpuser.user=user;
	m_root_lock.Lock();
	it2=m_all_user.find(user);
	if(it2==m_all_user.end()){
		m_root_lock.Unlock();
		return false;
	}
	(*it2).second->last_ip=ip;
	(*it2).second->state=1;//set the user login
	(*it2).second->last_time=tmpuser.login_time;
	tmpuser.user=(*it2).second;
	tmpuser.user_id=(*it2).second->uid;
	tmpuser.gids=&(*it2).second->gids;
	m_root_lock.Unlock();
	if(tmpuser.user_id==0){
		klog(ERR_LOG,"user %s user_id is 0,It may be a bug in %s:%d\n",user,__FILE__,__LINE__);
		return false;
	}
	m_lock.Lock();
	it=m_user.find(ip);
	if(it!=m_user.end())
		goto err;
	ForceLogout(user,false);
	m_user[ip]=tmpuser;
	m_lock.Unlock();
	char ips[18];
	make_ip(ip,ips);
	klog(RUN_LOG,"user %s(%d) login from %s success\n",user,tmpuser.user_id,ips);
	return true;
err:
	m_lock.Unlock();
	return false;
}
void KUser::UpdateSendRecvSize(unsigned long ip,int send_size,int recv_size,unsigned uid)
{
	assert(send_size>=0 && recv_size>=0);
	KUser_int_map::iterator it;
	if(uid>STARTUID){
		map<unsigned,UserInfo *>::iterator it2;
		m_root_lock.Lock();
		it2=m_all_user2.find(uid);
		if(it2!=m_all_user2.end()){
			(*it2).second->send_size+=send_size;
			(*it2).second->recv_size+=recv_size;
		}
		m_root_lock.Unlock();
		return;
	}
	m_lock.Lock();
	it=m_user.find(ip);
	if(it!=m_user.end()){
		(*it).second.user->send_size+=send_size;
		(*it).second.user->recv_size+=recv_size;
	}
	m_lock.Unlock();
}
int KUser::Logout(unsigned long ip)
{
	unsigned stay_time=0;
	KUser_int_map::iterator it;
	m_lock.Lock();
	it=m_user.find(ip);
	if(it==m_user.end()){
		m_lock.Unlock();
		return stay_time;
	}
	stay_time=time(NULL)-(*it).second.login_time;
	//m_user.total_time+=stay_time;
	(*it).second.user->total_time+=stay_time;
	(*it).second.user->state=0;//set the user logout
	klog(RUN_LOG,"user %s logout success\n",(*it).second.user->user.c_str());
	m_user.erase(it);
	m_lock.Unlock();	
	return stay_time;
}
unsigned KUser::CheckPasswd(const char *user,const char *passwd)
{
	unsigned uid=0;
	char md5_passwd[100];
	if(handle)
		return user_model.check_passwd(user,passwd);
//	printf("user=%s.passwd=%s.\n",user,passwd);
	stringmap::iterator it;
	m_root_lock.Lock();
	it=m_all_user.find(user);
	if(it==m_all_user.end()){
	//	printf("user don't exitst.\n");
		goto err;
	}
	MD5(passwd,md5_passwd);
//	printf("md5_passwd=%s,true md5 passwd=%s.\n",md5_passwd,(*it).second->passwd.c_str());
	if((*it).second->passwd==md5_passwd){
//		printf("passwd is right.\n");
		uid=(*it).second->uid;
	}
err:
//	printf("right passwd=%s.\n",(*it).second->passwd.c_str());
	m_root_lock.Unlock();
	return uid;
	/*
	const char *value=m_user_file.GetValue(user);
	if(value==NULL)
		return false;
	char right_passwd[64];
	sscanf(value,"%64s",right_passwd);
	if(right_passwd==NULL||strcmp(passwd,right_passwd)!=0||strcmp(right_passwd,"*")==0)
		return false;
	return true;
	*/
}
bool KUser::ChangePasswd(const char *user,const char *new_passwd)
{
	int uid=1;
//	char new_value[128];
//	const char *value;
	char md5_passwd[100];
	if(handle)
		return user_model.change_passwd(user,new_passwd);
	stringmap::iterator it;
	m_root_lock.Lock();
	it=m_all_user.find(user);
	if(it==m_all_user.end()){
		m_root_lock.Unlock();
		return false;
	}
//	printf("new_passwd=%s\n",new_passwd);
	MD5(new_passwd,md5_passwd);
	(*it).second->passwd=md5_passwd;
	m_root_lock.Unlock();
	return Save();
	/*
	value=m_user_file.GetValue(user);
	if(value==NULL)
		return false;
	sscanf(value,"%*s%d",&uid);
	snprintf(new_value,sizeof(new_value),"%s %d",new_passwd,uid);
	return m_user_file.SetValue(user,new_value,0,true);
	*/
}
bool KUser::NewUser(const char *user)
{
//	bool ret;
	if(handle)
		return user_model.new_user(user);
	stringmap::iterator it;
	UserInfo *m_user_info;
	m_root_lock.Lock();
	it=m_all_user.find(user);
	if(it!=m_all_user.end()){
		m_root_lock.Unlock();
		return false;
	}
	m_user_info=new UserInfo;
	m_user_info->user=user;
	m_user_info->passwd="*";
	m_user_info->uid=++maxuid;
	m_user_info->last_ip=0;
	m_user_info->last_time=0;
	m_user_info->total_time=0;
	m_user_info->send_size=0;
	m_user_info->recv_size=0;
	m_user_info->state=0;
	m_all_user.insert(make_pair<string,UserInfo *>(user,m_user_info));
	m_all_user2.insert(make_pair<unsigned,UserInfo *>(m_user_info->uid,m_user_info));
	m_root_lock.Unlock();
	return Save();
	/*
	m_lock.Lock();
	const char *value=m_user_file.GetValue("maxuid");
	m_user_file.print_all_item();
	int maxuid=1;
	char maxuid_s[5];
	char new_value[128];
	if(value==NULL){
//		printf("no maxuid.\n");
		m_user_file.AddValue("maxuid","1");
	}else{
		maxuid=atoi(value)+1;
		snprintf(maxuid_s,sizeof(maxuid_s),"%d",maxuid);
		m_user_file.SetValue("maxuid",maxuid_s);
	}
	if(m_user_file.GetValue(user)!=NULL){
		ret=true;
		goto done;
	}
	snprintf(new_value,sizeof(new_value),"* %d",maxuid);
	ret=m_user_file.AddValue(user,new_value,false,true);
done:
	m_lock.Unlock();
	return ret;
*/
}
bool KUser::DelUser(const char *user)
{
//	char new_value[16];
	int uid=0;
	if(handle)
		return user_model.del_user(user);
	stringmap::iterator it;
/*	if(CheckLogin(user))//the user is still login now
		return false;
*/
	ForceLogout(user);
	m_root_lock.Lock();
	it=m_all_user.find(user);
	if(it==m_all_user.end()){
		m_root_lock.Unlock();
		return false;
	}
	m_all_user2.erase(m_all_user2.find((*it).second->uid));
	delete (*it).second;
	m_all_user.erase(it);
	m_root_lock.Unlock();
	return Save();

}
bool KUser::Save()
{
	string tmp=conf.path;
	string user_file=conf.path;
	if(handle)
		return true;
	KConfig m_config;
	char maxuids[6];
	int i=0;
//	snprintf(tmp,sizeof(tmp),"%s%s",conf.path,USER_FILE);

	tmp+=USER_TMP;
	user_file+=USER_FILE;
	memset(maxuids,0,sizeof(maxuids));
	snprintf(maxuids,sizeof(maxuids)-1,"%d",maxuid);
	m_config.AddValue("maxuid",maxuids);
	stringmap::iterator it;
	m_root_lock.Lock();
	for(it=m_all_user.begin();it!=m_all_user.end();it++){
		i++;
		m_config.AddValue((*it).first.c_str(),make_user_info(*(*it).second).c_str(),false,true);
	}
	assert(i>0);
	if(i<=0)
		return false;
	bool ret=m_config.SaveFile(tmp.c_str());
	assert(ret);
	if(ret){
#ifdef _WIN32
		unlink(user_file.c_str());
#endif
		int result=rename(tmp.c_str(),user_file.c_str());
	}
	m_root_lock.Unlock();
	return ret;

}
void KUser::SaveAll()
{	

	unsigned stay_time=0;
	KUser_int_map::iterator it;
	int now_time=time(NULL);
	m_lock.Lock();
	for(it=m_user.begin();it!=m_user.end();it++){
		(*it).second.user->total_time+=now_time-(*it).second.login_time;
	}
	m_lock.Unlock();	
	Save();
}
void KUser::List()
{
	if(handle)
		return;
	//m_user_file.print_all_item();
}
std::vector<std::string> KUser::getAllGroupName(const string &user)
{
	std::vector<std::string> allGroup;
	string groupName;
	stringstream groupNames;
	stringmap::iterator it;
	int i=0;
	m_root_lock.Lock();
	it=m_all_user.find(user);
	if(it==m_all_user.end()){
		goto done;
	}
	for(i=0;i<(*it).second->gids.size();i++){	
		if(!GetUserName((*it).second->gids[i],groupName,false)){
			groupNames.str("");
			groupNames << "$" << (*it).second->gids[i] ;
			allGroup.push_back(groupNames.str());
			
		}else{
			allGroup.push_back(groupName);
		}	
	}		
done:
	m_root_lock.Unlock();
	return allGroup;
}
std::vector<std::string> KUser::getAllUserName()
{
	std::vector<std::string> allUser;
	stringmap::iterator it;
	m_root_lock.Lock();
	for(it=m_all_user.begin();it!=m_all_user.end();it++){
		allUser.push_back((*it).second->user);
	}
	m_root_lock.Unlock();
	return allUser;
}
std::string  KUser::ListUser()
{
	stringstream s;
//	std::vector<std::string> user_list;
//	const char *name;
//	const char *value;
	/*
	if(handle)
		return user_list;
*/	SaveAll();
	s << "<table border=1><tr><td>operator</td><td>user</td><td>last from</td><td>last time</td><td>total time</td><td>total send size</td><td>total recv size</td><td>in groups</td></tr>\n";
	stringmap::iterator it;
	string name;
	m_root_lock.Lock();
	for(it=m_all_user.begin();it!=m_all_user.end();it++){
		char buf[81];		
		if((*it).second->last_time>0){
			CTIME_R((time_t *)&(*it).second->last_time,buf,sizeof(buf));
			buf[24]=0;
		}else
			strcpy(buf,"never");
		s << "<tr><td><a href='javascript:if(confirm(\"are you sure to delete user:" << (*it).first << "\")) {window.location=\"deluser?user=" << (*it).first << "\"}'>del</a> <a href=\"javascript:show('setpasswordform?user=" << (*it).first ;
		s << "');\">password</a> <a href=\"javascript:show('addgroupform?user=" << (*it).first << "');\">add group</a></td><td>" << (*it).first << "</td><td>" << ((*it).second->last_ip>0?make_ip((*it).second->last_ip):"never") << "</td><td>" << buf << "</td><td>" << (*it).second->total_time << "</td><td>" << (*it).second->send_size/1024 << "kb</td><td>" << (*it).second->recv_size/1024 << "kb</td><td>";
		s << (*it).first;		
		for(int i=0;i<(*it).second->gids.size();i++){
			if(!GetUserName((*it).second->gids[i],name,false)){
				stringstream tmp;
				tmp << "$" << (*it).second->gids[i];
				name=tmp.str();
			}
			s << "<br>" << name << " <a href=delgroup?user=" << (*it).first << "&group=" << name << ">del</a>";

		}
		s << "</td></tr>";
		//user_list.push_back((*it).first);
	}
	m_root_lock.Unlock();
	s << "</table>";
	return s.str();
	/*
	for(int i=0;;i++){
		name=m_user_file.GetName(i);
		if(name==NULL)
			return user_list;
		value=m_user_file.GetValue(name);
		if(strncmp(value,"*",1)!=0 && strcmp(name,"maxuid")!=0)
			user_list.push_back(name);
	}
	*/

}
std::string KUser::ListLoginUser()
{
	stringstream s;
	int now_time=time(NULL);
//	std::vector<std::string> user_list;
	s << "<table><tr><td></td><td>user</td><td>from</td><td>stay time(sec)</td><td>inactive time(sec)</td></tr>\n";
	KUser_int_map::iterator it;
	m_lock.Lock();
	for(it=m_user.begin();it!=m_user.end();it++){
		s << "<tr><td><a href=/killloginuser?user=" << (*it).second.user->user << ">kill</a></td><td>" << (*it).second.user->user << "</td><td>" << make_ip((*it).first) << "</td><td>" << (now_time-(*it).second.login_time) << "</td><td>" << (now_time-(*it).second.last_active_time) << "</td></tr>\n";
	}
//		user_list.push_back((*it).second.user);
	m_lock.Unlock();
	s << "</table>";
	return s.str();
}
bool KUser::CheckUser(unsigned long ip,unsigned gid)
{
	KUser_int_map::iterator it;
	m_lock.Lock();
	it=m_user.find(ip);
	if(it!=m_user.end()){
		if(gid==ALLUSER||gid==(*it).second.user_id){
	//		printf("gid==0 or gid == loginid\n");
			goto done;
		}
		for(int i=0;i<(*it).second.gids->size();i++){
			if(gid==(*it).second.gids->operator[](i)){

	//			printf("loginid in group gid.\n");
				goto done;
			}
		}
	}
	m_lock.Unlock();
	return false;
done:
	(*it).second.last_active_time=time(NULL);
	m_lock.Unlock();
	return true;
}
void KUser::FlushLoginUser()
{
	if(conf.user_time_out<=0)
		return;
	int now_time=time(NULL);
	KUser_int_map::iterator it;
	m_lock.Lock();
	for(it=m_user.begin();it!=m_user.end();){
		if(now_time-(*it).second.last_active_time>conf.user_time_out){
	#ifndef _WIN32
			(*it).second.user->state=0;
			m_user.erase(it);
			it++;
	#else
			it=m_user.erase(it);
	#endif
		}else{
			it++;
		}
	}
	m_lock.Unlock();
}
unsigned KUser::CheckLogin(unsigned long ip,LoginUserInfo *user_info)
{	
	unsigned uid=0;
	KUser_int_map::iterator it;
	m_lock.Lock();
	it=m_user.find(ip);
	if(it!=m_user.end()){
		uid=(*it).second.user_id;
		if(user_info!=NULL){
			//memcpy(user_info,&(*it).second,sizeof(LoginUserInfo));
			user_info->user_id=uid;
			user_info->user=(*it).second.user;
			user_info->login_time=(*it).second.login_time;
			//user_info->last_ip=(*it).second.last_ip;
		}
	}
	m_lock.Unlock();
	return uid;


}
bool KUser::CheckGroup(unsigned uid,unsigned gid)
{
	int i;
//	printf("uid=%d,gid=%d\n",uid,gid);
	if((uid==gid) || gid==ALLUSER)
		return true;
	uintmap::iterator it,it2;
	m_root_lock.Lock();
	it=m_all_user2.find(uid);
	if(it==m_all_user2.end()){
//		printf("uid is err.\n");
		goto err;
	}
	/*
	it2=m_all_user2.find(uid);
	if(it2==m_all_user2.end()){
		printf("uid is err.\n");
		goto err;
	}
*/
	for(i=0;i<(*it).second->gids.size();i++){
		if(gid==(*it).second->gids[i])
			goto done;
	}	
err:
	m_root_lock.Unlock();	
	//printf("uid %d isn't in gid %d\n",uid,gid);
	return false;
done:
	m_root_lock.Unlock();
	//printf("uid %d is in gid %d\n",uid,gid);
	return true;
	
}
bool KUser::ForceLogout(const char *user,bool use_lock)
{
	KUser_int_map::iterator it;
	if(use_lock)
		m_lock.Lock();
	for(it=m_user.begin();it!=m_user.end();it++){
		if((*it).second.user->user==user){
			(*it).second.user->state=0;
			klog(RUN_LOG,"Force logou user %s success\n",(*it).second.user->user.c_str());
			m_user.erase(it);
			m_lock.Unlock();
			return true;
		}
	}
	if(use_lock)
		m_lock.Unlock();
	return false;
}
bool KUser::CheckLogin(const char *user)
{
	KUser_int_map::iterator it;
	m_lock.Lock();
	for(it=m_user.begin();it!=m_user.end();it++){
		if((*it).second.user->user==user){
			m_lock.Unlock();
			return true;
		}
	}
	m_lock.Unlock();
	return false;
}
unsigned KUser::GetUserID(const char *user)
{
	unsigned uid=0;
//	printf("user=%s.\n",user);
	if(handle)
		return user_model.get_user_id(user);
	stringmap::iterator it;
	m_root_lock.Lock();
	it=m_all_user.find(user);
	if(it!=m_all_user.end()){
		uid=(*it).second->uid;
	}
	m_root_lock.Unlock();
	return uid;
	/*
	const char *value=m_user_file.GetValue(user);
//	printf("value=%s.\n",value);
	if(value==NULL)
		return 0;
	sscanf(value,"%*s%d",&uid);
	return uid;
	*/
}
bool KUser::GetUserName(unsigned uid,string &name,bool use_lock)
{
	if(handle){
		char user[64];
		int ret=user_model.get_user_name(uid,user,sizeof(user));
		if(ret<=0)
			return false;
		name=user;
		return true;
	}
	stringmap::iterator it;
	if(use_lock)
		m_root_lock.Lock();
	for(it=m_all_user.begin();it!=m_all_user.end();it++){
		if((*it).second->uid==uid){
			name=(*it).second->user;
			m_root_lock.Unlock();
			return true;
		}
	}
	if(use_lock)
		m_root_lock.Unlock();
	return false;
}
bool KUser::LoadUserInfo(const char *user,UserInfo &m_userinfo)
{
	stringmap::iterator it;
	m_root_lock.Lock();
	it=m_all_user.find(user);
	if(it==m_all_user.end()){
		m_root_lock.Unlock();
		return false;
	}
	m_userinfo=*(*it).second;
	m_root_lock.Unlock();
	return true;
}
bool KUser::AddGroup(const char *user,const char *group,string &errmsg)
{
	unsigned gid=GetUserID(group);
	int i;
	if(gid==0){
		errmsg="group name is err.";
		return false;
	}
/*	UserInfo m_userinfo;
	if(!LoadUserInfo(user,m_userinfo)){
		errmsg="Load user info failed.";
		return false;
	}
*/	stringmap::iterator it;
	m_root_lock.Lock();
	it=m_all_user.find(user);
	if(it==m_all_user.end()){
		errmsg="Load user failed";
		goto err;
	}
	if(gid==(*it).second->uid){
		errmsg="user is already in group.";
		goto err;
	}
	for(i=0;i<(*it).second->gids.size();i++){
		if(gid==(*it).second->gids[i]){
		//	m_root_lock.Unlock();
			errmsg="user is already in group.";
		//	return false;
			goto err;
		}
	}
	(*it).second->gids.push_back(gid);
	m_root_lock.Unlock();
	errmsg="err while save user file";
	return Save();
err:
	m_root_lock.Unlock();
	return false;
}
bool KUser::DelGroup(const char *user,const char *group,string &errmsg)
{
	unsigned gid=0;
	stringstream s;
	if(group[0]=='$'){
		gid=atoi(group+1);
	}else{
		gid=GetUserID(group);
	}	
	if(gid==0){
		s << "no group name " << group;
		errmsg=s.str();
		return false;
	}
	/*
	UserInfo m_userinfo;
	if(!LoadUserInfo(user,m_userinfo)){
		errmsg="Load user info failed.";
		return false;
	}
	*/
	errmsg="user is already not in group.";
	vector<unsigned>::iterator it;
	stringmap::iterator it2;
	m_root_lock.Lock();
	it2=m_all_user.find(user);
	if(it2==m_all_user.end()){
		errmsg="load user failed";
		goto err;
	}
	for(it=(*it2).second->gids.begin();it!=(*it2).second->gids.end();it++){
		if(gid==(*it)){
			(*it2).second->gids.erase(it);
			m_root_lock.Unlock();
			errmsg="err while save user file";
			return Save();
		}
	}
err:
	if(gid==(*it2).second->uid)
		errmsg="cannt del user in own group.";
	m_root_lock.Unlock();
	return false;
}
#endif
