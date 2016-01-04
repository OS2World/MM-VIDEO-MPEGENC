/*

os2port.h - Bits needed to complete port to OS/2

Fake up 'times' interface found on UNIX boxes.
The times stuff seems to be to allow the encoder to display how long it
has taken to encode the data.

Provide popen() and pclose().

@@@ Andy Key

*/

#include <time.h>

struct tms { long tms_utime, tms_stime; };
extern void times(struct tms *t);

#include <stdio.h>

extern FILE *popen(char *cmd, char *mode);
extern void pclose(FILE *fp);
