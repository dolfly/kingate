#include "kingate.h"
#include "cache.h"
#include "http.h"
#include "log.h"
#ifdef HAVE_CONFIG_H
#include "config.h"
#endif
#ifdef HAVE_SSTREAM
#include <sstream>
#else
#include "mysstream"
#endif
#include "malloc_debug.h"
#define SPLIT_VALUE	99999
volatile int cache_model=CLEAN_CACHE;
pthread_mutex_t	obj_list_mutex_t;
obj_list * l_head=NULL;
obj_list * l_end=NULL;
obj_list * l_dhead=NULL;
bool swap_out_obj(mem_obj *obj);
int total_obj_count=0;
using namespace std;
/*******************

  .........................................................
  ^oldest obj in disk(l_dhead) ^oldest obj in mem(l_head) ^newest obj in mem(l_end)

*********************/
string get_cache_index_file()
{
	string name=conf.path;
	name+="/var/cache/cache.index";
	return name;
}           
void add_list(obj_list * m_list)
{
	m_list->next=NULL;
	m_list->prev=NULL;

	if(l_end==NULL){
		l_end=m_list;
		l_head=m_list;
		l_dhead=m_list;
	}else{
		l_end->next=m_list;
		m_list->prev=l_end;
		l_end=m_list;
	}
	total_obj_count++;
};
void update_list(obj_list * m_list,bool in_mem)
{
	if(!in_mem)
		return ;
	LOCK_OBJ_LIST
	if(l_end==NULL || m_list==l_end)
		goto done;
	if(m_list==l_head){
		l_head=l_head->next;
	}
	if(m_list==l_dhead){
		l_dhead=l_dhead->next;
		l_dhead->prev=NULL;
	}
	if(m_list->prev)
		m_list->prev->next=m_list->next;
	if(m_list->next)
		m_list->next->prev=m_list->prev;	
	l_end->next=m_list;
	m_list->prev=l_end;
	l_end=m_list;
	l_end->next=NULL;
done:
	UNLOCK_OBJ_LIST
}

void dead_list(obj_list * m_list)
{
	LOCK_OBJ_LIST

	if(m_list==l_dhead)
		goto done;
	if(m_list==l_end){
		if(m_list==l_head)
			l_head=l_head->prev;
		l_end=m_list->prev;
		l_end->next=NULL;
	}else{
		if(m_list==l_head)
			l_head=l_head->next;
	}
	if(m_list->next)
		m_list->next->prev=m_list->prev;
	if(m_list->prev)
		m_list->prev->next=m_list->next;	
	l_dhead->prev=m_list;
	m_list->next=l_dhead;
	l_dhead=m_list;
	l_dhead->prev=NULL;
done:
	UNLOCK_OBJ_LIST
}
int write_string(const char *str,FILE *fp)
{
	assert(fp);
	int len=0;
	if(str!=NULL){
		len=strlen(str);
		if(len<=0){
			klog(ERR_LOG,"write a string len =%d\n",len);
		}
	}else{
		klog(ERR_LOG,"write NULL to file\n");
	}
	if(fwrite((char *)&len,1,sizeof(int),fp)<sizeof(int)){
		klog(ERR_LOG,"cann't write string %s to file,len=%d\n",str,len);
		return -1;
	}
	if(len>0){
		if(fwrite(str,1,len,fp)<len){
			klog(ERR_LOG,"cann't write string length value to file len=%d\n",len);
			return -1;
		}
	}
	return len;
}
char *read_string(FILE *fp,int max_len=2048)
{
	assert(fp);
	int len=0;
	if(fread((char *)&len,1,sizeof(int),fp)<sizeof(int))
		return NULL;
	if(len>max_len || len<0)
		return NULL;
	char *str=(char *)malloc(len+1);
	memset(str,0,len+1);
	if(len>0){
		if(fread(str,1,len,fp)<len){
			free(str);
			return NULL;
		}
	}
	return str;
}
void save_obj_index(struct mem_obj *obj,FILE *fp)
{
        assert(obj && fp);
       int len;
     /*   len=strlen(obj->url.proto);
        fwrite((char *)&len,1,sizeof(int),fp);
        fwrite(obj->url.proto,1,len,fp);
        len=strlen(obj->url.host);
        fwrite((char *)&len,1,sizeof(int),fp);
        fwrite(obj->url.host,1,len,fp);
        len=strlen(obj->url.path);
        fwrite((char *)&len,1,sizeof(int),fp);
        fwrite(obj->url.path,1,len,fp);
       */
       	write_string(obj->url.proto,fp);
	write_string(obj->url.host,fp);
	write_string(obj->url.path,fp);
        fwrite((char *)&obj->url.port,1,sizeof(u_short),fp);
        struct av *tmp=obj->headers;
        while(tmp!=NULL){
		write_string(tmp->attr,fp);
		write_string(tmp->val,fp);
                tmp=tmp->next;
        }
        len=SPLIT_VALUE;
        fwrite((char *)&len,1,sizeof(len),fp);
        fwrite((char *)obj,1,mem_obj_len,fp);

}

struct mem_obj * restore_obj_index(FILE *fp)
{
	assert(fp);
	int len=0;
	struct av *tmp=NULL;
	struct av *end=NULL;
	#define MAX_MALLOC_LEN   2048
//	len=strlen(obj->url.proto);
	char *test_str=read_string(fp);
	if(test_str==NULL){
		return NULL;
	}
	struct mem_obj *obj=(mem_obj *)malloc(sizeof(struct mem_obj));
	if(obj==NULL){
		free(test_str);
		return NULL;
	}
	memset(obj,0,sizeof(struct mem_obj));
	pthread_mutex_init(&obj->lock,NULL);
	if(len>MAX_MALLOC_LEN){
		klog(ERR_LOG,"%s:%d\n",__FILE__,__LINE__);
		goto err;
	}
	obj->url.proto=test_str;
	/*
	if(fread((char *)&len,1,sizeof(int),fp)<sizeof(int)){
		klog(ERR_LOG,"%s:%d\n",__FILE__,__LINE__);
		goto err;
	}
	
	if(len>MAX_MALLOC_LEN){
		klog(ERR_LOG,"%s:%d\n",__FILE__,__LINE__);
                goto err;
	}
	
	obj->url.host=(char *)malloc(len+1);
	memset(obj->url.host,0,len+1);
	fread(obj->url.host,1,len,fp);
	*/
	if((obj->url.host=read_string(fp))==NULL)
		goto err;
//	printf("read host=%s\n",obj->url.host);
//	len=strlen(obj->url.path);
/*
	if(fread((char *)&len,1,sizeof(int),fp)<sizeof(int)){
		klog(ERR_LOG,"%s:%d\n",__FILE__,__LINE__);
		goto err;
	}
        if(len>MAX_MALLOC_LEN){
		klog(ERR_LOG,"%s:%d\n",__FILE__,__LINE__);
                goto err;
	}
	obj->url.path=(char *)malloc(len+1);
	memset(obj->url.path,0,len+1);
	fread(obj->url.path,1,len,fp);
	*/
	if((obj->url.path=read_string(fp))==NULL)
		goto err;
//	printf("read path=%s\n",obj->url.path);
	if(fread((char *)&obj->url.port,1,sizeof(u_short),fp)<sizeof(u_short)){
		klog(ERR_LOG,"%s:%d\n",__FILE__,__LINE__);
		goto err;
	}
//	printf("read port=%d\n",obj->url.port);
	for(;;){
	//	len=strlen(tmp->attr);
		if(fread((char *)&len,1,sizeof(int),fp)<sizeof(int)){
			klog(ERR_LOG,"%s:%d\n",__FILE__,__LINE__);
			goto err;
		}
		if(len==SPLIT_VALUE)
			break;
		tmp=(av *)malloc(sizeof(struct av));
		memset(tmp,0,sizeof(struct av));
		tmp->next=NULL;
		if(tmp==NULL){
			klog(ERR_LOG,"%s:%d\n",__FILE__,__LINE__);
			goto err;
		}
		if(end==NULL){
			obj->headers=tmp;
			end=obj->headers;
		}else{
			end->next=tmp;
			end=tmp;
		}
		if(len>MAX_MALLOC_LEN || len<0){
			klog(ERR_LOG,"%s:%d\n",__FILE__,__LINE__);
              		goto err;
		}
		tmp->attr=(char *)malloc(len+1);
		memset(tmp->attr,0,len+1);
		if(fread(tmp->attr,1,len,fp)<len){
			klog(ERR_LOG,"%s:%d\n",__FILE__,__LINE__);
			goto err;
		}
//		printf("read av attr=%s\n",tmp->attr);
/*
		if(fread((char *)&len,1,sizeof(len),fp)<sizeof(int)){
			klog(ERR_LOG,"%s:%d\n",__FILE__,__LINE__);
			goto err;
		}
		if(len>MAX_MALLOC_LEN){
			klog(ERR_LOG,"len=%d is too long\n",len);
           		goto err;
		}
		tmp->val=(char *)malloc(len+1);
		memset(tmp->val,0,len+1);
		fread(tmp->val,1,len,fp);
		*/
		if((tmp->val=read_string(fp))==NULL)
			goto err;
//		printf("read av val=%s\n",tmp->val);
		
	}
	if(fread((char *)obj,1,mem_obj_len,fp)<mem_obj_len){
		klog(ERR_LOG,"%s:%d\n",__FILE__,__LINE__);
		goto err;
	}
	SET(obj->flags,FLAG_IN_DISK);
	CLR(obj->flags,FLAG_IN_MEM);
	CLR(obj->flags,KEY_CHECKED);
	return obj;
err:
	fprintf(stderr,"something happen error while load disk obj index,Please reformat the disk cache.\n");
//	assert(false);
	destroy_obj(obj,0);
	return NULL;
}

void load_all_from_disk()
{
    FILE *fp=fopen(get_cache_index_file().c_str(),"rb");
    if(fp==NULL){
            fprintf(stderr,"Can't read disk cache index.please use kingate -z to format the disk cache,kingate do not use disk cache now\n");
            klog(ERR_LOG,"Can't read disk cache index.please use kingate -z to format the disk cache\n");
		conf.use_disk_cache=false;
            return ;
    }
    struct mem_obj *obj=NULL;
	int i=0;
    for(;;){
            obj=restore_obj_index(fp);
            if(obj==NULL)
                    break;
		i++;
		stored_obj(obj);
 	   }
	fclose(fp);	
	printf("total loaded %d object from disk\n",i);
	klog(ERR_LOG,"total loaded %d object from dist\n",i);
 
	LOCK_OBJ_LIST
	l_head=l_end;
	UNLOCK_OBJ_LIST

}

void init_cache()
{
	pthread_mutex_init(&obj_list_mutex_t,NULL);
	if(conf.use_disk_cache)
		load_all_from_disk();
}
void save_index_file(bool use_obj_lock)
{
	FILE *fp=fopen(get_cache_index_file().c_str(),"wb");
	if(fp==NULL){
		klog(ERR_LOG,"Can't write disk cache index.\n");
		return;
	}
	obj_list *tmp;
	if(use_obj_lock){
		LOCK_OBJ_LIST
	}
	tmp=l_dhead;
	int i=0;
	mem_obj *obj;
	while(tmp!=NULL){
		obj=(mem_obj *)tmp->obj;
		pthread_mutex_lock(&obj->lock);
		if(
			(!TEST(obj->flags,FLAG_DEAD)) &&
			(TEST(obj->flags,FLAG_IN_DISK))
		){
			save_obj_index(obj,fp);
			i++;
		}
		pthread_mutex_unlock(&obj->lock);
		tmp=tmp->next;
	}
	if(use_obj_lock){
		UNLOCK_OBJ_LIST
	}
	fclose(fp);
	klog(START_LOG,"total save %d object to disk\n",i);

}
void save_all_to_disk()
{
	delete_cache(-1);//save all file to disk
	save_index_file();
}

void clean_disk(int m_size)
{
	obj_list * tmp;
	int kill_size=m_size;
	pthread_mutex_t * lock=NULL;
	mem_obj *obj=NULL;
	obj_list *t_head=NULL,*t_end=NULL;
	LOCK_OBJ_LIST
	cache_model=CLEAN_CACHE;
	for(;;){
		if(l_dhead==NULL){
			klog(RUN_LOG,"disk_monitor:l_dhead==NULL now\n");
			break;
		}
		tmp=l_dhead->next;
		obj=(mem_obj *)l_dhead->obj;
		lock=&obj->hash_back->lock;
		pthread_mutex_lock(lock);
		if(obj->refs<=0 || kill_size==-1){	
			if(TEST(obj->flags,FLAG_DEAD)){		
					klog(RUN_LOG,"disk_monitor:delete dead obj now.url=%s%s.\n",obj->url.host,obj->url.path);
					if(l_head==l_dhead)
						l_head=tmp;
					destroy_obj(obj,1);
					total_obj_count--;
					//				printf("destroy obj success\n");
					pthread_mutex_unlock(lock);
					l_dhead=tmp;
					continue;
			}
			if((m_size<=0) && (kill_size!=-1)){
				pthread_mutex_unlock(lock);
				break;
			}
	
	#ifndef MALLOCDEBUG
			if(TEST(obj->flags,FLAG_IN_MEM)){
				//it all in MEM FLAG
				klog(ERR_LOG,"disk_monitor:it is may a bug.or your disk cache is lower than mem cache.");
				pthread_mutex_unlock(lock);
				break;
			}
	#endif
			m_size-=obj->resident_size;
		//	printf("delete obj\n");
			klog(RUN_LOG,"disk_monitor:clean some obj. url=%s%s.\n",obj->url.host,obj->url.path);
		//	swap_out_obj((mem_obj *)l_head->obj);
			if(l_head==l_dhead)
				l_head=tmp;
			destroy_obj(obj,1);			
			total_obj_count--;
			pthread_mutex_unlock(lock);	
			l_dhead=tmp;
		}else{
			if(t_end==NULL){
				t_head=t_end=l_dhead;
				t_head->next=t_head->prev=NULL;
			}else{
				l_dhead->prev=t_end;
				t_end->next=l_dhead;
				t_end=t_end->next;
				t_end->next=NULL;
			}
			if(l_head==l_dhead)
				l_head=tmp;
			l_dhead=tmp;
			
			klog(RUN_LOG,"disk_monitor:***********************************************delete next time\n");
			pthread_mutex_unlock(lock);
		//	cache_model=DROP_DEAD;
		//	break;			
		}
//		printf("haha.................\n");
	}
done:
	if(l_dhead){
		if(t_head){
			l_dhead->prev=t_end;
			t_end->next=l_dhead;
			l_dhead=t_head;
			cache_model=DROP_DEAD;
		}else{
			l_dhead->prev=NULL;
		}
	}else{
		if(t_head){
			l_dhead=l_head;
			l_head=l_end=t_end;
			cache_model=DROP_DEAD;
		}else{
			l_head=NULL;
			l_end=NULL;
		}
	}
	UNLOCK_OBJ_LIST
	
//	printf("return delete cache\n");
}
void delete_cache(int m_size)
{
	obj_list * tmp;
	int kill_size=m_size;
	pthread_mutex_t * lock=NULL;
//	cache_model=CLEAN_CACHE;
	struct mem_obj *obj=NULL;
	LOCK_OBJ_LIST
	do{
		if(l_head==NULL){
			klog(RUN_LOG,"cache_monitor:l_head == NULL now\n");
			goto done;
		}
		#ifndef MALLOCDEBUG
		if(kill_size!=-1 && l_head->next==NULL)
			goto done;
		#endif
		tmp=l_head->next;
		obj=(mem_obj *)l_head->obj;
		lock=&(obj->hash_back->lock);
		pthread_mutex_lock(lock);
		if((obj->refs<=0) || (kill_size==-1)){
			m_size-=obj->resident_size;
		//	klog(RUN_LOG,"cache_monitor:clean some cache. url=%s%s.\n",obj->url.host,obj->url.path);
			swap_out_obj(obj);
			pthread_mutex_unlock(lock);	
		}else{
			klog(RUN_LOG,"cache_monitor:***********************************************delete next time\n");
			pthread_mutex_unlock(lock);
	//		cache_model=DROP_DEAD;
		//	break;			
		}
		l_head=tmp;
	}while(m_size>0 || kill_size==-1);
done:
/*	if(conf.use_disk_cache)
		save_index_file(false);
*/	UNLOCK_OBJ_LIST
}

