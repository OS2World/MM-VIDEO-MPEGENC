#include <time.h>
#include "os2port.h"

/* Process will 'consume' user time, but no system time */
void times(struct tms *t)
	{
	t->tms_utime = time(NULL);
	t->tms_stime = 0L;
	}

static char tmpfn[] = "popen.tmp";

FILE *popen(char *cmd, char *mode)
	{
	char fullcmd[500+1];
	sprintf(fullcmd, "cmd /c \"%s\" > %s", cmd, tmpfn);
	if ( system(fullcmd) == -1 )
		return NULL;
	return fopen(tmpfn, mode);
	}

void pclose(FILE *fp)
	{
	fclose(fp);
	remove(tmpfn);
	}
