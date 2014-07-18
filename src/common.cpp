/*
Copyright (C) 1999 Igor Khasilev, igor@paco.net

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.

*/
#include "kingate.h"
#include <time.h>
#include	"oops.h"
#include "malloc_debug.h"

/* read with timeout */
int
readt(int so, char* buf, int len, int tmo)
{
int		to_read = len, sr,  got;
char		*p = buf;
struct	pollarg	pollarg;

	if ( so < 0 ) return(0);
	if ( len <= 0 ) return(0);

	pollarg.fd = so;
	pollarg.request = FD_POLL_RD;
	sr = poll_descriptors(1, &pollarg, tmo*1000);
	if ( sr < 0 ) {
	//	if ( ERRNO == EINTR ) return(readed);
		return(sr);
	}
	if ( sr == 0 ) {
		/* timeout */
		return(-2);
	}
	if ( IS_HUPED(&pollarg) ) return(0);
	/* have somethng to read	*/
	got = recv(so, p, to_read,0);
	if ( got == 0 ) {
	    my_xlog(OOPS_LOG_DBG|OOPS_LOG_INFORM, "readt(): got: %d, tmo: %d, to_read: %d\n", got, tmo, to_read);
	}
	return(got);
}

int
wait_for_read(int so, int tmo_m)
{
struct	timeval tv;
int		sr;
struct	pollarg	pollarg;

	if ( so < 0 ) return(FALSE);
	tv.tv_sec  =  tmo_m/1000;
	tv.tv_usec = (tmo_m*1000)%1000000;
	pollarg.fd = so;
	pollarg.request = FD_POLL_RD;
	sr = poll_descriptors(1, &pollarg, tmo_m);
	if ( sr <= 0 )
		return(FALSE);
	return(TRUE);
}

int
sendstr(int so, char * str)
{

    if ( so < 0 ) return(0);
    return(writet(so, str, strlen(str), READ_ANSW_TIMEOUT));
}

int
writet(int so, char* buf, int len, int tmo)
{


int		to_write = len, sr, got, sent = 0;
char		*p = buf;
struct	timeval tv;
time_t		start, now;
struct pollarg	pollarg;
//int have_send=0,left_send;
//left_send=len;
/*
while(left_send>0){
	have_send=write(so,buf,left_send);
	if(have_send<=0)
		return -1;
	left_send-=have_send;
}
*/
//return len;
    if ( so < 0 ) return(-1);
    if ( len <= 0 ) return(0);
    now = start = time(NULL);

    while(to_write > 0) {
		if ( now - start > tmo )
			return(-1);
		tv.tv_sec  = tmo - (start-now);
		tv.tv_usec = 0;
		pollarg.fd = so;
		pollarg.request = FD_POLL_WR;
	//	/*
		sr = poll_descriptors(1, &pollarg, (tmo - (start-now))*1000);
		/*
		if ( sr < 0 ) {
			return(sr);
		}*/
		if ( sr <= 0 ) {
		   /* timeot */
		   return(-1);
		}
		if ( IS_HUPED(&pollarg) ) return(-1);
		/* have somethng to write	*/
		//*/
		got = send(so, p, to_write,0);
		if ( got > 0 ) {
			to_write -= got;
			sent += got;
			if ( to_write>0 ) {
				p += got;
				now = time(NULL);
				continue;
			}
			return(sent);
		}
		return(got);
    }
    return(0);
	
}
/*
int
writet_cv_cs(int so, char* buf, int len, int tmo, char *table, int escapes)
{
int		to_write = len, sr, got, sent=0;
char		*p, *recoded = NULL, *d;
struct	timeval tv;
time_t		start, now;
struct pollarg	pollarg;
u_char		*s;

    if ( so < 0 ) return(-1);
    if ( len <= 0 ) return(0);
    recoded = (char *)malloc(len+1);
    if ( !recoded )
	return(-1);
    s = (u_char*)buf; d = recoded;
    while ( (s-(u_char*)buf) < len ) {
	u_char	c, cd;
	if ( escapes
	    && ((s-(u_char*)buf) + 2 < len)
	    && (*s == '%' && isxdigit(*(s+1)) && isxdigit(*(s+2))) ) {
	    if ( isdigit(*(s+1)) ) {
		c = 16 * (*(s+1)-'0');
	    } else
		c = 16 * (toupper(*(s+1)) - 'A' + 10 );
	    if ( isdigit(*(s+2)) ) {
		c += (*(s+2)-'0');
	    } else
		c += (toupper(*(s+2)) - 'A' + 10 );
	    if ( c >= 128 ) {
		cd = table[c-128];
		s += 3;
		*d = '%';
		sprintf(d, "%%%02X", cd);
		d+=3;
	    } else {
		s += 3;
		*d = '%';
		sprintf(d, "%%%02X", c);
		d+=3;
	    }
	    continue;
	}
	if ( *s > 128 )
	    *d = table[(*s)-128];
	  else
	    *d = *s;
	s++;d++;
    }
    p = recoded;
    now = start = global_sec_timer;

    while(to_write > 0) {
	if ( now - start > tmo ) {
	    free(recoded);
	    return(-1);
	}
	tv.tv_sec  = tmo - (start-now);
	tv.tv_usec = 0;
	pollarg.fd = so;
	pollarg.request = FD_POLL_WR;
	sr = poll_descriptors(1, &pollarg, (tmo - (start-now))*1000);
	if ( sr < 0 ) {
	    free(recoded);
	    return(sr);
	}
	if ( sr == 0 ) {
	   / * timeot * /
	    free(recoded);
	    return(-1);
	}
	if ( IS_HUPED(&pollarg) ) return(-1);
	/ * have somethng to write	* /
	got = send(so, p, to_write, 0);
	if ( got > 0 ) {
	    to_write -= got;
	    sent += got;
	    if ( to_write>0 ) {
	        p += got;
	        now = global_sec_timer;
	        continue;
	    }
	    free(recoded);
	    return(sent);
	}
	free(recoded);
	return(got);
    }
    free(recoded);
    return(0);
}
*/
