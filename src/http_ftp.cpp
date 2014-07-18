#include "kingate.h"
#ifndef _WIN32
#include <pthread.h>
#include <signal.h>
#include <unistd.h>
#endif
#ifdef HAVE_SSTREAM
#include <sstream>
#else
#include "mysstream"
#endif
#include <string>
#include "server.h"
#include "utils.h"
#include "allow_connect.h"
#include "log.h"
#include "forwin32.h"
#include "http.h"
#include "ftp.h"
#include "kingate.h"
#include "malloc_debug.h"

#define ERR_MSG_SIZE	256
#define MAX_CONTENT_TYPE	100
static const char content_type[][2][MAX_CONTENT_TYPE]={
{"bin","application/macbinary"}  
,{"oda","application/oda"}    
,{"exe","application/octet-stream"}    
,{"pdf","application/pdf"}    
,{"ai","application/postscript"}
,{"eps","application/postscript"}
,{"ps","application/postscript"}
,{"rtf","application/x-rtf"}  
,{"Z","application/x-tar"} 
,{"gz","application/x-tar"}
,{"tgz","application/x-tar"}  
,{"csh","application/x-csh"}  
,{"dvi","application/x-dvi"}  
,{"hdf","application/x-hdf"}  
,{"latex","application/x-latex"} 
,{"lsm","text/plain"}
,{"nc","application/x-netcdf"}  
,{"cdf","application/x-netcdf"}  
,{"sh","application/x-sh"}   
,{"tcl","application/x-tcl"}  
,{"tex","application/x-tex"}  
,{"texi","application/x-texinfo"}
,{"texinfo","application/x-texinfo"}    
,{"t","application/x-troff"} 
,{"roff","application/x-troff"}  
,{"tr","application/x-troff"}
,{"man","application/x-troff-man"}     
,{"me","application/x-troff-me"}
,{"ms","application/x-troff-ms"}
,{"src","application/x-wais-source"}   
,{"zip","application/x-zip-compressed"}
,{"bcpio","application/x-bcpio"} 
,{"cpio","application/x-cpio"}
,{"gtar","application/x-gtar"}
,{"rpm","application/x-rpm"}  
,{"shar","application/x-shar"}
,{"sv4cpio","application/x-sv4cpio"}    
,{"sv4crc","application/x-sv4crc"}    
,{"tar","application/x-tar"}  
,{"ustar","application/x-ustar"} 
,{"au","audio/basic"}  
,{"snd","audio/basic"}  
,{"mp2","audio/basic"}  
,{"mp3","audio/basic"}  
,{"aif","audio/x-aiff"} 
,{"aiff","audio/x-aiff"}
,{"aifc","audio/x-aiff"}
,{"wav","audio/x-wav"}  
,{"ief","image/ief"} 
,{"jpeg","image/jpeg"}  
,{"jpg","image/jpeg"}
,{"jpe","image/jpeg"}
,{"tiff","image/tiff"}  
,{"tif","image/tiff"}
,{"ras","image/cmu-raster"}   
,{"pnm","image/x-portable-anymap"}     
,{"pbm","image/x-portable-bitmap"}     
,{"pgm","image/x-portable-graymap"}    
,{"ppm","image/x-portable-pixmap"}     
,{"rgb","image/x-rgb"}  
,{"xbm","image/x-xbitmap"}    
,{"xpm","image/x-xpixmap"}    
,{"xwd","image/x-xwindowdump"}
,{"html","text/html"}
,{"htm","text/html"} 
,{"c","text/plain"} 
,{"h","text/plain"} 
,{"cc","text/plain"}
,{"cpp","text/plain"}
,{"hh","text/plain"}
,{"m","text/plain"} 
,{"f90","text/plain"}
,{"txt","text/plain"}
,{"rtx","text/richtext"}
,{"tsv","text/tab-separated-values"}   
,{"etx","text/x-setext"}
,{"mpeg","video/mpeg"}  
,{"mpg","video/mpeg"}
,{"mpe","video/mpeg"}
,{"qt","video/quicktime"}    
,{"mov","video/quicktime"}    
,{"avi","video/x-msvideo"}    
,{"movie","video/x-sgi-movie"}
,{"hqx","application/mac-binhex40"}    
,{"mwrt","application/macwriteii"}     
,{"msw","application/msword"} 
,{"doc","application/msword"} 
,{"xls","application/msexcel"}
,{"wk[s1234]","application/vnd.lotus-1-2-3"}     
,{"mif","application/x-mif"}  
,{"sit","application/stuffit"}
,{"pict","application/pict"}  
,{"pic","application/pict"}   
,{"arj","application/x-arj-compressed"}
,{"lzh","application/x-lha-compressed"}
,{"lha","application/x-lha-compressed"}
,{"zlib","application/x-deflate"}
,{"core","application/octet-stream"}   
,{"png","image/png"} 
,{"cab","application/octet-stream"}  
,{"","application/octet-stream"}//default mime type
};
static const int max_len=512;
using namespace std;
int get_tmp(char m_char)
{
	if(m_char<='9' && m_char>='0')
                     return 0x30;
         if(m_char<='f' && m_char>='a')
                      return 0x57;
         if(m_char<='F' && m_char>='A')
               return 0x37;
       return 0;
															                                                                                                               
}
string url_encode(const char *str,size_t len_string=0)
{

        stringstream s;
        if(len_string==0)
         	len_string=strlen(str);
        const unsigned char *msg=(const unsigned char *)str;
        const char * conv = "0123456789ABCDEF";       
  	int i=0,j; 
        for(j=0;j<len_string;j++){
        	if((msg[j]>='.' && msg[j]<='9')||(msg[j]>='A' && msg[j]<='Z')||(msg[j]>='a' && msg[j]<='z')|| msg[j]=='_' )
        		s << msg[j];
        	else {
	                s << '%' << conv[msg[j]>>4] << conv[msg[j]&0xF];
                 }
        }
        return s.str();


}
int url_unencode(char *url_msg,size_t url_len)
{
	 int j=0;
	 int i=0;
	// printf("url=%s\n",url_msg);
         if(url_len==0)
         url_len=strlen(url_msg);
	  for(;;){
                   if(i>=url_len){
			   url_msg[j]=0;
                            return j;
		   }
	            if(url_msg[i]=='%' && (i<=url_len-3) && (get_tmp(url_msg[i+1])!=0) && (get_tmp(url_msg[i+2])!=0)){
	       		
	            	url_msg[j]=(url_msg[i+1]-get_tmp(url_msg[i+1]))*0x10+url_msg[i+2]-get_tmp(url_msg[i+2]);
	               	i+=3;
	          }else{
	               if(url_msg[i]=='+')
	               		url_msg[j]=' ';
	               else
	              		url_msg[j]=url_msg[i];
	               i++;
	          }
		j++;
              }
}

void http_ftp_start(request * rq)
{
	assert(rq);
	int file_size=0;
	char buffer[PACKAGE_SIZE+1];
	char ret[4];
	char err_msg[ERR_MSG_SIZE];//=NULL;
	
	bool err_msg_delete=false;
	mysocket * server=rq->server;
	SERVER m_server;
	mysocket data_client,client;
	char http_head[PACKAGE_SIZE];
//	char proxy_connection[20];
	memset(err_msg,0,ERR_MSG_SIZE);
	if(rq->url.login==NULL){
		rq->url.login=strdup("anonymous");
		rq->url.password=strdup("kingateUser");
	}else{
		
		url_unencode(rq->url.login);
		if(rq->url.password==NULL){
			rq->url.password=(char *)malloc(1);
			rq->url.password[0]=0;
		}else{
			url_unencode(rq->url.password);
		}
	}
	//get_http_head_value(m_state->buf,"Proxy-Connection",proxy_connection,20);
	assert(server);
	client.set_time(conf.time_out[HTTP]);
	if(!client.connect(rq->url.dst_ip,rq->url.dst_port)){
		klog(ERR_LOG,"[HTTP_FTP]error,cann't connect to %s:%d\n",rq->url.host,rq->url.port);
		strncpy(err_msg,"cann't connect to the host.",ERR_MSG_SIZE);
		goto clean;
	}
	if(is_local_ip(&client)){
		client.close();
		goto clean;
	}
	data_client.set_time(conf.time_out[HTTP]);
/*
	if(client.recv(buffer,PACKAGE_SIZE,"\r\n")<=0){
		klog(ERR_LOG,"[HTTP_FTP]error,cann't recv welcoming message from host:%s:%d\n",rq->url.host,rq->url.port);
		err_msg="remote host have closed the connection.while recv welcoming message.";
		goto clean;
	}
*/
//	buffer[length]=0;
//	printf("%s",buffer);
	
	if(!ftp_login(&client,&rq->url,err_msg)){
		klog(ERR_LOG,"[HTTP_FTP]user name or passwd is error.\n");
		goto clean;
	}	
	if(!ftp_set_type(&client,"I",err_msg)){
		klog(ERR_LOG,"[HTTP_FTP]error while set type to I.\n");
		goto clean;
	}
	if(!build_data_connect(&client,&data_client,err_msg))
		goto clean;

	//try to get url->file
	url_unencode((char *)rq->url.path);
	memset(buffer,0,sizeof(buffer));
	snprintf(buffer,sizeof(buffer)-1,"RETR %s\r\n",rq->url.path);
	/*client.send("RETR ");
	client.send(rq->url.path);
*/
	if(client.send(buffer)<0)
		goto clean;
	if(client.recv(buffer,PACKAGE_SIZE)<=0){
		strncpy(err_msg,"remote host have closed the connection while try to recv retr cmd result.",ERR_MSG_SIZE);
		goto clean;
	}
	//buffer[length]=0;
//	printf("%s",buffer);
//	memcpy(ret,buffer,3);
//	ret[3]=0;
	if(buffer[0]=='5'){
		klog(ERR_LOG,"[HTTP_FTP]error,the file don't exit or it is a directory.\n");
		
		if(!ftp_is_dir(server,&client,&rq->url,err_msg))
			goto clean;
		return;
		
	}else if(buffer[0]=='1'){
		//printf("%s\n",buffer);
		sscanf(buffer,"%*[^(](%d",&file_size);
		//printf("file_size=%d\n",file_size);
		klog(MODEL_LOG,"[HTTP_FTP] get file ftp://%s%s success\n",rq->url.host,rq->url.path);
		sprintf(http_head,"HTTP/1.0 200 OK\nServer: Kingate\nContent-Length: %d\nContent-Range: bytes 0-%d/%d\nContent-Type: %s\nConnection: Keep-active\n\n",file_size+1,file_size,file_size+1,get_content_type(rq->url.path));
		//printf("%s",http_head);
		if(server->send(http_head)<=0){
			client.send("QUIT\r\n");
			return;
			//goto clean;
		}
	//	data_client.set_time();
//		m_server.time_out=30;
		create_select_pipe(server,&data_client,conf.time_out[HTTP]);
		client.send("QUIT\r\n");
		return;
	
	}
clean:
	if(err_msg[0]==0)
		strncpy(err_msg,"unknow error happen",ERR_MSG_SIZE);
	file_not_found(server,&rq->url,err_msg);
//	printf("errmsg=%s\n",err_msg);
	return;
}
int ftp_split_host(URL_MESSAGE * url)
{
	int ret;
	char tmp[MAX_URL];
	memset(tmp,0,sizeof(tmp));
	strncpy(tmp,url->host,MAX_URL-1);
	ret=sscanf(tmp,"%[^:]:%[^@]@%s",url->user,url->pass,url->host);
	if(ret==1){
		memset(url->host,0,sizeof(url->host));
		strncpy(url->host,tmp,sizeof(url->host)-1);
		strcpy(url->user,"anonymous");
		strcpy(url->pass,"kingateUser");
	}else if(ret!=3)
		return 0;
	//printf("user=%s,pass=%s,host=%s\n",url->user,url->pass,url->host);
	return 1;
}
bool build_data_connect(mysocket * control_connect,mysocket * data_connect,char *err_msg)
{
	HOST_MESSAGE ftp_data_socket;
	int length;
	char ret[4];
	char buffer[PACKAGE_SIZE];
//	printf("PASV\n");
	data_connect->close();
	if(control_connect->send("PASV\r\n")<0)
		return false;
	while(1){
		length=control_connect->recv(buffer,PACKAGE_SIZE);
		if(length<=0){
			klog(ERR_LOG,"control connect is abort\n");
			strncpy(err_msg,"control connect is abort",ERR_MSG_SIZE);
			return false;
		}
		buffer[length]=0;
	//	printf("%s",buffer);
	//	memcpy(ret,buffer,3);
	//	ret[3]=0;
	//	printf("%s,\n",ret);
		if(strncmp(buffer,"227",3)==0)
			break;
	}
	get_host_message_from_ftp_str(buffer,&ftp_data_socket,PASV_CMD);
//	printf("host=%s,port=%d\n",ftp_data_socket.host,ftp_data_socket.port);
	//data_connect->set_time(conf.time_out[HTTP]);
	if(data_connect->connect(ftp_data_socket.host,ftp_data_socket.port,ADDR_IP)<=0){
		klog(ERR_LOG,"build data connect error\n");
		strncpy(err_msg,"cann't connect to remote data connect.",ERR_MSG_SIZE);
		return false;
	}
	return true;
}
void file_not_found(mysocket * server,url * m_url,const char *err_msg)
{
	char *http_content=(char *)malloc(425+strlen(m_url->login)+strlen(m_url->password)+2*strlen(m_url->host)+2*strlen(m_url->path)+strlen(err_msg)+strlen(VER_ID));
	if(http_content==NULL)
		return;
	klog(MODEL_LOG,"[HTTP_FTP] get file ftp://%s%s error.err_msg (%s)\n",m_url->host,m_url->path,err_msg);
	sprintf(http_content,"HTTP/1.0 404 NOT FOUND\r\nServer: Kingate\r\nConnection: close\r\nCache-Control: no-store, no-cache, must-revalidate, post-check=0, pre-check=0\r\nPragma: no-cache\r\nContent-Type: text/html\r\n\r\n<HTML>\n<HEAD>\n<TITLE>Error cann't get file</TITLE>\n</HEAD><BODY>\n<H1>Error</H1>\n<H2>Cann't get file<br></H2>File url:<A HREF=\"ftp://%s:%s@%s%s\">ftp://%s%s</A>\n</P><P>Err msg:<font color=\"red\">%s</font></P><P>\n<hr>Generated by kingate(%s)</P></BODY></HTML>\n",m_url->login,m_url->password,m_url->host,m_url->path,m_url->host,m_url->path,err_msg,VER_ID);
	server->send(http_content);
//	printf("http_content length=%d,%d\n",425+strlen(m_url->login)+strlen(m_url->password)+2*strlen(m_url->host)+2*strlen(m_url->path)+strlen(err_msg)+strlen(VER_ID),strlen(http_content));
	free(http_content);
}

const char * get_content_type(const char * file_name)
{
	char ext_name[20];
	int size=strlen(file_name);
	int i;
	for(i=size;i>0;i--){
		if(file_name[i-1]=='.'){
			memcpy(ext_name,file_name+i,MIN(size-i,sizeof(ext_name)));
			ext_name[size-i]=0;
			//printf("ext_name=%s,len=%d\n",ext_name,strlen(ext_name));
			for(i=0;i<MAX_CONTENT_TYPE;i++){
				if(strcasecmp(content_type[i][0],ext_name)==0)
					return content_type[i][1];
			}
			return content_type[MAX_CONTENT_TYPE][1];
		}
	}
	return content_type[MAX_CONTENT_TYPE][1];
}
bool ftp_is_dir(mysocket *server,mysocket *client,url * m_url,char *err_msg)
{
	mysocket data_connect;
	int length;
	char buf[PACKAGE_SIZE];
	char file[PACKAGE_SIZE];
	bool have_send_first=false;
//	printf("run ftp_is_dir,PACKAGE_SIZE=%d\n",PACKAGE_SIZE);
	if(!ftp_set_type(client,"A",err_msg)){
		return false;
	}
	if(!build_data_connect(client,&data_connect,err_msg))
		return false;
//	client->send("LIST");
	memset(buf,0,sizeof(buf));
	if(m_url->path[0]!=0){
		/*
		snprintf(buf,sizeof(buf),"LIST\r\n");
		//client->send("\r\n");
	//	printf("the file is empty\n");
		*/
	
		snprintf(buf,sizeof(buf)-1,"CWD %s\r\n",m_url->path);
	/*	client->send(" ");
		client->send(m_url->path);
		client->send("\r\n");
	*/
	}
	
	client->send(buf);
	length=client->recv(buf,PACKAGE_SIZE,"\r\n");
	buf[length]=0;
	if(buf[0]=='5'){
		strncpy(err_msg,buf,ERR_MSG_SIZE);
		return false;
	}
	//printf("%s",buf);
	memset(buf,0,sizeof(buf));
	if(m_url->path[0]==0){
		strncpy(buf,"LIST\r\n",sizeof(buf)-1);
	}else{
		snprintf(buf,sizeof(buf)-1,"LIST %s\r\n",m_url->path);
	}
	client->send(buf);
	int len=strlen(m_url->path);
	bool prev_path_split=true;
	bool have_prev_path=false;
	char *prev_path=(char *)"/";
	//printf("host=%s,path=%s\n",m_url->host,m_url->path);
	for(int i=len-1;i>=0;i--){
		if(m_url->path[i]=='/'){
		       if(prev_path_split)
				continue;	
			have_prev_path=true;
			prev_path=(char *)malloc(i+1);
			memset(prev_path,0,i+1);
			memcpy(prev_path,m_url->path,i);
			prev_path[i]=0;
			break;
		}
		prev_path_split=false;
	}
	
//	server->send(
	char file_size[16];
	char file_month[10];
	char file_day[5];
	char file_year[10];
	int file_type=0;
	memset(buf,0,sizeof(buf));

	//		server->send("<a href=\"javascript:history.go(-1);\">parent directory</a><br>");
	snprintf(buf,sizeof(buf)-1,"HTTP/1.0 200 OK\nServer: Kingate\nContent-Type: text/html\n\n<html><body><table><tr><td colspan=\"4\">[<a href=\"ftp://%s:%s@%s%s\">parent directory</a>]</td></tr>",url_encode(m_url->login).c_str(),url_encode(m_url->password).c_str(),m_url->host,url_encode(prev_path).c_str());
	server->send(buf);
	while((length=data_connect.recv(buf,PACKAGE_SIZE,"\r\n"))>0){
		buf[length]=0;
	//	printf("%s",buf);
		if(buf[0]=='d')
			file_type=1;//it is a dir;
		else if(buf[0]=='l')
			file_type=2;
		else
			file_type=0;
	//	PRINT(buf,length);
		file[0]=0;
		int ret=sscanf(buf,"%*s%*s%*s%*s%15s%9s%4s%9s %[^\n]",file_size,file_month,file_day,file_year,file);
		
		if(file_size[0]>'9' || file_size[0]<'0'){
			sscanf(buf,"%*s%*s%*s%15s%9s%4s%9s %[^\n]",file_size,file_month,file_day,file_year,file);
		}
	//	printf("ret=%d\n",ret);
		if(ret<1)
			continue;

		if(file_type==2){
			char *split_str_point=strstr(file," -> ");
			if(split_str_point!=NULL)
				file[split_str_point-file]=0;
			
		}
		if(strcmp(file,".")==0||strcmp(file,"..")==0)
			continue;
		memset(buf,0,sizeof(buf));	
		snprintf(buf,sizeof(buf)-1,"<tr><td><A href=\"ftp://%s:%s@%s%s%s%s\">%s</A></td><td>%s</td><td>%s %s</td><td>%s</td></tr>\n",url_encode(m_url->login).c_str(),url_encode(m_url->password).c_str(),m_url->host,url_encode(m_url->path).c_str(),(m_url->path[strlen(m_url->path)-1]=='/'?"":"/"),url_encode(file).c_str(),file,(file_type==1?"&lt;DIR&gt;":file_size),file_month,file_day,file_year);
		server->send(buf);
	}
	memset(buf,0,sizeof(buf));
	snprintf(buf,sizeof(buf)-1,"</table><hr>Generated by kingate(%s)</body></html>",VER_ID);
	server->send(buf);
	if(have_prev_path)
		free(prev_path);
//	printf("have recv %d size\n",data_connect.recv_size);
//	printf("data recv complete\n");
	return true;
	
}
bool ftp_set_type(mysocket * control_connect,const char * type,char *err_msg)
{
//	int length;
//	char * buffer=(char *)malloc(max_len+1);//new char[max_len+1];
	char buffer[max_len+1];
	memset(buffer,0,sizeof(buffer));
	snprintf(buffer,max_len,"TYPE %s\r\n",type);
	control_connect->send(buffer);
	if(control_connect->recv(buffer,max_len,"\r\n")<=0){
		strcpy(buffer,"remote host have closed the connection.while try to recv type result.");
		strncpy(err_msg,buffer,ERR_MSG_SIZE);
	//	printf("%s\n",buffer);
		false;
	}
	return true;
}
bool ftp_login(mysocket * control_connect,url * m_url,char *err_msg)
{
	char buffer[max_len+1];
	char tmp[max_len+1];
	int length=0;
	memset(buffer,0,sizeof(buffer));
	snprintf(buffer,max_len,"USER %s\r\n",m_url->login);
	control_connect->send(buffer);
//	return buffer;
	buffer[0]=0;
	for(;;){
//		tmp[length]=0;
//		memcpy(buffer,tmp,max_len);
	//	printf("buffer=%s\n",buffer);
		if((length=control_connect->recv(tmp,max_len-3,"\r\n"))<=0)
		{
			//			return buffer;
			strncpy(err_msg,buffer,ERR_MSG_SIZE);
			return false;
		}
		tmp[length]=0;
		strncpy(buffer,tmp,max_len);
	        if(strncmp(buffer,"331",3)==0)
	                 break;

//		strncpy(buffer,tmp,max_len);
	//	buffer[length]=0;
//		printf("buffer=%s\n",buffer);
	}
	memset(buffer,0,sizeof(buffer));
	snprintf(buffer,max_len,"PASS %s\r\n",m_url->password);
	control_connect->send(buffer);
	if(control_connect->recv(buffer,max_len,"\r\n")<=0){
		strncpy(err_msg,"remote host have closed the connection while try to recv pass result.",ERR_MSG_SIZE);
		return false;
	}
	if(strncmp(buffer,"230",3)!=0){
		klog(DEBUG_LOG,"ftp password is error\n");
		strncpy(err_msg,buffer,ERR_MSG_SIZE);
		return false;
	}
	return true;
}
