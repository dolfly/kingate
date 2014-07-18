#ifndef KDnsCache_lsdkjfsdf97sdf808sdf08slksdf8768d6723424
#define KDnsCache_lsdkjfsdf97sdf808sdf08slksdf8768d6723424
//#include "KRWLock.h"
#include "KMutex.h"
#include<map>
#include<string>
struct DNS_T
{
	unsigned ip;
	unsigned create_time;
};
typedef std::map<std::string,DNS_T> DNSMAP_T;
class KDnsCache
{
public:
	unsigned GetName(const char *name);
	void Flush();
private:
	DNSMAP_T m_dns;
	KMutex m_lock;
};
extern KDnsCache m_dns_cache;
#endif
