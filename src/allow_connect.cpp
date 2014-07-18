#ifndef _WIN32
#include<netdb.h>
#include<arpa/inet.h>

#include<unistd.h>
#include <pthread.h>
#endif
#include "allow_connect.h"
#include "log.h"
#include "utils.h"
#include "cron.h"
#include "assert.h"
#include "do_config.h"

#include "oops.h"
#include "malloc_debug.h"

//static LOCAL_IP * local_ip;
static int open_port[TOTAL_SERVICE];
static int max_open_service=0;
bool is_local_ip(mysocket *client)
{
	if(client->get_self_addr()!=client->get_remote_addr())
		return false;
	int dst_port=client->get_remote_port();
	for(int i=0;i<max_open_service;i++){
		if(dst_port==open_port[i]){
			if(strcmp(conf.bind_addr,"*")==0 || strcmp(conf.bind_addr,client->get_remote_name())==0 ){
				klog(RUN_LOG,"*******************It may be a hack***************\n");
				return true;
			}
		}
	}
	return false;
}
unsigned allow_connect2(mysocket * server,int service_port,const char *dst_addr,int dst_port,unsigned long &dst_ip,bool add_to_filter=true,unsigned uid=0)
{

	filter_time m_filter_time;
	m_filter_time.m_state.src_ip=server->get_remote_addr();
	m_filter_time.m_state.service_port=service_port;
//	int key=server->get_socket();
	if(dst_addr!=NULL){
		if(dst_addr==(const char *)1){//it is a localhost
	//		printf("dst addr is a localhost\n");
			m_filter_time.m_state.dst_ip=1;
		}else{
			m_filter_time.m_state.dst_ip=m_dns_cache.GetName(dst_addr);
			if(m_filter_time.m_state.dst_ip==0)
				goto bad_dns_name;
		}
	}else{
		m_filter_time.m_state.dst_ip=0;
	}
/*
	if(is_local_ip(server,m_filter_time.m_state.dst_ip,dst_port))
		goto deny;
*/	m_filter_time.m_state.dst_port=dst_port;
	m_filter_time.server=server;
	m_filter_time.m_state.uid=uid;
	if(conf.m_kfilter.Check(m_filter_time.m_state)){
		if(add_to_filter)
			add_filter_time(m_filter_time);
		dst_ip=m_filter_time.m_state.dst_ip;
		return ALLOW;
	}
deny:
	klog(RUN_LOG,"rule not allow pass src %s from %d to %s:%d\n",server->get_remote_name(),service_port,(dst_addr==NULL?"null":(dst_addr==(const char *)1?"localhost":dst_addr)),dst_port);
	return DENY;
bad_dns_name:
	klog(RUN_LOG,"bad dns name %s\n",dst_addr);
	return BAD_DNS_NAME;

}
int allow_connect(int model_name,mysocket * server,const char *dest_addr,int dest_port,unsigned uid)
{
	unsigned long dst_ip;
	return allow_connect2(server,(model_name>10?model_name:conf.port[model_name]),dest_addr,dest_port,dst_ip,true,uid);
}
int allow_connect(int model_name,mysocket * server,const char *dest_addr,int dest_port,unsigned long &dst_ip,bool add_to_filter,unsigned uid)
{
	return allow_connect2(server,(model_name>10?model_name:conf.port[model_name]),dest_addr,dest_port,dst_ip,add_to_filter,uid);
}

void init_allow_connect()
{
	for(int i=0;i<TOTAL_SERVICE;i++){
		if(conf.state[i]==1){
			open_port[max_open_service]=conf.port[i];
			max_open_service++;
		}
	}
}

