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
#ifdef HAVE_CONFIG_H
#include <config.h>
#endif
#include "kingate.h"
#ifndef DISABLE_HTTP
#include	"oops.h"
#include<time.h>
#include<string>
#ifdef HAVE_SSTREAM
#include <sstream>
#else
#include "mysstream"
#endif
//#include	"modules.h"
#include	"log.h" 
#include	"cache.h"
#include	"http.h"
#include	"utils.h"
#include	"allow_connect.h"
#include "malloc_debug.h"

#define		AND_PUT		1
#define		AND_USE		2

#define		SWITCH_TO_READER_ON(obj) {	lock_obj(obj);	obj->writers--;	obj->readers++;	role = ROLE_READER;	unlock_obj(obj);}

#define	DECR_READERS(o) {lock_obj(o);o->readers--;unlock_obj(o);}
#define	INCR_READERS(o) {lock_obj(o);o->readers++;unlock_obj(o);}
#define	DECR_WRITERS(o) {lock_obj(o);o->writers--;unlock_obj(o);		}
#define	INCR_WRITERS(o) {	lock_obj(o);	o->writers++;	unlock_obj(o);	}

#define		DONT_CHANGE_HTTPVER	1
using namespace std;
int		no_direct_connections	= FALSE;
static int		maxresident=5*1024*1024;//max cache size is 5m

static	char	*build_direct_request(const char *meth, struct url *url, char *headers, struct request *rq, int flags);
//static	int		can_recode_rq_content(struct request*);

//static	void	check_new_object_expiration(struct request*, struct mem_obj*);
//static	char	*check_rewrite_charset(char *, struct request *, struct av *, int*);

//static	int		continue_load(struct request*, int, int, struct mem_obj *);
static	int		downgrade(struct request *, struct mem_obj *);

//static	struct	mem_obj	*check_validity(int, struct request*, char *, struct mem_obj*);
//static	void	process_vary_headers(struct mem_obj*, struct request*);

//static	int		srv_connect(int, struct url *url, struct request*);
static	int		srv_connect(struct request *rq);
//static	int		srv_connect_silent(int, struct url *url, struct request*);
static	int		load_head(struct request *rq,struct mem_obj *obj);
static	int		load_body(struct request *rq,struct mem_obj *obj);
inline	static	int		add_header_av(char* avtext, struct mem_obj *obj);
inline	static	void	analyze_header(char *p, struct server_answ *a);
inline	static	int		is_attr(struct av*, const char*);
//inline	static	int		is_oops_internal_header(struct av *);
//inline	static	void	lock_obj_state(struct mem_obj *);
//inline	static	void	unlock_obj_state(struct mem_obj *);
//inline	static	void	lock_decision(struct mem_obj *);
//inline	static	void	unlock_decision(struct mem_obj *);
using namespace std;
bool swap_in_obj(struct mem_obj *obj);
struct head_info
{
	char *head;
	int len;
};
int check_server_headers(struct server_answ *a, struct mem_obj *obj, struct buff *b, struct request *rq);
int check_server_headers(struct server_answ *a, struct mem_obj *obj, const char *b,int len, struct request *rq);
bool CheckStringModel(const char *str,const char *model,int flag)
{
      size_t length=strlen(model);
      size_t length2=strlen(str);
      int i=0;
      int j=0;
     int c;
      bool model_match_all=false;
       for(;i<length;i++,j++){
	if(model[i]=='*'){
	  if(i==length-1){
	    return true;
	  }
	  for(int p=j;p<length2;p++){
	    if(str[p]==model[i+1]){
	      if(CheckStringModel(str+p,model+i+1,flag))
		return true;
	    }
	  }
	  model_match_all=true;
	}
	if(TEST(flag,IGNORE_CASE)){
		c=toupper(str[j]);
	}else{
		c=str[j];
	}
	if(model[i]!=c)
	  return false;
      }
      if(j<length2)
	return false;
      return true;

	
}
bool CheckFileModel(const url  *m_url,const char *model,int flag)
{

        bool http_ssl=false;
        bool ret;
        if(!m_url->proto || !m_url->host || !m_url->path)
                return false;
        char *path_file=strdup(m_url->path);
	if(path_file==NULL)
		return false;
        int len=strlen(path_file);
        for(int i=0;i<len;i++){
                if(path_file[i]=='?'){
                        path_file[i]=0;
                        break;
                }
        }
        int u_len=strlen(m_url->proto)+strlen(m_url->host)+strlen(path_file)+10;
        char *url_file=(char *)malloc(u_len);
        if(url_file==NULL){
                klog(RUN_LOG,"no mem to alloc in %s:%d.\n",__FILE__,__LINE__);
		free(path_file);
                return false;
        }
        memset(url_file,0,u_len);
        if(strcasecmp(m_url->proto,"https")==0)
                http_ssl=true;
        snprintf(url_file,u_len-1,"%s://%s:%d%s",m_url->proto,m_url->host,m_url->port,path_file);
        ret=CheckStringModel(url_file,model,flag);
	free(path_file);
        free(url_file);
        return ret;
}
#ifdef CONTENT_FILTER 
int check_filter_key(const char *msg,int msg_len)
{
	char *tmp;
	int key_len;
	bool matched;
	bool may_match=false;
	for(int i=0;i<conf.keys.size();i++){
		tmp=conf.keys[i];
		if(tmp==NULL){
			continue;
		}
		key_len=strlen(tmp);
		for(int j=0;j<msg_len;j++){
			matched=true;
			if(msg[j]==tmp[0]){
				for(int p=1;p<key_len;p++){
					if(j+p>=msg_len){
						may_match=true;
						matched=false;
						break;
					}
					if(tmp[p]!=msg[j+p]){
						matched=false;
						break;
					}
				}
				if(matched)
					return i;
			}
		}
	}
	if(may_match){
		return -2;
	}
	return -1;
}
#endif
static int load_head(struct request *rq,struct mem_obj *obj,head_info &m_head)
{
	//char	*answer = NULL;
//	struct pollarg pollarg[2];
	struct	av		*header = NULL;
	char *answer= (char *)malloc(ANSW_SIZE+1);
	char *buf=answer;
	struct	buff	*hdrs_to_send=NULL;
	int len=ANSW_SIZE;
	struct	server_answ	answ_state;
	int head_status=CONNECT_ERR;
	int r=0,downgrade_flags=0;
	SOCKET s1=rq->client->get_socket();
	SOCKET s2=rq->server->get_socket();

	int maxfd=MAX(s1,s2);
	url m_url;
	struct timeval tm;
	fd_set readfds;
	memset(&tm,0,sizeof(tm));
//	rq->client->set_time(0);
//	rq->server->set_time(0);

	m_head.len=0;
	memset(&answ_state,0, sizeof(answ_state));
	memset(&m_url,0,sizeof(m_url));
	if ( !answer ) 
		goto error;
	if(s2<=0)
		goto error;
/*
	pollarg[0].fd = rq->client->get_socket();
	pollarg[0].request = FD_POLL_RD;
	pollarg[1].fd = rq->server->get_socket();
	pollarg[1].request = FD_POLL_RD;
*/
	forever() {		
		FD_ZERO(&readfds);
		FD_SET(s1,&readfds);
		FD_SET(s2,&readfds);
	/*
		if((r = poll_descriptors(2, &pollarg[0], conf.time_out[HTTP]*1000))<=0){
			klog(DEBUG_LOG,"poll descriptors is error in file %s line %d\n",__FILE__,__LINE__);
			goto error;
		}
	*/
		tm.tv_sec=conf.time_out[HTTP];
		if(select(maxfd+1,&readfds,NULL,NULL,&tm)<=0){
		
			klog(DEBUG_LOG,"poll descriptors is error in file %s line %d,errno=%d\n",__FILE__,__LINE__,errno);
			goto error;
		}
		//if(IS_READABLE(&pollarg[1]) || IS_HUPED(&pollarg[1])){		
		if(FD_ISSET(s2,&readfds)){
			klog(DEBUG_LOG,"prev head value=%d,client close the connection\n",head_status);
			//head_status=CLIENT_HAVE_DATA;
			goto error;
		}
//		if(IS_READABLE(&pollarg[0]))
		else if(FD_ISSET(s1,&readfds)){
			r = rq->client->recv2(buf,len,0);
		}else{
			klog(DEBUG_LOG,"unknow error is happen2,errno=%d\n",errno);
			goto error;
		}
		if ( r <= 0  ){
			klog(DEBUG_LOG,"recv server data error,errno=%d\n",errno);
			goto error;
		}
		buf[r]=0;
//		printf("%s",buf);
		buf+=r;
		len-=r;
		if ( !(answ_state.state & GOT_HDR) ) {
			obj->size += r;
			if ( check_server_headers(&answ_state, obj, answer,r, rq) ) {
				head_status=HEAD_UNKNOW;
				rq->flags|=RESULT_CLOSE;
			//	printf("%s:%d\n",__FILE__,__LINE__);
				goto send_data;
			}
			
		}
		if ( answ_state.state & GOT_HDR ) {				
		
			obj->flags|= answ_state.flags;
			obj->times = answ_state.times;
//			printf("obj->times.last_modified=%d\n",obj->times.last_modified);
			if ( !obj->times.date ) 
				obj->times.date = global_sec_timer;
		//	check_new_object_expiration(rq, obj);
			obj->status_code = answ_state.status_code;
			if ( 
				(obj->status_code != STATUS_OK) && 
				(obj->status_code!=STATUS_NOT_MODIFIED)	&&
				(obj->status_code!=STATUS_MOVED) && 
				(obj->status_code!=302)
			){
				rq->flags|=RESULT_CLOSE;
				head_status=HEAD_NOT_OK;
				goto send_data;
			}
			hdrs_to_send = alloc_buff(CHUNK_SIZE); /* most headers fit this (?) */
			if ( !hdrs_to_send ) 
				goto error;
			header = obj->headers;
			if ( !header ) {
				rq->flags|=RESULT_CLOSE;
				goto send_data ;
			}
			while(header) {

				if(
				conf.http_accelerate && 
				(strncasecmp(header->attr,"Location:",9)==0) &&
				(parse_url(header->val,&m_url)==0) ){
			//	printf("**************\n");
					//	printf("rq->url.host=%s,m_url.host=%s\n",rq->url.host,m_url.host);
					if(
					(m_url.host) && 
					(strcasecmp(rq->url.host,m_url.host)==0) &&
					(rq->url.port!=m_url.port)){
					//	printf("rq->url.port=%d,m_url.port=%d\n",rq->url.port,m_url.port);
						stringstream s;
						s << "HTTP/1.1 302 Found\r\nServer: kingate(" << VER_ID << ")\r\nLocation: " << "http://" << m_url.host;
						if(rq->url.port!=80){
							s << ":" << rq->url.port;
						}
						s << m_url.path;
						s << "\r\nContent-Length: 0\r\nConnection: close\r\n\r\n";
						free(answer);
						answer=strdup(s.str().c_str());
						m_head.len=strlen(answer);
						rq->flags|=RESULT_CLOSE;
				//		printf("answer=%s\n",answer);
						head_status=HEAD_NOT_OK;
						goto send_data;
					}
				}
				if(strncasecmp(header->attr,"Set-Cookie:",7)!=0){//don't cache cookie
					if(strncasecmp(header->attr,"Connection:",11)==0){
						if((!TEST(rq->flags,RQ_HAS_KEEP_CONNECTION)) || (TEST(rq->flags,RESULT_CLOSE))){
							attach_av_pair_to_buff("Connection:","close",hdrs_to_send);
						}else{
							attach_av_pair_to_buff("Connection:","keep-alive",hdrs_to_send);
						}
						goto next_header;
					}
				}else{
				//	printf("server has set-cookie\n");
					head_status=HEAD_OK_NO_STORE;
					rq->flags|=RESULT_CLOSE;
					goto send_data;
				}
				attach_av_pair_to_buff(header->attr, header->val, hdrs_to_send);
			next_header:
				header = header->next;
			}
			attach_av_pair_to_buff("", "", hdrs_to_send);
			if ((TEST(obj->flags , ANSW_NO_STORE)) || (TEST(obj->flags, ANSW_SHORT_CONTAINER)) ){
				head_status=HEAD_OK_NO_STORE;
				rq->flags|=RESULT_CLOSE;
				goto send_data;
			}
			if(obj->status_code==STATUS_NOT_MODIFIED){
				head_status=HEAD_NOT_MODIFIED;
				m_head.head=hdrs_to_send->data;
				m_head.len=hdrs_to_send->used;
				hdrs_to_send->data=NULL;
			//	free_container(hdrs_to_send);
				goto done;
			}

		//	obj->container=obj->hot_buff;
		
			if(rq->server->send(hdrs_to_send->data,hdrs_to_send->used)<0){
			//	free_container(hdrs_to_send);
				goto error;
			}
		//	free_container(hdrs_to_send);
	//		printf("%s\n",hdrs_to_send->data);
			head_status=HEAD_OK;			
			goto done;
		}
		if(len<1){
		//	printf("head size is too large\n");
			head_status=HEAD_UNKNOW;
			rq->flags|=RESULT_CLOSE;
			goto send_data;//head size is too large
		}
		
    }
	
error://get head failed
	obj->flags |= FLAG_DEAD;
	if(hdrs_to_send)
		free_container(hdrs_to_send);
	IF_FREE(answer);
	free_url(&m_url);
	return head_status;
done://get head ok;
	if(hdrs_to_send)
		free_container(hdrs_to_send);
	free(answer);
	free_url(&m_url);
	return head_status;
send_data://得到头失败，但要转发数据
	if(hdrs_to_send)
		free_container(hdrs_to_send);
	m_head.head=answer;
	free_url(&m_url);
	if(m_head.len==0){
		m_head.len=ANSW_SIZE-len;
		answer[m_head.len]=0;
	}
	return head_status;
}
static int 
load_body(struct request *rq,struct mem_obj *obj)
{
//	char	*answer = NULL;
//	struct	server_answ	answ_state;
//	struct pollarg pollarg[2];
	//struct	av		*header = NULL;
	#ifdef CONTENT_FILTER
	string title_msg;
	#endif
	char *answer=(char *)malloc(ANSW_SIZE+1);
	int r=0;
        SOCKET s1=rq->client->get_socket();
        SOCKET s2=rq->server->get_socket();
        int maxfd=MAX(s1,s2);
	unsigned body_size=obj->container->used;
        struct timeval tm;
        fd_set readfds;
	int key_state;
		
       // fd_set readfds;
        memset(&tm,0,sizeof(tm));
	if(answer==NULL)
		goto error;
	if(s2<=0)
		goto error;
	if(obj->container && obj->container->used>0){
		#ifdef CONTENT_FILTER
		if(!TEST(obj->flags,KEY_CHECKED)){
			key_state=check_filter_key(obj->container->data,obj->container->used);
			if(key_state>=0){
				SET(obj->flags,KEY_CHECKED);
				rq->server->get_title_msg(title_msg);
				klog(ERR_LOG,"%s filter_key_matched[%s].\n",title_msg.c_str(),conf.keys[key_state]);
			}
		}	
		#endif
		if(rq->server->send(obj->container->data,obj->container->used)<=0){
			//printf("cann't send to client\n");
			goto error;
		}
	}
	if(obj->content_length<=0){//if content-length is zero ,we don't cache it
		if(create_select_pipe(rq->server,rq->client,conf.time_out[HTTP],0,-1,(TEST(obj->flags,KEY_CHECKED)?true:false))==-2)
		r=1;
		goto error;
	}
/*
 	pollarg[0].fd = rq->client->get_socket();
	pollarg[0].request = FD_POLL_RD;
	pollarg[1].fd = rq->server->get_socket();
	pollarg[1].request = FD_POLL_RD;
*/

	forever() {		
	//	if((r = poll_descriptors(2, &pollarg[0], conf.time_out[HTTP]*1000))<=0){
     	  	FD_ZERO(&readfds);
       	 	FD_SET(s1,&readfds);
        	FD_SET(s2,&readfds);
 		tm.tv_sec=conf.time_out[HTTP];
                if(select(maxfd+1,&readfds,NULL,NULL,&tm)<=0){
			klog(DEBUG_LOG,"poll descriptors is error in file %s line %d,errno=%d\n",__FILE__,__LINE__,errno);
		//	printf("1\n");
			goto done;
		}
/*
		if(IS_READABLE(&pollarg[1]) || IS_HUPED(&pollarg[1])){
*/
		if(FD_ISSET(s2,&readfds)){
			klog(DEBUG_LOG,"client close the connection in body\n");
	//		printf("2\n");
			goto done;
		}
//		if(IS_READABLE(&pollarg[0]))
		else if(FD_ISSET(s1,&readfds)){
			r = rq->client->recv2(answer, ANSW_SIZE,0);
		}else{
			klog(DEBUG_LOG,"unknow error is happen,errno=%d\n",errno);
		//	printf("3\n");
			goto done;
		}
		if ( r <=0  )
			goto done;
		#ifdef CONTENT_FILTER
		if(!TEST(obj->flags,KEY_CHECKED)){
			key_state=check_filter_key(answer,r);
			if(key_state>=0){
				SET(obj->flags,KEY_CHECKED);
				rq->server->get_title_msg(title_msg);
				klog(ERR_LOG,"%s filter_key_matched[%s].\n",title_msg.c_str(),conf.keys[key_state]);
			}
		}
		#endif
		body_size += r;
		if(conf.limit_speed>0 && body_size>=conf.min_limit_speed_size){//limit speed
			my_msleep(r*1000/conf.limit_speed);
		}
		if(body_size>obj->content_length){
		//	destroy_obj(obj,0);
			if(rq->server->send(answer,r)<0)
				goto error;
			if(create_select_pipe(rq->server,rq->client,conf.time_out[HTTP],0,-1)==-2)
				r=1;
			goto error;
		}
/* store data in hot_buff */
/*		if(writet(so,answer,r,READ_ANSW_TIMEOUT)<=0)
			goto error;
*/		if(rq->server->send(answer,r)<=0){
	//		printf("cann't send to client\n");
			goto error;
		}	
		if ( store_in_chain(answer, r, obj) ) {
			my_xlog(OOPS_LOG_SEVERE, "fill_mem_obj(): Can't store.\n");
			obj->flags |= FLAG_DEAD;
	//		printf("cann't store in chain\n");
			goto error;
		}
	}
done:
//	rq->server->set_time(s_time);
//	rq->client->set_time(c_time);
	if(body_size<=0 || body_size!=obj->content_length){//数据不完整，不保存
		//printf("body_size != content_length\n");
		goto error;
	}
	IF_FREE(answer);
	return 1;

error:
	IF_FREE(answer);
//	rq->server->set_time(s_time);
//	rq->client->set_time(c_time);
	SET(obj->flags,FLAG_DEAD);
	return r;
}
int stored_obj(struct mem_obj *obj)
{
//	printf("stored obj now\n");
	struct mem_obj *tmp_obj=NULL;
	int found=0;
	if(obj==NULL)
		return 0;
	if(obj->flags & (FLAG_DEAD)){
		destroy_obj(obj,0);
	//	printf("obj have set dead \n");
	//	obj=NULL;
		return -1;
	}
//	obj->last_access=time(NULL);
//	printf("obj->times.last_modified=%d %s:%d\n",obj->times.last_modified,__FILE__,__LINE__);

	obj->m_list.obj=(void *)obj;
	obj->prev=NULL;
	obj->resident_size =obj->content_length;//calculate_resident_size(obj);
	u_short url_hash=hash(&obj->url);
	LOCK_OBJ_LIST
	pthread_mutex_lock(&hash_table[url_hash].lock);
	tmp_obj=hash_table[url_hash].next;
	while(tmp_obj) {
	    if ( (tmp_obj->url.port==obj->url.port) &&
	         !strcmp(tmp_obj->url.path, obj->url.path) &&
	         !strcasecmp(tmp_obj->url.host, obj->url.host) &&
	         !strcasecmp(tmp_obj->url.proto, obj->url.proto) &&
	         !(tmp_obj->flags & FLAG_DEAD) ) {
				found=1;
				break;
		}
		tmp_obj=tmp_obj->next;
	}
	if(found){
		pthread_mutex_unlock(&hash_table[url_hash].lock);
		UNLOCK_OBJ_LIST
		destroy_obj(obj,0);
	//	printf("have another obj in\n");
		return 0;
	}

	obj->next=hash_table[url_hash].next;
	if (obj->next) obj->next->prev = obj;
	hash_table[url_hash].next=obj;
	obj->hash_back = &hash_table[url_hash];
	if(TEST(obj->flags,FLAG_IN_MEM)){	
		increase_hash_size(obj->hash_back, obj->resident_size);
	}
	if(TEST(obj->flags,FLAG_IN_DISK)){
		increase_hash_size(obj->hash_back, obj->resident_size,false);
	}
	add_list(&obj->m_list);
	pthread_mutex_unlock(&hash_table[url_hash].lock);
	UNLOCK_OBJ_LIST
	return 1;
};
static int 
srv_connect(struct request *rq)
{
	ERRBUF ;
	short port=rq->url.port;
//	rq->client->set_time(CONNECT_TIME_OUT);	
	if(!rq->client->connect(rq->url.dst_ip,rq->url.dst_port)){
		klog(ERR_LOG,"cann't connect host %s:%d ,ip %x:%d.\n",rq->url.host,rq->url.port,rq->url.dst_ip,rq->url.dst_port);
	//	say_bad_request("Can't connect to host.","",ERR_TRANSFER, rq);
	//	rq->client->close();
		return -1;
	}
	if(is_local_ip(rq->client)){
		//rq->client->close();
		return -1;
	}
	rq->client->set_time(conf.time_out[HTTP]);
//	rq->client->set_time(conf.time_out[HTTP]);	
	return 1;
}
int send_not_cached(SOCKET so, struct request *rq, char *headers)
{
//	int			server_so = -1, r, received = 0, pass=0;, to_write;
	char			*answer = NULL;
//	struct	url		*url = &rq->url;
	const char		*meth;
	int r;
//	int			have_code = 0;
//	unsigned int		sent = 0;
//	int			header_size = 0;
//	int			recode_request = FALSE, recode_answer = FALSE;
//	char			*table = NULL;
	ERRBUF ;
	int ret=CONNECT_ERR;

	//int client_left_recv=rq->content_length;
	if ( rq->meth == METH_GET ) meth="GET";
    else if ( rq->meth == METH_PUT ) meth="PUT";
    else if ( rq->meth == METH_POST ) meth="POST";
    else if ( rq->meth == METH_TRACE ) meth="TRACE";
    else if ( rq->meth == METH_HEAD ) meth="HEAD";
    else if ( rq->meth == METH_OPTIONS ) meth="OPTIONS";
    else if ( rq->meth == METH_PROPFIND ) meth="PROPFIND";
    else if ( rq->meth == METH_PROPPATCH ) meth="PROPPATCH";
    else if ( rq->meth == METH_DELETE ) meth="DELETE";
    else if ( rq->meth == METH_MKCOL ) meth="MKCOL";
    else if ( rq->meth == METH_COPY ) meth="COPY";
    else if ( rq->meth == METH_MOVE ) meth="MOVE";
    else if ( rq->meth == METH_LOCK ) meth="LOCK";
    else if ( rq->meth == METH_UNLOCK ) meth="UNLOCK";
    else
		return ret;
	if(srv_connect(rq)<=0)
		goto done;
    answer = build_direct_request(meth, &rq->url, NULL, rq, DONT_CHANGE_HTTPVER);
   // printf("send to server:\n%s",answer);
    if ( !answer )
		goto done;
//	r = writet(server_so, answer, strlen(answer), READ_ANSW_TIMEOUT);
	
    r=rq->client->send(answer);
    if ( r <= 0 ) {
		say_bad_request( "Can't send", "",
			ERR_TRANSFER, rq);
		goto done;
    }
/*	
    answer = (char *)xmalloc(ANSW_SIZE+1, "send_not_cached(): 1");
    if ( !answer ) goto done;
  */  if ( rq->data ) {
		char	*cp = NULL;
		int	rest= 0;
		/* send whole content to server				*/
		
		if ( rq->data ) {
			cp  = rq->data->data;
			rest= rq->data->used;
		}
		//cp[rq->data->used]=0;
		//client_left_recv-=rest;
		while ( rest > 0 ) {
			int to_send;
			to_send = MIN(2048, rest);
			/*
			r = writet(server_so, cp, to_send, 100);
			if ( r < 0 )
				goto done;
			*/
			//printf("to_send=%d\n",to_send);
			if((r=rq->client->send(cp,to_send))<=0)
				goto done;
			//printf("cp=%s,r=%d\n",cp,r);
			rest -= r;
			cp   += r;
		}
    }
   // printf("leave to read:%d bytes\n", rq->leave_to_read);
//	printf("content_length=%d\n",rq->content_length);
	//pump_data(rq->server->get_socket(),rq->client->get_socket());
	if(create_select_pipe(rq->server,rq->client,conf.time_out[HTTP],(rq->leave_to_read==0?0:-1),-1)<=0)
		ret=1;

done:
   // if ( server_so >0 ) CLOSE(server_so);
   // if ( answer ) xfree(answer);
	rq->client->close();
	IF_FREE(answer);
    	return ret;
}
static bool
check_object_expiration(struct mem_obj *obj)
{
	assert(obj);
	if ( !obj ) return false;
	int diff_time=obj->times.date-obj->created;
	int server_may_date=time(NULL)+diff_time;
	time_t lifttime;
	if ( obj->flags & ANSW_HAS_MAX_AGE ) 
		lifttime=obj->times.max_age;
	else if ( obj->times.expires > 0)
		lifttime=(MAX(0,obj->times.expires - obj->times.date));
	else if (obj->times.last_modified > 0)
		lifttime=obj->last_access - obj->times.last_modified;
	else 
		lifttime=24*3600;
	time_t age=time(NULL)-obj->created+obj->times.age;
//	printf("now time=%d,server_may_date=%d.server_expires=%d,last_modified=%d,last_access=%d,age=%d,lifttime=%d\n",time(NULL),server_may_date,obj->times.expires,obj->times.last_modified,obj->last_access,age,lifttime);
	if(age>lifttime)
		return true;
	return false;
}
int send_not_modify_from_mem(struct request *rq,struct mem_obj *obj)
{
	char buf[512];
	memset(buf,0,sizeof(buf));
	strcpy(buf,"HTTP/1.1 304 Not Modified\r\nDate: ");
	mk1123time(time(NULL), buf+33, 40);
	sprintf(buf+strlen(buf),"\r\nServer: kingate/%s (cached)\r\nConnection: ",VER_ID);
	if((!TEST(rq->flags,RQ_HAS_KEEP_CONNECTION)) || TEST(rq->flags,RESULT_CLOSE) ){
			strcpy(buf+strlen(buf),"close\r\n\r\n");	
	}else{
			strcpy(buf+strlen(buf),"keep-alive\r\n\r\n");
	}
	if(rq->server->send(buf)>0)
		return 6;
	return -1;

}
int send_from_mem(struct request *rq,struct mem_obj *obj)
{
//	int			server_so = -1;
	struct	mem_obj		*new_obj = NULL;
	int head_status=CONNECT_ERR;
//	struct	timeval		start_tv, stop_tv;
//	time_t			now;
//	int			delta_tv;// received;//, rc;
//	int			have_code = 0;
//	int new_object;
//	char			 *meth;
/*#define			ROLE_READER	1
#define			ROLE_WRITER	2
#define			ROLE_VALIDATOR	3
*/
//	int			role, no_more_logs = FALSE, source_type;
//	char			*origin;
//	struct	sockaddr_in	peer_sa;
//	hash_entry_t            *he = NULL;
/*	
    if ( rq->meth == METH_GET ) meth="GET";
    else if ( rq->meth == METH_PUT ) meth="PUT";
    else if ( rq->meth == METH_POST ) meth="POST";
    else if ( rq->meth == METH_HEAD ) meth="HEAD";
    else
		return -1;
*/
	bool expires=false;
	#ifdef CONTENT_FILTER
	if(TEST(rq->flags,RQ_NO_FILTER))
		SET(obj->flags,KEY_CHECKED);
	#endif
    //gettimeofday(&start_tv, NULL);
    if ( TEST(rq->flags , RQ_HAS_NO_CACHE) || TEST(rq->flags ,RQ_HAS_IF_MOD_SINCE) ) 
		goto revalidate;
	if(conf.refresh==REFRESH_ANY)
		goto revalidate;
 //   IF_STRDUP(rq->tag, tcp_tag);	
	if(conf.refresh==REFRESH_AUTO){
		if(rand()%5==0)
			goto revalidate;
	}
	expires=check_object_expiration(obj);
	if(expires){
	//	printf("obj is expiration.\n");
		goto revalidate;
	}
//	printf("send from local cache now.\n");
  	head_status=send_data_from_obj(rq,  obj);
	goto done;
revalidate:
//	check_validity(rq,obj);
//	printf("obj->last_access=%d\n",obj->last_access);
	if(time(NULL)-obj->last_access<conf.refresh_time){
		if(TEST(rq->flags,RQ_HAS_IF_MOD_SINCE) && (obj->times.last_modified?(rq->if_modified_since>=obj->times.last_modified):(rq->if_modified_since>obj->created))){
				head_status=send_not_modify_from_mem(rq,obj);
				head_status+=200;
		}else{
				head_status=send_data_from_obj(rq,  obj);
				head_status+=300;
		}
		goto done;
	}
	new_obj = locate_in_mem(&rq->url,PUT_NEW_ANYWAY, NULL, NULL);
	new_obj->created=obj->created;
//	memset(obj->times,0,sizeof(obj->times));
	head_status=fill_mem_obj(rq,new_obj,obj);
	obj->times.expires=0;
	//obj->last_access=time(NULL);
	if(head_status==HEAD_OK){		
		dead_obj(obj);
		stored_obj(new_obj);	
		return head_status;
	}else if(head_status==HEAD_NOT_MODIFIED){
		if(!TEST(rq->flags,RQ_HAS_IF_MOD_SINCE) ){
			send_data_from_obj(rq,obj);
			obj->last_access=time(NULL);
			head_status=56;
		}else{
			if(rq->if_modified_since<=obj->times.last_modified)
				obj->last_access=time(NULL);
		}
	}else{
		dead_obj(obj);
		return head_status;
	}
done:
	obj_rate(obj);
	if(expires)
		head_status+=10;
	return head_status;
	
}

int send_data_from_obj(struct request *rq, struct mem_obj *obj)
{
	struct buff * tmp=NULL;//obj->container;
	struct	av *header=obj->headers;
	struct buff *hdrs_to_send=alloc_buff(CHUNK_SIZE); /* most headers fit this (?) */
//	rq->server->set_time(60);
	char tmp_str[100];
	int start=0,send_len=obj->content_length;
	int this_send_len=0;
	int body_size=0;
	int key_state;
	#ifdef CONTENT_FILTER
	string title_msg;
	#endif
	pthread_mutex_lock(&obj->lock);
	if(!swap_in_obj(obj) ){
		pthread_mutex_unlock(&obj->lock);
		dead_obj(obj);
		goto error;
	}
	pthread_mutex_unlock(&obj->lock);
	tmp=obj->container;
	if(rq->range_to==-1)
		rq->range_to=obj->content_length-1;
	if ( TEST(rq->flags, RQ_HAVE_RANGE)){
		if(   // &&  (obj->state == OBJ_READY)
	    // &&  !content_chunked(obj)
	      ((rq->range_from >= 0 && rq->range_from<obj->content_length-1) 
			  && (rq->range_to == -1||rq->range_to>rq->range_from) )) {
			attach_av_pair_to_buff("HTTP/1.1","206 Partial Content",hdrs_to_send);
			memset(tmp_str,0,sizeof(tmp_str));
			snprintf(tmp_str,sizeof(tmp_str)-1,"%d-%d/%d",rq->range_from,rq->range_to,obj->content_length);
			/*
			if(rq->range_to!=-1)
				snprintf(tmp_str+strlen(tmp_str),sizeof(tmp_str)-1-strlen(tmp_str),"%d",MIN(rq->range_to,obj->content_length-1));
			snprintf(tmp_str+strlen(tmp_str),sizeof(tmp_str)-1-strlen(tmp_str),"/%d",obj->content_length);
			*/
			attach_av_pair_to_buff("Content-Range:",tmp_str,hdrs_to_send);
			header=header->next;
			start=rq->range_from;
			if(rq->range_to>=0)
				send_len=MIN(rq->range_to+1,obj->content_length)-rq->range_from;
			else
				send_len=obj->content_length-rq->range_from;
		}else if(rq->range_from>obj->content_length-1){
			attach_av_pair_to_buff("HTTP/1.1","416 Requested Range Not Satisfiable",hdrs_to_send);
			memset(tmp_str,0,sizeof(tmp_str));
			snprintf(tmp_str,sizeof(tmp_str)-1,"*/%d",obj->content_length);
			attach_av_pair_to_buff("Content-Range:",tmp_str,hdrs_to_send);
			start=-1;
			header=header->next;
		}
	}
	
	while(header) {
//		printf("attr=%s\n",header->attr);
		if(strncasecmp(header->attr,"Connection:",11)==0){
			if((!TEST(rq->flags,RQ_HAS_KEEP_CONNECTION)) || TEST(rq->flags,RESULT_CLOSE) ){
				attach_av_pair_to_buff("Connection:","close",hdrs_to_send);
			}else{
				attach_av_pair_to_buff("Connection:","keep-alive",hdrs_to_send);
			}
			goto next_head;
		}
		if(strncasecmp(header->attr,"Server:",7)==0){
			memset(tmp_str,0,sizeof(tmp_str));
			snprintf(tmp_str,sizeof(tmp_str)-1,"kingate/%s (cached)",VER_ID);
			attach_av_pair_to_buff("Server:",tmp_str,hdrs_to_send);
			goto next_head;
		}
		attach_av_pair_to_buff(header->attr, header->val, hdrs_to_send);
	next_head:
		header = header->next;
	}
	attach_av_pair_to_buff("", "", hdrs_to_send);
	if(rq->server->send(hdrs_to_send->data,hdrs_to_send->used)<=0)
		goto done;
//	printf("start=%d,send_len=%d.\n",start,send_len);
	if(start==-1)
		goto done;
	while(tmp!=NULL){
		if(tmp->used<start){
			start-=tmp->used;
			goto next_buffer;
		}
		if(send_len<=0)
			break;
		this_send_len=MIN((tmp->used-start),send_len);
		body_size+=this_send_len;
		#ifdef CONTENT_FILTER
		if(!TEST(obj->flags,KEY_CHECKED)){
			key_state=check_filter_key(tmp->data+start,this_send_len);
			if(key_state>=0){
				SET(obj->flags,KEY_CHECKED);
				rq->server->get_title_msg(title_msg);
				klog(ERR_LOG,"%s filter_key_matched[%s].\n",title_msg.c_str(),conf.keys[key_state]);
			}	
		}
		#endif
		if(conf.limit_speed>0 && body_size>=conf.min_limit_speed_size){//limit speed
			my_msleep(this_send_len*1000/conf.limit_speed);
		}
		if(rq->server->send(tmp->data+start,this_send_len)<=0)
			goto error;
		start=0;
		send_len-=tmp->used;
	next_buffer:
		tmp=tmp->next;
	}	
done:
	if(hdrs_to_send)
		free_container(hdrs_to_send);
	return SEND_FROM_MEM_OK;
error:
	if(hdrs_to_send)
		free_container(hdrs_to_send);
	return CONNECT_ERR;
}
int fill_mem_obj(request *rq,struct mem_obj *obj,struct mem_obj *old_obj)
{
//	struct	buff		*to_server_request = NULL;
	char *answer=NULL;
	char *fake_header;
	head_info m_head;
	int fake_header_len=0;
	int r=CONNECT_ERR;
	char mk1123buff[50];
	ERRBUF ;
	memset(&m_head,0,sizeof(m_head));
	if(srv_connect(rq)<=0)
		goto error;
	#ifdef CONTENT_FILTER
	if(TEST(rq->flags,RQ_NO_FILTER))
		SET(obj->flags,KEY_CHECKED);
	#endif
	/*
	to_server_request = alloc_buff(4*CHUNK_SIZE);
    if ( !to_server_request ) {
	//	change_state(obj, OBJ_READY);
		obj->flags |= FLAG_DEAD;
		goto error;
    }
	*/
//	stored_obj(NULL);
//	printf("rq->if_modified_since=%d,obj->times.last_modified=%d\n",rq->if_modified_since,old_obj->times.last_modified);
	if(old_obj && !TEST(rq->flags,RQ_HAS_IF_MOD_SINCE)){
		if(old_obj->times.last_modified){
			if (!mk1123time(old_obj->times.last_modified, mk1123buff, sizeof(mk1123buff)) ) {
				klog(ERR_LOG,"cann't mk1123time obj obj->times.date=%d.\n",old_obj->times.last_modified);
				goto error;
			}
		}else{
			if (!mk1123time(obj->created, mk1123buff, sizeof(mk1123buff)) ) {
				 klog(ERR_LOG,"cann't mk1123time obj created=%d.\n",old_obj->created);
				 goto error;
			}
		}
		fake_header_len=19+sizeof(mk1123buff)+4;
		fake_header = (char *)malloc(fake_header_len);
		if ( !fake_header ) {
			goto error;
			//error	goto validate_err;
		}
	//	printf("fake_header=%s\n",fake_header);
		memset(fake_header,0,fake_header_len);
		snprintf(fake_header,fake_header_len-1, "If-Modified-Since: %s\r\n", mk1123buff);
		answer=build_direct_request("GET", &rq->url, fake_header, rq, 0);
		free(fake_header);
	//	printf("client have no if-modified-since.\n");
	}else{
		answer=build_direct_request("GET",&rq->url,NULL,rq,0);//连接远程主机
	}
//	printf("answer=%s\n",answer);
	
    if ( !answer ) {
		klog(ERR_LOG,"no mem to alloc.\n");
		goto error;
	}
    /*
    if ( attach_data(answer, strlen(answer), to_server_request) ) {
		free_container(to_server_request);
		goto error;
    }
    if(answer){
		free(answer);
		answer = NULL;
	}
	*/

	r = rq->client->send(answer);
//	free(answer);
 //   free_container(to_server_request); to_server_request = NULL;
	//	printf("send data to remote is:%s",to_server_request->data);
    	if ( r <=0 ) {//send error
		say_bad_request("Can't send","",
			ERR_TRANSFER, rq);
	//	printf("cann't send to server.\n");
		//printf("errno=%d\n",ERRNO);
		goto error;
    	}
		r=load_head(rq,obj,m_head);
        switch(r){
        case HEAD_OK:/*
			if(old_obj && old_obj->times.last_modified>=obj->times.last_modified){
						 klog(ERR_LOG,"*****************web server may have a bug...\n");
			}
			*/
            if(load_body(rq,obj)<=0){
                    //printf("load body failed.\n");
                    r=CONNECT_ERR;
                    goto error;
            }
            goto done;
        case CONNECT_ERR:
                //printf("load head failed.\n");
                goto error;
        case HEAD_NOT_MODIFIED:
                if(old_obj && !TEST(rq->flags,RQ_HAS_IF_MOD_SINCE)){
                        goto error;
                }else{
                       if(rq->server->send(m_head.head,m_head.len)<0)
                                goto error;
                }
        //      printf("%s\n",m_head.head);
                break;
		
        default:
//              printf("obj->content_length=%d,path=%s\n",obj->content_length,obj->url.path);
	//			printf("m_head.head=%s\n",m_head.head);
                if(m_head.head){
                        if(rq->server->send(m_head.head,m_head.len)<0)
                                goto error;
                        if(create_select_pipe(rq->server,rq->client,conf.time_out[HTTP],0,-1)==-2)
                                r=1;
                }
        }

	/*
	switch(r){
	case HEAD_OK:
	       #ifdef IIS_SMB_BUG
                if(old_obj && old_obj->times.last_modified>=obj->times.last_modified){
        //              printf("***********************web server bug...\n");
                        r=HEAD_NOT_MODIFIED;
                        goto head_not_modified;
                }
                #endif
		if(load_body(rq,obj)<=0){		
			//printf("load body failed.\n");
			r=CONNECT_ERR;
			goto error;
		}
		goto done;
	case CONNECT_ERR:
		//printf("load head failed.\n");
		goto error;
	case HEAD_NOT_MODIFIED:	
	head_not_modified:
		if(old_obj && !TEST(rq->flags,RQ_HAS_IF_MOD_SINCE)){
			goto error;
		}else{
		 	if(old_obj){
                                send_not_modify_from_mem(rq,old_obj);
                                break;
                        }else if(rq->server->send(m_head.head,m_head.len)<0)
                                goto error;
		}
	//	printf("%s\n",m_head.head);
		break;
		
	default:
//		printf("obj->content_length=%d,path=%s\n",obj->content_length,obj->url.path);		
		if(m_head.head){
			if(rq->server->send(m_head.head,m_head.len)<0)
				goto error;
			if(create_select_pipe(rq->server,rq->client,conf.time_out[HTTP],0,-1)==-2)
				r=1;
		}
	}
	*/
error:
	IF_FREE(answer);
	IF_FREE(m_head.head);
	rq->client->close();
	destroy_obj(obj,0);
	return r;
done:
	IF_FREE(answer);
	IF_FREE(m_head.head);
	rq->client->close();
	if(!old_obj)
		stored_obj(obj);
	return r;
};

void
free_chain(struct buff *buff)
{
	struct	buff	*next;
    while(buff) {
		next = buff->next;
		xfree(buff->data);
		xfree(buff);
		buff = next;
    }
}

/* send answer like

 HTTP/1.0 XXX message\r\n\r\n
 
*/

void
send_error(int so, int code, const char * message)
{
	char	*hdr = NULL;
	int hdr_len=20+strlen(message);
    hdr = (char *)malloc(hdr_len);
    if ( !hdr ) {
		return;
    }
    memset(hdr,0,hdr_len);
    snprintf(hdr,hdr_len-1, "HTTP/1.0 %3d %s\r\n\r\n",
		code,
		message);
    writet(so, hdr, strlen(hdr), READ_ANSW_TIMEOUT);
    xfree(hdr);
    return;
}

inline
static int
is_attr(struct av *av, const char *attr)
{
    if ( !av || !av->attr || !attr ) return(FALSE);
    return(!strncasecmp(av->attr, attr, strlen(attr)));
}

char*
format_av_pair(const char* attr, const char* val)
{
	char	*buf;
	int buf_len=4;
    if ( *attr ){
	    	buf_len=strlen(attr)+strlen(val)+4;
		buf = (char *)malloc(buf_len);
		if(buf){
			sprintf(buf,"%s %s\r\n", attr, val);
		}
    } else {
		buf = (char *)malloc(buf_len);
		if(buf){
			strcpy(buf,"\r\n");
		//	snprintf(buf,buf_len, "\r\n");
		}
    }
    return(buf);
}

static char*
build_direct_request(const char *meth, struct url *url, char *headers, struct request *rq, int flags)
{
	int	rlen, authorization_done = FALSE;
	char	*answer = NULL, *fav=NULL, *httpv, *host=NULL;
	struct	buff 	*tmpbuff;
	struct	av	*av;
	int host_len=0;
	int	via_inserted = FALSE;
	bool 	x_forwarded_for_inserted=false;	
    int url_port=rq->url.port;
   // if (!TEST(flags, DONT_CHANGE_HTTPVER) ) {
//		httpv = "HTTP/1.1";
//		if ( TEST(rq->flags, RQ_HAS_HOST) )
//			host = rq->url.host;
//	} else {
	httpv = url->httpv;
	if ( !TEST(rq->flags, RQ_HAS_HOST) ) 
		host = rq->url.host;
//	}
    tmpbuff = alloc_buff(CHUNK_SIZE);
    if ( !tmpbuff ) return(NULL);
    rlen = strlen(meth) + 1/*sp*/ + strlen(url->path) + 1/*sp*/ +
		strlen(httpv) + 2/* \r\n */;
    if(!conf.http_accelerate && TEST(rq->flags,RQ_USE_PROXY) ){
	    rlen=rlen+7+strlen(rq->url.host);
	    if(rq->url.port!=80)
		    rlen=rlen+7;
    }
    answer = (char *)xmalloc(ROUND(rlen+1,CHUNK_SIZE), "build_direct_request(): 1"); /* here answer is actually *request* buffer */
    if ( !answer )
		goto fail;
   memset(answer,0,rlen+1);
   if(!conf.http_accelerate && TEST(rq->flags,RQ_USE_PROXY)){
	   if(rq->url.port!=80)
		   snprintf(answer,rlen,"%s http://%s:%d%s %s\r\n",meth,rq->url.host,rq->url.port,rq->url.path,httpv);
	   else
		   snprintf(answer,rlen,"%s http://%s%s %s\r\n",meth,rq->url.host,rq->url.path,httpv);
   }else{
    	sprintf(answer, "%s %s %s\r\n", meth, rq->url.path, httpv);
   }
 //  printf("host=%s.path=%s.\n",rq->url.host,rq->url.path);
 //  printf("answer=%s.\n",answer);
    if ( attach_data(answer, strlen(answer), tmpbuff) )
		goto fail;
    if ( headers ) { /* attach what was requested */
		if ( attach_data(headers, strlen(headers), tmpbuff) )
			goto fail;
    }
  	av = rq->av_pairs;
    while ( av ) {
		if ( is_attr(av, "Proxy-Connection:") )
			goto do_not_insert;
		if ( is_attr(av, "Proxy-Authorization:") ) /* hop-by-hop */
			goto do_not_insert;
		if ( is_attr(av, "Connection:") )
			goto do_not_insert;
		if( is_attr(av,"Host:")){
			host_len=strlen(rq->url.host)+10;
			host=(char *)malloc(host_len);
			if(host==NULL)
				goto fail;
			memset(host,0,host_len);
			if(conf.http_accelerate)
				url_port=rq->url.dst_port;
			if(url_port!=80)
			    snprintf(host,host_len-1,"%s:%d",rq->url.host,url_port);
			else
			    snprintf(host,host_len-1,"%s",rq->url.host);
			if(fav=format_av_pair("Host:",host)){
			    if(attach_data(fav,strlen(fav),tmpbuff))
		    		goto fail;
	   			 xfree(fav);
	    			fav=NULL;
    			}
			free(host);host=NULL;
			goto do_not_insert;
		}
		if ( conf.x_forwarded_for && (is_attr(av,"X-Forwarded-For:")||is_attr(av,"X-Forwarded_For:")||is_attr(av,"X_Forwarded-For:")||is_attr(av,"X_Forwarded_For:")) ){
			if(x_forwarded_for_inserted){
				goto do_not_insert;
			}
			if ( (fav = format_av_pair("X-Forwarded-For:", av->val)) != 0 ) {
               	 		char    *buf;
                		buf = strchr(fav, '\r');
				if ( buf ) *buf = 0;
				if ( !buf ) {
				 	buf = strchr(fav, '\n');
					if ( buf ) *buf = 0;
				}
				int buf_len=strlen(fav)+20;
				buf =(char *)malloc(buf_len);
				if ( !buf ) goto fail;
				memset(buf,0,buf_len);
				snprintf(buf,buf_len-1,"%s,%s\r\n",fav,rq->server->get_remote_name());
				if ( attach_data(buf, strlen(buf), tmpbuff) ) {
				    xfree(buf);
				    goto fail;
				}
				x_forwarded_for_inserted = true;
				xfree(buf);
				xfree(fav);
				fav = NULL;
		    	}
			goto do_not_insert;

		}
        	if ( is_attr(av, "Via:") && conf.insert_via   ) {
			if(via_inserted){
				goto do_not_insert;
			}
            		/* attach my Via: */
            		if ( (fav = format_av_pair(av->attr, av->val)) != 0 ) {
               	 		char    *buf;
                		buf = strchr(fav, '\r');
				if ( buf ) *buf = 0;
				if ( !buf ) {
				 	buf = strchr(fav, '\n');
					if ( buf ) *buf = 0;
				}
				int buf_len=strlen(fav)+2+18+7+10+strlen(VER_ID)+2+1;
				buf =(char *)malloc(buf_len);
				if ( !buf ) goto fail;
				memset(buf,0,buf_len);
				snprintf(buf,buf_len-1,"%s,%d.%d %s:%d (kingate/%s)\r\n",fav,rq->http_major,rq->http_minor,rq->client->get_self_name() ,conf.port[HTTP], VER_ID);
				if ( attach_data(buf, strlen(buf), tmpbuff) ) {
				    xfree(buf);
				    goto fail;
				}
				via_inserted = TRUE;
				xfree(buf);
				xfree(fav);
				fav = NULL;
		    	}
			goto do_not_insert;
		}
		if ( (fav=format_av_pair(av->attr, av->val)) != 0 ) {
			if ( attach_data(fav, strlen(fav), tmpbuff) )
				goto fail; 
			xfree(fav);fav = NULL;
		}
do_not_insert:
		av = av->next;
    }
    if ( (fav = format_av_pair("Connection:", "close")) != 0 ) {
		if ( attach_data(fav, strlen(fav), tmpbuff) )
			goto fail;
		xfree(fav);fav = NULL;
    }
    if(conf.x_forwarded_for && !x_forwarded_for_inserted){
	if( (fav=format_av_pair("X-Forwarded-For:",(char *)rq->server->get_remote_name()))!=0){
		if ( attach_data(fav, strlen(fav), tmpbuff) )
			goto fail;
		xfree(fav);fav = NULL;
	}
    }
    if (conf.insert_via && !via_inserted ) {
        char *buf;
	int buf_len=4+1+18+7+10+strlen(VER_ID)+2+1;
        buf =(char *) malloc(buf_len);
        if ( !buf ) goto fail;
	memset(buf,0,buf_len);
        snprintf(buf,buf_len-1,"Via: %d.%d %s:%d (kingate/%s)\r\n",rq->http_major,rq->http_minor, rq->client->get_self_name(),conf.port[HTTP], VER_ID);
        if ( attach_data(buf, strlen(buf), tmpbuff) ) {
            xfree(buf);
            goto fail;
        }
        xfree(buf);
        xfree(fav);fav = NULL;
    }

    /* CRLF  */
    if ( attach_data("\r\n", 2, tmpbuff) )
		goto fail;
    if ( attach_data("", 1, tmpbuff) )
		goto fail;
    IF_FREE(answer);
    answer = tmpbuff->data;
    tmpbuff->data = NULL;
    free_chain(tmpbuff);
   // printf("%s",answer);
    return answer;
fail:
    IF_FREE(fav);
    IF_FREE(host);
    if (tmpbuff) free_chain(tmpbuff);
    IF_FREE(answer);
    return NULL;
}
static int
downgrade(struct request *rq, struct mem_obj *obj)
{
	int	res = 0;
	const char	*transfer_encoding = NULL;
#if	defined(HAVE_ZLIB)
	const char	*content_encoding = NULL;
#endif	/* HAVE_ZLIB */
    if ( (rq->http_major  < obj->httpv_major) ||
		(rq->http_minor  < obj->httpv_minor) ) {
		res |= DOWNGRADE_ANSWER;
		if ( obj->headers )
			transfer_encoding = attr_value(obj->headers, "Transfer-Encoding");
		if ( transfer_encoding && !strncasecmp("chunked", transfer_encoding, 7)) {
			my_xlog(OOPS_LOG_HTTP|OOPS_LOG_DBG, "downgrade(): Turn on Chunked Gateway.\n");
			res |= UNCHUNK_ANSWER;
		}
    }
#if	defined(HAVE_ZLIB)
    content_encoding = attr_value(obj->headers, "Content-Encoding");
    if ( content_encoding && !strncasecmp(content_encoding, "gzip", 4) ) {
		/* we ungzip if useragent won't accept gzip	*/
		char	*ua_accept = attr_value(rq->av_pairs, "accept-encoding");
		
		if ( !ua_accept || !(strstr(ua_accept, "gzip")) )
			res |= UNGZIP_ANSWER;
    }
#endif
    return(res);
}

/*
int  
pump_data(int so, int server_so)
{
	int		r;//, pass = 0;
	struct	pollarg pollarg[2];
	
    forever() {
		pollarg[0].fd = server_so;		pollarg[1].fd = so;
		pollarg[0].request = FD_POLL_RD;
		pollarg[1].request = FD_POLL_RD;
		r = poll_descriptors(2, &pollarg[0],  conf.time_out[HTTP]*1000);
		if ( r <= 0) {
			goto done;
		}
		if ( IS_HUPED(&pollarg[0]) || IS_HUPED(&pollarg[1]) )
			goto done;
		if ( IS_READABLE(&pollarg[0]) ) {
			char b[1024];
			/* read from server * /
			r = read(server_so, b, sizeof(b));
/*			if ( (r < 0) && (ERRNO == EAGAIN) )
				goto sel_again;
* /			if ( r <= 0 )
				return server_so;//goto done;
/*			if ( rq ) rq->received += r;
			if ( rq ) rq->doc_received += r;
* /			r = write(so, b, r);//, conf.time_out[HTTP]);
			if ( r <= 0 ) return so;
//			if ( rq ) rq->doc_sent += r;
		}
		if ( IS_READABLE(&pollarg[1]) ) {
			char b[1024];			/* read from client * /
			r = read(so, b, sizeof(b));
/*			if ( (r < 0) && (ERRNO == EAGAIN) )
				goto sel_again;
* /			if ( r <= 0 )
				return so;
			r = write(server_so, b, r);//, conf.time_out[HTTP]);
			if ( r <= 0 ) return server_so;// goto done;
		}
    }
done:
    return -1;
}
*/
inline
static void
analyze_header(char *p, struct server_answ *a)
{
	char	*t;
	
   // my_xlog(OOPS_LOG_HTTP|OOPS_LOG_DBG, "analyze_header(): ---> `%s'.\n", p);
    if ( !a->status_code ) {
		/* check HTTP/X.X XXX */
		if ( !strncasecmp(p, "HTTP/", 5) ) {
			int	httpv_major, httpv_minor;
			if ( sscanf(p+5, "%d.%d", &httpv_major, &httpv_minor) == 2 ) {
				a->httpv_major = httpv_major;
				a->httpv_minor = httpv_minor;
			}
			t = strchr(p, ' ');
			if ( !t ) {
				my_xlog(OOPS_LOG_NOTICE|OOPS_LOG_DBG|OOPS_LOG_INFORM, "analyze_header(): Wrong_header: %s\n", p);
				return;
			}
			a->status_code = atoi(t);
			my_xlog(OOPS_LOG_DBG, "analyze_header(): Status code: %d\n", a->status_code);
		}
		return;
    }
   
    if ( !strncasecmp(p, "Content-length: ", 16) ) {
		char        *x;
		/* length */
		x=p + 16; /* strlen("content-length: ") */
		while( *x && IS_SPACE(*x) ) x++;
		a->content_len = atoi(x);
		return;
    }
    if ( !strncasecmp(p, "Date: ", 6) ) {
		char        *x;
		/* length */
		x=p + 6; /* strlen("date: ") */
		while( *x && IS_SPACE(*x) ) x++;
		a->times.date  = global_sec_timer;
		if (http_date(x, &a->times.date) ) my_xlog(OOPS_LOG_DBG|OOPS_LOG_INFORM, "analyze_header(): Can't parse date: %s\n", x);
		return;
    }
    if ( !strncasecmp(p, "Last-Modified: ", 15) ) {
		char        *x;
		/* length */
		x=p + 15; /* strlen("date: ") */
		while( *x && IS_SPACE(*x) ) x++;
		if (http_date(x, &a->times.last_modified) ) my_xlog(OOPS_LOG_DBG|OOPS_LOG_INFORM, "analyze_header(): Can't parse date: %s\n", x);
		else
			a->flags |= ANSW_LAST_MODIFIED;
		return;
    }
    if ( !strncasecmp(p, "Pragma: ", 8) ) {
		char        *x;
		/* length */
		x=p + 8; /* strlen("Pragma: ") */
		if ( strstr(x, "no-cache") ) a->flags |= ANSW_NO_STORE;
		return;
    }
    if ( !strncasecmp(p, "Age: ", 5) ) {
		char        *x;
		/* length */
		x=p + 5; /* strlen("Age: ") */
		a->times.age = atoi(x);
		return;
    }
    if ( !strncasecmp(p, "Cache-Control: ", 15) ) {
		char        *x;
		/* length */
		x=p + 15; /* strlen("Cache-Control: ") */
		while( *x && IS_SPACE(*x) ) x++;
		if ( strstr(x, "no-store") )
			a->flags |= ANSW_NO_STORE;
		if ( strstr(x, "no-cache") )
			a->flags |= ANSW_NO_STORE;
		if ( strstr(x, "private") )
			a->flags |= ANSW_NO_STORE;
		if ( strstr(x, "must-revalidate") )
			a->flags |= ANSW_MUST_REVALIDATE;
		if ( !strncasecmp(x, "proxy-revalidate", 15) )
			a->flags |= ANSW_PROXY_REVALIDATE;
		if ( sscanf(x, "max-age = %d", (int*)&a->times.max_age) == 1 )
			a->flags |= ANSW_HAS_MAX_AGE;
    }
    if ( !strncasecmp(p, "Connection: ", 12) ) {
		char        *x;
		/* length */
		x = p + 12; /* strlen("Connection: ") */
		while( *x && IS_SPACE(*x) ) x++;
		if ( !strncasecmp(x, "keep-alive", 10) )
			a->flags |= ANSW_KEEP_ALIVE;
		if ( !strncasecmp(x, "close", 5) )
			a->flags &= ~ANSW_KEEP_ALIVE;
    }
    if (    !TEST(a->flags, ANSW_HAS_EXPIRES) 
		&& !strncasecmp(p, "Expires: ", 9) ) {
		char        *x;
		/* length */
		x = p + 9; /* strlen("Expires: ") */
		while( *x && IS_SPACE(*x) ) x++;
		a->times.expires  = time(NULL);
		if (http_date(x, &a->times.expires)) {
			my_xlog(OOPS_LOG_DBG|OOPS_LOG_INFORM, "analyze_header(): Can't parse date: %s\n", x);
			return;
		}
		a->flags |= ANSW_HAS_EXPIRES;
		return;
    }
}

inline static int add_header_av(char* avtext, struct mem_obj *obj)
{
	struct	av	*new_t = NULL, *next;
	char		*attr = avtext, *sp = avtext, *val, holder;
	char		*new_attr = NULL, *new_val = NULL;
	char		nullstr[1];
	
    if ( *sp == 0 ) return(-1);
    while( *sp && IS_SPACE(*sp) ) sp++;
    while( *sp && !IS_SPACE(*sp) && (*sp != ':') ) sp++;
    if ( !*sp ) {
		my_xlog(OOPS_LOG_NOTICE|OOPS_LOG_DBG|OOPS_LOG_INFORM, "add_header_av(): Invalid header string: '%s'\n", avtext);
		nullstr[0] = 0;
		sp = nullstr;
    }
    if ( *sp == ':' ) sp++;
    holder = *sp;
    *sp = 0;
    if ( !strlen(attr) ) return(-1);
    new_t = (struct av *)xmalloc(sizeof(*new_t), "add_header_av(): for av pair");
    if ( !new_t ) goto failed;
    new_attr = (char *)xmalloc( strlen(attr)+1, "add_header_av(): for new_attr" );
    if ( !new_attr ) goto failed;
    strcpy(new_attr, attr);
    *sp = holder;
    val = sp; while( *val && IS_SPACE(*val) ) val++;
    /*if ( !*val ) goto failed;*/
    new_val = (char *)xmalloc( strlen(val) + 1, "add_header_av(): for new_val");
    if ( !new_val ) goto failed;
    strcpy(new_val, val);
    new_t->attr = new_attr;
    new_t->val  = new_val;
    new_t->next = NULL;
    if ( !obj->headers ) {
		obj->headers = new_t;
    } else {
		next = obj->headers;
		while (next->next) next=next->next;
		next->next=new_t;
    }
    return(0);
	
failed:
    *sp = holder;
    if ( new_t ) free(new_t);
    if ( new_attr ) free(new_attr);
    if ( new_val ) free(new_val);
    return(-1);
}
int check_server_headers(struct server_answ *a, struct mem_obj *obj, const char *b,int len, struct request *rq)
{
	char	*start, *beg, *end, *p;
	char	holder, its_here = 0, off = 2;
	char	*p1 = NULL;
	a->http_result=0;
	assert(a);
	assert(obj);
	assert(b);
	assert(rq);
	sscanf(b,"%*s %d",&a->http_result);
   // if ( !b || !b->data ) return(0);
    beg =(char *) b;
    end = (char *)b + len;
	
    if ( end - beg > MAX_DOC_HDR_SIZE ) {
        /* Header is too large */
        return(1);
    }
go:
    if ( a->state & GOT_HDR ) return(0);
    start = beg + a->checked;
    if ( !a->checked ) {
		p = (char *)memchr(beg, '\n', end-beg);
		holder = '\n';
		if ( !p ) {
			p = (char *)memchr(beg, '\r', end-beg);
			holder = '\r';
		}
		if ( !p ) return(0);
		if ( *p == '\n' ) {
			if ( *(p-1) == '\r' ) {
				p1 = p-1;
				*p1 = 0;
			}
		}
		*p = 0;
		a->checked = strlen(start);
		/* this is HTTP XXX yyy "header", which we will never rewrite */
		analyze_header(start, a);
		if ( add_header_av(start, obj) ) {
			*p = holder;
			if ( p1 ) {*p1 = '\r'; p1=NULL;}
			return(-1);
		}
		*p = holder;
		if ( p1 ) {*p1 = '\r'; p1=NULL;}
		goto go;
    }
    if ( (end - start >= 2) && !memcmp(start, "\n\n", 2) ) {
		its_here = 1;
		off = 2;
    }
    if ( (end - start >= 3) && !memcmp(start, "\r\n\n", 3) ) {
		its_here = 1;
		off = 3;
    }
    if ( (end - start >= 3) && !memcmp(start, "\n\r\n", 3) ) {
		its_here = 1;
		off = 3;
    }
    if ( (end - start >= 4) && !memcmp(start, "\r\n\r\n", 4) ) {
		its_here = 1;
		off = 4;
    }
    if ( its_here ) {
		struct buff	*body;
		int		all_siz;
		
		obj->insertion_point = start-beg;
		obj->tail_length = off;
		a->state |= GOT_HDR ;
		obj->httpv_major = a->httpv_major;
		obj->httpv_minor = a->httpv_minor;
		obj->content_length = a->content_len;
	//	b->used = ( start + off ) - beg;	/* trunc first buf to header siz	*/
		/* if requested don't cache documents without "Last-Modified" */

		/* allocate data storage */
		if ( a->content_len ) {
			if ( a->content_len > maxresident ) {
			/*
			-  This object will not be stored, we will receive it in
			-  small parts, in syncronous mode
			-  allocate as much as we need now...
				*/
				all_siz = ROUND_CHUNKS(end-start-off);
				/*
				- mark object as 'not for store' and 'don't expand container'
				*/
				a->flags |= (ANSW_NO_STORE | ANSW_SHORT_CONTAINER);
			} else {/* obj is not too large */
				all_siz = MIN(a->content_len, 8192);
           		}
		} else { /* no Content-Len: */
            char        *transfer_encoding = NULL;
			all_siz = ROUND_CHUNKS(end-start-off);
			transfer_encoding = attr_value(obj->headers, "Transfer-Encoding");
			if ((transfer_encoding && !strncasecmp("chunked", transfer_encoding, 7)) ) {
                a->flags |= (ANSW_NO_STORE | ANSW_SHORT_CONTAINER);
            }            
        }
		body = alloc_buff(all_siz);
		if ( !body ) {
			return(-1);
		}
	//	b->next = body;
		obj->container=obj->hot_buff = body;

		attach_data(start+off, end-start-off, obj->hot_buff);
		return(0);
    }
    p = start;
    while( (p < end) && ( *p == '\r' || *p == '\n' ) ) p++;
    if ( p < end && *p ) {
		char *t = (char *)memchr(p, '\n', end-p);
		char *tmp, *saved_tmp;
		
		holder = '\n';
		if ( !t ) {
			t = (char *)memchr(p, '\r', end-p);
			holder = '\r';
		}
		if ( !t ) return(0);
		if ( *t == '\n' ) {
			if ( *(t-1) == '\r' ) {
				p1 = t-1;
				*p1 = 0;
			}
		}
		*t = 0;
		saved_tmp = tmp = strdup(p);
		if ( !tmp )
			return(-1);
	//	do_redir_rewrite_header(&tmp, rq, NULL);
		analyze_header(tmp, a);
		if ( add_header_av(tmp, obj) ) {
			free(tmp);
			return(-1);
		}
		if ( saved_tmp != tmp ) {
			/* header was changed */
			if ( obj ) SET(obj->flags, ANSW_HDR_CHANGED);
		}
		free(tmp);
		*t = holder;
		if ( p1 ) { *p1 = '\r'; t=p1; p1 = NULL;}
		a->checked = t - beg;
		goto go;
    }
    return(0);
}
void dead_obj(mem_obj *obj)
{
	pthread_mutex_lock(&obj->lock);
   	SET(obj->flags,FLAG_DEAD);
	pthread_mutex_unlock(&obj->lock);
	dead_list(&obj->m_list);
	cache_model=DROP_DEAD;
//	sleep(10);
}
#endif
