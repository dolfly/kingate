#ifndef OTHER_H_654sad6f54as6df54a6sdf54asd65f4
#define OTHER_H_654sad6f54as6df54a6sdf54asd65f4
#include "utils.h"
#include "mysocket.h"
#include "socks.h"
#include "do_config.h"
#include "log.h"
#include "allow_connect.h"
#include "ftp.h"
//const char * get_smtp_server(mysocket * server);
mysocket * create_redirect_connect(SERVER * m_server);
void redirect_proxy(REDIRECT * redirect);
mysocket * create_smtp_connect(SERVER * m_server);

mysocket * create_pop3_connect(SERVER * m_server);

mysocket *create_telnet_connect(SERVER *m_server);//建立telnet 连接，成功返回mysocket连接，失败返回NULL。

mysocket *create_mms_connect(SERVER *m_server);//建立telnet 连接，成功返回mysocket连接，失败返回NULL。

mysocket *create_rtsp_connect(SERVER *m_server);//建立telnet 连接，成功返回mysocket连接，失败返回NULL。

void dns_proxy(SERVER *m_server);
mysocket * create_http_connect(SERVER * m_server);

//void init_other(CONFIG *m_conf);
#endif
