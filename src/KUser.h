#ifndef KUser_h_lskjdflsdfsdf87sdf978sd9f87sdf76dfishdfh
#define KUser_h_lskjdflsdfsdf87sdf978sd9f87sdf76dfishdfh
#ifdef HAVE_CONFIG_H
#include "config.h"
#endif
#ifndef MAXUSER
#define MAXUSER 64
#endif
#define 	ALLIP		0
#define		ALLUSER		1
#define		DELAYUSER	2
#define		STARTUID	100
#ifndef DISABLE_USER
#include <map>
#include <string>
#include <vector>
#ifdef HAVE_DLFCN_H
#include <dlfcn.h>
#endif
#include "KMutex.h"
#include "KConfig.h"
#include "user_model.h"
struct UserInfo
{
	std::string user;
	std::string passwd;
	unsigned uid;
	unsigned last_ip;
	unsigned last_time;
	unsigned total_time;
	unsigned send_size;
	unsigned recv_size;
	unsigned state;
	std::vector<unsigned> gids;
};
struct LoginUserInfo
{
	unsigned login_time;
	unsigned user_id;
	unsigned last_active_time;
	UserInfo *user;
	std::vector<unsigned> * gids;
};

struct user_model_fuction
{
	new_user_f new_user;
	del_user_f del_user;
	change_passwd_f change_passwd;
	check_passwd_f check_passwd;
	get_user_id_f get_user_id;
	get_user_name_f get_user_name;
	user_model_init_f user_model_init;
	user_model_finit_f user_model_finit;

};
typedef	 std::map<std::string,UserInfo *> stringmap;
typedef  std::map<unsigned,UserInfo *> uintmap;
typedef  std::map<unsigned long ,LoginUserInfo> KUser_int_map;
class KUser
{
public:
	KUser();
	~KUser();
	bool Login(unsigned long ip,const char *user);
	int Logout(unsigned long ip);
	bool LoadUser();
	unsigned CheckPasswd(const char *user,const char *passwd);
	bool ChangePasswd(const char *user,const char *new_passwd);
	bool Save();
	void SaveAll();
	void UpdateSendRecvSize(unsigned long ip,int send_size,int recv_size,unsigned uid=0);
	bool NewUser(const char *user);
	bool DelUser(const char *user);
	void List();
	void FlushLoginUser();
	int getRootGid();
	unsigned CheckLogin(unsigned long ip,LoginUserInfo *user_info=NULL);
	bool CheckUser(unsigned long ip,unsigned gid);
	bool CheckGroup(unsigned uid,unsigned gid);
	unsigned GetUserID(const char *user);
	bool GetUserName(unsigned uid,std::string &name,bool use_lock=true);
	bool ForceLogout(const char *user,bool use_lock=true);
	std::string ListUser();
	std::vector<std::string> getAllUserName();
	std::vector<std::string> getAllGroupName(const std::string &user);
	std::string ListLoginUser();
	bool LoadUserInfo(const char *user,UserInfo &m_userinfo);
	bool LoadLoginUser();
	bool AddGroup(const char *user,const char *group,std::string &errmsg);
	bool DelGroup(const char *user,const char *group,std::string &errmsg);
private:
	bool CheckLogin(const char *user);
	user_model_fuction user_model;
	KUser_int_map m_user;
	unsigned maxuid;
	stringmap m_all_user;
	uintmap m_all_user2;
	void *handle;
	int root_gid;
	KMutex m_lock;
	KMutex m_root_lock;

};
extern KUser m_user;
#endif
#endif
