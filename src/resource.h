#ifndef PCB_RESOURCE_H
#define PCB_RESOURCE_H

#include <stdio.h>

#ifdef __cplusplus
extern "C"
{
#endif

  struct Resource;

  typedef struct ResourceVal
  {
    char *name;
    char *value;
    struct Resource *subres;
  } ResourceVal;

#define FLAG_V	1
#define FLAG_NV	2
#define FLAG_S	4
#define FLAG_NS	8

  typedef struct Resource
  {
    struct Resource *parent;
    void *user_ptr;
    int flags;
    int c;			/* number of v[i] */
    ResourceVal *v;
  } Resource;

#define resource_type(resval) (((resval).name?100:0)+((resval).value?10:0)+((resval).subres?1:0))

/* res_parse.y */

/* Pass either filename OR stringtab.  */
  Resource *resource_parse (const char *filename, const char **stringtab);
  char *resource_value (const Resource * res, char *name);
  Resource *resource_subres (const Resource * res, const char *name);

  Resource *resource_create (Resource * parent);
  void resource_add_val (Resource * n, char *name, char *value,
			 Resource * subres);

  void resource_dump (Resource * res);

#ifdef __cplusplus
}
#endif

#endif
