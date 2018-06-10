%{

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <stdio.h>
#include <stdlib.h>

#ifdef HAVE_STRING_H
#include <string.h>
#endif

#define YYDEBUG 0
#define YYERROR_VERBOSE 1

  /* #define YYSTYPE void * */

#include "global.h"
#include "resource.h"
#include "res_parse.h"

#ifdef HAVE_LIBDMALLOC
#include <dmalloc.h>
#endif

static Resource *parsed_res;
static Resource *current_res;

int reserror(const char *);
int reslex();

#define f(x) current_res->flags |= x

%}

%name-prefix "res"
%start top_res

%union {
  int ival;
  char *sval;
  Resource *rval;
};

%token <sval> STRING INCLUDE
%type <rval> res

%%

top_res
 : { current_res = parsed_res = resource_create(NULL); }
   res_item_zm
 ;

res
 : { current_res = resource_create(current_res); }
   '{' res_item_zm '}'
   { $$ = current_res; current_res = current_res->parent; }
 ;

res_item_zm : res_item res_item_zm | ;
res_item
 : STRING		{ resource_add_val(current_res, 0, $1, 0); f(FLAG_V); }
 | STRING '=' STRING	{ resource_add_val(current_res, $1, $3, 0); f(FLAG_NV); }
 | INCLUDE		{ resource_add_val(current_res, 0, $1, 0); f(FLAG_S); }
 | res			{ resource_add_val(current_res, 0, 0, $1); f(FLAG_S); }
 | STRING '=' res	{ resource_add_val(current_res, $1, 0, $3); f(FLAG_NS); }
 | error
 ;

%%

static const char *res_filename = NULL;
static FILE *res_file = NULL;
static const char **res_strings = NULL;
static int res_string_idx = 0;
int res_lineno;

int
res_parse_getchars(char *buf, int max_size)
{
  if (res_file)
    {
      int i = fgetc(res_file);
      buf[0] = i;
      if (i == EOF)
	return 0;
    }
  else
    {
      if (res_strings[0] == 0)
	return 0;
      buf[0] = res_strings[0][res_string_idx];
      res_string_idx ++;
      if (buf[0] == 0)
	{
	  res_strings ++;
	  res_string_idx = 0;
	  buf[0] = '\n';
	  return 1;
	}
    }
  return 1;
}

Resource *
resource_parse(const char *filename, const char **strings)
{
  res_lineno = 1;
  if (filename)
    {
      res_file = fopen (filename, "r");
      if (res_file == NULL)
	{
	  perror(filename);
	  return NULL;
	}
      res_filename = filename;
    }
  else if (strings)
    {
      res_strings = strings;
      res_string_idx = 0;
    }
  else
    return NULL;
#if YYDEBUG
  yydebug = 1;
#endif
  if (resparse())
    parsed_res = NULL;
  if (filename)
    {
      fclose (res_file);
      res_filename = NULL;
      res_file = NULL;
    }
  else
    res_strings = NULL;
  return parsed_res;
}

Resource *
resource_create(Resource *parent)
{
  Resource *rv = (Resource *)malloc(sizeof(Resource));
  rv->parent = parent;
  rv->flags = 0;
  rv->c = 0;
  rv->v = (ResourceVal *)malloc(sizeof(ResourceVal));
  return rv;
}

void
resource_add_val(Resource *n, char *name, char *value, Resource *subres)
{
  n->v = (ResourceVal *)realloc(n->v, sizeof(ResourceVal)*(n->c+1));
  n->v[n->c].name = name;
  n->v[n->c].value = value;
  n->v[n->c].subres = subres;
  n->c++;
}

char *
resource_value(const Resource *res, char *name)
{
  int i;
  if (res == 0 || name == 0)
    return 0;
  for (i=0; i<res->c; i++)
    if (res->v[i].name && res->v[i].value
	&& NSTRCMP(res->v[i].name, name) == 0)
      return res->v[i].value;
  return 0;
}

Resource *
resource_subres(const Resource *res, const char *name)
{
  int i;
  if (res == 0 || name == 0)
    return 0;
  for (i=0; i<res->c; i++)
    if (res->v[i].name && res->v[i].subres
	&& NSTRCMP(res->v[i].name, name) == 0)
      return res->v[i].subres;
  return 0;
}

int
reserror(const char *str)
{
  fprintf(stderr, "Error: %s around line %d: %s\n",
	  res_file ? res_filename : "internal strings",
	  res_lineno, str);
  return 0;
}

static void
dump_res(Resource *n, int l)
{
  int i;
  for (i=0; i<n->c; i++)
    {
      if (n->v[i].subres)
	{
	  printf("%*cn[%s] = {\n", l, ' ', n->v[i].name? n->v[i].name :"");
	  dump_res(n->v[i].subres, l+3);
	  printf("%*c}\n", l, ' ');
	}
      else
	printf("%*cn[%s] = v[%s]\n", l, ' ', n->v[i].name? n->v[i].name :"", n->v[i].value? n->v[i].value :"");
    }
}

void
resource_dump (Resource *r)
{
  dump_res (r, 0);
}

