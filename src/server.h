#ifndef MAIN_H_3523213E1QWE3R1W3ER13W2ER1XC1V3D1S3F
#define MAIN_H_3523213E1QWE3R1W3ER13W2ER1XC1V3D1S3F
#include "do_config.h"
void shutdown(bool reboot=false);
int stop(int service);
void sigcatch(int sig);
void service_from_signal();
int service_to_signal(const char *cmd);
void save_pid();
int create_file_path(char **argv);
void restore_pid();
int parse_args(int argc,char ** argv);
void init_daemon();
void init_program();
int start(int service);
int main(int argc ,char **argv);
int forward_signal(const char *protocol);
int get_service(const char * service);
const char * get_service_name(int service);
int get_service_id(const char * service);
void set_user(const char *user);
typedef struct{
	int main;
	int accept;
	int service[TOTAL_SERVICE];
} SERVICE_PID;
extern int m_pid;
#endif
