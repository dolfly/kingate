/*
Copyright (C) 1999, 2000 Igor Khasilev, igor@paco.net
Copyright (C) 2000 Andrey Igoshin, ai@vsu.ru

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.

*/
#include "kingate.h"
#ifdef MALLOCDEBUG
#include <map>
#endif
#ifndef _WIN32
#include<syslog.h>
#endif
#ifdef HAVE_CONFIG_H
#include "config.h"
#endif
#include	<time.h>
#include 	<ctype.h>
#include	"oops.h"
#include	"do_config.h"
#include	"forwin32.h"
#include "malloc_debug.h"

//#include	"modules.h"
static	const char	*days[] = {"Sun", "Mon","Tue","Wed","Thu","Fri","Sat"};
static	const char	*months[] = {"Jan","Feb","Mar","Apr","May","Jun","Jul","Aug","Sep","Oct","Nov","Dec"};
//static	void	flush_log(void);
static	int		lookup_dns_cache(char* name, struct dns_cache_item *items, int counter);
static	int		my_gethostbyname(char *name);
//static	char	*my_gethostbyaddr(int);
static	void	get_hash_stamp(char*, int*, int*);
inline	static	int	tm_to_time(struct tm *, time_t *);

#ifdef MALLOCDEBUG
typedef struct _MEM
{
	void * addr;
	int line;
	char file[16];
	int size;
//	struct _MEM *prev;
//	struct _MEM *next;
} MEM;
using namespace std;
int total_block=0;
typedef map<int,MEM *> mem_map_t;
mem_map_t mem_map;
pthread_mutex_t malloc_mutex;
//MEM *head=NULL;
//MEM *end=NULL;
int list_free=0,list_malloc=0,list_all=0,total_size=0;
#endif
void
CTIME_R(time_t *a, char *b, size_t l)
{
#if	defined(HAVE_CTIME_R)
#if	defined(CTIME_R_3)
	ctime_r(a, b, l);
#else
	ctime_r(a, b);
#endif /* SOLARIS */
#else
	struct	tm	tm;
	memset(b,0,l);
	localtime_r(a, &tm);
	snprintf(b, l-1, "%s %02d %s %02d:%02d:%02d\n",
		 days[tm.tm_wday], tm.tm_mday,
		 months[tm.tm_mon],
		 tm.tm_hour, tm.tm_min, tm.tm_sec);
#endif /* HAVE_CTIME_R */
}

void
verb_printf(char *form, ...)
{   
    
}

void
my_xlog(int lvl, const char *form, ...)
{
	return;

}
void
do_exit(int code)
{
//   flush_log();
   assert(0);
 //  exit(code);
}


int
http_date(char *date, time_t *time)
{
#define	TYPE_RFC	0
#define	TYPE_ASC	1
#define	FIELD_WDAY	1
#define	FIELD_MDAY	2
#define	FIELD_MONT	3
#define	FIELD_YEAR	4
#define	FIELD_TIME	5

char		*p, *s;
char		*ptr;
int		type = TYPE_RFC, field=FIELD_WDAY, t;
int		wday = -1, mday = -1, month = -1, secs = -1, mins = -1, hour = -1;
int		year = -1;
char		*xdate;
struct	tm	tm;

    xdate = (char *)xmalloc(strlen(date) +1, "http_date(): http_date");
    if ( !xdate )
	return(-1);
    strcpy(xdate, date);
    p = date = xdate;

    while( (s = (char*)strtok_r(p, " ", &ptr)) != 0 ) {
	p = NULL;
    parse:
	switch(field) {
	case FIELD_WDAY:
		if ( strlen(s) < 3 ) {
		    my_xlog(OOPS_LOG_DBG|OOPS_LOG_INFORM, "http_date(): Unparsable date: %s\n", date);
		    free(xdate);
		    return(-1);
		}
		/* Sun, Mon, Tue, Wed, Thu, Fri, Sat */
		switch ( tolower(s[2]) ) {
		case 'n': /* Sun or Mon */
			if ( tolower(s[0]) == 's' ) wday = 0;
			    else		    wday = 1;
			break;
		case 'e': /* Tue	*/
			wday = 2;
			break;
		case 'd': /* Wed	*/
			wday = 3;
			break;
		case 'u': /*Thu		*/
			wday = 4;
			break;
		case 'i': /* Fri	*/
			wday = 5;
			break;
		case 't': /* Sat	*/
			wday = 6;
			break;
		default:
			my_xlog(OOPS_LOG_DBG|OOPS_LOG_INFORM, "http_date(): Unparsable date: %s\n", date);
			free(xdate);
			return(-1);
		}
		if ( !strchr(s,',') ) type = TYPE_ASC;
		if ( type == TYPE_RFC )
			field = FIELD_MDAY;
		    else
			field = FIELD_MONT;
		break;
	case FIELD_MDAY:
		if ( type == TYPE_RFC ) {
		    t = 0;
		    while( *s && IS_DIGIT(*s) ) {
			t = t*10 + (*s - '0');
			s++;
		    }
		    if ( t ) mday = t;
			else {
			     my_xlog(OOPS_LOG_DBG|OOPS_LOG_INFORM, "http_date(): Unparsable date: %s\n", date);
			     free(xdate);
			     return(-1);
		    }
		    field = FIELD_MONT;
		    if ( *s && (*s == '-') ) {
			/* this is dd-mmm-yy format */
			s++;
			goto parse;
		    }
		    if ( *s ) {
			free(xdate);
			return(-1);
		    }
		    break;
		} else {
		    t = 0;
		    while( *s && IS_DIGIT(*s) ) {
			t = t*10 + (*s - '0');
			s++;
		    }
		    if ( *s ) {
			free(xdate);
			return(-1);
		    }
		    if ( t ) mday = t;
			else {
			     my_xlog(OOPS_LOG_DBG|OOPS_LOG_INFORM, "http_date(): Unparsable date: %s\n", date);
			     free(xdate);
			     return(-1);
		    }
		    field = FIELD_TIME;
		}
		break;
	case FIELD_MONT:
		/* Jan Feb Mar Apr May Jun Jul Aug Sep Oct Nov Dec */
		if ( strlen(s) < 3 ) {
		    my_xlog(OOPS_LOG_DBG|OOPS_LOG_INFORM, "http_date(): Unparsable date: %s\n", date);
		    free(xdate);
		    return(-1);
		}
		switch ( tolower(s[2]) ) {
		    case 'n':
			if ( s[1] == 'a' ) month = 0;
			if ( s[1] == 'u' ) month = 5;
			break;
		    case 'b':
			month = 1;
			break;
		    case 'r':
			if ( s[1] == 'a' ) month = 2;
			if ( s[1] == 'p' ) month = 3;
			break;
		    case 'y':
			month = 4;
			break;
		    case 'l':
			month = 6;
			break;
		    case 'g':
			month = 7;
			break;
		    case 'p':
			month = 8;
			break;
		    case 't':
			month = 9;
			break;
		    case 'v':
			month = 10;
			break;
		    case 'c':
			month = 11;
			break;
		    default:
			my_xlog(OOPS_LOG_DBG|OOPS_LOG_INFORM, "http_date(): Unparsable date: %s\n", date);
			free(xdate);
			return(-1);
		}
		s+=3;
		if ( type == TYPE_ASC ) field = FIELD_MDAY;
		    else 		field = FIELD_YEAR;
		if ( *s && (*s=='-') ) {
		    /* this is dd-mmm-yy format */
		    s++;
		    goto parse;
		}
		break;
	case FIELD_YEAR:
		if ( type==TYPE_ASC && !IS_DIGIT(*s) ) /* here can be zonename */
		    break;
		year = atoi(s);
		if ( year == 0 ) {
		     my_xlog(OOPS_LOG_DBG|OOPS_LOG_INFORM, "http_date(): Unparsable date: %s\n", date);
		     free(xdate);
		     return(-1);
		}
		if ( strlen(s) <=2 )
		    year += 1900;
		if ( type == TYPE_RFC ) field = FIELD_TIME;
		     else		goto compose;
		break;
	case FIELD_TIME:
		hour = mins = secs = 0;
		while (*s && IS_DIGIT(*s) ) hour = hour*10 + ((*s++)-'0');
		if ( *s ) s++;
		while (*s && IS_DIGIT(*s) ) mins = mins*10 + ((*s++)-'0');
		if ( *s ) s++;
		while (*s && IS_DIGIT(*s) ) secs = secs*10 + ((*s++)-'0');
		if ( type == TYPE_ASC ) {
			field = FIELD_YEAR;
			break;
		}
		goto compose;
		break;
	}
    }

compose:
    memset(&tm,0, sizeof(tm));
    tm.tm_sec = secs;
    tm.tm_min = mins;
    tm.tm_hour= hour;
    tm.tm_wday= wday;
    tm.tm_mday= mday;
    tm.tm_mon = month;
    tm.tm_year= year - 1900;
    free(xdate);
    tm_to_time(&tm, time);
    return(0);
}

int
tm_cmp(struct tm *tm1, struct tm *tm2)
{
    if (tm1->tm_year  < tm2->tm_year ) return(-1);
    if (tm1->tm_mon < tm2->tm_mon ) return(-1);
    if (tm1->tm_mday < tm2->tm_mday ) return(-1);
    if (tm1->tm_hour < tm2->tm_hour ) return(-1);
    if (tm1->tm_min < tm2->tm_min ) return(-1);
    if (tm1->tm_sec < tm2->tm_sec ) return(-1);
    return(1);
}

int
mk1123time(time_t time, char *buf, int size)
{
struct	tm	tm;
time_t		holder = time;
char		tbuf[80];

    gmtime_r(&holder, &tm);
    sprintf(tbuf, "%s, %02d %s %d %02d:%02d:%02d GMT",
    		days[tm.tm_wday], tm.tm_mday,
    		months[tm.tm_mon], tm.tm_year+1900,
		tm.tm_hour, tm.tm_min, tm.tm_sec);
    memset(buf,0,size);
    strncpy(buf, tbuf, size-1);
    return(TRUE);
}

/* accept tm for GMT, return time */
inline
static int
tm_to_time(struct tm *tm, time_t *time)
{
struct		tm ttm;
time_t		res, dst;

    /* mktime convert from localtime, so shift tm to localtime	*/
    memcpy(&ttm, tm, sizeof(ttm));
    ttm.tm_isdst = -1;
    pthread_mutex_lock(&mktime_lock);
    res = mktime(&ttm);
    pthread_mutex_unlock(&mktime_lock);
    dst = 0;
    if ( ttm.tm_isdst > 0)
            dst = -3600;
#if	defined(SOLARIS) || defined(_AIX)
    res -= timezone + dst;
#elif	defined(_WIN32)
    res -= _timezone + dst;
#elif	defined(FREEBSD) || (defined(LINUX) && !defined(HAVE_STRUCT_TM___TM_GMTOFF__) )
    res += ttm.tm_gmtoff;
#elif	defined(HAVE_STRUCT_TM___TM_GMTOFF__)
    res += ttm.__tm_gmtoff__ - dst;
#else
    res += ttm.tm_gmtoff - dst;
#endif
    return(*time = res);
}

char*
html_escaping(char *src)
{
int	olen = strlen(src);
char	*p = src, *d, *res;
int	specials = 0;

    if ( !src ) return(NULL);
    while( p && *p ) {
    	if ( *p == '<' || *p == '>' || *p == '\"' || *p == '&' )
	    specials++;
	p++;
    }
    res = (char *)malloc(strlen(src) + 1 + specials*5 ); /* worst case */
    if ( !res ) return(NULL);

    if ( specials == 0 ) {
	memcpy(res, src, olen+1);
	return(res);
    }    
    p = src;
    d = res;
    while ( *p ) {
	if ( *p == '<' ) {
	    strcpy(d,"&lt;");d+=3;
	} else
	if ( *p == '>' ) {
	    strcpy(d,"&gt;");d+=3;
	} else
	if ( *p == '\"' ) {
	    strcpy(d,"&quot;"); d+=5;
	} else
	if ( *p == '&' ) {
	    strcpy(d,"&amp;"); d+=4;
	} else
	    *d = *p;
	p++;d++;
    }
    *d = 0;
    return(res);
}

char*
htmlize(char *src)
{
char	*res;
u_char	*s = (u_char*)src, *d;
u_char	xdig[17] = "0123456789ABCDEF";

    res = (char *)malloc(strlen(src) * 3 + 1 ); /* worst case */
    if ( !res ) return(NULL);
    d = (u_char*)res;
    while( *s ) {
	if ( *s!='/' &&
	     *s!='.' &&
	     *s!='-' &&
	     *s!='_' &&
	     *s!='~' &&
		((*s >= 0x80) || (*s <= 0x20) || !isalnum(*s) ) ) {
	    *d++ = '%';
	    *d++ = xdig[ (*s) / 16 ];
	    *d   = xdig[ (*s) % 16 ];
	} else
	    *d = *s;
	d++; s++;
    }
    *d=0;
    return(res);
}

#define	HEXTOI(arg)	(((arg)<='9')? ((arg)-'0'):(tolower(arg)-'a' + 10))
char*
dehtmlize(char *src)
{
char	*res;
u_char	*s = (u_char*)src, *d;

    res = (char *)xmalloc(strlen(src) + 1, "dehtmlize(): dehtmlize"); /* worst case */
    if ( !res ) return(NULL);
    d = (u_char*)res;
    while( *s ) {
	if ( (*s=='%') && isxdigit(*(s+1)) && isxdigit(*(s+2)) ) {
	    *d = (HEXTOI(*(s+1)) << 4) | (HEXTOI(*(s+2)));
	    s+=2;
	} else
	    *d = *s;
	d++; s++;
    }
    *d=0;
    return(res);
}

#if	!defined(_WIN32)
#if	!defined(HAVE_DAEMON)
int
daemon(int nochdir, int noclose)
{
pid_t	child;

    /* this is not complete */
    child = fork();
    if ( child < 0 ) {
	fprintf(stderr, "daemon(): Can't fork.\n");
	return(1);
    }
    if ( child > 0 ) {
	/* parent */
	exit(0);
    }
    if ( !nochdir ) {
	chdir("/");
    }
    if ( !noclose ) {
        fclose(stdin);
	fclose(stdout);
	fclose(stderr);
        close(3);
    }
    return(0);
}
#endif	/* !HAVE_DAEMON */
#else
int
daemon(int nochdir, int noclose)
{
    return(0);
}
#endif	

char *
STRERROR_R(int err, char *errbuf, size_t lerrbuf)
{
/*	
#if	defined(LINUX)
    return(strerror_r(err, errbuf, lerrbuf));
#else
    if ( strerror_r(err, errbuf, lerrbuf) == -1 )
	my_xlog(OOPS_LOG_DBG, "STRERROR_R(): strerror_r() returned (-1), errno = %d\n", err);
    return(errbuf);
#endif
	*/
	return NULL;
}

void
increase_hash_size(struct obj_hash_entry* hash, int size,bool mem_flag)
{
int	rc; 


	assert(hash);
	assert(size>=0);
	
    if ( !hash ) {
		my_xlog(OOPS_LOG_SEVERE, "increase_hash_size(): hash == NULL in increase_hash_size\n");
		return;
    }
    if ( size < 0 ) {
		my_xlog(OOPS_LOG_SEVERE, "increase_hash_size(): size<=0 in increase_hash_size\n");
		do_exit(1);
		return;
    }
	if ( !(rc = pthread_mutex_lock(&hash->size_lock)) ) {
		if(mem_flag)
			hash->size += size;
		else
			hash->disk_size += size;
		if ( hash->size < 0 ) {
			my_xlog(OOPS_LOG_SEVERE, "increase_hash_size(): increase: hash_size has negative value: %d!\n", hash->size);
			do_exit(1);
		}
		pthread_mutex_unlock(&hash->size_lock);
    } else {
#ifndef _WIN32
		syslog(LOG_ERR,"increase_hash_size(): Can't lock hash entry for increase size\n");
#endif	//	my_xlog(OOPS_LOG_SEVERE, ;
    }
}

void
decrease_hash_size(struct obj_hash_entry* hash, int size,bool mem_flag)
{
int	rc;

    if ( !hash ) {
		return;
    }
    if ( size < 0 ) {
		my_xlog(OOPS_LOG_SEVERE, "decrease_hash_size(): size<0 in decrease_hash_size\n");
		do_exit(1);
		return;
    }
//    total_alloc -= size;
    if ( !(rc=pthread_mutex_lock(&hash->size_lock)) ) {
		if(mem_flag){
			hash->size -= size;
		}else{
			hash->disk_size-=size;
		}
		if ( hash->size < 0 ) {
			my_xlog(OOPS_LOG_SEVERE, "decrease_hash_size(): decrease: hash_size has negative value: %d!\n", hash->size);
			do_exit(1);
		}
		pthread_mutex_unlock(&hash->size_lock);
	} else {
#ifndef _WIN32
		syslog(LOG_ERR,"increase_hash_size(): Can't lock hash entry for increase size\n");
#endif		//my_xlog(OOPS_LOG_SEVERE, "decrease_hash_size(): Can't lock hash entry for decrease size\n");
    }
}

int
calculate_resident_size(struct mem_obj *obj)
{
int		rs =0;// sizeof(struct mem_obj);
struct	buff	*b = obj->container;
//struct	av	*av = obj->headers;
    while( b ) {
		rs +=  b->curr_size;
		b = b->next;
    }
/*
    while( av ) {
		rs += sizeof(*av);
		if ( av->attr ) rs+=strlen(av->attr);
		if ( av->val ) rs+= strlen(av->val);
		av = av->next;
    }
*/
    return(rs);
}

int
calculate_container_datalen(struct buff *b)
{
int		rs = 0;

    while( b ) {
	rs += b->used;
	b = b->next;
    }
    return(rs);
}

void
my_sleep(int sec)
{
#if	defined(OSF)
    /* DU don't want to sleep in poll when number of descriptors is 0 */
    sleep(sec);
#elif	defined(_WIN32)
    Sleep(sec*1000);
#else
    (void)poll_descriptors(0, NULL, sec*1000);
#endif
}

void my_msleep(int msec)
{
#if	defined(OSF)
    /* DU don't want to sleep in poll when number of descriptors is 0 */
    usleep(msec*1000);
#elif	defined(_WIN32)
    Sleep(msec*1000);
#else
	struct timeval      tv;
        tv.tv_sec =  msec/1000 ;
        tv.tv_usec = (msec%1000)*1000 ;
        select(1, NULL, NULL, NULL, &tv);
#endif
}

int
poll_descriptors(int n, struct pollarg *args, int msec)
{
int	rc = -1;

    if ( n > 0 ) {

#if	defined(HAVE_POLL) && !defined(LINUX) && !defined(FREEBSD)
	struct	pollfd	pollfd[MAXPOLLFD], *pollptr,
			    *pollfdsaved = NULL, *pfdc;
	struct	pollarg *pa;
	int		i;

	if ( msec < 0 ) msec = -1;
	if ( n > MAXPOLLFD ) {
	    pollfdsaved = pollptr =(struct pollfd*)malloc(n*sizeof(struct pollfd));
	    if ( !pollptr ) return(-1);
	} else
	    pollptr = pollfd;
	/* copy args to poll argument */
	pfdc = pollptr;
memset(pollptr,0, n*sizeof(struct pollfd));
	pa = args;
	for(i=0;i<n;i++) {
	    if ( pa->fd>0)
		pfdc->fd = pa->fd;
	      else
		pfdc->fd = -1;
	    pfdc->revents = 0;
	    if ( pa->request & FD_POLL_RD ) pfdc->events |= POLLIN|POLLHUP;
	    if ( pa->request & FD_POLL_WR ) pfdc->events |= POLLOUT|POLLHUP;
	    if ( !(pfdc->events & (POLLIN|POLLOUT) ) )
		pfdc->fd = -1;
	    pa->answer = 0;
	    pa++;
	    pfdc++;
	}
	//printf("poll now\n");
	rc = poll(pollptr, n, msec);
	if ( rc <= 0 ) {
	    if ( pollfdsaved ) xfree(pollfdsaved);
	    return(rc);
	}
	/* copy results back */
	pfdc = pollptr;
	pa = args;
	for(i=0;i<n;i++) {
	    if ( pfdc->revents & (POLLIN) ) pa->answer  |= FD_POLL_RD;
	    if ( pfdc->revents & (POLLOUT) ) pa->answer |= FD_POLL_WR;
	    if ( pfdc->revents & (POLLHUP|POLLERR) ) pa->answer |= FD_POLL_HU;
	    pa++;
	    pfdc++;
	}
	if ( pollfdsaved ) xfree(pollfdsaved);
	return(rc);
#else
	
	fd_set	rset, wset;
	int	maxfd = 0,i, have_read = 0, have_write = 0;
	struct	pollarg *pa;
	struct	timeval	tv, *tvp = &tv;

//   restart:
	if ( msec >= 0 ) {
	    tv.tv_sec =  msec/1000 ;
	    tv.tv_usec = (msec%1000)*1000 ;
	} else {
	    tvp = NULL;
	}
//	printf("sec=%d,usec=%d\n",tv.tv_sec,tv.tv_usec);
	FD_ZERO(&rset);
	FD_ZERO(&wset);
	pa = args;
	for(i=0;i<n;i++) {
	    if ( pa->request & FD_POLL_RD ) {
			have_read = 1;
			FD_SET(pa->fd, &rset);
			maxfd = MAX(maxfd, pa->fd);
	    }
	    if ( pa->request & FD_POLL_WR ) {
			have_write = 1;
			FD_SET(pa->fd, &wset);
			maxfd = MAX(maxfd, pa->fd);
	    }
	    pa->answer = 0;
	    pa++;
	}

//	printf("select now\n");
	rc = select(maxfd+1,
		    (have_read  ? &rset : NULL),
		    (have_write ? &wset : NULL),
		    NULL, tvp);

	if ( rc <= 0 ) {

#if	defined(FREEBSD)
	    if ( (rc < 0) && (ERRNO == EINTR) )
		goto restart;
#endif	/* FREEBSD */

	    return(rc);
	}
	/* copy results back */
	pa = args;
	for(i=0;i<n;i++) {
	    if ( pa->request & FD_POLL_RD ) {
		/* was request on read */
		if ( FD_ISSET(pa->fd, &rset) )
			pa->answer |= FD_POLL_RD;
	    }
	    if ( pa->request & FD_POLL_WR ) {
		/* was request on write */
		if ( FD_ISSET(pa->fd, &wset) )
			pa->answer |= FD_POLL_WR;
	    }
	    pa++;
	}
	return(rc);
#endif
    } else {
//以下是实现精确sleep用
#if	defined(HAVE_POLL) && !defined(LINUX) && !defined(FREEBSD)
	rc = poll(NULL, 0, msec);
#else
	struct timeval	tv;

//   restart0:
	tv.tv_sec =  msec/1000 ;
	tv.tv_usec = (msec%1000)*1000 ;
	rc = select(1, NULL, NULL, NULL, &tv);
#if	defined(FREEBSD)
	if ( (rc < 0) && (ERRNO == EINTR) )
		goto restart0;
#endif	/* FREEBSD */
#endif

    }
    return(rc);
}

#if	defined(FREEBSD)
/* Under FreeBSD all threads get poll/select interrupted (even in
   threads with signals blocked, so we need version of poll_descriptors
   which can detect interrupts, and version which ignore interrupts
   This function don't ignore and must be called from main thread
   only.
 */
int
poll_descriptors_S(int n, struct pollarg *args, int msec)
{
int	rc = -1;

    if ( n > 0 ) {
	fd_set	rset, wset;
	int	maxfd = 0,i, have_read = 0, have_write = 0;
	struct	pollarg *pa;
	struct timeval	tv, *tvp = &tv;


	if ( msec >= 0 ) {
	    tv.tv_sec =  msec/1000 ;
	    tv.tv_usec = (msec%1000)*1000 ;
	} else {
	    tvp = NULL;
	}
	FD_ZERO(&rset);
	FD_ZERO(&wset);
	pa = args;
	for(i=0;i<n;i++) {
	    if ( pa->request & FD_POLL_RD ) {
		have_read = 1;
		FD_SET(pa->fd, &rset);
		maxfd = MAX(maxfd, pa->fd);
	    }
	    if ( pa->request & FD_POLL_WR ) {
		have_write = 1;
		FD_SET(pa->fd, &wset);
		maxfd = MAX(maxfd, pa->fd);
	    }
	    pa->answer = 0;
	    pa++;
	}

	rc = select(maxfd+1,
		    (have_read  ? &rset : NULL),
		    (have_write ? &wset : NULL),
		    NULL, tvp);

	if ( rc <= 0 )
	    return(rc);
	/* copy results back */
	pa = args;
	for(i=0;i<n;i++) {
	    if ( pa->request & FD_POLL_RD ) {
		/* was request on read */
		if ( FD_ISSET(pa->fd, &rset) )
			pa->answer |= FD_POLL_RD;
	    }
	    if ( pa->request & FD_POLL_WR ) {
		/* was request on write */
		if ( FD_ISSET(pa->fd, &wset) )
			pa->answer |= FD_POLL_WR;
	    }
	    pa++;
	}
	return(rc);
    } else {
	struct timeval	tv;

	tv.tv_sec =  msec/1000 ;
	tv.tv_usec = (msec%1000)*1000 ;
	rc = select(1, NULL, NULL, NULL, &tv);
    }
    return(rc);
}
#endif	/* FREEBSD */

struct av*
lookup_av_by_attr(struct av *avp, char *attr)
{
struct av	*res = NULL;

    if ( !attr ) return(NULL);

    while( avp ) {
	if ( avp->attr && !strncasecmp(avp->attr, attr, strlen(attr)) ) {
	    res = avp;
	    break;
	}
	avp = avp->next;
    }
    return(res);
}

int
put_av_pair(struct av **pairs, char *attr, char *val)
{
struct	av	*new_t = NULL, *next;
char		*new_attr=NULL, *new_val = NULL;

    new_t = (struct	av	*)xmalloc(sizeof(*new_t), "put_av_pair(): for av pair");
    if ( !new_t ) goto failed;
    memset(new_t, 0,sizeof(*new_t));
    new_attr=(char *)xmalloc( strlen(attr)+1, "put_av_pair(): for new_attr" );
    if ( !new_attr ) goto failed;
    strcpy(new_attr, attr);
    new_val=(char *)xmalloc( strlen(val)+1, "put_av_pair(): for new_val" );
    if ( !new_val ) goto failed;
    strcpy(new_val, val);
    new_t->attr = new_attr;
    new_t->val = new_val;
    if ( !*pairs ) {
	*pairs = new_t;
    } else {
	next = *pairs;
	while (next->next) next=next->next;
	next->next=new_t;
    }
    return(0);

failed:
    if ( new_t ) xfree(new_t);
    if ( new_attr ) xfree(new_attr);
    if ( new_val ) xfree(new_val);
    return(1);
}
/*
#undef	malloc

struct	malloc_buf {
	int			state;
	int			size;
	int			current_size;
	void			*data;
	char			*descr;
	struct	malloc_buf	*next;
};
struct	malloc_bucket {
	struct	malloc_buf	*first;
	struct	malloc_buf	*last;
};
*/
#define	BU_FREE	1
#define	BU_BUSY	2

#if	defined(MALLOCDEBUG)
#undef malloc
#undef free
#undef strdup
#undef new
int   malloc_mutex_inited=0;

void
list_all_malloc()
{
	if ( !malloc_mutex_inited ) {
		pthread_mutex_init(&malloc_mutex, NULL);
		malloc_mutex_inited = 1;
    	}
	
	pthread_mutex_lock(&malloc_mutex);
	
//	MEM * tmp=head;
	mem_map_t::iterator it;
	for(it=mem_map.begin();it!=mem_map.end();it++)
		syslog(LOG_ERR,"%s,%d,%x,%d\n",(*it).second->file,(*it).second->line,(*it).second->addr,(*it).second->size);
	syslog(LOG_ERR,"total_block=%d,total_size=%d\n",total_block,total_size);
	pthread_mutex_unlock(&malloc_mutex);
}
void *
xmalloc2(size_t size, const char *file,int line)
{

	if ( !malloc_mutex_inited ) {
		pthread_mutex_init(&malloc_mutex, NULL);
		malloc_mutex_inited = 1;
    }
	if(size<=0){
		syslog(LOG_ERR,"big bug... in %s:%d malloc size=%d\n",file,line,size);
		return NULL;
	}
	MEM *m_this=(MEM *)malloc(sizeof(MEM));
	if(m_this==NULL)
		return NULL;
	memset(m_this,0,sizeof(m_this));
//	m_this->next=NULL;
//	m_this->prev=NULL;
	pthread_mutex_lock(&malloc_mutex);
	m_this->addr=malloc(size);
	if(m_this->addr==NULL){
		free(m_this);
		goto done;
	}
	strncpy(m_this->file,file,sizeof(m_this->file));
	m_this->line=line;
	m_this->size=size;
	total_block++;
	total_size+=size;
	mem_map[(int)(m_this->addr)]=m_this;
//	printf("#malloc m_this=%x %s:%d %x\n",m_this,file,line,m_this->addr);
done:	
	pthread_mutex_unlock(&malloc_mutex);

	return m_this->addr;



}
void
xfree2(void *ptr,const char *file,int line)
{

	if(ptr==NULL)
		return;
	if ( !malloc_mutex_inited ) {
		pthread_mutex_init(&malloc_mutex, NULL);
		malloc_mutex_inited = 1;
    	}
	
	pthread_mutex_lock(&malloc_mutex);
	mem_map_t::iterator it=mem_map.find((int)ptr);
	if(it!=mem_map.end()){
		total_block--;
		total_size-=(*it).second->size;
		free((*it).second);
		mem_map.erase(it);	
	}else{
		printf("########## %s,%d\n",file,line);
	}
done:
	free(ptr);
	pthread_mutex_unlock(&malloc_mutex);
}
char * xstrdup(const char *s,const char * file,int line)
{
	if ( !malloc_mutex_inited ) {
		pthread_mutex_init(&malloc_mutex, NULL);
		malloc_mutex_inited = 1;
 	}

	if(s==NULL){
		return NULL;
	}
	int size=strlen(s);
	MEM *m_this=(MEM *)malloc(sizeof(MEM));
	if(m_this==NULL)
		return NULL;
	memset(m_this,0,sizeof(m_this));
	pthread_mutex_lock(&malloc_mutex);
	m_this->addr=strdup(s);
	if(m_this->addr==NULL){
		free(m_this);
		goto done;
	}
	strncpy(m_this->file,file,sizeof(m_this->file));
	m_this->line=line;
	m_this->size=size;
	total_block++;
	total_size+=size;
	mem_map[(int)(m_this->addr)]=m_this;
done:	
	pthread_mutex_unlock(&malloc_mutex);

	return (char *)(m_this->addr);
}
void * operator new(size_t m_size,const char *file,int line)
{

       return xmalloc2(m_size,file,line);
}
void operator delete(void *p)
{
     xfree2(p,"delete",0);
};
void * operator new[](size_t m_size,const char *file,int line)
{

       return xmalloc2(m_size,file,line);
}
void operator delete[](void *p)
{
     xfree2(p,"delete[]",0);
};


#define malloc(x)	xmalloc2(x,__FILE__,__LINE__)
#define free(x)		xfree2(x,__FILE__,__LINE__)
#define strdup(x)	xstrdup(x,__FILE__,__LINE__)
#define new new(__FILE__, __LINE__)
#define delete delete(__FILE__,__LINE__)

#endif

#define	      BASE64_VALUE_SZ	256
int	      base64_value[BASE64_VALUE_SZ];
unsigned char alphabet[65] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
void
base_64_init(void)
{
int i;

    for (i = 0; i < BASE64_VALUE_SZ; i++)
	base64_value[i] = -1;

    for (i = 0; i < 64; i++)
	base64_value[(int) alphabet[i]] = i;
    base64_value['='] = 0;

}

char*
base64_encode(char *src) {
int		bits, char_count, len;
char		*res, *o_char, *lim, *o_lim;
unsigned char 	c;

    if ( !src ) return(NULL);
    len = strlen(src);
    lim = src+len;

    res = (char *)xmalloc((len*4)/3+5, "base64_encode(): 1");
    if ( !res )
	return(NULL);
    o_char = res;
    o_lim  = res + (len*4)/3 + 1 ;
    char_count = 0;
    bits = 0;
    while ( (src < lim) && (o_char < o_lim) ) {
	c = *(src++);
	bits += c;
	char_count++;
	if (char_count == 3) {
	    *(o_char++) = alphabet[bits >> 18];
	    *(o_char++) = alphabet[(bits >> 12) & 0x3f];
	    *(o_char++) = alphabet[(bits >> 6) & 0x3f];
	    *(o_char++) = alphabet[bits & 0x3f];
	    bits = 0;
	    char_count = 0;
	} else {
	    bits <<= 8;
	}
    }
    if (char_count != 0) {
	bits <<= 16 - (8 * char_count);
	*(o_char++) = alphabet[bits >> 18];
	*(o_char++) = alphabet[(bits >> 12) & 0x3f];
	if (char_count == 1) {
	    *(o_char++) = '=';
	    *(o_char++) = '=';
	} else {
	    *(o_char++) = alphabet[(bits >> 6) & 0x3f];
	    *(o_char++) = '=';
	}
    }
    *(o_char) = 0;
    return(res);
}

char *
base64_decode(char *p)
{
char		*result;
int		j;
unsigned int	k;
int		c, base_result_sz;
long		val;

    if (!p)
	return NULL;

    base_result_sz = strlen(p);
    result = (char *)xmalloc(base_result_sz+1,"base64_decode(): 1");

    val = c = 0;
    for (j = 0; *p && j + 3 < base_result_sz; p++) {
	k = (int) *p % BASE64_VALUE_SZ;
	if (base64_value[k] < 0)
	    continue;
	val <<= 6;
	val += base64_value[k];
	if (++c < 4)
	    continue;
	result[j++] = (char) (val >> 16);
	result[j++] = (val >> 8) & 0xff;
	result[j++] = val & 0xff;
	val = c = 0;
    }
    result[j] = 0;
    return result;
}

void
free_avlist(struct av *av)
{
struct	av *next;

    while ( av ) {
		next = av->next;
		xfree(av->attr);
		xfree(av->val);
		xfree(av);
		av = next;
    }
}

int
insert_header(char *attr, char *val, struct mem_obj *obj)
{
char		tbuf[10], *fmt, *a, *v;
int		size_incr, a_len,v_len;
struct av	*avp, *new_av;

    if ( !obj || !obj->container ) return(1);
    if ( !obj->insertion_point || obj->tail_length<=0 ) return(1);
    if ( obj->tail_length >= sizeof(tbuf) ) return(1);
    if ( !attr || !val ) return(1);

    memcpy(tbuf, obj->container->data + obj->insertion_point, obj->tail_length);
    a_len = strlen(attr);
    v_len = strlen(val);
    fmt = (char *)malloc(2 + a_len + 1 + v_len + 1);
    if ( !fmt ) return(1);
    sprintf(fmt,"\r\n%s %s", attr, val);
    size_incr = strlen(fmt);
    obj->container->used -= obj->tail_length;
    attach_data(fmt, size_incr, obj->container);
    attach_data(tbuf, obj->tail_length, obj->container);
    obj->insertion_point += size_incr;
    obj->size += size_incr;
    xfree(fmt);
    if ( !obj->headers )
	return(0);
    new_av =(struct av	*) malloc(sizeof(*new_av));
    if ( !new_av )
	return(0);
    a = strdup(attr);
    v = strdup(val);
    if ( !a || !v || !new_av ) {
	if (new_av ) xfree(new_av);
	if ( a ) xfree(a);
	if ( v ) xfree(v);
	return(0);
    }
    memset(new_av,0, sizeof(*new_av));
    new_av->attr = a;
    new_av->val  = v;
    avp = obj->headers;
    while( avp && avp->next ) avp = avp->next;
    if ( avp ) avp->next = new_av;
    return(0);
}

char*
fetch_internal_rq_header(struct mem_obj *obj, char *header)
{
struct av	*obj_hdr;
unsigned int	offset = sizeof("X-oops-internal-rq");
    if ( !obj || !obj->headers || !header ) return(NULL);
    obj_hdr = obj->headers;
    while ( obj_hdr ) {
	if ( obj_hdr->attr ) {
	    if (
	         (obj_hdr->attr[0] == 'X') &&
		 (obj_hdr->attr[1] == '-') &&
		 (obj_hdr->attr[2] == 'o') &&
		 (obj_hdr->attr[3] == 'o') && 
		 (strlen(obj_hdr->attr) >= offset) &&
		 (!strcasecmp(obj_hdr->attr+offset, header)) ) {
		return(obj_hdr->val);
	    }
	}
	obj_hdr = obj_hdr->next;
    }
    return(NULL);
}


void
memcpy_to_lower(char *d, char *s, size_t size)
{
    if ( !s || !d || ((signed)size <= 0) ) return;
    while(size) {
	*d = tolower(*s);
	d++;s++;size--;
    }
}
