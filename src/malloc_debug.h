#ifndef malloc_debug_h_lskdjflskdjfskdj92394u2394234
#define malloc_debug_h_lskdjflskdjfskdj92394u2394234
#ifndef _WIN32
//#define MALLOCDEBUG
#endif
#ifdef MALLOCDEBUG
#define xmalloc(x,y)		xmalloc2(x,__FILE__,__LINE__)
#define xfree(x)		xfree2(x,__FILE__,__LINE__)
#define malloc(x)		xmalloc2(x,__FILE__,__LINE__)
#define free(x)			xfree2(x,__FILE__,__LINE__)
#define strdup(x)		xstrdup(x,__FILE__,__LINE__)
void xfree2(void *ptr,const char *file,int line);
void *xmalloc2(size_t size, const char *file,int line);
char *xstrdup(const char * s,const char *file,int line);
void list_all_malloc();
void * operator new(size_t m_size,const char *file,int line);
void * operator new[](size_t m_size,const char *file,int line);
void operator delete(void *p);
void operator delete[](void *p);
extern int list_all;
#define new new(__FILE__, __LINE__)
#else
#define xmalloc(x,y)	malloc(x)
#define xfree(x)		free(x)
//#define DEBUG_NEW		new
#endif
#endif
