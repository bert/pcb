/*!
 * \file src/free_atexit.c
 *
 * \brief .
 *
 * This tiny library is to assist cleaning up harmless memory leaks
 * caused by (growing) buffers allocated in static variables in
 * functions.\n
 * The library provides leaky_ prefixed variants of the common
 * allocation routines.\n
 * These wrappers will remember all pointers they return and can free
 * all memory used, at the end of the application.
 */

#include <stdlib.h>

#ifdef NDEBUG
#define leaky_init()
#define leaky_uninit()
#define leaky_malloc(size) malloc(size)
#define leaky_calloc(nmemb, size) calloc(nmemb, size)
#define leaky_realloc(old_memory, size) realloc(old_memory, size)
#else

void leaky_init (void);
void leaky_uninit (void);
void *leaky_malloc (size_t size);
void *leaky_calloc (size_t nmemb, size_t size);
void *leaky_realloc (void* old_memory, size_t size);


#endif
