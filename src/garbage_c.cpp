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
#include	"kingate.h"
#ifdef HAVE_CONFIG_H
#include "config.h"
#endif
#include	"oops.h"
#include	"cache.h"
#include	"utils.h"
#include	"log.h"
#include	"server.h"
#include	"lib.h"
#include	"cron.h"
#include	"utils.h"
#include	"KThreadPool.h"
#include	"KDnsCache.h"

#include 	<string>
#ifdef		HAVE_SSTREAM
#include <sstream>
#else
#include "mysstream"
#endif
#include	<time.h>
#ifndef _WIN32
#include    <dirent.h>
#endif
#include	"KUser.h"
#include	"KFilter.h"
#include "malloc_debug.h"
void list_all_malloc();
#define		CHECK_INTERVAL	5
using namespace std;
static int count_file_name=0;
volatile bool quit_kingate=false;
string get_service_to_name(int port);
void flush_net_request();
struct filter_time_result
{
	filter_time f;
	string title_msg;
};
string get_disk_cache_dir(int url_hash,int file_name=0)
{
	stringstream s;
        int i=url_hash/1000;
        int j=url_hash/100-i*10;
        int k=url_hash/10-i*100-10*j;
        int p=url_hash-1000*i-100*j-10*k;
#ifndef _WIN32
        s << conf.path << "/var/cache/" << i << "/" << j << "/" << k << "/" << p << "/" ;

#else
        s << conf.path << "\\var\\cache\\" << i << "\\" << j << "\\" << k << "\\" << p << "\\" ;
#endif
	if(file_name>0)
		s << file_name;
       return s.str();

}
void check_time()
{
	filter_time_map_t::iterator it;
	filter_time_lock.Lock();
	for(it=filter_time_list.begin();it!=filter_time_list.end();){
	//	printf("have one.uid=%d\n",(*it).second.m_state.uid);
		if(!conf.m_kfilter.Check((*it).second.m_state)){
			(*it).second.server->close();
#ifndef _WIN32
			pthread_kill((*it).first,KINGATE_NOTICE_THREAD);
			filter_time_list.erase(it);
			it++;
#else
			it=filter_time_list.erase(it);
#endif
			klog(RUN_LOG,"now force to close now.\n");
		}else{
			it++;
		}
	}	
clean:
	filter_time_lock.Unlock();
}
void closeAllConnection()
{
	filter_time_map_t::iterator it;
	filter_time_lock.Lock();
	pthread_t tid;
	for(it=filter_time_list.begin();it!=filter_time_list.end();){
			(*it).second.server->close();
			tid=(*it).first;
#ifndef _WIN32
			pthread_kill(tid,KINGATE_NOTICE_THREAD);
			filter_time_list.erase(it);
			it++;
#else
			it=filter_time_list.erase(it);
			LogEvent("now force to close the connection.\n");
#endif
			klog(RUN_LOG,"now force to close the connection(thread_id=%d).\n",tid);
	}
clean:
	filter_time_lock.Unlock();
}
void getConnectionTr(filter_time_result &m_filter_time_result,stringstream &s,int now_time)
{
		s << "<tr><td>" << make_ip(m_filter_time_result.f.m_state.src_ip);
		#ifndef DISABLE_USER
		if(m_filter_time_result.f.m_state.uid>STARTUID){
			string user;
			m_user.GetUserName(m_filter_time_result.f.m_state.uid,user);
			s << "[" << user << "]";
		}
		#endif
		s << "</td><td>" << get_service_to_name(m_filter_time_result.f.m_state.service_port) ;
		s << "</td><td>" << make_ip(m_filter_time_result.f.m_state.dst_ip);
		s << "</td><td>" << m_filter_time_result.f.m_state.dst_port;
		s << "</td><td>" << (now_time-m_filter_time_result.f.start_time);
		s << "</td><td>" ;
		if(m_filter_time_result.title_msg.size()>0){
			if(strncasecmp(m_filter_time_result.title_msg.c_str(),"http://",7)==0){
				s << "<a href=\"" << m_filter_time_result.title_msg << "\" target=_blank>" << m_filter_time_result.title_msg << "</a>";
			}else{
				s << m_filter_time_result.title_msg;
			}
		}else{ 
			s << "NULL";
		}
		s << "</td></tr>";
}
string getConnectionInfo(int sortby,bool desc)
{
	unsigned now_time=time(NULL);
	bool have_insert;
	list<filter_time_result> m_tmp;
	list<filter_time_result>::iterator it2;
	filter_time_result	tmp_result;
	stringstream s;
	string m1,m2;
	filter_time_map_t::iterator it;
	filter_time_lock.Lock();
	for(it=filter_time_list.begin();it!=filter_time_list.end();it++){
		have_insert=false;
		memcpy(&tmp_result,&(*it).second,sizeof(filter_time));
		tmp_result.f.server->get_title_msg(tmp_result.title_msg);
		for(it2=m_tmp.begin();it2!=m_tmp.end();it2++){
			if((sortby==1 && tmp_result.f.start_time>(*it2).f.start_time) || (sortby==0 && tmp_result.f.m_state.src_ip>(*it2).f.m_state.src_ip) || (sortby==2 && tmp_result.title_msg>(*it2).title_msg)){
				m_tmp.insert(it2,tmp_result);
				have_insert=true;
				break;
			}
		}	
		if(!have_insert){
			m_tmp.push_back(tmp_result);
		}
	}
	if(desc){
		for(it2=m_tmp.begin();it2!=m_tmp.end();it2++){
			getConnectionTr((*it2),s,now_time);
		}
	}else{
		list<filter_time_result>::reverse_iterator it3;
		for(it3=m_tmp.rbegin();it3!=m_tmp.rend();it3++){
			getConnectionTr((*it3),s,now_time);
		}
	}
	filter_time_lock.Unlock();
//	s << "<hr><center>Generated by kingate(" << VER_ID << ")</center>";
	return s.str();
}
void get_cache_size(unsigned long *total_mem_size,unsigned long *total_disk_size)
{
	int i;
	for (i=0;i<HASH_SIZE;i++) {
		assert(hash_table[i].size>=0);
		*total_mem_size += hash_table[i].size;
		*total_disk_size+=hash_table[i].disk_size;
    	}
}
void flush_mem_cache(void)
{
	unsigned long total_size=0;
	int kill_size;//destroyed, gc_mode;
	unsigned long total_disk_size=0;
	get_cache_size(&total_size,&total_disk_size);
//	#ifdef MALLOCDEBUG
//	list_all_malloc();	
//	printf("now have total mem cache size=%d(%d-%d), total disk cache size=%d(%d-%d).\n",total_size,conf.mem_min_cache,conf.mem_max_cache,total_disk_size,conf.disk_min_cache,conf.disk_max_cache);
//	#endif
	kill_size=total_size-conf.mem_cache;
	if (kill_size>0){
		delete_cache(kill_size);
	}
	kill_size=total_disk_size-conf.disk_cache;
	if(kill_size>0){
		clean_disk(kill_size);
	}else if(cache_model==DROP_DEAD){
		clean_disk(0);
	}
	return;
}
/*
int sync_cache_dir(int index)
{
#ifndef _WIN32
	string dir=get_disk_cache_dir(index);
	string dir_file_name;
	DIR *dp=opendir(dir.c_str());
        dirent *dt;
	mem_obj *obj;
	bool file_exsit=false;
	char file_name[32];
	if(dp==NULL)
		return 0;
	pthread_mutex_lock(&hash_table[index].lock);
          for(;;){
                dt=readdir(dp);
                if(dt==NULL)
                      break;
		if(strcmp(dt->d_name,".")==0 || strcmp(dt->d_name,"..")==0)
			continue;
		obj=hash_table[index].next;
		file_exsit=false;
		for(;;){
			if(obj==NULL)
				break;
			memset(file_name,0,sizeof(file_name));
			snprintf(file_name,31,"%d",obj->file_name);
			if(strcmp(file_name,dt->d_name)==0){
				file_exsit=true;
				break;
			}
			obj=obj->next;
		}
		if(!file_exsit){
			dir_file_name=dir+dt->d_name;
			klog(ERR_LOG,"remove unexsit file %s\n",dir_file_name.c_str());
			unlink(dir_file_name.c_str());	
		} 
          }
	  pthread_mutex_unlock(&hash_table[index].lock);
#endif
	  return 1;

}
*/
FUNC_TYPE FUNC_CALL time_thread(void* arg)
{
	int i=1;
	KTimeMatch m_log_rotate;
	m_log_rotate.set(conf.log_rotate.c_str());
	quit_kingate=false;
	int last_rotate_i=-31;
	int rotate_len=0;
	//setuid(500);
	forever() {
		my_sleep(CHECK_INTERVAL);	
		if(quit_kingate)
			break;
		flush_mem_cache();
		check_time();
		klog_flush();
		flush_net_request();

	/*
		if(count_sync_complete<=HASH_SIZE){
			sync_cache_dir(count_sync_complete++);		
		}
	*/
		#ifndef DISABLE_USER
		m_user.FlushLoginUser();
		#endif
		if(m_log_rotate.check()){//rotate log now
			rotate_len=i-last_rotate_i;
			if(rotate_len>30 || rotate_len<-30){
				klog_rotate();
				save_index_file();
				last_rotate_i=i;		
			}
		}
		if(i%12==0){//every minute
			m_thread.Flush();			
	//		m_dns_cache.Flush();
		}
		if(i%720==0){//every hour
			m_dns_cache.Flush();
			conf.m_kfilter.checkExpireChain();
			#ifndef DISABLE_USER
			m_user.SaveAll();
			#endif
		}
		i++;

    	} 
				return

#ifndef _WIN32

			NULL

#endif

			;}
string get_disk_cache_file(struct mem_obj *obj)
{
	assert(obj);
	u_short url_hash = hash(&obj->url);
     	if(obj->file_name==0){
                obj->file_name=time(NULL)+count_file_name++;
        }
	return get_disk_cache_dir(url_hash,obj->file_name);
}

bool swap_out_obj(struct mem_obj *obj)
{
	string name;
	FILE *fp=NULL;
	struct buff *tmp;
	if(!conf.use_disk_cache)
		goto swap_out_failed;
	if(!TEST(obj->flags,FLAG_IN_MEM)){
		return true;
	}
	if(TEST(obj->flags,FLAG_IN_DISK)){
		goto skip_save_disk;
	}
	name=get_disk_cache_file(obj);
	fp=fopen(name.c_str(),"wb");
	if(fp==NULL){
		klog(ERR_LOG,"Cann't open file %s to write.\n",name.c_str());
		goto swap_out_failed;
	}
	
	klog(RUN_LOG, "Now swap out obj %s:%d%s\n",obj->url.host,obj->url.port,obj->url.path);
	tmp=obj->container;
	while(tmp!=NULL){
		if(fwrite(tmp->data,1,tmp->used,fp)<tmp->used){
			klog(ERR_LOG,"Cann't write file %s to disk.\n",name.c_str());
			goto swap_out_failed;
		}
		tmp=tmp->next;
	}
	fclose(fp);
	SET(obj->flags,FLAG_IN_DISK);
	increase_hash_size(obj->hash_back, obj->resident_size,false);//increase disk size
skip_save_disk:
	CLR(obj->flags,FLAG_IN_MEM);
	free_container(obj->container);
	obj->container=obj->hot_buff=NULL;
	decrease_hash_size(obj->hash_back, obj->resident_size);
	return true;
swap_out_failed:
	SET(obj->flags,FLAG_DEAD);
	cache_model=DROP_DEAD;
	free_container(obj->container);
	obj->container=obj->hot_buff=NULL;
//	decrease_hash_size(obj->hash_back, obj->resident_size);
	return false;
}
bool swap_in_obj(struct mem_obj *obj)
{	
	bool result=true;
	if(TEST(obj->flags,FLAG_IN_MEM))
		return true;
	string name=get_disk_cache_file(obj);
	FILE *fp=fopen(name.c_str(),"rb");
	if(fp==NULL){
		klog(ERR_LOG,"Cann't open file %s to read.\n",name.c_str());
		SET(obj->flags,FLAG_DEAD);
		return false;
	}
	klog(RUN_LOG,"Now swap in obj %s:%d%s\n",obj->url.host,obj->url.port,obj->url.path);
	char *buf=(char *)malloc(2048);
	if(buf==NULL){
		fclose(fp);
		SET(obj->flags,FLAG_DEAD);
		return false;
	}
	int total_len=0;
	for(;;){
		int len=fread(buf,1,2048,fp);
		if(len<=0)
			break;
		total_len+=len;
      		if(store_in_chain(buf, len, obj) ) {
               		klog(ERR_LOG, "Can't store.\n");
	              	obj->flags |= FLAG_DEAD;
			free_container(obj->container);
			fclose(fp);
			free(buf);
			return false;
        	}
	}
	free(buf);
	if(total_len!=obj->content_length){
		result=false;
                klog(ERR_LOG, "load from disk length: %d  is not equale content_length\n",total_len,obj->content_length );
		SET(obj->flags,FLAG_DEAD);
        }
	fclose(fp);
	if(result){
	//	CLR(obj->flags,FLAG_IN_DISK);
		SET(obj->flags,FLAG_IN_MEM);
		increase_hash_size(obj->hash_back, obj->resident_size);
	}
	return result;
}
