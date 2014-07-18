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
#include "kingate.h"
#ifndef DISABLE_HTTP
#include<time.h>
#ifdef HAVE_SSTREAM
#include <sstream>
#else
#include "mysstream"
#endif
#include<map>
#include<string>
#include<list>
#include	"oops.h"

//#include	"modules.h"
#include	"mysocket.h"
#include	"utils.h"
#include	"http.h"
#include	"cache.h"
#include	"allow_connect.h"
#include	"log.h"
#include 	"KHttpManage.h"
#include	"malloc_debug.h"

//extern struct	err_module	*err_first;

#define		READ_REQ_TIMEOUT	(10*60)		/* 10 minutes */
#define		READ_BUFF_SZ		(2048)

#define		REQUEST_EMPTY	0
#define		REQUEST_READY	1
#define 	MIN_SLEEP_TIME	3
static	int	in_stop_cache(struct request *);
static	int	parse_connect_url(char* src, char *httpv, struct url *url,struct request *rq);
static	int	parse_http_request(char *start, struct request *rq);


inline	static	int	add_request_av(char* avtext, struct request *request);
inline	static	int	check_headers(struct request *rq, char *beg, char *end, int *checked);
inline	static	void	free_request(struct request *rq);
inline  void	https_start(request *rq);
inline  bool	http_start(request *rq);
bool	manage_start(request *rq);
using namespace std;
/*
KMutex net_obj_lock;
typedef map<string,net_obj *> net_obj_t;
net_obj_t m_net_obj;
*/
struct net_obj_entry net_table[NET_HASH_SIZE];
bool get_url(url *m_url,string &url_str)
{
        stringstream s;
        char *file=NULL;
	int file_len;
        if(m_url->path==NULL)
                return false;
        file=strdup(m_url->path);
	if(file==NULL)
		return false;
        int i,j;
	file_len=strlen(file);
        for(i=0;i<file_len;i++){
                if(file[i]=='?'){
                        file[i]=0;
                        break;
                }
        }
	file_len=strlen(file);
	bool isafurl;
	char *tmp;
	for(i=0;i<conf.afurls.size();i++){
		tmp=conf.afurls[i];
		if(tmp==NULL)
			continue;
		//printf("tmp=%s...file=%s..\n",tmp,file);
		int tmp_len=strlen(tmp);
		if(file_len<tmp_len){
			continue;
		}
		isafurl=true;
		for(j=1;j<=tmp_len;j++){
			if(tolower(file[file_len-j])!=tmp[tmp_len-j]){
				isafurl=false;
			//	printf("file[file_len-j]=%c,tmp[tmp_len-j]=%c\n",file[file_len-j],tmp[tmp_len-j]);
				break;
			}
				
		}
		if(isafurl){
		  	s << m_url->host << file;
			s.str().swap(url_str);
			free(file);
			return true;
		}
	}/*
        if((file[i-1]=='/' && len>0) || (len>3 && (strcmp((file+(i-3)),"php")==0))){
                s << m_url->host << file;
                s.str().swap(url_str);
                free(file);
                return true;
        }else{
	*/
	free(file);
	return false;
       //	 }
}
int run_client(SERVER * work)
{
char			*buf = NULL;
int			got;//, rc;
char			*cp, *ip;//,*nb;
char			*headers;
struct	request		rq;
time_t			started;
//struct	mem_obj		*stored_url;
size_t			current_size;
int			status, checked_len = 0;//, mod_flags;
int i=0;
mysocket client;
mysocket * server=work->server;
int model=work->model;
assert(work);
	server->set_title_msg("wait");
	buf = (char *)malloc(READ_BUFF_SZ+1);// "run_client(): For client request.");	
	for(;;){
		memset(&rq,0,sizeof(rq));
		if(work->client!=NULL){
			SET(rq.flags,RESULT_CLOSE);
		}
		rq.server=server;
		rq.client=&client;
		rq.request_time = started = time(NULL);
		checked_len=0;
		if ( !buf ) {
			klog(ERR_LOG,"run_client(): No mem for header!\n");
			goto done;
		}
		current_size = READ_BUFF_SZ;
		cp = buf; 
		ip = buf;
		forever() {
			got=server->recv2(cp,current_size-(cp-ip));
			if(got<=0){
		//		printf("client closed the connection.\n");
				goto client_close;
			}
		//	cp[got]=0;
		//	printf("recv %d bytes :\n%s",got,cp);
			cp += got;
			if ( (unsigned)(cp - ip) >= current_size ) {
				char *nb = (char *)malloc(current_size+CHUNK_SIZE+1);
				/* resize buf */
				if ( !nb ) {
					klog(ERR_LOG, "run_client(): No mem to read request.\n");
					goto done;
				}	    
				memcpy(nb, buf, current_size);
				free(buf);
				buf=ip=nb;
				cp=ip+current_size;
				*cp=0;
				current_size=current_size+CHUNK_SIZE;
				/*
				say_bad_request( "Bad request format,the http head is too large\n", "",
				ERR_BAD_URL, &rq);
				goto client_close;
				*/
			} else
				*cp=0;
			status = check_headers(&rq,ip,cp, &checked_len);
			if ( status) {
				klog(ERR_LOG, "run_client(): Failed to check headers.\n");
				say_bad_request( "Bad request format.\n", "",
					ERR_BAD_URL, &rq);
				goto client_close;
			}
			if ( rq.state == REQUEST_READY )
				break;
		}
		if(rq.url.host==NULL){
			klog(ERR_LOG, "run_client(): Failed to check headers.\n");
			say_bad_request( "Bad request format.\n", "",
				ERR_BAD_URL, &rq);
			goto client_close;
		}

		if ( rq.headers_off <= 0 ) {
			klog(ERR_LOG, "run_client(): Something wrong with headers_off: %d\n", rq.headers_off);
			goto client_close;
		}
		#ifdef MALLOCDEBUG
		if(strcmp(rq.url.host,"www.kingate_clean.net")==0){
			conf.mem_min_cache=0;
			conf.mem_max_cache=0;
			conf.disk_min_cache=0;
			conf.disk_max_cache=0;
			list_all=1;
			say_bad_request("现在将cache清空，请注意", "",ERR_BAD_URL, &rq);
			goto client_close;
		}
		#endif
		if( (work->model==MANAGE) || (strcmp(server->get_self_name(),rq.url.host)==0 && conf.port[MANAGE]==rq.url.port)){
			model=MANAGE;
		}else{
			model=HTTP;
		}
		if(allow_connect(model,server,(conf.http_accelerate?NULL:rq.url.host),rq.url.port,rq.url.dst_ip)==DENY){
			stringstream s;	
			s << "Ruler don't allow you to access this host <font color=red>" << rq.url.host << ":" << rq.url.port << "</font>,maybe you haven't login kingate.<br><a href=\"http://" << server->get_self_name() << ":" << conf.port[MANAGE] << "\">click here to login</a>\n";		
			say_bad_request((char *)s.str().c_str(), "",ERR_BAD_URL, &rq);
			goto client_close;
		}
		if(model==MANAGE){
			if((!manage_start(&rq))){
				goto client_close;
			}
			goto done;
		}
		headers = (char*)buf + rq.headers_off;
		if ( rq.url.proto && !strcasecmp(rq.url.proto, "http") ){
			rq.proto = PROTO_HTTP;
		}else{
			if ( rq.url.proto && !strcasecmp(rq.url.proto, "ftp") )
				rq.proto = PROTO_FTP;
			else{
				rq.proto = PROTO_OTHER;
			}
		}
		if(rq.meth==METH_CONNECT)
			rq.proto=PROTO_HTTPS;
		rq.url.dst_port=rq.url.port;
		if(rq.proto==PROTO_HTTP){
			for(i=0;i<conf.m_http_redirects.size();i++){// check http redirect
			  	if(MatchIPModel(conf.m_http_redirects[i].dst,rq.url.dst_ip,rq.url.port) &&
		    	 	( (conf.m_http_redirects[i].file_ext==NULL) || (CheckFileModel(&rq.url,conf.m_http_redirects[i].file_ext,conf.m_http_redirects[i].flag)))){
		   	 	int index=rand()%conf.m_http_redirects[i].hosts.size();
			    	rq.url.dst_ip=conf.m_http_redirects[i].hosts[index].ip;
		    		rq.url.dst_port=conf.m_http_redirects[i].hosts[index].port;
                   //	 	if(TEST(conf.m_http_redirects[i].flag,USE_PROXY))
		     // 			SET(rq.flags,RQ_USE_PROXY);
		    //		else
		//			rq.url.port=rq.url.dst_port;    
				SET(rq.flags,RQ_USE_PROXY);
				#ifdef CONTENT_FILTER
				if(TEST(conf.m_http_redirects[i].flag,NO_FILTER)){
					SET(rq.flags,RQ_NO_FILTER);
				}
				#endif
/*
		   	 	if(TEST(conf.m_http_redirects[i].flag,USE_LOG))
		      			klog(MODEL_LOG,"[HTTP]%s:%d request http://%s:%d%s redirect to %s:%d\n",rq.server->get_remote_name(),rq.server->get_remote_port(),rq.url.host,rq.url.port,rq.url.path,make_ip(conf.m_http_redirects[i].hosts[index].ip).c_str(),conf.m_http_redirects[i].hosts[index].port);
				*/
		    		break;
		  		}
		    	} 
		}
		if(rq.url.dst_ip==0){
			stringstream s;
			s << "Bad dns name.(" << rq.url.host << ")";
			say_bad_request((char *)s.str().c_str(),"",ERR_BAD_URL,&rq);
			goto client_close;
		}
		if(conf.http_accelerate && rq.proto!=PROTO_HTTP){
			say_bad_request("kingate do not accept protocols except http","",ERR_BAD_URL,&rq);
			goto client_close;

		}
		switch(rq.proto){
			case PROTO_HTTP:					
				if(!http_start(&rq)){
			//		printf("server close the connection now\n");
					goto client_close;
				}
				break;
			case PROTO_HTTPS:
				https_start(&rq);
				goto client_close;
			case PROTO_FTP:
				http_ftp_start(&rq);
				goto client_close;
			default:
			//	printf("proto error\n");
				goto client_close;
		}	
	done:
		if(!TEST(rq.flags,RQ_HAS_KEEP_CONNECTION)){
	//		printf("client request haven't keep connection.\n");
			goto client_close;
		}
		free_request(&rq);
		
	}
client_close:
	free_request(&rq);
	IF_FREE(buf);
	return 1;
}


void
destroy_obj(struct mem_obj *obj,int in_hash)
{
        if(TEST(obj->flags,FLAG_IN_DISK)){
		u_short url_hash = hash(&obj->url);
             	string name=get_disk_cache_file(obj);
             	if(unlink(name.c_str())!=0){
				klog(ERR_LOG,"Error!cann't delete file %s errno=%d\n",name.c_str(),errno);
		}
             	decrease_hash_size(&hash_table[url_hash], obj->resident_size,false);
        }
	if(in_hash){
		if ( obj->prev )
			obj->prev->next=obj->next;
		else
			obj->hash_back->next=obj->next;
		if ( obj->next ) 
			obj->next->prev=obj->prev; 
		if(TEST(obj->flags,FLAG_IN_MEM))
			decrease_hash_size(obj->hash_back, obj->resident_size);	
	}
    	free_url(&obj->url);
    	free_container(obj->container);
    	free_avlist(obj->headers);
    	pthread_mutex_destroy(&obj->lock);
 	xfree(obj);
	
}
struct mem_obj * locate_in_mem(struct url *url, int flags, int *new_object, struct request *rq)
{
struct	mem_obj	*obj=NULL;
//struct mem_obj *obj_tmp=NULL;
u_short 	url_hash = hash(url);
int		found=0;//, mod_flags = 0;
	//printf("call locate in mem\n");
    if ( new_object ) *new_object = FALSE;
    if ( pthread_mutex_lock(&hash_table[url_hash].lock) ) {
		klog(ERR_LOG, "locate_in_mem(): Failed mutex lock\n");
		return(NULL);
    }
	obj=hash_table[url_hash].next;
	if ( !(flags & PUT_NEW_ANYWAY) ) {
		while(obj) {
		 if ( (url->port==obj->url.port) &&
	         !strcmp(url->path, obj->url.path) &&
	         !strcasecmp(url->host, obj->url.host) &&
	         !strcasecmp(url->proto, obj->url.proto) &&
	         !(obj->flags & (FLAG_DEAD))
	         ) {

				found=1;

			//	if (  flags & AND_USE ) {
				if ( pthread_mutex_lock(&obj->lock) ) {	//锁定失败					
					pthread_mutex_unlock(&hash_table[url_hash].lock);
	/*			    pthread_mutex_unlock(&obj_chain);*/
					return(NULL);
				}
				obj->refs++;
		/*		if(TEST(obj->flags,FLAG_DEAD)){//如果正在destroy
					pthread_mutex_unlock(&obj->lock);
					return NULL;
				}
			*/	pthread_mutex_unlock(&obj->lock);
			//	}
		//		obj->last_access = global_sec_timer;
	//			obj->accessed += 1;
	//			obj_rate(obj);
				pthread_mutex_unlock(&hash_table[url_hash].lock);
	//			obj_rate(obj);
	/*		    pthread_mutex_unlock(&obj_chain);*/
				return(obj);
			}
			obj=obj->next;
		}
	}
	pthread_mutex_unlock(&hash_table[url_hash].lock);

	if ( !found /*&& ( flags & AND_PUT ) */) {
		/* need to insert */
		obj = (struct mem_obj *)xmalloc(sizeof(struct mem_obj), "locate_in_mem(): for object");
		if(obj==NULL)
			return NULL;
		memset(obj,0,sizeof(struct mem_obj));
//		bzero(obj, sizeof(struct mem_obj));
		obj->m_list.obj=(void *)obj;
		obj->created = global_sec_timer;
		obj->last_access = time(NULL);
//		    obj->accessed = 1;
	//    obj->rate = 0;
		/* copy url */
		SET(obj->flags,FLAG_IN_MEM);
		obj->url.port = url->port;			
		obj->url.proto = (char *)xmalloc(strlen(url->proto)+1, "locate_in_mem(): for obj->url.proto");
		if ( obj->url.proto ) {
			strcpy(obj->url.proto, url->proto);
		} else {
			xfree(obj);
			obj = NULL;
			goto done;
		}
		obj->url.host = (char *)xmalloc(strlen(url->host)+1, "locate_in_mem(): for obj->url.host");
		if ( obj->url.host ) {
			strcpy(obj->url.host, url->host);
		} else {
			xfree(obj->url.proto);
			xfree(obj); obj = NULL;
			goto done;
		}
		obj->url.path = (char *)xmalloc(strlen(url->path)+1, "locate_in_mem(): for obj->url.path");
		if ( obj->url.path ) {
			strcpy(obj->url.path, url->path);
		} else {
			xfree(obj->url.proto);
			xfree(obj->url.host);
			xfree(obj); obj = NULL;
			goto done;
		}
		obj->url.httpv = (char *)xmalloc(strlen(url->httpv)+1, "locate_in_mem(): locate_in_mem4");
		if ( obj->url.httpv ) {
			strcpy(obj->url.httpv, url->httpv);
		} else {
			xfree(obj->url.proto);
			xfree(obj->url.host);
			xfree(obj->url.path);
			xfree(obj); obj = NULL;
			goto done;
		}
		found = 1;
		pthread_mutex_init(&obj->lock, NULL);
	//	pthread_mutex_init(&obj->state_lock, NULL);
//		pthread_cond_init(&obj->state_cond, NULL);
	//	pthread_mutex_init(&obj->decision_lock, NULL);
//		pthread_cond_init(&obj->decision_cond, NULL);
		if ( new_object ) *new_object = TRUE;
	//	pthread_mutex_unlock(&hash_table[url_hash].lock);
	 
		return(obj);
	}
	
done:
//    pthread_mutex_unlock(&hash_table[url_hash].lock);
/*    pthread_mutex_unlock(&obj_chain);*/
    return(obj);
}



inline
static int
add_request_av(char* avtext, struct request *request)
{
struct	av	*new_t=NULL, *next;
char		*attr=avtext, *sp=avtext, *val,holder;
char		*new_attr=NULL, *new_val=NULL;

    while( *sp && !IS_SPACE(*sp) && (*sp != ':') ) sp++;
    if ( !*sp ) {
	my_xlog(OOPS_LOG_SEVERE, "add_request_av(): Invalid request string: %s\n", avtext);
	return(-1);
    }
    if ( *sp ==':' ) sp++;
    holder = *sp;
    *sp = 0;
    new_t = (struct av *)xmalloc(sizeof(*new_t), "add_request_av(): for av pair");
    if ( !new_t ) goto failed;
    new_attr=(char *)xmalloc( strlen(attr)+1, "add_request_av(): for new_attr" );
    if ( !new_attr ) goto failed;
    strcpy(new_attr, attr);
    *sp = holder;
    val = sp; while( *val && IS_SPACE(*val) ) val++;
    if ( !*val ) goto failed;
    new_val = (char *)xmalloc( strlen(val) + 1, "add_request_av(): for val");
    if ( !new_val ) goto failed;
    strcpy(new_val, val);
    new_t->attr = new_attr;
    new_t->val  = new_val;
    new_t->next = NULL;
    if ( !request->av_pairs ) {
		request->av_pairs = new_t;
    } else {
		next = request->av_pairs;
		while (next->next) next=next->next;
		next->next=new_t;
    }
    return(0);
failed:
    *sp = holder;
    IF_FREE(new_t);
    IF_FREE(new_attr);
    IF_FREE(new_val);
    return(-1);
}

/* check any new headers we received and update struct request
   and checked accordingly.
   checked poins to the character next to the last recognized header
 */
inline
static int
check_headers(struct request *request, char *beg, char *end, int *checked)
{
char	*start;
char	*p = NULL, saved=0;
int	r;

go:
    if ( request->state == REQUEST_READY ) return(0);
    start = beg + *checked;
    if ( !*checked ) {
        char    *pn, *pr;
        pr = (char *)memchr(beg, '\r', end-beg);
        pn = (char *)memchr(beg, '\n', end-beg);
        if        ( (pn == NULL) && (pr != NULL) ) {
            p = pr;
            saved = '\r';
        } else if ( (pr == NULL) && (pn != NULL) ) {
            p = pn;
            saved = '\n';
        } else if ( (pr != NULL) && (pn != NULL) ) {
            if ( pr < pn ) {
                p = pr;
                saved = '\r';
            } else {
                p = pn;
                saved = '\n';
            }
        }
        if ( p == NULL ) return(0);
		/* first line in request */
	if(p==beg){
		beg++;
		p=NULL;
		goto go;
	}
		*p = 0;
		r = parse_http_request(start, request);
	//	printf("request->url.path=%s.\n",request->url.path);
		*checked = strlen(start);
		*p = saved;
		request->headers_off = p-beg+2;
		if ( r ) {
			return(-1);
		}
		if ( !*checked ) return(-1);
		goto go;
    }
    /* checked points to last visited \r */
    if ( !request->data && (end - start >= 4) && !strncmp(start, "\r\n\r\n", 4) ) {
		if ( !request->content_length ) {
			request->state = REQUEST_READY;
			return(0);
		} else
		if ( request->content_length && !request->data ) {
			request->leave_to_read = request->content_length;
			if ( request->content_length <= 0 ) {
			request->state = REQUEST_READY;
			return(0);
			}
			request->data = alloc_buff(CHUNK_SIZE);
			if ( !request->data ) {
			my_xlog(OOPS_LOG_DBG|OOPS_LOG_INFORM, "check_headers(): req_data.\n");
				return(-1);
			}
			start += 4;
		}
    } else
		if ( !request->data && (end - start >= 2) && !strncmp(start, "\n\n", 2) ) {
			if ( !request->content_length ) {
				request->state = REQUEST_READY;
				return(0);
			} else
			if ( request->content_length && !request->data ) {
				request->leave_to_read = request->content_length;
				request->data = alloc_buff(CHUNK_SIZE);
				if ( !request->data ) return(-1);
				start += 2;
			}
		}
    if ( request->content_length && request->leave_to_read ) {
		if ( request->data && (end-start > 0) ) {
			if ( attach_data(start, end-start, request->data ) )
			return(-1);
		}
		request->leave_to_read -= end - start;
		/* we will read/send request body directly from/to client/server */
		request->state = REQUEST_READY;
		*checked = end-beg;
		return(0);
    }
    p = start;
    while( *p && ( *p == '\r' || *p == '\n' ) ) p++;
    if ( *p ) {
		char *t, saver = '\n';

		if ( !request->headers_off ) request->headers_off = p-beg;
		t = strchr(p, '\n');
		if ( !t ) {
			t = strchr(p, '\r');
			saver = '\r';
		}
		if ( !t ) return(0);
		if ( *t == '\n' && *(t-1) =='\r' ) {
			t--;
			saver = '\r';
		}
		*t = 0;
		/* check headers of my interest */
		my_xlog(OOPS_LOG_HTTP|OOPS_LOG_DBG, "check_headers(): ---> `%s'\n", p);
		if ( !request->data ) /* we don't parse POST data now */
			add_request_av(p, request);
		if ( !strncasecmp(p, "Content-length: ", 16) ) {
			char	*x;
			/* length */
			x=p + 16; /* strlen("content-length: ") */
			while( *x && IS_SPACE(*x) ) x++;
			request->content_length = atoi(x);
			request->flags |= RQ_HAS_CONTENT_LEN;
		}
		if ( !strncasecmp(p, "If-Modified-Since: ", 19) ) {
			char	*x;
			x=p + 19; /* strlen("content-length: ") */
			while( *x && IS_SPACE(*x) ) x++;

			memset(&request->if_modified_since,0, sizeof(request->if_modified_since));
//			bzero(&request->if_modified_since, sizeof(request->if_modified_since));
			if (!http_date(x, &request->if_modified_since))
				request->flags |= RQ_HAS_IF_MOD_SINCE;
		}
		if ( !strncasecmp(p, "Pragma: ", 8) ) {
			char	*x;
			x=p + 8; /* strlen("pragma: ") */
			while( *x && IS_SPACE(*x) ) x++;
			if ( strstr(x, "no-cache") ) request->flags |= RQ_HAS_NO_CACHE;
		}
		if( !strncasecmp(p,"Referer: ",9)){
			char *x;
			x=p+9;
			while( *x && IS_SPACE(*x) ) x++;
			request->referer=strdup(x);
			
		}
		if ( !strncasecmp(p, "Authorization: ", 15) ) {
			request->flags |= RQ_HAS_AUTHORIZATION;
		}
		/*
		if ( !strncasecmp(p, "Cookie: ", 8) ) {//don't cache cookie
			request->flags |= RQ_HAS_COOKIE;
		}
		*/
		if ( !strncasecmp(p, "Host: ", 6) ) {
		//	printf("request->url.path=%s.\n",request->url.path);
			char *x;
			int port=0;
		//	char *tmp;//[MAXHOSTNAMELEN];
			x=p+6;
			while( *x && IS_SPACE(*x) ) x++;
			if(request->url.host==NULL){
		//		printf("haha...........................\n");
//				if(strlen(x)>MAXHOSTNAMELEN)
//					return -1;
			//	printf("x=%s.\n",x);
				request->url.host=strdup(x);//(char *)malloc(strlen(x)+1);
				if(request->url.host==NULL)
					return -1;
			//	tmp[0]=0;
			//	sscanf(x,"%[^:]:%d",tmp,&port);
				request->url.port=split_host_port(request->url.host,':');//port;
				if(request->url.port==0)
					request->url.port=80;
			//	printf("port=%d.\n",request->url.port);
			//	request->url.host=tmp;//strdup(tmp);
				//printf("host=%s,port=%d\n",request->url.host,request->url.port);
			}
			request->flags |= RQ_HAS_HOST;
		//	printf("request->url.path=%s.\n",request->url.path);
		}
		if ( !strncasecmp(p, "Connection: ", 12) ) {
			char *x = p+12;

			while( *x && IS_SPACE(*x) ) x++;
			if ( !strncasecmp(x, "keep-alive", 10) )
				request->flags |= RQ_HAS_KEEP_CONNECTION;
		}else if(!strncasecmp(p, "Proxy-Connection: ", 18) ){
			char *x = p+18;
			while( *x && IS_SPACE(*x) ) x++;
			if ( !strncasecmp(x, "keep-alive", 10) )
				request->flags |= RQ_HAS_KEEP_CONNECTION;
		}
		if ( !strncasecmp(p, "Cache-Control: ", 15) ) {
			char	*x;

			x=p + 15; /* strlen("Cache-Control: ") */
			while( *x && IS_SPACE(*x) ) x++;
			if      ( !strncasecmp(x, "no-store", 8) )
				//request->flags |= RQ_HAS_NO_STORE;
				request->flags |= RQ_HAS_NO_CACHE;
			else if ( !strncasecmp(x, "no-cache", 8) )
				request->flags |= RQ_HAS_NO_CACHE;
			else if ( !strncasecmp(x, "no-transform", 12) )
				request->flags |= RQ_HAS_NO_TRANSFORM;
			else if ( !strncasecmp(x, "only-if-cached", 14) )
				request->flags |= RQ_HAS_ONLY_IF_CACHED;
			else if ( sscanf(x, "max-age = %d", &request->max_age) == 1 )
				request->flags |= RQ_HAS_MAX_AGE;
			else if ( sscanf(x, "min-fresh = %d", &request->min_fresh) == 1 )
				request->flags |= RQ_HAS_MIN_FRESH;
			else if ( !strncasecmp(x, "max-stale", 9) ) {
				request->flags |= RQ_HAS_MAX_STALE;
				request->max_stale = 0;
				sscanf(x, "max-stale = %d", &request->max_stale);
			}
		} else
		if ( !strncasecmp(p, "Range: ", 7) ) {
			char *x;
			/* we recognize "Range: bytes=xxx-" */
			x = p + 7;
			while( *x && IS_SPACE(*x) ) x++;
			if ( !strncasecmp(x, "bytes=", 6) ) {
				int	from=-1,to=-1;
				/* x+6 must be 'xxx-' */
				sscanf(x+6,"%d-%d", &from, &to);
				request->range_from = from;
				request->range_to = to;
			/*	if ( (from >= 0 ) && (to == -1 ) ) {
			
				}
			*/
			}
			request->flags |= RQ_HAVE_RANGE;
		}
		*t = saver;
		*checked = t - beg;
		goto go;
    }
    return(0);
}

static int
parse_http_request(char* src, struct request *rq)
{
char	*p, *httpv;
int	http_major, http_minor;
//printf("parse_http_request\n");
//printf("src=%s\n",src);
    p = strchr(src, ' ');
    if ( !p )
	return(-1);
    *p=0;
         if ( !strcasecmp(src, "get") )  rq->meth = METH_GET;
    else if ( !strcasecmp(src, "head") ) rq->meth = METH_HEAD;
    else if ( !strcasecmp(src, "put") )  rq->meth = METH_PUT;
    else if ( !strcasecmp(src, "post") ) rq->meth = METH_POST;
    else if ( !strcasecmp(src, "trace") ) rq->meth = METH_TRACE;
    else if ( !strcasecmp(src, "connect") ) rq->meth = METH_CONNECT;
    else if ( !strcasecmp(src, "PROPFIND") ) rq->meth = METH_PROPFIND;
    else if ( !strcasecmp(src, "PROPPATCH") ) rq->meth = METH_PROPPATCH;
    else if ( !strcasecmp(src, "MKCOL") ) rq->meth = METH_MKCOL;
    else if ( !strcasecmp(src, "DELETE") ) rq->meth = METH_DELETE;
    else if ( !strcasecmp(src, "COPY") ) rq->meth = METH_COPY;
    else if ( !strcasecmp(src, "MOVE") ) rq->meth = METH_MOVE;
    else if ( !strcasecmp(src, "LOCK") ) rq->meth = METH_LOCK;
    else if ( !strcasecmp(src, "UNLOCK") ) rq->meth = METH_UNLOCK;
    else if ( !strcasecmp(src, "PURGE") ) rq->meth = METH_PURGE;
    else if ( !strcasecmp(src, "OPTIONS") ) rq->meth = METH_OPTIONS;
    else {
		my_xlog(OOPS_LOG_SEVERE, "parse_http_request(): Unrecognized method `%s'.\n", src);
		*p = ' ';
		return(-1);
    }
    IF_FREE(rq->method); rq->method = strdup(src);
    *p = ' ';
    p++;
    /* next space must be before HTTP */
    httpv = strrchr(p, 'H');
    if ( rq->meth == METH_CONNECT ) {//if ssl connection
        if ( httpv )
            *httpv = 0;
		if ( parse_connect_url(p, httpv+1, &rq->url,rq) ) {
			if ( httpv ) *httpv = ' ';
			return(-1);
		}
		if ( httpv ) *httpv = ' ';
			return(0);
    }
    if ( !httpv )
		return(-1);
    if ( (httpv <= p) )
		return(-1);
    httpv--;
    *httpv = 0;
	
    if ( parse_url(p, &rq->url, httpv+1) ) {
		*httpv = ' ';
		return(-1);
    }
	/*
	if(parse_raw_url(p,&rq->url)){
		*httpv= ' ';
		return -1;
	}
	*/
  //  printf("path=%s.\n",rq->url.path);
    *httpv = ' ';
    if ( sscanf(httpv+1, "HTTP/%d.%d", &http_major, &http_minor) == 2 ) {
		rq->http_major = http_major;
		rq->http_minor = http_minor;
    } else
	return(-1);
    return(0);
}

static int
parse_connect_url(char* src, char *httpv, struct url *url, struct request *rq)
{

	char	*ss, *host=NULL;
//	printf("src=%s,httpv=%s\n",src,httpv);
    if ( !src ) return(-1);
    ss = strchr(src, ':');
    if ( !ss ) {
		say_bad_request( "Bad request, no proto:", src, ERR_BAD_URL, rq);
		return(-1);
    }
    *ss = 0;
    host = (char *)malloc(strlen(src)+1);
    if (!host)
	goto err;
    memcpy_to_lower(host, src, strlen(src)+1);
    url->host = host;
    url->port = atoi(ss+1);
    goto done;
err:
    *ss = ':';
    if (host) xfree(host);
    return(-1);
done:
    *ss = ':';
    return(0);
}

int parse_url(char *src, struct url *url, char *httpv)
{
char	*proto=NULL, *host=NULL, *path=NULL, *httpver = NULL;
char	*ss, *se, *he, *sx, *sa, holder;
char	number[10];
char	*login = NULL, *password = NULL;
int	p_len, h_len, i;
u_short	pval;
//	printf("src=%s.\n",src);
    if ( !src )
	return(-1);
    if ( *src == '/' ) {/* this is 'GET /path HTTP/1.x' request */
		se = src;
		proto = strdup("http");
		goto only_path;
    }
    ss = strchr(src, ':');
    if ( !ss ) {
	//	say_bad_request( "Bad request, no proto:", src, ERR_BAD_URL, rq);
		return(-1);
    }
    if ( memcmp(ss, "://", 3) ) {
	//	say_bad_request( "Bad request:", src, ERR_BAD_URL,rq);
		return(-1);
    }
    p_len = ss - src;
    proto = (char *)xmalloc(p_len+1, "parse_url(): proto");
    if ( !proto )
	return(-1);
    memcpy(proto, src, p_len); proto[p_len] = 0;
    ss += 3; /* skip :// */
    sx = strchr(ss, '/');
    se = strchr(ss, ':');
    sa = strchr(ss, '@');
    /* if we have @ and (there is no '/' or @ stay before '/') */ 
    if ( sa && ( !sx || ( sa < sx )) ) {
	/* ss   points to login				*/
	/* se+1 points to password			*/
	/* sa+1 points to host...			*/
	/* ss	se	 sa				*/
	/* login:password@hhost:port/path		*/
	if ( se < sa ) {
	    if ( se ) {
			*se = 0;
			login = (char *)xmalloc(ROUND(strlen(ss)+1, CHUNK_SIZE), "parse_url(): login");
			strcpy(login, ss);
			*se = ':';
			holder = *(sa);
			*(sa) = 0;
			password = (char *)xmalloc(ROUND(strlen(se+1)+1, CHUNK_SIZE), "parse_url(): password");
			strcpy(password, se+1);
				*(sa) = holder;
	    } else {
			holder = *sa;
			*sa = 0;
			login = (char *)xmalloc(ROUND(strlen(ss)+1, CHUNK_SIZE), "parse_url(): login2");
			strcpy(login, ss);
				password = NULL;
			*sa = holder;
	    }
	    ss = sa+1;
	    sx = strchr(ss, '/');
	    se = strchr(ss, ':');
	    goto normal;
	} else {
	    /* ss   sa	 se			*/
	    /* login@host:port/path		*/
	    holder = *sa;
	    *sa = 0;
	    login = (char *)xmalloc(ROUND(strlen(ss)+1, CHUNK_SIZE), "parse_url(): login3");
	    strcpy(login, ss);
	    password = NULL;
	    *sa = holder;
	    ss = sa+1;
	    sx = strchr(ss, '/');
	    goto normal;
	}
    }
normal:;
    if ( se && (!sx || (sx>se)) ) {
	/* port is here */
	he = se;
	h_len = se-ss;
	host = (char *)xmalloc(h_len+1, "parse_url(): host");
	if ( !host ) {
	    IF_FREE(login);
	    IF_FREE(password);
	    xfree(proto);
	    return(-1);
	}
	memcpy_to_lower(host, ss, h_len); host[h_len] = 0;
	se++;
	for(i=0; (i<10) && *se && IS_DIGIT(*se); i++,se++ ) {
	    number[i]=*se;
	}
	number[i] = 0;
	if ( (pval=atoi(number)) != 0 )
		url->port = pval;
	    else {
		    /* so can be -1 if called from icp.c */
	//	say_bad_request( "Bad port value:", number,	ERR_BAD_PORT,rq);
		IF_FREE(login);
		IF_FREE(password);
		xfree(proto);
		xfree(host);
		return(-1);
	}
    } else { /* there was no port */
	
	se = strchr(ss, '/');
	if ( !se )
	    se = src+strlen(src);
	h_len = se-ss;
	host = (char *)xmalloc(h_len+1, "parse_url(): host2");
	if ( !host ) {
	    IF_FREE(login);
	    IF_FREE(password);
	    xfree(proto);
	    return(-1);
	}
	memcpy_to_lower(host, ss, h_len); host[h_len] = 0;
	if ( !strcasecmp(proto, "http") ) url->port=80;
	if ( !strcasecmp(proto, "ftp") )  url->port=21;
    }
only_path:
//	printf("only path\n");
    if ( *se == '/' ) {		
		ss = se;
		for(i=0;*se++;i++);
		if ( i ) {
	//		printf("path=%s.\n",path);
			path = (char *)xmalloc(i+1, "parse_url(): 4");
			if ( !path ) {
				IF_FREE(login);
				IF_FREE(password);
				IF_FREE(host);
				IF_FREE(proto);
				return(-1);
			}
			memcpy(path, ss, i);
			path[i] = 0;
	//		printf("path=%s.\n",path);
		}
	//	*/
    } else {
		path=(char *)xmalloc(2, "parse_url(): 5");
		if ( !path ) {
			IF_FREE(login);
			IF_FREE(password);
			IF_FREE(host);
			IF_FREE(proto);
			return(-1);
		}
		path[0] = '/'; path[1] = 0;
    }
    if ( httpv ) {
		httpver = (char *)xmalloc(strlen(httpv) + 1, "parse_url(): httpver");
		if ( !httpver ) {
			IF_FREE(login);
			IF_FREE(password);
			IF_FREE(host);
			IF_FREE(proto);
			return(-1);
		}
		memcpy(httpver, httpv, strlen(httpv)+1);
    }
    url->host  = host;
    url->proto = proto;
    url->path  = path;
    url->httpv = httpver;
    url->login = login;
    url->password = password;
   //printf("url->host=%s,path=%s.\n",host,path);
    return(0);
}

int
parse_raw_url(char *src, struct url *url)
{
char	*proto=NULL, *host=NULL, *path=NULL;
char	*ss, *se, *he, *sx, *sa, holder;
char	number[10];
char	*login = NULL, *password = NULL;
int	p_len, h_len, i;
u_short	pval;

    if ( !src )
	return(-1);
    if ( *src == '/' ) {/* this is 'GET /path HTTP/1.x' request */
		se = src;
		proto = strdup("http");
		goto only_path;
    }
    ss = strchr(src, ':');
    if ( !ss ) {
		proto = strdup("http");
		ss = src;
		goto only_host_here;
    }
    if ( memcmp(ss, "://", 3) ) {
		proto = strdup("http");
		ss = src;
		goto only_host_here;
    }
    p_len = ss - src;
    proto = (char *)xmalloc(p_len+1, "parse_raw_url(): proto");
    if ( !proto )
	return(-1);
    memcpy(proto, src, p_len); proto[p_len] = 0;
    ss += 3; /* skip :// */
only_host_here:
    sx = strchr(ss, '/');
    se = strchr(ss, ':');
    sa = strchr(ss, '@');
    /* if we have @ and (there is no '/' or @ stay before '/') */ 
    if ( sa && ( !sx || ( sa < sx )) ) {
	/* ss   points to login				*/
	/* se+1 points to password			*/
	/* sa+1 points to host...			*/
	/* ss	se	 sa				*/
	/* login:password@hhost:port/path		*/
	if ( se < sa ) {
	    if ( se ) {
		*se = 0;
		login = (char *)xmalloc(ROUND(strlen(ss)+1, CHUNK_SIZE), "parse_raw_url(): login");
		strcpy(login, ss);
		*se = ':';
		holder = *(sa);
		*(sa) = 0;
		password = (char *)xmalloc(ROUND(strlen(se+1)+1, CHUNK_SIZE), "parse_raw_url(): password");
		strcpy(password, se+1);
	    	*(sa) = holder;
	    } else {
		holder = *sa;
		*sa = 0;
		login = (char *)xmalloc(ROUND(strlen(ss)+1, CHUNK_SIZE), "parse_raw_url(): login2");
		strcpy(login, ss);
	        password = NULL;
		*sa = holder;
	    }
	    ss = sa+1;
	    sx = strchr(ss, '/');
	    se = strchr(ss, ':');
	    goto normal;
	} else {
	    /* ss   sa	 se			*/
	    /* login@host:port/path		*/
	    holder = *sa;
	    *sa = 0;
	    login = (char *)xmalloc(ROUND(strlen(ss)+1, CHUNK_SIZE), "parse_raw_url(): login3");
	    strcpy(login, ss);
	    password = NULL;
	    *sa = holder;
	    ss = sa+1;
	    sx = strchr(ss, '/');
	    goto normal;
	}
    }
normal:;
    if ( se && (!sx || (sx>se)) ) {
	/* port is here */
	he = se;
	h_len = se-ss;
	host = (char *)xmalloc(h_len+1, "parse_raw_url(): host");
	if ( !host ) {
	    IF_FREE(login);
	    IF_FREE(password);
	    xfree(proto);
	    return(-1);
	}
	memcpy_to_lower(host, ss, h_len); host[h_len] = 0;
	se++;
	for(i=0; (i<10) && *se && IS_DIGIT(*se); i++,se++ ) {
	    number[i]=*se;
	}
	number[i] = 0;
	if ( (pval=atoi(number)) != 0 )
		url->port = pval;
	    else {
		IF_FREE(login);
		IF_FREE(password);
		xfree(proto);
		xfree(host);
		return(-1);
	}
    } else { /* there was no port */
	
	se = strchr(ss, '/');
	if ( !se )
	    se = src+strlen(src);
	h_len = se-ss;
	host = (char *)xmalloc(h_len+1, "parse_raw_url(): host2");
	if ( !host ) {
	    IF_FREE(login);
	    IF_FREE(password);
	    xfree(proto);
	    return(-1);
	}
	memcpy_to_lower(host, ss, h_len); host[h_len] = 0;
	if ( !strcasecmp(proto, "http") ) url->port=0;
	if ( !strcasecmp(proto, "ftp") )  url->port=21;
    }
only_path:
    if ( *se == '/' ) {
	ss = se;
	for(i=0;*se++;i++);
	if ( i ) {
	    path = (char *)xmalloc(i+1, "parse_raw_url(): 4");
	    if ( !path ) {
		IF_FREE(login);
		IF_FREE(password);
		IF_FREE(host);
		IF_FREE(proto);
		return(-1);
	    }
	    memcpy(path, ss, i);
	    path[i] = 0;
	}
    } else {
	path = (char *)xmalloc(2, "parse_raw_url(): 5");
	if ( !path ){
	    IF_FREE(login);
	    IF_FREE(password);
	    IF_FREE(host);
	    IF_FREE(proto);
	    return(-1);
	}
	path[0] = '/'; path[1] = 0;
    }
    url->host  = host;
    url->proto = proto;
    url->path  = path;
    url->login = login;
    url->password = password;
    return(0);
}

u_short hash(struct url *url)
{
u_short		res = 0;
int		i;
char		*p;

    p = url->host;
    if ( p && *p ) {
		p = p+strlen(p)-1;
		i = 35;
		while ( (p >= url->host) && i ) i--,res += *p**p--;
    }
    p = url->path;
    if ( p && *p ) {
		p = p+strlen(p)-1;
		i = 35;
		while ( (p >= url->path) && i ) i--,res += *p**p--;
    }
    return(res & HASH_MASK);
}
u_short string_hash(const char *str)
{
	u_short         res = 0;
	int             i;
	const	char            *p;
	p=str;
	if ( p && *p ) {
                p = p+strlen(p)-1;
                i = 35;
                while ( (p >= str) && i ) i--,res += *p**p--;
    }
    return(res & NET_HASH_MASK);
}
void release_obj(struct mem_obj *obj)
{
/* just decrement refs */
	assert(obj->refs>0);
    pthread_mutex_lock(&obj->lock);
	obj->refs--;
    pthread_mutex_unlock(&obj->lock);
}
/*
void
leave_obj(struct mem_obj *obj,int new_obj)
{
	return;


}
*/
inline static void free_request( struct request *rq)
{
	struct	av	*av, *next;

    free_url(&rq->url);
    av = rq->av_pairs;
    while(av) {
		xfree(av->attr);
		xfree(av->val);
		next = av->next;
		xfree(av);
		av = next;
    }
    IF_FREE( rq->method );
	IF_FREE(rq->referer);
  //  IF_FREE(rq->original_host);
    if ( rq->data ) free_container(rq->data);
 //   if ( rq->redir_mods ) leave_l_mod_call_list(rq->redir_mods);
 //   if ( rq->cs_to_server_table ) leave_l_string_list(rq->cs_to_server_table);
  //  if ( rq->cs_to_client_table ) leave_l_string_list(rq->cs_to_client_table);
  //  IF_FREE(rq->matched_acl);
  //  IF_FREE(rq->source);
  //  IF_FREE(rq->tag);
   // IF_FREE(rq->c_type);
  //  IF_FREE(rq->hierarchy);
  //  IF_FREE(rq->proxy_user);
    //IF_FREE(rq->original_path);
    //IF_FREE(rq->peer_auth);
   // IF_FREE(rq->decoding_buff);
   // remove_request_from_ip_hash(rq);
}

void
free_url(struct url *url)
{
    IF_FREE(url->host);
    IF_FREE(url->proto);
    IF_FREE(url->path);
    IF_FREE(url->httpv);
    IF_FREE(url->login);
    IF_FREE(url->password);
}

/*
 * %M - message
 * %R - reason (strerror, or free text)
 * %U - URL
 */
void
say_bad_request(const char* reason, const char *r, int code, struct request *rq)
{

const char	*hdr = "HTTP/1.0 200 OK\nConent-Type: text/html\n\n<html>\
		<head><title>http 400 bad request</title></head><body>\
		<i><h2>Invalid request:</h2></i><p><pre>";
const char	*rf= "</pre><b>";
const char	*tail1="\
		</b><p>Please, check URL.<p>\
		<hr>\
		Generated by <a href=\"http://www.kingate.net\"> kingate(";
const char	*tail2=")</a>.</body></html>";
int len=strlen(hdr)+strlen(rf)+strlen(reason)+strlen(tail1)+strlen(VER_ID)+strlen(tail2)+2;
char	*content=(char *)malloc(len);
	if(!content)
		return;
	memset(content,0,len);
	snprintf(content,len-1,"%s%s%s%s%s%s",hdr,rf,reason,tail1,VER_ID,tail2);
	if(rq!=NULL)
		rq->server->send(content,len);
	free(content);
	return; 
	
}

static int in_stop_cache(struct request *rq)
{
/*struct	string_list	*l = stop_cache;

    while(l) {
	if ( strstr((*rq).url.path, l->string) )
	    return(1);
	l = l->next;
    }

 // if ( stop_cache_acl )
//	return(check_acl_access(stop_cache_acl, rq));

    return(0);
	*/
	if(strstr(rq->url.path,"?"))//不cache url中有?
		return 1;
/*	if(strstr(rq->url.path,".php"))
		return 1;
	if(strstr(rq->url.path,".jsp"))
		return 1;
	if(strstr(rq->url.path,".asp"))
		return 1;
*/	
	return 0;
}
/*
int
set_socket_options(int so)
{
#if	!defined(FREEBSD)
int	on = -1;
#if	defined(TCP_NODELAY)
     setsockopt(so, IPPROTO_TCP, TCP_NODELAY, (char*)&on, sizeof(on));
#endif
#endif /* !FREEBSD * /
    return(0);
}
*/
int
obj_rate(struct mem_obj *obj)
{
	bool in_mem=true;
	if(!TEST(obj->flags,FLAG_IN_MEM))
		in_mem=false;
	update_list(&obj->m_list,in_mem);
    	return(0);
}
void https_start(request *rq)//https代理
{
//	mysocket client;
	klog(MODEL_LOG,"[HTTPS]%s:%d connect https://%s:%d\n",rq->server->get_remote_name(),rq->server->get_remote_port(),rq->url.host,rq->url.port);
	if( rq->server->send("HTTP/1.0 200 Connection established\r\n\r\n")<=0 ) 
		return;
//	rq->clientset_time(conf.time_out[HTTP]);
	if(rq->client->connect(rq->url.dst_ip,rq->url.dst_port)>0){
		if(is_local_ip(rq->client)){
			return;
		}
		create_select_pipe(rq->server,rq->client,conf.time_out[HTTP]);
	}
}
bool manage_start(request *rq)
{
		KHttpManage m_httpManage;
		return m_httpManage.start(rq);
}
void flush_net_request()
{
	net_obj *m_net_obj;
	net_obj *m_tmp;
	int now_time=time(NULL);
	for(int i=0;i<NET_HASH_SIZE;i++){
		net_table[i].lock.Lock();
		m_net_obj=net_table[i].next;
		for(;;){
			if(m_net_obj==NULL)
				break;
			if((m_net_obj->refs==0) && (now_time-m_net_obj->last_access>conf.total_seconds) ){
					m_tmp=m_net_obj->next;
					if(m_net_obj->next){
						m_net_obj->next->prev=m_net_obj->prev;
					}
					if(m_net_obj->prev){
						m_net_obj->prev->next=m_net_obj->next;
					}else{
						net_table[i].next=m_net_obj->next;
					}
					delete m_net_obj;
					m_net_obj=m_tmp;
				
			}else{
				m_net_obj=m_net_obj->next;
			}
		}
		net_table[i].lock.Unlock();
	}
}
net_obj * net_request_start(request *rq,u_short &u_hash,int &fatboy)
{
	string url;
	v_ip m_ip;
	int now_time=time(NULL);
	int total_time=-1;
	list<v_ip>::iterator it;
	if(!get_url(&rq->url,url)){

		return NULL;
	}
	m_ip.start_time=now_time;
	u_hash=string_hash(url.c_str());
	net_table[u_hash].lock.Lock();
	net_obj *m_net_obj=net_table[u_hash].next;
	for(;;){
		if(m_net_obj==NULL){
			break;	
		}
		if(m_net_obj->url==url){
			break;	
		}
		m_net_obj=m_net_obj->next;
	}
	if(m_net_obj==NULL){//new
		m_net_obj=new net_obj;
		if(m_net_obj==NULL){
			net_table[u_hash].lock.Unlock();
			return NULL;
		}
		m_net_obj->url=url;
		m_net_obj->ips.push_back(m_ip);
		m_net_obj->prev=NULL;
		m_net_obj->next=net_table[u_hash].next;
		m_net_obj->refs=1;
		m_net_obj->last_access=now_time;
		if(net_table[u_hash].next)
			net_table[u_hash].next->prev=m_net_obj;
		net_table[u_hash].next=m_net_obj;		
	}else{//old
		m_net_obj->refs++;
		m_net_obj->last_access=now_time;
		m_net_obj->ips.push_back(m_ip);
		for(;;){
			if(m_net_obj->ips.size()>conf.max_request){
				m_net_obj->ips.pop_front();
				it=m_net_obj->ips.begin();
				total_time=now_time-(*it).start_time;
			}else{
				break;
			}
		}
	}
	net_table[u_hash].lock.Unlock();
	if((total_time>=0) && (total_time<conf.total_seconds)){
		klog(ERR_LOG,"may have fatboy attack ip=%s:%d,url=%s *************\n",rq->server->get_remote_name(),rq->server->get_remote_port(),url.c_str());
		fatboy=1;
		//my_sleep(2);
	}
	return m_net_obj;
}
void net_request_end(net_obj *m_net_obj,u_short u_hash)
{
	if(m_net_obj==NULL)
		return;
	net_table[u_hash].lock.Lock();
	m_net_obj->refs--;
	net_table[u_hash].lock.Unlock();

}
bool http_start(request *rq)//http代理
{
	struct	mem_obj		*stored_url;
	char			*headers=NULL;
	int new_object;
	//int			mem_send_flags = 0;
	u_short u_hash;
	int fatboy=0;
	int head_status=HEAD_OK;
	const char *msg=NULL;
	char *url_msg=NULL;
	bool result=true;
	int start_time=time(NULL);
	int start_size=rq->server->send_size;
	net_obj *m_net_obj=NULL;
//	if(rq->server->title_msg){
	int msg_len=20+strlen(rq->url.host)+strlen(rq->url.path);
	url_msg=(char *)malloc(msg_len);
	if(url_msg==NULL)
		return false;
	memset(url_msg,0,msg_len);
	if(rq->url.port==80)
		snprintf(url_msg,msg_len,"http://%s%s",rq->url.host,rq->url.path);
	else
		snprintf(url_msg,msg_len,"http://%s:%d%s",rq->url.host,rq->url.port,rq->url.path);
	rq->server->set_title_msg(url_msg);
//	free(msg); 
//	}
	if ( !(rq->meth==METH_GET) 		||
	// ( rq->flags & RQ_HAS_NO_STORE) 	||
	 ( rq->flags & RQ_HAS_AUTHORIZATION)||
	 ( rq->url.login ) ||
	 in_stop_cache(rq) ) {
		m_net_obj=net_request_start(rq,u_hash,fatboy);
	//	printf("request has no store or request has authorization\n");
		msg="no-cache";
		if(fatboy==0){
			send_not_cached(rq->server->get_socket(), rq, headers);
		}
		result=false;
		net_request_end(m_net_obj,u_hash);
	/*	}
		if(TEST(rq->flags,RESULT_CLOSE))
			result=false;
		rq->client->close();
	*/	
		goto done;
	}/*
	if ( in_stop_cache(rq) ) {
		msg="stop-cache";
		if(send_not_cached(rq->server->get_socket(), rq, headers)<=0){
			result=false;
		}
		if(TEST(rq->flags,RESULT_CLOSE))
			result=false;
		rq->client->close();
		goto done;
	}*/
	if ( rq->flags & RQ_HAS_ONLY_IF_CACHED ) {
		stored_url = locate_in_mem(&rq->url, AND_USE, &new_object, rq);
		if ( !stored_url ) {
			send_error(rq->server->get_socket(), 504, "Gateway Timeout. Or not in cache");
			goto done;
		}
		msg="only-cache";
		send_data_from_obj(rq, stored_url);
	//	rq->server->close();
		result=false;
		goto done;
	}
	stored_url = locate_in_mem(&rq->url, AND_PUT|AND_USE, &new_object, rq);
	if ( !stored_url ) {
		result=false;
		goto done;
	}

	if ( new_object ) {//It is a new object
		msg="net";
		m_net_obj=net_request_start(rq,u_hash,fatboy);
		if(fatboy==0){
			head_status=fill_mem_obj(rq,stored_url);
		}else{
			result=false;
		}
		net_request_end(m_net_obj,u_hash);
		if( (head_status==CONNECT_ERR) || TEST(rq->flags,RESULT_CLOSE) ){
			result=false;
		}
	} else {
		msg="cache";
		head_status=send_from_mem(rq, stored_url);
		if( (head_status==CONNECT_ERR) || TEST(rq->flags,RESULT_CLOSE) )
			result=false;
		release_obj(stored_url);
	}		
done:	
	klog(MODEL_LOG,"[HTTP%s]%s:%d %s get %s[%s] %d %d %d\n",(result==true?"":"_SHORT"),rq->server->get_remote_name(),rq->server->get_remote_port(),msg,url_msg,(rq->referer==NULL?"":rq->referer),(time(NULL)-start_time),head_status,rq->server->send_size-start_size);
	free(url_msg);
	return result;
}
#endif
