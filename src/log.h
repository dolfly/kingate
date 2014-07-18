#ifndef LOG_H_23541234123413241234
#define LOG_H_23541234123413241234
#include "do_config.h"
#define START_LOG	0
#define ERR_LOG		0
#define RUN_LOG		1
#define MODEL_LOG	2
#define DEBUG_LOG	3
void klog(int level,const char *fmt,...);
int klog_start();
void klog_close();
void klog_rotate();
void klog_flush();
void CTIME_R(time_t *a, char *b, size_t l);
#ifdef _WIN32
void LogEvent(const char *pFormat, ...);
#endif
#endif
