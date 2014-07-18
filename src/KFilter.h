#ifndef KFilter_h_sldkjflsdjfsd8f7s97fsdfsdfus9d7f
#define KFilter_h_sldkjflsdjfsd8f7s97fsdfsdfus9d7f
#include <vector>
#include <string>
#include "KTimeMatch.h"
#include "KConfig.h"
#include "mysocket.h"
#define DENY	0
#define ALLOW	1
struct IP_MODEL
{
	unsigned ip;
	unsigned port;
	unsigned mask;
	bool ip_revers;
	bool port_revers;
};
class CHAIN
{
public:
	IP_MODEL src;
	IP_MODEL dst;
	unsigned service_port;
	bool service_revers;
	int user_id;
	unsigned user_revers;
	bool allip;
	int expire_time;
	KTimeMatch m_time;
	CHAIN()
	{
		memset(&src,0,sizeof(src));
		memset(&dst,0,sizeof(dst));
		service_port=0;
		service_revers=false;
		user_id=0;
		user_revers=false;
		allip=false;
		expire_time=0;
	}
};
struct STATE
{
	unsigned src_ip;
	int service_port;
	unsigned dst_ip;
	int dst_port;
	unsigned uid;
};
typedef std::vector<CHAIN> CHAINLIST_T;
class KFilter
{
public:
	KFilter();
	~KFilter();
	bool AddChain(int model,CHAIN &m_chain);
	bool DelChain(int index,int model);
	std::string MakeChain(const char *str,const char *time_str,int model,int expire_time=0);
	int  Check(STATE &m_state);
	bool LoadConfig(const char *file_name);
	void checkExpireChain();
	void PrintFilter();
	bool SaveConfig();
	void changeFirst();
	int first;
	std::string ChainList();
private:
	bool CheckChain(CHAINLIST_T &m_chain,STATE &m_state,int model);
	CHAINLIST_T m_chains[2];
	std::string access_file;

};
unsigned ConvertIP(const char *ip);
bool AddIPModel(const char *ip_model,IP_MODEL &m_ip);
bool MatchIPModel(IP_MODEL &m_ip,unsigned ip,short port,int model=ALLOW);
std::string make_ip(unsigned long ip,bool mask=false);

#endif
