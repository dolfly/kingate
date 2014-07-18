#include "kingate.h"
#include<time.h>
#include "forwin32.h"

#include "malloc_debug.h"

#ifdef _WIN32
int  pthread_mutex_init(pthread_mutex_t *mutex,void *t)
{
//	char tmp[100];
//	sprintf(tmp,"lock%d%d",rand(),time(NULL));
	*mutex=CreateMutex(NULL,FALSE,NULL);
	return 0;
};
int pthread_mutex_destroy(pthread_mutex_t *mutex)
{
	CloseHandle(*mutex);
	return 0;
}
/*
int
strerror_r(int err, char *errbuf, size_t lerrbuf)
{
LPTSTR	lpszMsgBuf = NULL;
char	b[80];

    FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER |
		  FORMAT_MESSAGE_FROM_SYSTEM |
		  FORMAT_MESSAGE_IGNORE_INSERTS,
		  NULL, (DWORD)err,
		  MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
                  (LPTSTR)&lpszMsgBuf, 0, NULL);

    if ( lpszMsgBuf == NULL ) {
	sprintf(b, "Error: (%d)", err);
	if ( lerrbuf > 0 ) strncpy(errbuf, b, lerrbuf-1);
	return(-1);

    }

    strncpy(errbuf, lpszMsgBuf, MIN(lerrbuf-1, strlen(lpszMsgBuf)-1));
    LocalFree(lpszMsgBuf);
    return(0);
}
*/
#endif
