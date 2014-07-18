#include "KDnsCache.h"
#include<time.h>
#include<string.h>
#ifndef _WIN32
#include <netdb.h>
#include <sys/socket.h>
#include<arpa/inet.h>
#include<unistd.h>
#endif

#include <stdio.h>
#include <sys/types.h>

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif


#ifndef _WIN32
#include <syslog.h>
#endif
#ifndef HAVE_GETHOSTBYNAME_R
#include "KMutex.h"
#endif
#include "malloc_debug.h"


using namespace std;
const int max_avail_time=24*60*60;//1 days
//const int max_avail_time=24*60*60;

#ifndef HAVE_GETHOSTBYNAME_R
KMutex m_gethostbyname_lock;
#endif
unsigned KDnsCache::GetName(const char *name)
{
	struct hostent	he_b;
	int		he_errno=0;
	int		rc = 0;
	struct hostent	*he_x=NULL;
	struct hostent *h=NULL;
	unsigned long ip4_addr;
	DNSMAP_T::iterator it;
	bool allnum=true;
	for(int i=0;i<strlen(name);i++){
		if(name[i]!='.' && (name[i]<'0' || name[i]>'9')){
			allnum=false;
			break;
		}
	}
	if(allnum){
	//	printf("name=%s is allnum.\n",name);
		return (unsigned)inet_addr(name);
	//	return (unsigned)s.s_addr;
	}
	m_lock.Lock();
	it=m_dns.find(name);
	if(it!=m_dns.end()){
		ip4_addr=(*it).second.ip;
		m_lock.Unlock();
	//	printf("get name in cache:%s\n",name);
/*		if((*it).second.ip<=0){
		#ifdef HAVE_SYSLOG_H
			syslog(LOG_ERR,"[KDNSCACHE]name :%s in cache value=%d",name,(*it).second.ip);
		#endif	
		}
	*/		

		return ip4_addr;//(*it).second.ip;
	}
	m_lock.Unlock();
	//h=gethostbyname(name);
	//printf("call gethostbyname_r for look %s\n",name);
	DNS_T m_dns_tmp;
#ifdef  HAVE_GETHOSTBYNAME_R
	char	he_strb[2048];
	rc = gethostbyname_r(name, &he_b, he_strb, sizeof(he_strb),
				&he_x,
				&he_errno);
	if(rc!=0||he_errno!=0){
		
		syslog(LOG_ERR,"[KDNSCACHE]name :%s is error,rc=%d,he_errno=%d.\n",name,rc,he_errno);
	
		return 0;
	}
	m_dns_tmp.ip=*((unsigned long *)he_b.h_addr);
#else
#ifndef _WIN32
	m_gethostbyname_lock.Lock();
#endif
	h=gethostbyname(name);
	if(h==NULL){
		m_gethostbyname_lock.Unlock();	
		return 0;	
	}
	m_dns_tmp.ip=*((unsigned long *)h->h_addr);
#ifndef _WIN32
	m_gethostbyname_lock.Unlock();
#endif
#endif
	m_dns_tmp.create_time=time(NULL);
	m_lock.Lock();
	m_dns.insert(make_pair<string,DNS_T>(name,m_dns_tmp));
	m_lock.Unlock();
	return m_dns_tmp.ip;
err:
	m_lock.Unlock();
	return 0;
}
void KDnsCache::Flush()
{
	unsigned now_time=time(NULL);
	DNSMAP_T::iterator it;
	m_lock.Lock();
	for(it=m_dns.begin();it!=m_dns.end();){
		if(now_time-(*it).second.create_time>max_avail_time){
#ifndef _WIN32
			m_dns.erase(it);
			it++;
#else
			it=m_dns.erase(it);
#endif
		}else{
			it++;
		}
	}
	m_lock.Unlock();
}
