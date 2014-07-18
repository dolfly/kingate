#if	!defined(_HASH_H_INCLUDED_)
#define _HASH_H_INCLUDED_
#ifndef _WIN32
#include    <unistd.h>
#include    <pthread.h>
#endif
#include    <stdlib.h>
#include    <errno.h>


#include    <assert.h>

#define     HASH_ENTRY_DELETED  1

typedef	struct	hash_entry_ {
    struct  hash_entry_ *next,          /* link between entries	    */
                        *prev;
    struct  hash_row_   *row_back_ptr;  /* pointer to row           */
    int                 ref_count;
    char                flags;
    void                *key;           /* stored   key             */
    void                *data;          /* associated data          */
//    pthread_cond_t      ref_count_cv;   /* changes on ref_count     */
} hash_entry_t;

typedef	struct	hash_row_ {
    pthread_mutex_t     lock;           /* protect ALL data in row	        */
    hash_entry_t        *first;         /* link to first entry		        */
    int                 counter;        /* number of entries in row	        */
    int                 deleted;        /* number of deleted entries in row */
} hash_row_t;

#define HASH_VALID          0x0BBAADD0
#define HASH_KEY_INT        0
#define HASH_KEY_STRING     1
#define HASH_KEY_UNDEF      2
typedef struct  hash_ {
    pthread_mutex_t     lock;
    int                 valid;
    unsigned int        rows;           /* total rows in table      */
    int                 type;           /* key type                 */
    hash_row_t          *table;         /* table rows               */
} hash_t;

/* 
    this function initialize hash
    prepare structure and set key type 
*/
hash_t      *hash_init(int size, int type);

/*
    this function place data in the hash.
    it returns "referenced" hash entry or NULL if fail
    "Referenced" means: you hold reference count to this hash
    entry. Hash code will return error if requested to remove this
    entry. It doesn't mean - you lock your data, just hash entry.
    To remove lock (that is to unref this hash entry) call hash_unref()

    return  EEXIST if key exist in hash
            ENOMEM if no memory to allocate new hash entry
            0      if placed in hash. entry reference incremented
*/
int     hash_put(hash_t *hash, void *key, void* data, hash_entry_t **res);

/*
    this function find and increment reference counter for key

    return ENOENT   if not found
           0        if found. hash_entry is locked.
*//*
int     hash_get(hash_t *hash, void *key, hash_entry_t **he);
int     hash_unref(hash_t *hash, hash_entry_t *he);
int	delete_hash_entry(hash_t *hash, hash_entry_t *he, void (*f)(void*));
int	hash_operate(hash_t *hash, int (*f)(hash_entry_t*));
int	hash_destroy(hash_t *hash, void (*user_function)(void*));
*/
#endif	/* !_HASH_H_INCLUDED_ */
