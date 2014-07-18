#ifndef CLASS_KConfig_H_9238741023841324
#define CLASS_KConfig_H_9238741023841324
#define ITEM_HEIGHT	200
#define ITEM_WIDTH	200
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#ifndef _WIN32
#include <unistd.h>
#endif
#include<vector>
#include<string>
//配制文件读取类
#define KConfig_FILE_SIZE 4068
//#ifndef MAX_WIDTH
//#define MAX_WIDTH 256
//#endif
typedef struct
{
	std::string name;//[MAX_WIDTH+1];
	std::string value;//[MAX_WIDTH+1];
	bool name_multi;
	bool value_multi;
} ITEM_T;
typedef std::vector<ITEM_T> item_vector;
class KConfig
{
public:
	KConfig();
	~KConfig();
	int open(const char * m_file);
	int create(char *str);

	void print_all_item();
	const char *GetValue(const char *name,int index=0);
	const char *GetName(int index);
	const char *GetValue(int index);
	int getSize();
	int GetName(const char *name,int index=0);
	bool SetValue(const char *name,const char *value,int index=0,bool value_multi=false);
	bool SetValue(int index,const char *value);
	bool AddValue(const char *name,const char *value,bool name_multi=false,bool value_multi=false,bool sort=false);
	bool DelName(const char *name,int index=0);
	bool SaveFile(const char *filename);
	bool delItem(int index);
	

private:
	int skip_next_line(const char *str);
	int get_word(char *str,std::string &word,bool &multi,bool &is_line_end);
	int match_name(char *str,std::string &name,std::string &value);
	int split_config_file(char *str);

private:
	int h_item_num;
	item_vector item;
	bool name_multi;
	bool value_multi;
};
#endif
