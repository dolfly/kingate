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


#if	!defined(_EXTERN_H_INCLUDED_)
#define _EXTERN_H_INCLUDED_
extern struct	obj_hash_entry	hash_table[HASH_SIZE];
extern volatile	time_t	global_sec_timer;
extern int			ns_configured;
extern pthread_mutex_t	mktime_lock;
extern volatile bool quit_kingate;
extern struct net_obj_entry net_table[NET_HASH_SIZE];
#endif	/* !_EXTERN_H_INCLUDED_ */
