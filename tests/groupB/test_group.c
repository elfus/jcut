#include <stdio.h>

char gchar;
unsigned char guchar;
short gshort;
unsigned short gushort;
int gint;
unsigned guint;
long int glint;
unsigned long int gulint;
float gfloat;
double gdouble;

void print_gvar()
{
	printf("------------------------------------------------------------\n");
	printf("gchar = %c (%d)\n",gchar, gchar);
	printf("guchar = %c (%u)\n",guchar, guchar);
	printf("gshort = %d\n",gshort);
	printf("gushort = %u\n",gushort);
	printf("gint = %d\n",gint);
	printf("guint = %u\n",guint);
	printf("glint = %ld\n",glint);
	printf("gulint = %lu\n",gulint);
	printf("gfloat = %f\n",gfloat);
	printf("gdouble = %f\n",gdouble);
}

char getgchar() { return gchar; }
unsigned char getguchar() { return guchar; }
short getgshort() { return gshort; }
unsigned short getgushort() { return gushort; }
int getgint() { return gint; }
unsigned int getguint() { return guint; }
long int getglint() { return glint; }
unsigned long int getgulint() { return gulint; }
float getgfloat() {	return gfloat; }
double getgdouble() { return gdouble; }
