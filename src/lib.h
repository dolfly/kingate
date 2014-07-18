#if	!defined(_LIB_H_INCLUDED_)
#define _LIB_H_INCLUDED_
#include "log.h"
#include "malloc_debug.h"
void 		my_xlog(int lvl, const char *form, ...);
int str_to_sa(char *val, struct sockaddr *sa );
char		*base64_encode(char*);
char		*base64_decode(char*);
int		calculate_resident_size(struct mem_obj *);
void		increase_hash_size(struct obj_hash_entry*, int,bool mem_flag=true);
void		decrease_hash_size(struct obj_hash_entry*, int,bool mem_flag=true);
void		say_bad_request(const char*, const char*, int, struct request *);
char		*STRERROR_R(int, char *, size_t);
struct mem_obj	*locate_in_mem(struct url*, int, int*, struct request *);
int		obj_rate(struct mem_obj*);
int		insert_header(char *, char *, struct mem_obj *);
int		http_date(char *date, time_t*);
void 		my_msleep(int msec);
int		wait_for_read(int, int);
int		readt(int, char*, int, int);
void		free_url(struct url*);
void		memcpy_to_lower(char *, char *, size_t);
void		free_avlist(struct av *);
void 	send_error(int, int, const char*);
int mk1123time(time_t time, char *buf, int size);
inline	static	struct	buff *alloc_buff(int size);
inline	static	int	attach_av_pair_to_buff(const char* attr, const char *val, struct buff *buff);
inline	static	int	attach_data(const char* src, int size, struct buff *buff);
inline	static	void	free_container(struct buff *buff);
inline	static	char	*my_inet_ntoa(struct sockaddr_in *sa);
inline	static	int	store_in_chain(char *src, int size, struct mem_obj *obj);

inline
static struct	buff *
alloc_buff(int size)
{
char		*t, *d;
struct buff	*b;

    if ( size <=0 ) return(NULL);
    t = (char *)xmalloc(sizeof(struct buff), "alloc_buff(): 1");
/*	#ifdef MALLOCDEBUG
	printf("t=%x\n",t);
	if(fork()==0)
		assert(false);
	#endif
  */  if ( !t ) return(NULL);
    memset(t,0,sizeof(struct buff));
 //   bzero(t, sizeof(struct buff));
    d = (char *)xmalloc(size, "alloc_buff(): 2");
    if ( !d ) {
		xfree(t);
		return(NULL);
	}
    b = (struct buff*)t;
    b->data = d;
    b->curr_size = size;
    b->used = 0;
    return(b);
}

inline
static int
attach_av_pair_to_buff(const char* attr, const char *val, struct buff *buff)
{
    if ( !attr || !val || !buff )return(-1);

    if ( *attr ) {
		attach_data(attr, strlen(attr), buff);
		attach_data(" ", 1, buff);
		attach_data(val, strlen(val), buff);
    }
    attach_data("\r\n", 2, buff);
    return(0);
}

/* concatenate data in continuous buffer */
inline
static int
attach_data(const char* src, int size, struct buff *buff)
{
	char	*t;
	int	tot;

    if ( size <= 0 ) return(-1);
    if ( !buff->data ) {
		t = (char *)xmalloc(((size / CHUNK_SIZE) + 1) * CHUNK_SIZE, "attach_data(): 1");
		if (!t) return(-1);
		buff->data = t;
		memcpy(t, src, size);
		buff->curr_size = ((size / CHUNK_SIZE) + 1) * CHUNK_SIZE;
		buff->used = size;
		return(0);
    }
    if ( buff->used + size <= buff->curr_size ) {
		memcpy(buff->data+buff->used, src, size);
		buff->used += size;
    } else {
		tot = buff->used + size;
		tot = ((tot / CHUNK_SIZE) + 1) * CHUNK_SIZE;
		t = (char *)xmalloc(tot, "attach_data(): 2");
		if (!t ) {
			klog(ERR_LOG, "attach_data(): No mem in attach data.\n");
			return(-1);
		}
		memcpy(t, buff->data, buff->used);
		memcpy(t+buff->used, src, size);
	//	xfree(buff->data); buff->data = t;
		if(buff->data)
			free(buff->data);
		buff->data=t;
		buff->used += size;
		buff->curr_size = tot;
    }
    return(0);
}

inline char *attr_value(struct av *avp, const char *attr)
{
char	*res = NULL;

    if ( !attr ) return(NULL);

    while( avp ) {
	if ( avp->attr && !strncasecmp(avp->attr, attr, strlen(attr)) ) {
	    res = avp->val;
	    break;
	}
	avp = avp->next;
    }
    return(res);
}

inline
static void
free_container(struct buff *buff)
{
struct buff *next;

    while(buff) {
	next = buff->next;
	/*my_xlog(OOPS_LOG_DBG, "free_container(): Free buffer: %d of %d, next: %p\n", buff->size, buff->curr_size, buff->next);*/
	if ( buff->data ) xfree(buff->data);
	xfree(buff);
	buff = next;
    }
}

inline
static char *
my_inet_ntoa(struct sockaddr_in *sa)
{
char * res = (char *)xmalloc(20, "my_inet_ntoa(): 1");
uint32_t	ia = ntohl(sa->sin_addr.s_addr);
uint32_t	a, b, c, d;

    if ( !res ) return(NULL);
    a =  ia >> 24;
    b = (ia & 0x00ff0000) >> 16;
    c = (ia & 0x0000ff00) >> 8;
    d = (ia & 0x000000ff);
    sprintf(res, "%d.%d.%d.%d",
	(unsigned)(ia >> 24),
	(unsigned)((ia & 0x00ff0000) >> 16),
	(unsigned)((ia & 0x0000ff00) >> 8),
	(unsigned)((ia & 0x000000ff)));
    return(res);
}

/* store in hot_buff, allocate buffs if need*/
inline
static int
store_in_chain(char *src, int size, struct mem_obj *obj)
{
	struct buff *hot = obj->hot_buff, *new_t;

    if (!hot) {
		hot=alloc_buff(ROUND_CHUNKS(size));
		if(!hot)
			return (-1);
		obj->container=obj->hot_buff=hot;	
		my_xlog(OOPS_LOG_SEVERE, "store_in_chain(): hot == NULL!\n");
    }
    if (!obj) {
		my_xlog(OOPS_LOG_SEVERE, "store_in_chain(): obj == NULL!\n");
		return(-1);
    }
    if ( size < 0 ) {
		my_xlog(OOPS_LOG_SEVERE, "store_in_chain(): size = %d!\n", size);
		return(-1);
    }
    if ( hot->used + size <= hot->curr_size ) {
		memcpy( hot->data + hot->used, src, size);
		hot->used += size;
    } else {
		int	moved, to_move;
		/* copy part */
		memcpy(hot->data + hot->used, src, hot->curr_size - hot->used);
		moved=hot->curr_size - hot->used;
		hot->used = hot->curr_size;
		to_move = size - moved;
		/* allocate  */
		new_t = alloc_buff(ROUND_CHUNKS(to_move));
		if ( !new_t ) return(-1);
		/* copy rest */
		memcpy(new_t->data, src+moved, to_move);
		new_t->used = to_move;
		hot->next = new_t;
		obj->hot_buff = new_t;
    }
    return(0);
}

#endif	/* !_LIB_H_INCLUDED_ */
