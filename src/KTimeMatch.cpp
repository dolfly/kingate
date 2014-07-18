#include <string.h>
#include <stdlib.h>
#include <time.h>
#include <stdio.h>
#include "KTimeMatch.h"
#include "malloc_debug.h"
#include "log.h"

KTimeMatch::KTimeMatch()
{
	memset(min,0,sizeof(min));
	memset(hour,0,sizeof(hour));
	memset(day,0,sizeof(day));
	memset(month,0,sizeof(month));
	memset(week,0,sizeof(week));
	point=0;
	open=false;
	
}
KTimeMatch::~KTimeMatch()
{

}
bool KTimeMatch::Set(const char * time_model)
{
	int i=0;
	if(time_model==NULL||strlen(time_model)==0){
		open=true;
		time_str="";
		return true;
	}
	int len=strlen(time_model);
	for(i=0;i<len;i++){
		if(!( (time_model[i]<='9' && time_model[i]>='0') || time_model[i]==' ' || time_model[i]=='\t' || time_model[i]=='*' || time_model[i]=='-' || time_model[i]==',')){
			klog(ERR_LOG,"time_str \"%s\" have unknow char :%c\n",time_model,time_model[i]);
			return false;
		}
	}
	if(!Set(time_model,min,FIRST_MIN,LAST_MIN,true))
		goto time_err;
	if(!Set(time_model,hour,FIRST_HOUR,LAST_HOUR))
		goto time_err;
	if(!Set(time_model,day,FIRST_DAY,LAST_DAY))
		goto time_err;
	if(!Set(time_model,month,FIRST_MONTH,LAST_MONTH))
		goto time_err;
	if(!Set(time_model,week,FIRST_WEEK,LAST_WEEK))
		goto time_err;
	for(i=point;i<len;i++){
		if(time_model[i]!=' ' && time_model[i]!='\t'){
	
			klog(ERR_LOG,"time_str \"%s\" have more char :%c\n",time_model,time_model[i]);
	
			return false;

		}
	
	}
	if(week[LAST_WEEK] || week[0]){// sunday is 0 or 7
		week[0]=true;
		week[LAST_WEEK]=true;
	}
	time_str=time_model;
	return true;
time_err:
	klog(ERR_LOG,"set time_str \"%s\" error,error point at %d!Please check.\n",time_model,GetErrPoint());
	return false;	       
}
int KTimeMatch::GetErrPoint()
{
	return point;
}
bool KTimeMatch::Set(const char * time_model,bool s[],int low,int hight,bool first)
{
	int i;
	int len=strlen(time_model);
	bool have_atoi=false;
	bool have_add=false;
	int num;
	int end;
//	printf("low=%d,hight=%d\n",low,hight);
	if(!first){
		
		while(true){
			if(point>len)
				break;
			if(time_model[point]=='\t' || time_model[point]==' ')
				break;
			point++;
		}
		
	}
	while(true){
		if(point>len)
			break;
		if((time_model[point]!='\t') && (time_model[point]!=' '))
			break;
		point++;
	}
	while(!((time_model[point]=='\t') || (time_model[point]==' '))){
		if(point>len){
//			printf("~~~~~~~~~~~~~~\n");
			return have_add;
		}
		if(time_model[point]=='*'){
			point++;
			for(i=low;i<=hight;i++){
				s[i]=true;
			}
			return true;
		}
		
		if(time_model[point]==','){
			point++;
			have_atoi=false;
			continue;
		}
		if(time_model[point]=='-'){
		//	printf("_____________________\n");
			point++;
			end=atoi(time_model+point);
			if(end<low||end>hight||end<=num){

				klog(ERR_LOG,"end value=%d is error in time_str \"%s\" point %d\n",end,time_model,point);
				return false;
			}
			for(i=num;i<=end;i++)
				s[i]=true;
			have_atoi=true;
		}
	//	printf("******************\n");
		if(!have_atoi){
			have_atoi=true;
			have_add=true;
			num=atoi(time_model+point);
		//	printf("char=%c,num=%d,point=%d\n",time_model[point],num,point);
			if(num<low||num>hight){
				klog(ERR_LOG,"value %d is out of range in time_str \"%s\" point %d,allowed values :%d-%d \n",num,time_model,point,low,hight);
				return false;
			}
			s[num]=true;
		}
		point++;
			
	}
	return true;
}
void KTimeMatch::Show()
{
	int i;
	printf("min:");
	for(i=FIRST_MIN;i<=LAST_MIN;i++){
		if(min[i]){
			printf("%d,",i);
		}
	}
	printf("\nhour:");
	for(i=FIRST_HOUR;i<=LAST_HOUR;i++){
		if(hour[i])
			printf("%d,",i);
	}
	printf("\nday:");
	for(i=FIRST_DAY;i<=LAST_DAY;i++){
		if(day[i])
			printf("%d,",i);
	}
	printf("\nmonth:");
	for(i=FIRST_MONTH;i<=LAST_MONTH;i++){
		if(month[i])
			printf("%d,",i);
	}
	printf("\nweek:");
	for(i=FIRST_WEEK;i<=LAST_WEEK;i++){
		if(week[i])
			printf("%d,",i);
	}
	printf("\n");
}
bool KTimeMatch::Check()
{
	struct tm * tm;
	time_t now_tm;
	if(open)
		return true;
	now_tm=time(NULL);
	tm=localtime(&now_tm);
//	printf("min=%d,hour=%d,day=%d,mon=%d,wday=%d.\n",tm->tm_min,tm->tm_hour,tm->tm_mday,tm->tm_mon,tm->tm_wday);
	if(min[tm->tm_min]&&hour[tm->tm_hour]&&day[tm->tm_mday]&&month[tm->tm_mon+1]&&week[tm->tm_wday])
		return true;
	return false;
}
const char *KTimeMatch::GetTime()
{
	return time_str.c_str();

}


