#include "kingate.h"
#ifndef _WIN32
#include <syslog.h>
#include <pthread.h>
#include <signal.h>
#endif
#include <stdarg.h>
#include <string.h>
#include <stdio.h>
#include <time.h>
#include "utils.h"
#include "log.h"
#define OPEN_FILE
#include "forwin32.h"
#include "malloc_debug.h"

int log_fp=0;
/**********************************************************
level数字越低,表明级别越高.
level=0		启动级别,启动过程的最基本的信息,以及运行时出错信息.
level=1		运行级别,运行过程最基本的信息记录
level=2		模块级别,运行过程记录到模块级别,及模块内的运行状态.
level=3		调试级别,记录一些调试信息
***********************************************************/
FILE *m_fp=NULL;
pthread_mutex_t log_lock;
#ifndef _WIN32
static sigset_t m_log_blockset;
#endif
void klog(int level,const char *fmt,...)
{

	if(level>conf.log_level)
		return;
	if(conf.log_model==LOG_NONE_MODEL)
		return;
	if(m_fp==NULL)
		return;
	va_list ap;
#ifndef _WIN32
	sigset_t m_old_sigset;
	pthread_sigmask(SIG_BLOCK,&m_log_blockset,&m_old_sigset);
#endif
	va_start(ap, fmt);
	/*
#ifndef _WIN32
	if(conf.log_model==LOG_SYSTEM_MODEL){
		syslog(LOG_NOTICE,fmt,ap);
		return;
	}	
#endif 
*/
	if(conf.log_model==LOG_USER_MODEL){
		time_t ltime;
		time(&ltime);
		char tm[30];
	//	snprintf(tm,20,"%s",ctime(&ltime));
		CTIME_R(&ltime,tm,sizeof(tm));
		tm[19]=0;
		pthread_mutex_lock(&log_lock);
		fprintf(m_fp,"%s|",tm);
		vfprintf(m_fp, fmt, ap);
		pthread_mutex_unlock(&log_lock);
	//	fflush(m_fp);
		va_end(ap);
		// close(log_fp);
	}else if(conf.log_model==LOG_DEBUG_MODEL){
		vprintf(fmt,ap);
	}
#ifndef _WIN32
	pthread_sigmask(SIG_SETMASK,&m_old_sigset,NULL);
#endif	
}
void klog_open_file()
{

	if(m_fp!=NULL)
		return;
	char *file_name=(char *)malloc(conf.path.size()+17);
	strcpy(file_name,conf.path.c_str());
	strcat(file_name,"/var/kingate.log");
	m_fp = fopen(file_name,"a+");
	free(file_name);
	if(m_fp == NULL)
	{
		fprintf(stderr,"Open log file error!\n");
		
	}

}
int klog_start()
{/*
	if(log_fp<=0){
		//	printf("open log file now\n");
		if((log_fp=open(conf.log_file,O_SYNC|O_APPEND|O_WRONLY))<=0)
			//  printf("open append model failed\n");
			if((log_fp=open(conf.log_file,O_SYNC|O_CREAT|O_WRONLY))<=0){
				//conf.log_model=LOG_SYSTEM;
				//klog("cann't open log file: %s,force to use system model\n",conf.log_file);
				return 0;
			}
		
	}
	return 1;
*/
#ifndef _WIN32
	sigemptyset(&m_log_blockset);
	sigfillset(&m_log_blockset);
#endif		 
	klog_open_file();
	pthread_mutex_init(&log_lock,NULL);
	return 1;
}
void klog_flush()
{
	fflush(m_fp);
}
void klog_close_file()
{
	klog_flush();
	if(m_fp)
		fclose(m_fp);
	m_fp=NULL;
}
void klog_close()
{
	klog_close_file();
	pthread_mutex_destroy(&log_lock);
}
void klog_rotate()
{
	struct tm * tm;
	char *new_file=(char *)malloc(conf.path.size()+1+2*17+32);
	if(new_file==NULL)
		return;
	char *old_file=(char *)malloc(conf.path.size()+16+1);
	if(old_file==NULL){
		free(new_file);
		return;
	}
	time_t now_tm;
	now_tm=time(NULL);
	tm=localtime(&now_tm);
	sprintf(old_file,"%s/var/kingate.log",conf.path.c_str());
	sprintf(new_file,"%s/var/kingate.log%02d%02d_%02d%02d%02d\0",conf.path.c_str(),tm->tm_mon+1,tm->tm_mday,tm->tm_hour,tm->tm_min,tm->tm_sec);
	pthread_mutex_lock(&log_lock);
	klog_close_file();
//	system(cmd);	
	rename(old_file,new_file);
	klog_open_file();
	pthread_mutex_unlock(&log_lock);
	klog(RUN_LOG,"rotate log success\n");
	free(new_file);
	free(old_file);
}
