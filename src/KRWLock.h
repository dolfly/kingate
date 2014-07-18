#ifndef krwlock_h_lsdkjfs0d9fsdfsdafasdfasdf8sdf9
#define krwlock_h_lsdkjfs0d9fsdfsdafasdfasdf8sdf9

#ifndef _WIN32
class KRWLock
{
public:
	KRWLock();
	~KRWLock();
	int RLock();
	int WLock();
	int Unlock() ;
};
#else
#define RLock()			Lock()
#define WLock()			Lock()
#define KRWLock         KMutex
#endif

#endif
