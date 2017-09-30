#ifndef _XMLOUT
#define _XMLOUT 1

#include <stdio.h>

struct s_xmlout
{
	FILE* fd;
	int count;
};
struct s_xmlout xmlout;

char* indent[] =
{"\n", "\n\t", "\n\t\t", "\n\t\t\t", "\n\t\t\t\t", "\n\t\t\t\t\t", "\n\t\t\t\t\t\t", "\n\t\t\t\t\t\t\t"};

#define XPRINTF fprintf
#define XNEWLINE indent[xmlout.count]
#define XOUT_DETENT() if(xmlout.count) xmlout.count--
#define XOUT_INDENT() xmlout.count++

#define XOUT_ELEMENT_ATTR_START( name, id, val) XPRINTF(xmlout.fd, "<%s %s=\"%s\">", name, id, val);\
		XOUT_INDENT()
#define XOUT_ELEMENT_START( name) XPRINTF(xmlout.fd, "<%s>", name);\
		XOUT_INDENT()
		
#define XOUT_ELEMENT_END( name) XPRINTF(xmlout.fd, "</%s>", name);\
		XOUT_DETENT()

#define XOUT_ELEMENT_EMPTY(name) XPRINTF(xmlout.fd, "<%s/>", name)
#define XOUT_ELEMENT_ATTR_EMPTY(name, id, val) XPRINTF(xmlout.fd, "<%s %s=\"%s\"/>", name, id, val)

#define XOUT_ELEMENT_DATA(data) XPRINTF(xmlout.fd, "%s", data)

#define XOUT_NEWLINE() XPRINTF( xmlout.fd, XNEWLINE)


#define XOUT_ELEMENT(name, data) XOUT_ELEMENT_START(name);\
		XOUT_ELEMENT_DATA(data);\
		XOUT_ELEMENT_END(name);\
		XOUT_NEWLINE()

#define XOUT_ELEMENT_ATTR(name, id, val, data) XOUT_ELEMENT_ATTR_START(name, id, val);\
		XOUT_ELEMENT_DATA(data);\
		XOUT_ELEMENT_END(name);\
		XOUT_NEWLINE()

#define XOUT_HEADER() XPRINTF(xmlout.fd, "<?xml version=\"1.0\" encoding=\"UTF-8\"?>"); \
		XOUT_NEWLINE()
		
#define XOUT_INIT(filename)  xmlout.count=0;\
		 xmlout.fd = fopen(filename, "w")

#define XOUT_CLOSE()  xmlout.count=0;\
		 fclose(xmlout.fd); \
		 xmlout.fd = NULL
		 
		 			

#endif
