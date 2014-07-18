/*
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
#ifndef OOPS_H_sldfkjsdlfkjsdlkj3453l2k4jl23k4j2l3k4j23lkj4l23kj4234
#define OOPS_H_sldfkjsdlfkjsdlkj3453l2k4jl23k4j2l3k4j23lkj4l23kj4234
#include <assert.h>
#include <stdlib.h>
#include<list>
#include "mysocket.h"

//#include "kingate.h"
#if	defined(_WIN32)
//#include	"lib/win32/config.h"
//#include	"environment32.h"
#else
#include "config.h"
#include	"environment.h"
#endif
#include "malloc_debug.h"

/*
#if	DB_VERSION_MAJOR >= 3
#undef	USE_INTERNAL_DB_LOCKS
#endif
*/
#define	forever()	for(;;)
/*
#if	defined(REGEX_H)
#include	REGEX_H
#endif
*/
/* libpcre3/libpcre2/libpcre1 backward compatibility: */

#if     !defined(REG_EXTENDED)
#define REG_EXTENDED 0
#endif

#if     !defined(REG_NOSUB)
#define REG_NOSUB 0
#endif

/* :libpcre3/libpcre2/libpcre1 backward compatibility */
#include "forwin32.h"
typedef	struct	hash_entry_ {
    struct  hash_entry_ *next,          /* link between entries	    */
                        *prev;
    struct  hash_row_   *row_back_ptr;  /* pointer to row           */
    int                 ref_count;
    char                flags;
    void                *key;           /* stored   key             */
    void                *data;          /* associated data          */
//    pthread_cond_t      ref_count_cv;   /* changes on ref_count     */
} hash_entry_t;
//#include "llt.h"
//#include "workq.h"

typedef struct	tm	tm_t;

#if	!defined(HAVE_UINT32_T) && !defined(_UINT32_T)
typedef	unsigned	uint32_t;
#endif

#if	!defined(HAVE_UINT16_T)
typedef	unsigned short	uint16_t;
#endif

#if   defined(BSDOS) || defined(LINUX) || defined(FREEBSD) || defined(OSF) || defined(OPENBSD)
#define       flock_t struct flock
#endif


#if	!defined(MAXHOSTNAMELEN)
#define	MAXHOSTNAMELEN	256
#endif

#define	DECREMENT_REFS(obj)	{		lock_obj(obj);		obj->refs--;		unlock_obj(obj);	}
#define	INCREMENT_REFS(obj)	{		lock_obj(obj);		obj->refs++;		unlock_obj(obj);	}

#define	MUST_BREAK	( kill_request | reconfig_request )
#ifdef MALLOCDEBUG
#define	IF_FREE(p)	{if ( p ) xfree2(p,__FILE__,__LINE__);p=NULL;}
#define	IF_STRDUP(d,s)	{if ( d ) xfree2(d,__FILE__,__LINE__); d = xstrdup(s,__FILE__,__LINE__);}
#else
#define	IF_FREE(p)	{if ( p ) free(p);p=NULL;}
#define	IF_STRDUP(d,s)	{if ( d ) free(d); d = strdup(s);}
#endif

#define MID(A)		((40*group->cs0.A + 30*group->cs1.A + 30*group->cs2.A)/100)
#define MID_IP(A)	((40*(A->traffic0) + 30*(A->traffic1) + 30*(A->traffic2))/100)
#if	!defined(ABS)
#define	ABS(x)		((x)>0?(x):(-(x)))
#endif
#define	ROUND(x,b)	((x)%(b)?(((x)/(b)+1)*(b)):(x))

#define	OOPSMAXNS	(5)
#define	MAXBADPORTS	(10)

#define		STORAGE_PAGE_SIZE	((off_t)4096)

#define	CHUNK_SIZE	(64)

#define	ROUND_CHUNKS(s)	((((s) / CHUNK_SIZE) + 1) * CHUNK_SIZE)

#define	HASH_SIZE	(4096)
#define	HASH_MASK	(HASH_SIZE-1)
#define NET_HASH_SIZE	(1024)
#define NET_HASH_MASK	(NET_HASH_SIZE-1)

#define	OOPS_DB_PAGE_SIZE	(4*1024)

#define	METH_GET	0
#define	METH_HEAD	1
#define	METH_POST	2
#define	METH_PUT	3
#define	METH_CONNECT	4
#define	METH_TRACE	5
#define	METH_PROPFIND	6
#define	METH_PROPPATCH	7
#define	METH_DELETE	8
#define	METH_MKCOL	9
#define	METH_COPY	10
#define	METH_MOVE	11
#define	METH_LOCK	12
#define	METH_UNLOCK	13
#define	METH_PURGE	14
#define	METH_OPTIONS	15

#define	AND_PUT		1
#define	AND_USE		2
#define	PUT_NEW_ANYWAY	4
#define	NO_DISK_LOOKUP	8
#define	READY_ONLY	16

#define	OBJ_EMPTY	0
#define	OBJ_INPROGR	2
#define	OBJ_READY	3

#define	FLAG_DEAD		1	/* obj is unusable anymore	*/
#define	FLAG_IN_MEM		(1<<1)	/* obj in mem		*/
#define	ANSW_HAS_EXPIRES	(1<<2)
#define	ANSW_NO_CACHE		(1<<3)
#define	ANSW_NO_STORE		(1<<4)
#define	ANSW_HAS_MAX_AGE	(1<<5)
#define	ANSW_MUST_REVALIDATE	(1<<6)
#define	ANSW_PROXY_REVALIDATE	(1<<7)
#define	ANSW_LAST_MODIFIED	(1<<8)
#define	ANSW_SHORT_CONTAINER	(1<<9)
#define	ANSW_KEEP_ALIVE		(1<<10)
#define	ANSW_HDR_CHANGED	(1<<11)	/* if some server headers was changed		*/
#define	ANSW_EXPIRES_ALTERED	(1<<12)	/* if expires was altered because of refr_patt	*/
#define	FLAG_CREATING		(1<<13)
#define FLAG_DESTROYED		(1<<14)
#define FLAG_IN_DISK		(1<<15)/* obj in dist */
#define KEY_CHECKED		(1<<16)

#define	STATUS_OK				200
#define	STATUS_NOT_MODIFIED		304
#define STATUS_FORBIDEN			403
#define STATUS_GATEWAY_TIMEOUT  504
#define STATUS_MOVED			301

#define	RQ_HAS_CONTENT_LEN	1
#define	RQ_HAS_IF_MOD_SINCE	(1<<1)
#define	RQ_HAS_NO_STORE		(1<<2)
#define	RQ_HAS_NO_CACHE		(1<<3)
#define	RQ_HAS_MAX_AGE		(1<<4)
#define	RQ_HAS_MAX_STALE	(1<<5)
#define	RQ_HAS_MIN_FRESH	(1<<6)
#define	RQ_HAS_NO_TRANSFORM	(1<<7)
#define	RQ_HAS_ONLY_IF_CACHED	(1<<8)
#define	RQ_HAS_AUTHORIZATION	(1<<9)
#define RQ_GO_DIRECT		(1<<10)
#define	RQ_HAS_KEEP_CONNECTION 	(1<<11)
#define	RQ_HAS_BANDWIDTH	(1<<12)
#define	RQ_CONVERT_FROM_CHUNKED (1<<13)
#define RQ_NO_ICP		(1<<15)
#define RQ_FORCE_DIRECT		(1<<16)
#define RQ_HAS_HOST		(1<<17)
#define RQ_HAVE_RANGE		(1<<18)
#define RQ_HAVE_PER_IP_BW	(1<<19)
#define	RQ_CONVERT_FROM_GZIPPED (1<<20)
#define	RQ_SERVED_DIRECT        (1<<21)
#define RQ_HAS_COOKIE		(1<<22)
#define RQ_USE_PROXY		(1<<23)
#define RESULT_CLOSE		(1<<24)
#define RESULT_CLIENT_ERR	(1<<25)
#define RQ_NO_FILTER		(1<<26)

#define	DOWNGRADE_ANSWER	1
#define	UNCHUNK_ANSWER		2
#define	UNGZIP_ANSWER		4

#define	ACCESS_DOMAIN		1
#define ACCESS_PORT		2
#define	ACCESS_METHOD		3

#define	MEM_OBJ_MUST_REVALIDATE	1
#define	MEM_OBJ_WARNING_110	2
#define	MEM_OBJ_WARNING_113	4

#define	CRLF			"\r\n"

#define	ANSW_SIZE		(2*1024)
#define	READ_ANSW_TIMEOUT	(1*60)		/* 1 minutes	*/

#define	DEFAULT_EXPIRE_VALUE	(7*24*3600)	/* 7 days	*/
#define	DEFAULT_EXPIRE_INTERVAL	(1*3600)	/* each hour	*/
#define	FTP_EXPIRE_VALUE	(7*24*3600)	/* expire for ftp */

#define	DEFAULT_LOW_FREE	(5)		/* these values for BIG storages */
#define	DEFAULT_HI_FREE		(6)
#define	DEFAULT_MAXRESIDENT	(1024*1024)	/* 1MB		*/
#define	DEFAULT_DNS_TTL		(30*60)		/* 30 min's	*/
#define	DEFAULT_ICP_TIMEOUT	(1000000)	/* 1 sec	*/

#define	RESERVED_FD		(20)		/* reserve low number file descriptors */

#define	ADDR_AGE		(3600)

#define	DECODING_BUF_SZ		(1024)

#define	ERR_BAD_URL		1
#define	ERR_BAD_PORT		2
#define	ERR_ACC_DOMAIN		3
#define	ERR_DNS_ERR		4
#define	ERR_INTERNAL		5
#define	ERR_ACC_DENIED		6
#define	ERR_TRANSFER		7
#define	ERR_ACL_DENIED		8

#define	OOPS_LOG_STOR		1
#define	OOPS_LOG_FTP		2
#define	OOPS_LOG_HTTP		4
#define	OOPS_LOG_DNS		8
#define	OOPS_LOG_DBG		16
#define	OOPS_LOG_PRINT		32
#define	OOPS_LOG_INFORM		4096
#define	OOPS_LOG_NOTICE		8192
#define	OOPS_LOG_SEVERE		16384
#define	OOPS_LOG_CACHE		(OOPS_LOG_SEVERE*2)

#define MAX_DOC_HDR_SIZE    (32*1024)

#define	SET(a,b)		(a|=(b))
#define	CLR(a,b)		(a&=~b)
#define	TEST(a,b)		((a)&(b))




struct	url {
    	char		*proto;
	char		*host;
	unsigned long 	dst_ip;//connect to host ip
	u_short		dst_port;//connect to host port
    	u_short		port;
    	char		*path;
    	char		*httpv;
	char		*login;		/* for non-anonymous */
    	char		*password;	/* FTP sessions	     */
};

struct	obj_times {
	time_t			date;		/* from the server answer	*/
	time_t			expires;	/* from the server answer	*/
	int				age;		/* Age: from stored answer	*/
	int				max_age;	/* max-age: from server		*/
	time_t			last_modified;	/* Last-Modified: from server	*/
};

struct	buff {
	struct	buff	*next;
	uint32_t	used;		/* size of stored data			*/
					/* this changes as we add more data	*/
	uint32_t	curr_size;	/* size of the buffer itself		*/
					/* this grows as we expand buffer	*/
	char		*data;
};

struct	refresh_pattern	{
	struct  refresh_pattern	*next;	/* if we link them in list	*/
				int	min;
				int	lmt;
				int	max;
				int	named_acl_index;
				int	valid;
};
typedef	struct  refresh_pattern refresh_pattern_t;


#define	MAXACLNAMELEN		32

#define	PROTO_HTTP	0
#define	PROTO_FTP	1
#define	PROTO_OTHER	2
#define PROTO_HTTPS 3
struct	request {
	//struct			sockaddr_in client_sa;
	//struct			sockaddr_in my_sa;
	time_t			request_time;	/* time of request creation	*/
	int			state;
	int			http_major;
	int			http_minor;
	int			meth;
	int			connection;
	char			*method;
	struct	url		url;
	char			proto;
	int			headers_off;
	int			flags;
	int			content_length;
	int			leave_to_read;
	time_t			if_modified_since;
	int			max_age;
	int			max_stale;
	int			min_fresh;
	struct	av		*av_pairs;
	struct	buff		*data;		/* for POST				*/
	char 			*referer;
//        l_mod_call_list_t       *redir_mods;	/* redir modules			*/
//	refresh_pattern_t	refresh_pattern;/* result of refresh_pattern		*/
//	char			*original_host;	/* original value of Host: if redir-ed	*/
//	char			src_charset[16];
//	char			dst_charset[16];
//	struct	l_string_list	*cs_to_server_table;
//	struct	l_string_list	*cs_to_client_table;
//	char			*matched_acl;
//	int			accepted_so;	/* socket where was accept-ed		*/
//	char			*source;	/* content_source (for access_log)	*/
//	char			*tag;		/* HIT/MISS/... (for access_log)	*/
//	char			*hierarchy;	/* hierarchy				*/
//	char			*c_type;
//	int			code;		/* code 				*/
//	int			received;
//	char			*proxy_user;	/* if proxy-auth used			*/
//	char			*original_path;	/* original path			*/
	int			range_from;
	int			range_to;
//	char			*peer_auth;
//	int			sess_bw;	/* session bandwidth			*/
//	int			per_ip_bw;	/* per ip bw				*/
//	time_t			last_writing;	/* second of last_writing		*/
//	int			s0_sent;	/* data size sent during last second	*/
//	int			so;		/* socket				*/
//	struct	sockaddr_in	conn_from_sa;	/* connect from address			*/
//	struct	request		*next;		/* next in hash				*/
//	struct	request		*prev;		/* prev in hash				*/
//	int			doc_size;	/* corr. document size			*/
//	int			doc_received;
//	int			doc_sent;
//	ip_hash_entry_t		*ip_hash_ptr;
//	char			*decoding_buff;	/* for inflate or any other content decoding */
//	char			*decoded_beg, *decoded_end;
/*
#if	defined(HAVE_ZLIB)
	z_streamp		strmp;
	z_stream		strm;
	char			inflate_started;
#endif
*/
	mysocket * server;
	mysocket * client;
//	unsigned dst_ip;
};

/*
struct	rq_hash_entry {
	pthread_mutex_t	lock;
	struct	request	*link;
};
*/
#define	_MINUTE_	(60)
#define	_HOUR_		(3600)
#define	_DAY_		(24*_HOUR_)
#define	_WEEK_		(7*_DAY_)
#define	_MONTH_		(4*_WEEK_)
/*
typedef	struct	hg_entry_ {
	int	from, to;
	int	sum;
} hg_entry;
*/
struct	av {
	char		*attr;
	char		*val;
	struct	av	*next;
};

#define	HTTP_DOC	0
#define	FTP_DOC		1
typedef struct _obj_list
{
	void * obj;
	struct _obj_list *prev;
	struct _obj_list * next;
} obj_list;
/*
struct disk_obj {
	struct disk_obj *next;
	struct disk_obj *prev;
	struct disk_hash_entry *hash_back;
	size_t size;
	struct url url;
	
};
*/
struct	mem_obj {
	int			httpv_major;
	int			httpv_minor;
	int			state;
	int			flags;
	int			file_name;	//file name save to disk
	int			status_code;	/* from the server nswer	*/
	int			insertion_point;/* where to insert additional headers	*/
	int			tail_length;	/* length of \n\n or \r\n\r\n et al.	*/
	size_t			size;		/* current data size		*/
	size_t			content_length;	/* what server say about content */
	size_t			resident_size;	/* size of object in memory	*/

	time_t			created;	/* when created in memory	*/
	time_t			last_access;	/* last time locate finished ot this object	*/	
	
	struct	obj_times	times;
	////////////////////////////////////////////////
	obj_list		m_list;
	struct	mem_obj		*next;		/* in hash			*/
	struct	mem_obj		*prev;		/* in hash			*/
	struct	obj_hash_entry	*hash_back;	/* back pointer to hash chain	*/
	struct	buff		*container;	/* data storage			*/
	struct	buff		*hot_buff;	/* buf to write			*/
	pthread_mutex_t		lock;		/* for refs and obj as whole	*/
	int			refs;		/* references			*/
	struct	url		url;		/* url				*/
	struct	av		*headers;	/* headers */
};
const int mem_obj_len=8*sizeof(int)+3*sizeof(size_t)+2*sizeof(time_t)+sizeof(struct  obj_times);
/*
#define	MAX_INTERNAL_NAME_LEN	24
typedef struct internal_doc_tag {
	char		internal_name[MAX_INTERNAL_NAME_LEN];
	char		*content_type;
	int		content_len;
	int		expire_shift;
	unsigned char	*body;
} internal_doc_t;

struct	output_object {
	struct	av	*headers;
	struct	buff	*body;
	int		flags;
};
*/
struct	obj_hash_entry {
	struct	mem_obj		*next;
	pthread_mutex_t		lock;
	int			size;		/* size of objects in this hash */
	int			disk_size;
	pthread_mutex_t		size_lock;	/* lock to change size		*/
};
/*
struct disk_hash_entry {
	struct disk_obj 	*next;
	pthread_mutex_t		lock;
	int 			size;
	pthread_mutex_t		size_lock;
};
*/
struct v_ip
{
//	unsigned ip;
	int start_time;
};
struct net_obj
{
	int refs;
	int last_access;
	std::string url;
	std::list<v_ip> ips;
	net_obj *next;
	net_obj *prev;
};
struct net_obj_entry {
	struct net_obj 		*next;
	KMutex			lock;
};
struct	server_answ {
#define	GOT_HDR	(1)
	int			state;
	int			flags;
	size_t			content_len;
	size_t			x_content_length;
	int			checked;
	int			status_code;
	struct	obj_times	times;
	struct	av		*headers;
	time_t			response_time;	/* these times can be filled	*/
	time_t			request_time;	/* when we load obj from disk	*/
	int			httpv_major;
	int			httpv_minor;
	int			http_result;
};

struct	ftp_r {
	int		control;	/* control socket	*/
	int		data;		/* data socket		*/
	int		client;		/* client socket	*/
	uint32_t	size;		/* result of 'SIZE' cmd */
#define	MODE_PASV	0
#define	MODE_PORT	1
	int		mode;		/* PASV or PORT		*/
	struct	request	*request;	/* referer from orig	*/
	struct	mem_obj	*obj;		/* object		*/
	char		*dehtml_path;	/* de-hmlized path	*/
	char		*server_path;	/* path as server report*/
	struct	buff	*server_log;	/* ftp server answers	*/
	struct	string_list *nlst;	/* NLST results		*/
	uint32_t	received;	/* how much data received */
	char		*type;		/* mime type		*/
#define	FTP_TYPE_DIR	1
#define	FTP_TYPE_FILE	2
	int		file_dir;	/* file or dir		*/
	struct	buff	*container;
#define	PARTIAL_ANSWER	1
	int		ftp_r_flags;
};
/*
struct	domain_list	{
	char				*domain;
	int				length;
	struct	domain_list		*next;
};
*/
#define	ACL_URLREGEX        1
#define	ACL_PATHREGEX       2
#define	ACL_URLREGEXI       3
#define	ACL_PATHREGEXI      4
#define	ACL_USERCHARSET     5
#define	ACL_SRC_IP          6
#define	ACL_METHOD          7
#define	ACL_PORT            8
#define	ACL_DSTDOM          9
#define	ACL_DSTDOMREGEX     10
#define	ACL_SRCDOM          11
#define	ACL_SRCDOMREGEX     12
#define	ACL_TIME            13
#define	ACL_CONTENT_TYPE	14
#define	ACL_USERNAME        15
#define ACL_HEADER_SUBSTR   16
/*
struct	domain {
	char			*dom;
	struct	domain		*next;
};

#define	MAX_DNS_ANSWERS	(15)

struct	dns_hash_head {
	struct dns_cache	*first;
	struct dns_cache	*last;
};
struct	dns_cache_item {
	time_t		time;		/* when created or become bad	* /
	char		good;		/* good or bad			* /
	struct	in_addr	address;	/* address itself		* /
};
struct	dns_cache {
	struct dns_cache *next;		/* link				* /
	time_t		time;		/* when filled			* /
	int		stamp;		/* to speed-up search		* /
	char		*name;		/* host name			* /
	short		nitems;		/* how much answers here	* /
	short		nlast;		/* last answered		* /
	short		ngood;		/* how much good entries here	* /
	struct dns_cache_item *items;
};
#define	SOURCE_DIRECT	0
#define	PEER_PARENT	1
#define	PEER_SIBLING	2

#define	PEER_UP		0
#define	PEER_DOWN	1

struct  charset {
	struct  charset		*next;
	char			*Name;
	struct  string_list	*CharsetAgent;
	u_char			*Table;
};
*/
#define	MAXPOLLFD	(8)
#define	FD_POLL_RD	(1)
#define	FD_POLL_WR	(2)
#define	FD_POLL_HU	(4)
#define	IS_READABLE(a)	(((a)->answer)&FD_POLL_RD)
#define	IS_WRITEABLE(a)	(((a)->answer)&FD_POLL_WR)
#define	IS_HUPED(a)	(((a)->answer)&FD_POLL_HU)

struct	pollarg {
	int	fd;
	short	request;
	short	answer;
};

#define        IS_SPACE(a)     isspace((unsigned)a)
#define        IS_DIGIT(a)     isdigit((unsigned)a)

#define	ERRBUF		char	errbuf[256]
#define	ERRBUFS		errbuf, sizeof(errbuf)-1

//#include	"dataq.h"

#define	FILEBUFFSZ	(16*1024)
/*
typedef	struct	filebuff_ {
	int		fd;
	int		buffered;
	pthread_mutex_t	lock;
	struct	buff	*buff;
	//dataq_t		queue;
} filebuff_t;
*/

#define	DB_API_RES_CODE_OK		0
#define	DB_API_RES_CODE_ERR		1
#define	DB_API_RES_CODE_NOTFOUND	2
#define	DB_API_RES_CODE_EXIST		3

#define	DB_API_CURSOR_NORMAL		0
#define	DB_API_CURSOR_CHECKDISK		1
/*
typedef	struct	db_api_arg_ {
	void	*data;
	size_t	size;
	int	flags;
} db_api_arg_t;

typedef	struct	eraser_data_ {
	char	*url;
	void	*disk_ref;
} eraser_data_t;
*/
#include	"extern.h"

/*
 typedef struct  icp_job_tag {
        struct  sockaddr_in     my_icp_sa;
        struct  sockaddr_in     icp_sa;
        int                     icp_so;
        char                    *icp_buf;
        int                     icp_buf_len;
} icp_job_t;
*/
#include "lib.h"




void	release_obj(struct mem_obj *);
u_short	hash(struct url *url);
int	 	pump_data(int so, int server_so);
int		poll_descriptors(int, struct pollarg*, int);
int		writet(int, char*, int, int);
void		my_sleep(int);


#endif
