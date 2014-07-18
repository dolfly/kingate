#ifndef ALLOW_CONNECT_H_ASDL23421341324
#define ALLOW_CONNECT_H_ASDL23421341324
#include "do_config.h"
#include "kingate.h"
#include "mysocket.h"
#define		BAD_DNS_NAME	2
int allow_connect(int model_name,mysocket * server,const char *dest_addr,int dest_port,unsigned uid=0);
int allow_connect(int model_name,mysocket * server,const char *dest_addr,int dest_port,unsigned long &dst_ip,bool add_to_filter=true,unsigned uid=0);
void init_allow_connect();
int name2ip(const char *name,char *ip);//获取主机name的第一个ip地
bool is_local_ip(mysocket *client);
#endif
