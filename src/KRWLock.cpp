#ifndef _WIN32
#define _XOPEN_SOURCE 500
#include<pthread.h>
#include "KRWLock.h"
static pthread_rwlock_t m_rw_lock;
KRWLock::KRWLock()
{
	pthread_rwlock_init(&m_rw_lock,NULL);
}
KRWLock::~KRWLock()
{
		pthread_rwlock_destroy(&m_rw_lock);

}
int KRWLock::RLock()
{ 
	return pthread_rwlock_rdlock(&m_rw_lock);
}
int KRWLock::WLock()
{
	 return pthread_rwlock_wrlock(&m_rw_lock);
}
int KRWLock::Unlock()
{
	return pthread_rwlock_unlock(&m_rw_lock); 
}
#endif