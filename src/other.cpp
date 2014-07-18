#ifdef HAVE_CONFIG_H
#include <config.h>
#endif
#include "kingate.h"
#include <assert.h>
#ifdef HAVE_SSTREAM
#include <sstream>
#else
#include "mysstream"
#endif
#include "utils.h"
#include "other.h"
#include "log.h"
#include "malloc_debug.h"
#ifndef DISABLE_SMTP
static const char *b64alpha =
  "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
#define B64PAD '='
/* returns 0 ok, 1 illegal, -1 problem */
using namespace std;
unsigned int str_chr(const char *s,int c)	
{
  register char ch;
  register const char *t;
	
  ch = c;
  t = s;
  for (;;) {
    if (!*t) break; if (*t == ch) break; ++t;
    if (!*t) break; if (*t == ch) break; ++t;
    if (!*t) break; if (*t == ch) break; ++t;
    if (!*t) break; if (*t == ch) break; ++t;
  }
  return t - s;
}

int b64decode(const unsigned char *in,int l,char *out)
{
  int i, j;
  int len;
  unsigned char a[4];
  unsigned char b[3];
  char *s;
//  stringstream s;
//	printf("in=%s\n",in);
  if (l <= 0)
  {
 //   if (!stralloc_copys(out,""))
	//	return -1;

    return 0;
  }

 // if (!stralloc_ready(out,l + 2)) return -1; /* XXX generous */
 // s = out;//->s;
//	out=(char *)malloc(2*l+2);
	s=out;
  for (i = 0;i < l;i += 4) {
    for (j = 0;j < 4;j++)
      if ((i + j) < l && in[i + j] != B64PAD)
      {
        a[j] = str_chr(b64alpha,in[i + j]);
        if (a[j] > 63) {
	//		printf("bad char=%c,j=%d\n",a[j],j);
			return -1;
		}
      }
      else a[j] = 0;

    b[0] = (a[0] << 2) | (a[1] >> 4);
    b[1] = (a[1] << 4) | (a[2] >> 2);
    b[2] = (a[2] << 6) | (a[3]);

    *s = b[0];
	s++;

    if (in[i + 1] == B64PAD) break;
    *s = b[1];
	s++;

    if (in[i + 2] == B64PAD) break;
     *s = b[2];
	 s++;
  }

  len = s - out;
//  printf("len=%d\n",len);
  while (len && !out[len - 1]) --len; /* XXX avoid? */
  return len;

//  return s.str();
}

string b64encode(const unsigned char *in,int len)
/* not null terminated */
{
  unsigned char a, b, c;
  int i;
 // char *s;
	stringstream s;
  if (len <= 0)
  {
	return "";
  }

 // if (!stralloc_ready(out,in->len / 3 * 4 + 4)) return -1;
 // s = out->s;

  for (i = 0;i < len;i += 3) {
    a = in[i];
    b = i + 1 < len ? in[i + 1] : 0;
    c = i + 2 < len ? in[i + 2] : 0;

    s << b64alpha[a >> 2];
    s << b64alpha[((a & 3 ) << 4) | (b >> 4)];

    if (i + 1 >= len) s << B64PAD;
    else s <<  b64alpha[((b & 15) << 2) | (c >> 6)];

    if (i + 2 >= len) s << B64PAD;
    else s << b64alpha[c & 63];
  }
//  out->len = s - out->s;
  return s.str();
}
mysocket * create_smtp_connect(SERVER * m_server)
{
	int port=0;
//	char m_smtp[HOST_LEN];
	mysocket * client=new mysocket;
	mysocket * server=m_server->server;
	char *user=NULL;
	char *real_user=NULL;
	char *host=NULL;
	int ulen=0;
	int ret=0,length;
	string msg;
//	stringstream s;
	if(client==NULL)
		return NULL;
	char *buf=(char *)malloc(1025);//[1024*4];
	if(buf==NULL){
		delete client;
		return NULL;
	}
	server->send("220 ESMTP\r\n");
	for(;;){
		length=server->recv(buf,1024,"\r\n");
		if(length<=0)
			goto clean;
	//	printf("buf=%s.\n",buf);
		if(strncasecmp(buf,"EHLO",4)==0){
	//		printf("send ehlo resp now.\n");
			server->send("250-\r\n250-AUTH=LOGIN\r\n250-PIPELINING\r\n250 8BITMIME\r\n");
			continue;
		}
		if(strncasecmp(buf,"AUTH",4)==0){
			server->send("334 VXNlcm5hbWU6\r\n");
			if((length=server->recv(buf,1024,"\r\n"))<=0)
				goto clean;
			user=(char *)malloc(length+2);
			if(user==NULL)
				goto clean;
			ulen=b64decode((unsigned char *)buf,strlen(buf),user);
	//		printf("buf=%s,ulen=%d,length=%d\n",buf,ulen,length);
			if(ulen<=0)
				goto clean;
			user[ulen]=0;	
			real_user=(char *)malloc(ulen+1);
			host=(char *)malloc(ulen+1);
			if(real_user==NULL||host==NULL)

				goto clean;
	//		printf("user=%s\n",user);
			ret=split_user_host(user,real_user,ulen,host,ulen);
			if(ret==0)
				goto clean;
			if((port=split_host_port(host,':',ulen))<=0)
				port=25;
			if(ret==2){//the host don't need login
	//			printf("the host don't need login*************************************\n");
				server->send("334 UGFzc3dvcmQ6\r\n");
				if(server->recv(buf,1024,"\r\n")<=0)
					goto clean;
			}
			break;
		}
		if(strncasecmp(buf,"QUIT",4)==0){
			goto clean;
		}
		
	}
//	printf("real_user=%s,host=%s,port=%d\n",real_user,host,port);

	if(allow_connect(SMTP,m_server->server,host,port)!=ALLOW){
		server->send("500 proxy server don't allow you access this host\r\n");
		goto clean;
	}
	client->set_time(conf.time_out[SMTP]);
	if(client->connect(host,port)<0)
		goto clean;
	if(is_local_ip(client))
		goto clean;

//	printf("connect to %s:%d success\n",host,port);
//	printf("ret=%d\n",ret);
	if(ret==2){
		client->send("HELO kingate_net\r\n");
//		printf("use not login model\n");
	}else{
		client->send("EHLO kingate_net\r\n");
//		printf("use login model\n");
	}
	for(;;){
		length=client->recv(buf,1024,"\r\n");
		if(length<=0)
			goto clean;
	//	buf[length]=0;
//		printf("buf=%s,length=%d\n",buf,length);
		if(length<=3)
			break;
		if(strncasecmp(buf,"250 ",4)==0){
//			printf("end msg now\n");
			break;
		}
	}
	if(ret==2){
		server->send("235 ok, go ahead (#2.0.0)\r\n");
		goto done;
	}
	client->send("AUTH LOGIN\r\n");
//	printf("now login remote\r\n");
	length=client->recv(buf,1024,"\r\n");
//	printf("recv remote reply:%s\n",buf);
	if(length<=0)
		goto clean;
	msg=b64encode((unsigned char *)real_user,strlen(real_user))+"\r\n";
	client->send(msg.c_str());
	klog(MODEL_LOG,"[SMTP] %s:%d connect host %s:%d success\n",m_server->server->get_remote_name(),m_server->server->get_remote_port(),host,port);
done:
	free(buf);
	if(host)
		free(host);
	if(user)
		free(user);
	if(real_user)
		free(real_user);
	return client;
clean:
	free(buf);
	klog(MODEL_LOG,"[SMTP] %s:%d use smtp proxy error\n",m_server->server->get_remote_name(),m_server->server->get_remote_port());
	if(host)
		free(host);
	if(user)
		free(user);
	if(real_user)
		free(real_user);
	server->send("505 something happen error!\r\n");
	delete client;	
	return NULL;
	
}
#endif
#ifndef DISABLE_POP3
mysocket * create_pop3_connect(SERVER * m_server)
{
	int length,port=0;
	char host[64],msg[128],str[128];	
	host[0]='\0';msg[0]='\0';str[0]='\0';
	mysocket * client;
	
	mysocket * server=m_server->server;
	if(server->send("+OK kingate pop3 proxy\r\n")<=0)
		return NULL;
	for(int i=0;i<3;i++){
             if((length=server->recv(str,sizeof(str)-1,"\r\n"))<=0)
                              return NULL;
              	 str[length]=0;
	//       printf("str=%s\n",str);	
	       	 if(strncasecmp(str,"USER",4)==0)
                         goto start_pop3;
	         if(server->send("-ERR authorization first\r\n")<=0)
	                 return NULL;
     	}
        return NULL;
start_pop3:
	split_user_host(str,msg,sizeof(msg)-4,host,sizeof(host));
//	printf("str=%s,msg=%s,host=%s.\n",str,msg,host);
	strcat(msg,"\r\n");
	if((port=split_host_port(host,':',sizeof(host)))<=0)
		port=110;
	client=new mysocket;
	client->set_time(conf.time_out[POP3]);	
	if(allow_connect(POP3,server,host,port)!=ALLOW)
		goto cleanup;
	if(client->connect(host,port)<=0)
		goto cleanup;	
	if(is_local_ip(client))
		goto cleanup;
	if(client->recv(str,sizeof(str))<=0)
		goto cleanup;
	if(client->send(msg)<=0)
		goto cleanup;
	klog(MODEL_LOG,"[POP3] %s:%d connect host %s:%d sucess\n",server->get_remote_name(),server->get_remote_port(),host,port);
	return client;

cleanup:
	klog(MODEL_LOG,"[POP3] %s:%d connect remotehost %s:%d error\n",server->get_remote_name(),server->get_remote_port(),host,port);
	if(client!=NULL)
		delete client;
	return NULL;

}
#endif
#ifndef DISABLE_TELNET
mysocket *create_telnet_connect(SERVER *m_server)//建立telnet 连接，成功返回mysocket连接，失败返回NULL。
{
	mysocket *server=m_server->server;
	int length,port=23;
	int host_len;
	const char control_str[]={0xff,0xfd,0x18,0xff,0xfd,0x20,0xff,0xfd,0x23,0xff,0xfd,0x27,0xff,0xfa,0x20,0x01,0xff,0xf0,0xff,0xfa,0x27,0x01,0xff,0xf0,0xff,0xfa,0x18,0x01,0xff,0xf0,0xff,0xfb,0x03,0xff,0xfd,0x01,0xff,0xfd,0x1f,0xff,0xfb,0x05,0xff,0xfd,0x21,0xff,0xfb,0x01,0x00};
	char backspace[][8]={{0x1b,0x5b,0x44,0x20,0x1b,0x5b,0x44,0x00},{0x08,0x20,0x08,0x00}};
	const char s_reset[]={0x1b,0x5d,0x30,0x3b,0x6b,0x69,0x6e,0x67,0x40,0x65,0x32,0x70,0x3a,0x7e,0x07,0x00};
	mysocket * client=new mysocket;
	client->set_time(conf.time_out[TELNET]);
	char host[256],msg[1024];
	int backno;
	memset(msg,0,sizeof(msg));
	snprintf(msg,sizeof(msg)-1,"Welcome to kingate %s telnet proxy.\r\nPlease enter host and port\r\nexample: abc.com 23\r\n",VER_ID);
	server->send(msg);
	for(;;){
		host[0]='\0';
		host_len=0;
		server->send("kingate >");
		for(;;){
			length=server->recv(msg,128);
			if(length<=0)
				goto err;
		//	printf("host_len=%d.\n",host_len);
			if((msg[0]==0x7f)||(msg[0]==0x08)){
				if(msg[0]==0x08){
					backno=0;
				}else{
					backno=1;
				}
				if(host_len<=0)
					continue;
				else{
					host_len--;
				}
				if(server->send(backspace[backno],strlen(backspace[backno]))<=0){
					delete client;
					return NULL;
				}
				continue;
			}else{
			//	printf("%02x\n",msg[0]);
				if(msg[0]==0x0d){
					server->send("\r\n");
					break;
				}
		/*		if(msg[0]<33||msg[0]>127)
					continue;
		*/		msg[length]=0;
		//		printf("msg=%s.\n",msg);
				if(host_len+length>128)
					break;
				memcpy(host+host_len,msg,length);
				host_len+=length;
				if(strstr(host,"\r\n")){
					host_len-=2;
					break;
				}
			}
			if(server->send(msg,length)<=0){
				goto err;	
			}
		}
		host[host_len]=0;
		if((port=split_host_port(host,' ',sizeof(host)))<=0)
			port=23;
		memset(msg,0,sizeof(msg));
		snprintf(msg,sizeof(msg)-1,"Now kingate try to connect %s %d...\r\n",host,port);
		server->send(msg);
		if(allow_connect(TELNET,server,host,port)!=ALLOW){
				server->send("kingate not allow you to connect the host.\r\nPlease contact the Administrator.\r\n");
				goto err;
		}
	//	server->send(s_reset,sizeof(s_reset));
		if(client->connect(host,port)<=0){
			server->send("kingate cann't connect the host.\r\n");
		/*	delete client;
			return NULL;

			*/
			continue;
		}
		if(is_local_ip(client))
			goto err;
	//	client->recv(msg,128);
	return client;
	}
err:
	delete client;
	return NULL;
}
#endif
#ifndef DISABLE_MMS
mysocket *create_mms_connect(SERVER *m_server)//建立mms 连接，成功返回mysocket连接，失败返回NULL。
{
	mysocket *server=m_server->server;
	mysocket *client=new mysocket;
	int length,i=0,j=0,port=0;
	char str[1024];
	char tmp[1024];
	char host[HOST_LEN];
	str[0]=0;tmp[0]=0;host[0]=0;
	length=server->recv(str,1024);
	if(length-44<=0)
		goto clean;
	memcpy(tmp,str+44,length-44);
	if(tmp[i]==0)
		i++;
	for(;i<length-44;i=i+2){
		tmp[j]=tmp[i];
		j++;
	}
	if(sscanf(tmp,"%*[^Host] Host: %100[^:]:%d",host,&port)<1)
		goto clean;
	if(port==0)
		port=1755;
	if(allow_connect(MMS,server,host,port)!=ALLOW)
		goto clean;
	client->set_time(conf.time_out[MMS]);
	if(client->connect(host,port)<=0)
		goto clean;
	if(is_local_ip(client))
		goto clean;
//	client->set_time();
	if(client->send(str,length)<=0)
		goto clean;
	return client;
clean:
	delete client;
	klog(MODEL_LOG,"[MMS] %s:%d cann't connect host %s:%d",server->get_remote_name(),server->get_remote_port(),host,port);
	return NULL;
}
#endif
#ifndef DISABLE_RTSP
mysocket *create_rtsp_connect(SERVER *m_server)//建立mms 连接，成功返回mysocket连接，失败返回NULL。
{
	mysocket *server=m_server->server;
	mysocket *client=new mysocket;
	int length,port=0;
	char str[1024];
	char host[257];
	str[0]=0;host[0]=0;
	int left_recv=sizeof(str)-1;
	char *buf=str;
	for(;;){
		length=server->recv(buf,left_recv);
		if(length<=0)
			goto clean;
		buf[length]=0;
		if((strstr(str,"\r\n\r\n")!=NULL) || (strstr(str,"\n\n")!=NULL))
			break;	
		left_recv-=length;
		buf+=length;
		if(left_recv<=0)
			break;
	}
//	printf(str,"%s\n",str);
	if(sscanf(str,"%*s %*[^/]//%256[^:]:%d",host,&port)<1)
		goto clean;
	if(port==0)
		port=554;
//	printf("host=%s,port=%d\n",host,port);
	if(allow_connect(RTSP,server,host,port)!=ALLOW)
		goto clean;
	client->set_time(conf.time_out[RTSP]);
	if(client->connect(host,port)<=0)
		goto clean;
	if(is_local_ip(client))
		goto clean;
	if(client->send(str,length)<=0)
		goto clean;
//	client->set_time();
	return client;
clean:
	delete client;
	return NULL;
}
#endif
#ifndef DISABLE_REDIRECT
mysocket * create_redirect_connect(SERVER * m_server)
{
	REDIRECT * redirect=(REDIRECT *)m_server->ext;
	mysocket *server=m_server->server;
	mysocket *client=new mysocket;
	
	if(allow_connect(redirect->src_port,server,redirect->dest_addr,redirect->dest_port)!=ALLOW)
		goto cleanup;
	client->set_time(redirect->time_out);
	server->set_time(redirect->time_out);
	if(client->connect(redirect->dest_addr,redirect->dest_port)<=0)
		goto cleanup;
	return client;
cleanup:
	if(client!=NULL)
		delete client;
	return NULL;
}

void redirect_proxy(REDIRECT * redirect)
{
	assert(redirect);
	if(redirect==NULL)
		return;
	SERVICE *tmp=new SERVICE;
	if(tmp==NULL)
		return;//no mem to alloc
	memset(tmp,0,sizeof(tmp));
	if(tmp->server.open(redirect->src_port)<=0){
		klog(ERR_LOG,"cann't open redirect port:%d\n",redirect->src_port);
//		printf("cann't open redirect port:%d\n",redirect->src_port);
		delete tmp;
		return;
	}
	klog(START_LOG,"open redirect port %d success\n",redirect->src_port);
//	printf("open redirect port %d success\n",redirect->src_port);
//	tmp->m_server.m_filter=&redirect->m_filter;
//	tmp->m_server.time_out=0;
//	tmp->m_server.create_connect=create_redirect_connect;
	tmp->m_server.ext=(void *)redirect;
//	tmp->m_server.port=redirect->src_port;
	tmp->next=service_head;
	tmp->m_server.model=redirect->src_port;
	service_head=tmp;
	return;	
}
#endif
