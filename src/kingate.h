#ifndef KINGATE_H_asdfkjl23kj4234
#define KINGATE_H_asdfkjl23kj4234
#ifdef HAVE_CONFIG_H
#include "config.h"
#endif 
#ifdef	_WIN32
#define VERSION   		"2.0-win32"
#endif 
#define PROGRAM_NAME	"kingate" 
#define VER_ID			VERSION
#define	VER_STRING		"Author king(email:khj99@tom.com)\r\nhttp://sourceforge.net/projects/kingate/"
//#define VERSION		VER_ID
#define MAX_URL			255
#define MAX_HOST		MAX_URL
#define HOST_LEN		MAX_URL		//主机最大长度
#if	!defined(MAX)
#define	MAX(a,b)		((a)>(b)?(a):(b))
#endif
#if	!defined(MIN) 
#define	MIN(a,b)		((a)>(b)?(b):(a))
#endif
#define PACKAGE_SIZE		2048	//定义一次处理多少数据
#define PASV_CMD		1
#define PORT_CMD		2		
//#define PID_FILE	"kingate.pid"
//#define CMD_FILE	"kingate.cmd"

#define CONNECT_TIME_OUT	30



#define HTTP_TRANS		HTTP+100
#define HTTPS			HTTP+101

#define MODEL_SOCKS_UDP		3000


#define NORMAL_PROXY		0	//传透代理
#define TRANS_PROXY		1	//透明代理

#define	LOG_USER_MODEL		0
#define	LOG_NONE_MODEL		1
#define LOG_SYSTEM_MODEL	2
#define LOG_DEBUG_MODEL		3

#define FILTER_FILE		0
#define FILTER_OPEN		1
#define FILTER_CLOSE		2


//得到X的值并放到Y里面，其默认值是Z。
#define GET(X,Y,Z) get_value(config_name,config_value,X,Y,Z)

#ifndef ACCESS
	#define ACCESS
	#define DENY		0
	#define ALLOW		1
#endif
/**************************************************************************************/
#define TOTAL_SERVICE		9
/************************/
#define MANAGE			0
#define RTSP			1
#define	FTP				2
#define POP3			3
#define SMTP			4
#define TELNET			5
#define MMS				6
#define SOCKS			7
#define HTTP			8
/**************************************************************************************/
#define HTTP_SHORT		9
static const char service_name[][12]={"manage","rtsp","ftp","pop3","smtp","telnet","mms","socks","http"};

#ifdef _WIN32
#define HAVE_SSTREAM 1
#endif
extern char *lang;
extern int kingate_start_time;
#endif

