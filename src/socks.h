#ifndef SOCKS5_H
#define SOCKS5_H
#include "do_config.h"
#include "utils.h"
#include "mysocket.h"
void socks_proxy();
mysocket * create_socks5_connect(SERVER *m_server);
//void init_socks(CONFIG * m_conf);
#endif
