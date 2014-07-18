#ifndef KHttpManageInclude_randomsdflisf97sd9f09s8df
#define KHttpManageInclude_randomsdflisf97sd9f09s8df

#include <map>
#include <string>
#include "kingate.h"
#include "do_config.h"
#include "mysocket.h"
#include "log.h"
#include "http.h"
#include "KUser.h"

class KHttpManage
{
public:
	KHttpManage();
	~KHttpManage();
	bool start(request *rq);	
private:
	request *rq;
	std::map<std::string,std::string> urlParam;
	std::string getUrlValue(std::string name);
	bool sendHttp(const std::string &msg);
	void sendTest();
	bool parseUrlParam(char *param);
	bool parseUrl(char *url);
	
	bool sendErrPage(const char *err_msg,int closed_flag=0);
	bool sendMainPage();
	bool sendLeftMenu();
	bool sendMainFrame();
	#ifndef DISABLE_USER
	bool userList();
	bool loginList();
	bool sendLogin();
	#endif
	bool config();
	bool configsubmit();
	bool sendRedirect(const char *newUrl);
	#ifndef DISABLE_USER
	LoginUserInfo user_info;
	#endif
};
#endif

