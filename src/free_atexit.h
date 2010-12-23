/* This tiny library is to assist cleaning up harmless memory leaks caused
   by (growing) buffers allocated in static variables in functions. The
   library provides leaky_ prefixed variants of the common allocation
   routines. These wrappers will remember all pointers they return and
   can free all memory used, at the end of the applocation.
*/

#include <stdlib.h>

#ifdef NDEBUG
#define leaky_init()
#define leaky_uninit()
#define leaky_malloc(size) malloc(size)
#define leaky_calloc(nmemb, size) calloc(nmemb, size)
#define leaky_realloc(old_memory, size) realloc(old_memory, size)
#else

/* set up atexit() hook - can be avoided if leaky_uninit() is called by hand */
void leaky_init (void);

/* free all allocations */
void leaky_uninit (void);

/* allocate memory, remember the pointer and free it after exit from the application */
void *leaky_malloc (size_t size);

/* same as leaky_malloc but this one wraps calloc() */
void *leaky_calloc (size_t nmemb, size_t size);

/* reallocate memory, remember the new pointer and free it after exit from the application */
void *leaky_realloc (void* old_memory, size_t size);


#endif
