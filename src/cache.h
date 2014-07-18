#ifndef cache_h_saldkfjsaldkfjalsfdjasdf987a9sd7f9adsf7
#define cache_h_saldkfjsaldkfjalsfdjasdf987a9sd7f9adsf7
#include "oops.h"
#include "KMutex.h"
#include <string>
#include <assert.h>
#define CLEAN_CACHE		0
#define DROP_DEAD		1
#define LOCK_OBJ_LIST pthread_mutex_lock(&obj_list_mutex_t);
#define UNLOCK_OBJ_LIST pthread_mutex_unlock(&obj_list_mutex_t);
#define LOCK_DISK_LIST pthread_mutex_lock(&disk_list_mutex_t);
#define UNLOCK_DISK_LIST pthread_mutex_unlock(&disk_list_mutex_t);
void add_list(obj_list *m_list);
void update_list(obj_list *m_list,bool in_mem);
void delete_cache(int m_size);
void dead_list(obj_list *m_list);
void init_cache();
void destroy_obj(struct mem_obj*,int in_hash=1);
void save_index_file(bool use_obj_lock=true);
void save_all_to_disk();
std::string get_disk_cache_file(struct mem_obj *obj);
std::string get_cache_index_file();
void clean_disk(int m_size);
void get_cache_size(unsigned long *total_mem_size,unsigned long *total_disk_size);
extern volatile  int cache_model;
extern pthread_mutex_t	obj_list_mutex_t;
extern int total_obj_count;
//extern int count_sync_complete;
/*
extern KMutex mem_cache_size_lock;
extern KMutex disk_cache_size_lock;
extern int mem_cache_size;
extern int disk_cache_size;
 inline void increase_mem_size(int size)
 {
	assert(size>=0);
	mem_cache_size_lock.Lock();
	mem_cache_size+=size;
	mem_cache_size_lock.Unlock();
	assert(mem_cache_size>=0);
 }
 inline void decrease_mem_size(int size)
 {
	 	 assert(size>=0);
	mem_cache_size_lock.Lock();
	mem_cache_size-=size;
	mem_cache_size_lock.Unlock();
	assert(mem_cache_size>=0);

 }
 inline void increase_disk_size(int size)
 {
	 	 assert(size>=0);

	disk_cache_size_lock.Lock();
	disk_cache_size+=size;
	disk_cache_size_lock.Unlock();
	assert(disk_cache_size>=0);
 }
inline void decrease_disk_size(int size)
 {
	 	 assert(size>=0);

	disk_cache_size_lock.Lock();
	disk_cache_size-=size;
	disk_cache_size_lock.Unlock();
	assert(disk_cache_size>=0);
 }
*/
#endif
