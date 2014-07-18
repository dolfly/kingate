#include "KConfig.h"
#include <iostream>
#include <fstream>
#include <assert.h>
#include "malloc_debug.h"
using namespace std;
long file_size(FILE *fp)
{
	long begin,end,current;
	assert(fp);
	if(fp==NULL)
		return -1;
	current=ftell(fp);
	fseek(fp,0,SEEK_SET);
	begin=ftell(fp);
	fseek(fp,0,SEEK_END);
	end=ftell(fp);
	fseek(fp,current,SEEK_SET);
	return end-begin;
	
}
KConfig::~KConfig()
{

}
KConfig::KConfig()
{
	//memset(config_name,0,sizeof(config_name));
}

int KConfig::open(const char * m_file)
{
	int i;
	char *str;
	FILE *fp;
	if((fp=fopen(m_file,"r"))==NULL){
	//	fprintf(stderr,"cann't open config file,filename is:%s\n",m_file);
		return 0;

	}
	long len=file_size(fp)+1;
	str=(char *)malloc(len);
	if(str==NULL){
	//	fprintf(stderr,"cann't alloc mem in open function\n");
		return 0;
	}
//	printf("malloc str=%x\n",str);
	memset(str,0,len);
	i=fread(str,1,len-1,fp);
	fclose(fp);
	str[i]=0;
	h_item_num=split_config_file(str);
//	printf("free str=%x\n",str);
	free(str);
	return 1;
}
int KConfig::create(char *str)
{
//	config_name[0][0]=0;
	h_item_num=split_config_file(str);
	return 1;
}
int KConfig::skip_next_line(const char *str)
{
	int size=strlen(str);
	int i;
//	printf("str=%s\n",str);
	for(i=0;i<size-1;i++){
		if(str[i]=='\n')
			return i+1;
	}
	return 0;
}
int KConfig::get_word(char *str,string &word,bool &multi,bool &is_line_end)
{	
	int i,p,size=strlen(str);
	//printf("str=%s.\n",str);
	multi=false;
	int tmp=0;
	if(size<=0){
		return -1;
	}
	for(i=0;i<size;i++){
		if( (str[i]=='#') || (str[i]=='\n') || (str[i]=='\r')  )
			return 0;
		if(str[i]=='{'){
			if(i==size-1){
				return 0;
			}
			multi=true;
			i++;
			break;
		}
		if((str[i]!=32)&&(str[i]!='\t'))
			break;
	}	
	p=i;
	if(!multi){
		for(i=p+1;i<size;i++){
			if((str[i]==32)||(str[i]=='\n')||(str[i]=='\t')||(str[i]=='\r'))
				break;
		}
	}else{
		for(i=p+1;i<size;i++)
			if(str[i]=='}')
				break;
	}
//	printf("str=%s\n",str);
	if(str[i]=='\n'){
		is_line_end=true;
	}/*
		tmp=skip_next_line(str+i);
	}else{
		tmp=1;
	}*/
	str[i]=0;/*
	int len=((MAX_WIDTH>(i-p))?i-p:MAX_WIDTH );

	if(len<=0)
		return 0;
		
	//printf("len=%d ",len);
	memcpy(word,str+p,len);
	word[len]='\0';
	*/
	word=str+p;
//	printf("word=%s.i=%d,p=%d\n",word,i,p);
	return i+1;
}
int KConfig::match_name(char *str,string &name,string &value)
{
	int point;
	int total_point=0;
	bool is_line_end=false;
	int tmp=0;
//	printf("str=%s\n",str);
get_name:
	if((point=get_word(str,name,name_multi,is_line_end))<=0){
//		printf("point error=%d\n",point);
		if(point==-1){
			return point;
		}else{
			tmp=skip_next_line(str);
			if(tmp<=0){
				return -1;
			}
//			printf("tmp=%d\n");
			str+=tmp;
			total_point+=tmp;
			goto get_name;
		}
	}
	total_point+=point;
	if(is_line_end){
//		printf("name line is end,name=%s\n",name.c_str());
		tmp=skip_next_line(str+total_point);
		total_point+=tmp;
//		printf("tmp=%d\n,total_point=%d\n",tmp,total_point);
		return total_point;
	}
//	printf("point=%d\n",point);
	point=get_word(str+point,value,value_multi,is_line_end);
//	printf("name=%s,value=%s\n",name.c_str(),value.c_str());
	//	return 1;
	total_point+=point;
//	printf("point2=%d\n",point);
	if(!is_line_end){
//		printf("total_point=%d\n",total_point);
		tmp=skip_next_line(str+total_point);
		total_point+=tmp;
//		printf("value is line not end total_point=%d\n",total_point);
	}
	return total_point;

}

int KConfig::split_config_file(char *str)
{
	int i=0,point=0;
	int skip=0;
	int tmp;
	ITEM_T t_item;
//	char name[MAX_WIDTH+1];
//	char value[MAX_WIDTH+1];
	//printf("str=%s\n",str);
	while(1){
		t_item.name="";
		t_item.value="";
		skip=match_name(str,t_item.name,t_item.value);
		if(skip>=0){
			t_item.name_multi=name_multi;
			t_item.value_multi=value_multi;
		//	t_item.name=name;
		//	t_item.value=value;
//			t_item.skip=false;
			item.push_back(t_item);		
		}else{
		//	printf("skip=%d\n",skip);
			break;
		}
		if(skip==0){
			break;
		}
	//	printf("skip=%d\n",skip);
		str=str+skip;
	/*
		if((tmp=skip_next_line(str))<=0)
			break;
		printf("tmp=%d\n",tmp);
		point=tmp;	
		str+=tmp;
		*/
	//	printf("str=%s\n",str);
	//	printf("i=%d,point=%d\n",i,point);
	}
	return i;
	
}
const char *KConfig::GetValue(const char *name,int index)
{
	int i;
	int len=item.size();
	int p=0;
	for(i=0;i<len;i++){
		if( (strcmp(item[i].name.c_str(),name)==0) ){
			if(index==p++)
				return item[i].value.c_str();
		}
	}
	return "";
}
void KConfig::print_all_item()
{
	int len=item.size();
	for(int i=0;i<len;i++)
		printf("%s=%s.\n",item[i].name.c_str(),item[i].value.c_str());

}
int KConfig::GetName(const char *name,int index)
{
	int i;
	int len=item.size();
	int p=0;
	for(i=0;i<len;i++){
		if( (strcmp(item[i].name.c_str(),name)==0) ){
			if(index==p++)
				return i;
		}
	}
	return -1;
}
const char *KConfig::GetName(int index)
{
	if(index<0 || index>=item.size())
		return "";
	return item[index].name.c_str();
}
const char *KConfig::GetValue(int index)
{
	if(index<0 || index>=item.size())
		return "";
	return item[index].value.c_str();
}
int KConfig::getSize()
{
	return item.size();
}
bool KConfig::SaveFile(const char *filename)
{
	FILE *fp=fopen(filename,"wt");
	if(fp==NULL){
		return false;
	}
	for(int i=0;i<item.size();i++){
		if(item[i].name_multi){
			fwrite("{",1,1,fp);
		}
		fwrite(item[i].name.c_str(),1,item[i].name.size(),fp);
		if(item[i].name_multi){
			fwrite("}",1,1,fp);
		}
		fwrite(" ",1,1,fp);
		if(item[i].value_multi){
			fwrite("{",1,1,fp);
		}
		fwrite(item[i].value.c_str(),1,item[i].value.size(),fp);
		if(item[i].value_multi){
			fwrite("}",1,1,fp);
		}
		fwrite("\r\n",1,2,fp);
	}
	fclose(fp);
	return true;
}
bool KConfig::AddValue(const char *name,const char *value,bool name_multi,bool value_multi,bool sort)
{
	ITEM_T tmp_item;
	tmp_item.name_multi=name_multi;
	tmp_item.value_multi=value_multi;
	tmp_item.name=name;
	tmp_item.value=value;
//	tmp_item.skip=false;
	
	if(!sort){
	
		item.push_back(tmp_item);

	}else{
	
		int i=GetName(name);
	
		if(i<0){
		
			item.push_back(tmp_item);

		}else{

			item.insert(item.begin()+i,tmp_item);

		}
	}
	return true;

}
bool KConfig::SetValue(const char *name,const char *value,int index,bool value_multi)
{
	int p=GetName(name,index);
	if(p<0){
		return AddValue(name,value,false,value_multi);
	}
	item[p].value=value;
	item[p].value_multi=value_multi;
	return true;
}
bool KConfig::SetValue(int index,const char *value)
{
	if(index<0 || index>=item.size())
		return false;
	item[index].value=value;
	//item[index].value_multi=true;
	return true;
}
bool KConfig::delItem(int index)
{	
	if(index<0 || index>=item.size())
		return false;
	item.erase(item.begin()+index);
	return true;
}
bool KConfig::DelName(const char *name,int index)
{
	int p=GetName(name,index);
	if(p<0)
		return false;
	return delItem(p);
}
