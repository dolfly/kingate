#ifndef for_win32_include_skdjfskdfkjsdfj
#define for_win32_include_skdjfskdfkjsdfj
/*
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
*/
#ifdef _WIN32//for win32
#include <io.h>
#include <stdio.h>
#include<process.h>
#include<windows.h>
/*
#define FUNC_CALL	__stdcall
typedef unsigned FUNC_TYPE;
#define snprintf _snprintf
#define O_SYNC	_O_WRONLY
#define pthread_create(a,b,c,d)		_beginthreadex(NULL,10*1024,c,d,0,a)//&id,&attr,(func)server_thread,(void *)m2_server);
*/
#define FUNC_CALL
#define FUNC_TYPE void
#define snprintf _snprintf
#define O_SYNC	_O_WRONLY
#define pthread_create(a,b,c,d)		_beginthread(c,0,d)//&id,&attr,(func)server_thread,(void *)m2_server);

#define pthread_mutex_lock(x)		WaitForSingleObject(*x,INFINITE)
#define pthread_mutex_unlock(x)		ReleaseMutex(*x)
#define getpid()					GetCurrentThreadId()
#define pthread_self()				GetCurrentThreadId()
#define sleep(a)					Sleep(1000*a)
#define   pthread_mutex_t HANDLE
#define pthread_t	unsigned
int  pthread_mutex_init(pthread_mutex_t *mutex,void *t);
int	 pthread_mutex_destroy(pthread_mutex_t *mutex);
#ifndef bzero
#define bzero(x,y)	memset(x,0,y)
#endif
#define syslog		klog
#define		strncasecmp	strnicmp
#define		strcasecmp	stricmp
#define		ERRNO			WSAGetLastError()
#define		CLOSE(so)		closesocket(so)
#define		strtok_r(a,b,c)		strtok(a,b)
#define ctime_r( _clock, _buf ) 	( strcpy( (_buf), ctime( (_clock) ) ), (_buf) )

#define gmtime_r( _clock, _result ) ( *(_result) = *gmtime( (_clock) ), (_result) )

#define localtime_r( _clock, _result ) 	( *(_result) = *localtime( (_clock) ), (_result) )
#define mkdir(a)  _mkdir(a)
#define unlink(a)	_unlink(a)
void WINAPI kingateMain(DWORD argc, LPTSTR * argv);
bool InstallService(const char * szServiceName);
bool UninstallService(const char * szServiceName);
void LogEvent(LPCTSTR pFormat, ...);
void Start();
void Stop();
//int strerror_r(int err, char *errbuf, size_t lerrbuf);
#else//for unix
#define FUNC_CALL
typedef  void * FUNC_TYPE;
#endif


#endif
