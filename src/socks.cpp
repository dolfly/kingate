/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/
#include "socks.h"
#include "utils.h"
#include "do_config.h"
#include "log.h"
#include "allow_connect.h"
#include "forwin32.h"
#include "KUser.h"
#include "malloc_debug.h"

mysocket * create_socks4_connect(SERVER *m_server)//建立socks4连接
{
	char host[17];
	int port;
	mysocket * server=m_server->server;
	mysocket * client=new mysocket;
	if(client==NULL)
		return NULL;
	memset(host,0,sizeof(host));
	client->set_time(conf.time_out[SOCKS]);
	char * client_request=(char *)m_server->ext;
	snprintf(host,16,"%d.%d.%d.%d",0xff&client_request[4],0xff&client_request[5],0xff&client_request[6],0xff&client_request[7]);
	port=(0xff&client_request[2])*0x100+(0xff&client_request[3]);
	if(allow_connect(SOCKS,server,host,port)!=ALLOW)
		goto clean_up;
	klog(MODEL_LOG,"socks4 client %s connect host %s:%d\n",server->get_remote_name(),host,port);
	if(client_request[1]==1){
		client_request[0]=0;
		if(client->connect(host,port)>0){
			if(is_local_ip(client))
				goto clean_up;
			client_request[1]=90;
			if(server->send(client_request,8)<=0)
				goto clean_up;
			return client;
		}else{
			client_request[1]=91;
			server->send(client_request,8);
    		goto clean_up;
		}
	//connect option;
	}else if(client_request[1]==2){
		//bind option;
	}else{
		//error;
	}
clean_up:
	delete client;
	return NULL;
}

mysocket * create_socks5_connect(SERVER *m_server)//建立socks连接，成功返回mysocket指针。失败返回NULL。
{
	#define MAXUSER		64
	int length,host_type=ADDR_IP;
	char str[PACKAGE_SIZE],host[128];
	char user[MAXUSER],passwd[MAXUSER];
	int user_len,passwd_len;
	bool user_passwd=false;
	int port_offset=8,port;
	int host_len=0,i;
	unsigned uid=0;
	mysocket * server=m_server->server;
	mysocket *client=NULL;
	socks_udp *m_socks_udp=NULL;
	str[2]=0;
	if((length=server->recv(str,PACKAGE_SIZE))<=0){
		goto cleanup;
	}
	if(str[0]==4){//如果是socks4，转socks连接。
		m_server->ext=(void *)str;
		return create_socks4_connect(m_server);
	}
	if(str[0]!=5){
		return NULL;
	}
	str[length]=0;
	//PRINT(str,length);
	client=new mysocket ;
	if(client==NULL)
		goto cleanup;
	client->set_time(conf.time_out[SOCKS]);
	str[0]=5;
	for(i=0;i<MIN(str[1],length-2);i++){//check if client use user/password auth
		if(str[i+2]==2){
			user_passwd=true;
			break;
		}
	}
	#ifndef DISABLE_USER
	if(!conf.socks5_user)
	#endif
		user_passwd=false;	
	if(user_passwd){
		str[1]=2;
	}else{
		str[1]=0;
	}
	
	if(server->send(str,2)<=0){
		goto cleanup;
	}
//	PRINT(str,2);
	if((length=server->recv(str,PACKAGE_SIZE))<=0){
		goto cleanup;
	}
//	PRINT(str,length);
	#ifndef DISABLE_USER
	if(user_passwd){
		user_len=MIN((sizeof(user)-1),str[1]);
		if(user_len<=0)
			goto skip_check;
		memcpy(user,str+2,user_len);
////		PRINT(str,length);
		user[user_len]=0;
		passwd_len=MIN((sizeof(passwd)-1),str[2+user_len]);
	//	printf("passwd_len=%d.",passwd_len);
		if(passwd_len<=0)
			goto skip_check;
		memcpy(passwd,str+3+user_len,passwd_len);
		passwd[passwd_len]=0;
		if((uid=m_user.CheckPasswd(user,passwd))==0){
			str[1]=1;
			server->send(str,2);
			klog(ERR_LOG,"user=%s.passwd=%s. is error from %s:%d\n",user,passwd,server->get_remote_name(),server->get_remote_port());
			goto cleanup;
		}
skip_check:
		str[0]=1;
		str[1]=0;
		//client->uid=uid;
//		printf("uid=%d.\n",uid);
		if(server->send(str,2)<=0){
	//		printf("%s:%d\n",__FILE__,__LINE__);
			goto cleanup;
		}
		if((length=server->recv(str,PACKAGE_SIZE))<=0){
	//		printf("%s:%d\n",__FILE__,__LINE__);
			goto cleanup;
		}
	}
	#endif
	if(str[3]==1){
		memset(host,0,sizeof(host));
		snprintf(host,16,"%d.%d.%d.%d",0xff&str[4],0xff&str[5],0xff&str[6],0xff&str[7]);
	}else{
		host_len=MIN(str[4],(sizeof(host)-1));
		if(host_len<=0)
			goto cleanup;
		memcpy(host,str+5,host_len);
		host[str[4]]=0;
		port_offset=5+str[4];
		host_type=ADDR_NAME;
	}
	port=(0xff&str[port_offset])*0x100+(0xff&str[port_offset+1]);
	if(allow_connect(SOCKS,server,str[1]!=1?(char *)1:host,port,uid)!=ALLOW){
		goto cleanup;
	}
	klog(MODEL_LOG,"socks5 client %s:%d connect host %s:%d\n",server->get_remote_name(),server->get_remote_port(),host,port);
	convert_addr(server->get_self_name(),str+4);
	if(str[1]==1){
		if(client->connect(host,port)<=0){
			str[1]=1;
			server->send(str,length);
			goto cleanup;
		}
		if(is_local_ip(client))
			goto cleanup;
		str[1]=0;str[3]=1;
		str[8]=client->get_self_port()/0x100;
		str[9]=client->get_self_port()-str[8]*0x100;
		server->send(str,10);		
		//printf("connect action\n");
	}else if(str[1]==2){
		klog(ERR_LOG,"connect refuse because socks bind operator not support\n");
		//socks5 中bind 操作，目前还没有实现
		goto cleanup;
		
	}else if(str[1]==3){//UDP ASSOCIATE
		client->set_protocol(UDP);
		m_socks_udp=new socks_udp;
		memset(m_socks_udp,0,sizeof(socks_udp));
		m_socks_udp->uid=uid;
		m_server->ext=(void *)m_socks_udp;
		if(client->open(port)>0){
			str[1]=0;
			klog(MODEL_LOG,"client %s udp associate at port %d success\n",server->get_remote_name(),port);
		}else{
			client->close();
			if(client->open()>0){
				str[1]=0;
				str[8]=client->get_self_port()/0x100;
				str[9]=client->get_self_port()-str[8]*0x100;
				klog(MODEL_LOG,"client %s udp random open port %d success\n",server->get_remote_name(),client->get_self_port());
			}else{
				str[1]=1;
				klog(ERR_LOG,"udp associate error :code=%d\n",str[1]);	
			}	
			
		}
		server->send(str,length);
//		PRINT(str,length);
		if(str[1]!=0){	
			goto cleanup;
		}
		
	}else{
	
		goto cleanup;
	}
	return client;
cleanup:
  klog(DEBUG_LOG,"something have error\n");
  if(client!=NULL)
	  delete client;
  if(m_socks_udp)
	  delete m_socks_udp;
	return NULL;
}
