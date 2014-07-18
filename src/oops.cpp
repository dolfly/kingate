/*
Copyright (C) 1999, 2000 Igor Khasilev, igor@paco.net
Copyright (C) 2000 Andrey Igoshin, ai@vsu.ru

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
#include	"kingate.h"
#include <time.h>
#define		OOPS_MAIN
#include	"oops.h"
#undef		OOPS_MAIN

#include	"cache.h"
//#include "y.tab.h"
#include "malloc_debug.h"

struct	obj_hash_entry	hash_table[HASH_SIZE];


refresh_pattern_t	*global_refresh_pattern;
struct	domain_list 	*local_domains;
int			ns_configured;
volatile	time_t	global_sec_timer;
pthread_mutex_t		mktime_lock;
//int             insert_via;
//int             insert_x_forwarded_for;
//int		dont_cache_without_last_modified;
volatile bool oops_http_have_init=false;

int
oops_http_init()
{
int	i;
	if(oops_http_have_init)
		return 1;
	oops_http_have_init=true;
    for(i=0;i<HASH_SIZE;i++) {
	    	memset(&hash_table[i],0,sizeof(hash_table[i]));
//		bzero(&hash_table[i], sizeof(hash_table[i]));
		pthread_mutex_init(&hash_table[i].lock, NULL);
		pthread_mutex_init(&hash_table[i].size_lock, NULL);
    }
	for(i=0;i<NET_HASH_SIZE;i++){
		net_table[i].next=NULL;
	}
    pthread_mutex_init(&mktime_lock, NULL);
    global_sec_timer = time(NULL);
   // global_refresh_pattern = NULL;

    	ns_configured	= 1;
   // 	insert_via		= TRUE;
    //	dont_cache_without_last_modified = FALSE;
	init_cache();
	return 1;
}
