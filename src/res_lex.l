%{

#include "res_parse.h"

#define YY_INPUT(buf,result,max_size) { result = res_parse_getchars(buf, max_size); }

extern int res_lineno;

%}

%option prefix="res"
%option outfile="lex.yy.c"
%option yylineno
%option noyywrap

PARENSTR	([^ (){}=\"\'\t\r\n]|\([^\)]*\))+
INCSTR		@[a-z0-9A-Z_]+

%%

\"[^"]*\"	{ reslval = (int)strdup(yytext+1);
		  ((char *)reslval)[strlen((char *)reslval)-1] = 0;
		  return STRING; }

\'[^']*\'	{ reslval = (int)strdup(yytext+1);
		  ((char *)reslval)[strlen((char *)reslval)-1] = 0;
		  return STRING; }

[ \t\r\n]	{ if (yytext[0] == '\n') res_lineno++; }

{INCSTR}	{ reslval = strdup(yytext+1);
		  	  return INCLUDE; }

{PARENSTR}	{ reslval = strdup(yytext);
		  	  return STRING; }

.		{ return yytext[0]; }

%%

/* ' */
