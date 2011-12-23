%{

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#ifdef HAVE_STRING_H
#include <string.h>
#endif

#include "global.h"
#include "resource.h"
#include "res_parse.h"

#ifdef HAVE_LIBDMALLOC
#include <dmalloc.h>
#endif

#define YY_NO_INPUT
#define YY_INPUT(buf,result,max_size) { result = res_parse_getchars(buf, max_size); }
 
#ifdef FLEX_SCANNER
#define yyunput ATTRIBUTE_UNUSED yyunput
#endif

extern int res_lineno;
extern int res_parse_getchars(char *buf, int max_size);

%}

%option prefix="res"
%option outfile="lex.yy.c"
%option yylineno
%option noyywrap

PARENSTR	([^ (){}=\"\'\t\r\n]|\([^\)]*\))+
INCSTR		@[a-z0-9A-Z_]+
COMMENT		#[^\n]*

%%

\"[^"]*\"	{ reslval.sval = strdup(yytext+1);
		  reslval.sval[strlen(reslval.sval) - 1] = 0;
		  return STRING; }

\'[^']*\'	{ reslval.sval = strdup(yytext+1);
		  reslval.sval[strlen(reslval.sval) - 1] = 0;
		  return STRING; }

{COMMENT}\n	{ res_lineno++; }
[ \t\r\n]	{ if (yytext[0] == '\n') res_lineno++; }

{INCSTR}	{ reslval.sval = strdup(yytext);
		  	  return INCLUDE; }

{PARENSTR}	{ reslval.sval = strdup(yytext);
		  	  return STRING; }

.		{ return yytext[0]; }

%%

/* ' */
