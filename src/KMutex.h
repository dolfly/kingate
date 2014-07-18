#ifndef kmutex_h_lsdkjfs0d9f8sdf9
#define kmutex_h_lsdkjfs0d9f8sdf9
#ifndef _WIN32
#include<pthread.h>
#else
#include "forwin32.h"
#endif
class KMutex
{
public:
	KMutex(){ pthread_mutex_init(&lock,NULL);}
	~KMutex(){ pthread_mutex_destroy(&lock);}
	int Lock(){ return pthread_mutex_lock(&lock);}
	int Unlock() { return pthread_mutex_unlock(&lock); }
private:
	pthread_mutex_t lock;
};
#endif

