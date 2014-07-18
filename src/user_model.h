#ifndef user_model_h_987sdf8sd7f98s7dfsidfoisdf
#define user_model_h_987sdf8sd7f98s7dfsidfoisdf
extern "C"
{
	int new_user(const char *user);
	int del_user(const char *user);
	int change_passwd(const char *user,const char *passwd);
	int check_passwd(const char *user,const char *passwd);
	unsigned get_user_id(const char *user);
	int get_user_name(int uid,char *user,unsigned user_len);
	int user_model_init();
	int user_model_finit();

}
typedef int (*new_user_f)(const char *user) ;
typedef int (*del_user_f)(const char *user) ;
typedef int (*change_passwd_f)(const char *user,const char *passwd) ;
typedef int (*check_passwd_f)(const char *user,const char *passwd);
typedef unsigned (*get_user_id_f)(const char *user) ;
typedef int (*get_user_name_f)(int uid,char *user,unsigned user_len);
typedef int (*user_model_init_f)() ;
typedef int (*user_model_finit_f)();
#endif
