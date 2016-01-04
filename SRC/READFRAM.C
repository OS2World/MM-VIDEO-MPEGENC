/*===========================================================================*
 * readframe.c								     *
 *									     *
 *	procedures to read in frames					     *
 *									     *
 * EXPORTED PROCEDURES:							     *
 *	ReadFrame							     *
 *	SetFileType							     *
 *	SetFileFormat							     *
 *									     *
 *===========================================================================*/

/*
 * Copyright (c) 1993 The Regents of the University of California.
 * All rights reserved.
 *
 * Permission to use, copy, modify, and distribute this software and its
 * documentation for any purpose, without fee, and without written agreement is
 * hereby granted, provided that the above copyright notice and the following
 * two paragraphs appear in all copies of this software.
 *
 * IN NO EVENT SHALL THE UNIVERSITY OF CALIFORNIA BE LIABLE TO ANY PARTY FOR
 * DIRECT, INDIRECT, SPECIAL, INCIDENTAL, OR CONSEQUENTIAL DAMAGES ARISING OUT
 * OF THE USE OF THIS SOFTWARE AND ITS DOCUMENTATION, EVEN IF THE UNIVERSITY OF
 * CALIFORNIA HAS BEEN ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 * THE UNIVERSITY OF CALIFORNIA SPECIFICALLY DISCLAIMS ANY WARRANTIES,
 * INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY
 * AND FITNESS FOR A PARTICULAR PURPOSE.  THE SOFTWARE PROVIDED HEREUNDER IS
 * ON AN "AS IS" BASIS, AND THE UNIVERSITY OF CALIFORNIA HAS NO OBLIGATION TO
 * PROVIDE MAINTENANCE, SUPPORT, UPDATES, ENHANCEMENTS, OR MODIFICATIONS.
 */

/*  
 *  $Header: /n/picasso/users/keving/encode/src/RCS/readframe.c,v 1.1 1993/07/22 22:23:43 keving Exp keving $
 *  $Log: readframe.c,v $
 * Revision 1.1  1993/07/22  22:23:43  keving
 * nothing
 *
 */


/*==============*
 * HEADER FILES *
 *==============*/

/* @@@ Fake up popen stuff */
#ifdef OS2
#include "os2port.h"
#endif
#include "all.h"
#include <time.h>
#include <errno.h>
#include "mtypes.h"
#include "frames.h"
#ifdef OS2
/* @@@ FAT 8.3 convention */
#include "prototyp.h"
#else
#include "prototypes.h"
#endif
#include "parallel.h"
#include "param.h"
#ifdef OS2
/* @@@ FAT 8.3 convention */
#include "readfram.h"
#else
#include "readframe.h"
#endif
#include "fsize.h"
#include "rgbtoycc.h"

#define PPM_READ_STATE_MAGIC	0
#define PPM_READ_STATE_WIDTH	1
#define PPM_READ_STATE_HEIGHT	2
#define PPM_READ_STATE_MAXVAL	3
#define PPM_READ_STATE_DONE	4


/*==================*
 * STATIC VARIABLES *
 *==================*/

static int  fileType = BASE_FILE_TYPE;
static int  baseFormat;


/*===============================*
 * INTERNAL PROCEDURE prototypes *
 *===============================*/

static char *ScanNextString _ANSI_ARGS_((char *inputLine, char *string));
static void ReadPNM _ANSI_ARGS_((FILE * fp, MpegFrame * mf));
static boolean	ReadPPM _ANSI_ARGS_((MpegFrame *mf, FILE *fpointer));
static void ReadYUV _ANSI_ARGS_((MpegFrame * mf, FILE *fpointer,
				 int width, int height));
static void ReadSub4 _ANSI_ARGS_((MpegFrame * mf, FILE *fpointer,
				  int width, int height));


/*=====================*
 * EXPORTED PROCEDURES *
 *=====================*/

/*===========================================================================*
 *
 * ReadFrame
 *
 *	reads the given frame, performing conversion as necessary
 *	if addPath = TRUE, then must add the current path before the
 *	file name
 *
 * RETURNS:	frame modified
 *
 * SIDE EFFECTS:    none
 *
 *===========================================================================*/
void
ReadFrame(frame, fileName, conversion, addPath)
    MpegFrame *frame;
    char *fileName;
    char *conversion;
    boolean addPath;
{
    FILE	*ifp;
    char	command[1024];
    char	fullFileName[1024];
#ifdef BLEAH
    static int32	readDiskTime = 0;
    int32	diskStartTime, diskEndTime;

time(&diskStartTime);
#endif

    if ( addPath ) {
	sprintf(fullFileName, "%s/%s", currentPath, fileName);
    } else {
	sprintf(fullFileName, "%s", fileName);
    }

#ifdef BLEAH
    if ( ! childProcess ) {
	fprintf(stdout, "+++++READING Frame %d  (type %d):  %s\n", frame->id,
		frame->type, fullFileName);
    }
#endif

    if ( fileType == ANY_FILE_TYPE ) {
	char *convertPtr, *commandPtr, *charPtr;

	/* replace every occurrence of '*' with fullFileName */
	convertPtr = conversion;
	commandPtr = command;
	while ( *convertPtr != '\0' ) {
	    while ( (*convertPtr != '\0') && (*convertPtr != '*') ) {
		*commandPtr = *convertPtr;
		commandPtr++;
		convertPtr++;
	    }

	    if ( *convertPtr == '*' ) {
		/* copy fullFileName */
		charPtr = fullFileName;
		while ( *charPtr != '\0' ) {
		    *commandPtr = *charPtr;
		    commandPtr++;
		    charPtr++;
		}

		convertPtr++;   /* go past '*' */
	    }
	}
	*commandPtr = '\0';

/* @@@ Open binary mode for OS/2, Andy Key */
#ifdef OS2
	if ( (ifp = popen(command, "rb")) == NULL ) {
#else
	if ( (ifp = popen(command, "r")) == NULL ) {
#endif
	    fprintf(stderr, "ERROR:  Couldn't execute input conversion command:\n");
	    fprintf(stderr, "\t%s\n", command);
	    fprintf(stderr, "errno = %d\n", errno);
	    if ( ioServer ) {
		fprintf(stderr, "IO SERVER:  EXITING!!!\n");
	    } else {
		fprintf(stderr, "SLAVE EXITING!!!\n");
	    }
	    exit(1);
	}
    } else {
/* @@@ Open binary mode for OS/2, Andy Key */
#ifdef OS2
	if ( (ifp = fopen(fullFileName, "rb")) == NULL ) {
#else
	if ( (ifp = fopen(fullFileName, "r")) == NULL ) {
#endif
	    fprintf(stderr, "ERROR:  Couldn't open input file %s\n",
		    fullFileName);
	    exit(1);
	}
    }

    switch(baseFormat) {
	case YUV_FILE_TYPE:
	    ReadYUV(frame, ifp, realWidth, realHeight);
	    break;
	case PPM_FILE_TYPE:
	    if ( ! ReadPPM(frame, ifp) ) {
		fprintf(stderr, "Error reading PPM input file!!!\n");
		exit(1);
	    }
	    PPMtoYUV(frame);
	    break;
	case PNM_FILE_TYPE:
	    ReadPNM(ifp, frame);
	    PNMtoYUV(frame);
	    break;
	case SUB4_FILE_TYPE:
	    ReadSub4(frame, ifp, yuvWidth, yuvHeight);
	    break;
	default:
	    break;
    }

    if ( fileType == ANY_FILE_TYPE ) {
	pclose(ifp);
    } else {
	fclose(ifp);
    }

#ifdef BLEAH
time(&diskEndTime);

    readDiskTime += (diskEndTime-diskStartTime);

fprintf(stdout, "cumulative disk read time:  %d seconds\n", readDiskTime);
#endif

    MotionSearchPreComputation(frame);
}


/*===========================================================================*
 *
 * SetFileType
 *
 *	set the file type to be either a base type (no conversion), or
 *	any type (conversion required)
 *
 * RETURNS:	nothing
 *
 * SIDE EFFECTS:    fileType
 *
 *===========================================================================*/
void
SetFileType(conversion)
    char *conversion;
{
    if ( strcmp(conversion, "*") == 0 ) {
	fileType = BASE_FILE_TYPE;
    } else {
	fileType = ANY_FILE_TYPE;
    }
}


/*===========================================================================*
 *
 * SetFileFormat
 *
 *	set the file format (PPM, PNM, YUV)
 *
 * RETURNS:	nothing
 *
 * SIDE EFFECTS:    baseFormat
 *
 *===========================================================================*/
void
SetFileFormat(format)
    char *format;
{
    if ( strcmp(format, "PPM") == 0 ) {
	baseFormat = PPM_FILE_TYPE;
    } else if ( strcmp(format, "YUV") == 0 ) {
	baseFormat = YUV_FILE_TYPE;
    } else if ( strcmp(format, "PNM") == 0 ) {
	baseFormat = PNM_FILE_TYPE;
    } else if ( strcmp(format, "JPEG") == 0 ) {
	fprintf(stderr, "SORRY:  JPEG not available YET\n");
	exit(1);
    } else if ( strcmp(format, "SUB4") == 0 ) {
	baseFormat = SUB4_FILE_TYPE;
    } else {
	fprintf(stderr, "ERROR:  Invalid file format:  %s\n", format);
	exit(1);
    }
}


/*===========================================================================*
 *
 * ReadPNM
 *
 *	read a PNM file
 *
 * RETURNS:	mf modified
 *
 * SIDE EFFECTS:    none
 *
 *===========================================================================*/
static void
ReadPNM(fp, mf)
    FILE *fp;
    MpegFrame *mf;
{
    int x, y;
    xelval maxval;
    int format;

    mf->rgb_data = pnm_readpnm(fp, &x, &y, &maxval, &format);
    ERRCHK(mf, "pnm_readpnm");

    if (format != PPM_FORMAT) {
	if (maxval < 255) {
	    pnm_promoteformat(mf->rgb_data, x, y, maxval, format, 255, PPM_FORMAT);
	    maxval = 255;
	} else {
	    pnm_promoteformat(mf->rgb_data, x, y, maxval, format, maxval, PPM_FORMAT);
	}
    }
    if (maxval < 255) {
	pnm_promoteformat(mf->rgb_data, x, y, maxval, format, 255, format);
	maxval = 255;
    }
    /*
     * if this is the first frame read, set the global frame size
     */
    Fsize_Note(mf->id, x, y);

    mf->rgb_maxval = maxval;
    mf->rgb_format = PPM_FORMAT;
}



/*===========================================================================*
 *
 * ReadIOConvert
 *
 *	do conversion; return a pointer to the appropriate file
 *
 * RETURNS:	pointer to the appropriate file
 *
 * SIDE EFFECTS:    none
 *
 *===========================================================================*/
FILE *
ReadIOConvert(fileName)
    char *fileName;
{
    FILE	*ifp;
    char	command[1024];
    char	fullFileName[1024];
    char *convertPtr, *commandPtr, *charPtr;

    sprintf(fullFileName, "%s/%s", currentPath, fileName);

#ifdef BLEAH
    if ( ! childProcess ) {
	fprintf(stdout, "+++++READING (IO CONVERT) Frame %d  (type %d):  %s\n", frame->id,
		frame->type, fullFileName); }
#endif

    if ( strcmp(ioConversion, "*") == 0 ) {
/* @@@ Open binary mode for OS/2, Andy Key */
#ifdef OS2
	ifp = fopen(fullFileName, "rb");
#else
	ifp = fopen(fullFileName, "r");
#endif

	ERRCHK(ifp, "fopen");

	return ifp;
    }

    /* replace every occurrence of '*' with fullFileName */
    convertPtr = ioConversion;
    commandPtr = command;
    while ( *convertPtr != '\0' ) {
	while ( (*convertPtr != '\0') && (*convertPtr != '*') ) {
	    *commandPtr = *convertPtr;
	    commandPtr++;
	    convertPtr++;
	}

	if ( *convertPtr == '*' ) {
	    /* copy fullFileName */
	    charPtr = fullFileName;
	    while ( *charPtr != '\0' ) {
		*commandPtr = *charPtr;
		commandPtr++;
		charPtr++;
	    }

	    convertPtr++;   /* go past '*' */
	}
    }
    *commandPtr = '\0';

/* @@@ Open binary mode for OS/2, Andy Key */
#ifdef OS2
    if ( (ifp = popen(command, "rb")) == NULL ) {
#else
    if ( (ifp = popen(command, "r")) == NULL ) {
#endif
	fprintf(stderr, "ERROR:  Couldn't execute input conversion command:\n");
	fprintf(stderr, "\t%s\n", command);
	fprintf(stderr, "errno = %d\n", errno);
	if ( ioServer ) {
	    fprintf(stderr, "IO SERVER:  EXITING!!!\n");
	} else {
	    fprintf(stderr, "SLAVE EXITING!!!\n");
	}
	exit(1);
    }

    return ifp;
}



/*===========================================================================*
 *
 * ReadPPM
 *
 *	read a PPM file
 *
 * RETURNS:	TRUE if successful; FALSE otherwise; mf modified
 *
 * SIDE EFFECTS:    none
 *
 *===========================================================================*/
static boolean
ReadPPM(mf, fpointer)
    MpegFrame *mf;
    FILE *fpointer;
{
    char    inputBuffer[71];
    char    string[71];
    char    *inputLine;
    int	    height = 0, width = 0, maxVal;
    uint8   junk[4096];
    register int y;
    int	    state;

    state = PPM_READ_STATE_MAGIC;

    while ( state != PPM_READ_STATE_DONE ) {
	if ( fgets(inputBuffer, 71, fpointer) == NULL ) {
	    return FALSE;
	}
	
        inputLine = inputBuffer;
 
	if ( inputLine[0] == '#' ) {
	    continue;
	}

	if ( inputLine[strlen(inputLine)-1] != '\n' ) {
	    return FALSE;
	}

	switch(state) {
	    case PPM_READ_STATE_MAGIC:
	        if ( (inputLine = ScanNextString(inputLine, string)) == NULL ) {
		    return FALSE;
		}

		if ( strcmp(string, "P6") != 0 ) {
		    return FALSE;
		}
		state = PPM_READ_STATE_WIDTH;
		/* no break */
	    case PPM_READ_STATE_WIDTH:
	        if ( (inputLine = ScanNextString(inputLine, string)) == NULL ) {
		    if ( inputLine == inputBuffer ) {
		        return FALSE;
		    } else {
		        break;
		    }
		}

		width = atoi(string);

		state = PPM_READ_STATE_HEIGHT;

		/* no break */
	    case PPM_READ_STATE_HEIGHT:
	        if ( (inputLine = ScanNextString(inputLine, string)) == NULL ) {
		    if ( inputLine == inputBuffer ) {
		        return FALSE;
		    } else {
		        break;
		    }
		}

		height = atoi(string);

		state = PPM_READ_STATE_MAXVAL;

		/* no break */
	    case PPM_READ_STATE_MAXVAL:
	        if ( (inputLine = ScanNextString(inputLine, string)) == NULL ) {
		    if ( inputLine == inputBuffer ) {
		        return FALSE;
		    } else {
		        break;
		    }
		}

		maxVal = atoi(string);

		state = PPM_READ_STATE_DONE;
		break;
	} /* end of switch */
    }

    Fsize_Note(mf->id, width, height);

    mf->rgb_maxval = maxVal;

    Frame_AllocPPM(mf);

    for ( y = 0; y < Fsize_y; y++ ) {
	fread(mf->ppm_data[y], sizeof(char), 3*Fsize_x, fpointer);

	/* read the leftover stuff on the right side */
	fread(junk, sizeof(char), 3*(width-Fsize_x), fpointer);
    }

    return TRUE;
}


/*===========================================================================*
 *
 * ReadYUV
 *
 *	read a YUV file
 *
 * RETURNS:	mf modified
 *
 * SIDE EFFECTS:    none
 *
 *===========================================================================*/
static void
ReadYUV(mf, fpointer, width, height)
    MpegFrame *mf;
    FILE *fpointer;
    int width;
    int height;
{
    register int y;
    uint8   junk[4096];

    Fsize_Note(mf->id, width, height);

    Frame_AllocYCC(mf);

    for (y = 0; y < Fsize_y; y++) {			/* Y */
	fread(mf->orig_y[y], 1, Fsize_x, fpointer);

	/* read the leftover stuff on the right side */
	if ( width != Fsize_x ) {
	    fread(junk, 1, width-Fsize_x, fpointer);
	}
    }

    /* read the leftover stuff on the bottom */
    for (y = Fsize_y; y < height; y++) {
	fread(junk, 1, width, fpointer);
    }

    for (y = 0; y < Fsize_y / 2; y++) {			/* U */
	fread(mf->orig_cb[y], 1, Fsize_x / 2, fpointer);

	/* read the leftover stuff on the right side */
	if ( width != Fsize_x ) {
	    fread(junk, 1, (width-Fsize_x)/2, fpointer);
	}
    }

    /* read the leftover stuff on the bottom */
    for (y = Fsize_y / 2; y < height / 2; y++) {
	fread(junk, 1, width/2, fpointer);
    }

    for (y = 0; y < Fsize_y / 2; y++) {			/* V */
	fread(mf->orig_cr[y], 1, Fsize_x / 2, fpointer);

	/* read the leftover stuff on the right side */
	if ( width != Fsize_x ) {
	    fread(junk, 1, (width-Fsize_x)/2, fpointer);
	}
    }

    /* ignore leftover stuff on the bottom */
}


/*===========================================================================*
 *
 * ReadSub4
 *
 *	read a YUV file (subsampled even further by 4:1 ratio)
 *
 * RETURNS:	mf modified
 *
 * SIDE EFFECTS:    none
 *
 *===========================================================================*/
static void
ReadSub4(mf, fpointer, width, height)
    MpegFrame *mf;
    FILE *fpointer;
    int width;
    int height;
{
    register int y;
    register int x;
    uint8   buffer[1024];

    Fsize_Note(mf->id, width, height);

    Frame_AllocYCC(mf);

    for (y = 0; y < height/2; y++) {			/* Y */
	fread(buffer, 1, width/2, fpointer);
	for ( x = 0; x < width/2; x++ ) {
	    mf->orig_y[2*y][2*x] = buffer[x];
	    mf->orig_y[2*y][2*x+1] = buffer[x];
	    mf->orig_y[2*y+1][2*x] = buffer[x];
	    mf->orig_y[2*y+1][2*x+1] = buffer[x];
	}
    }

    for (y = 0; y < height / 4; y++) {			/* U */
	fread(buffer, 1, width/4, fpointer);
	for ( x = 0; x < width/4; x++ ) {
	    mf->orig_cb[2*y][2*x] = buffer[x];
	    mf->orig_cb[2*y][2*x+1] = buffer[x];
	    mf->orig_cb[2*y+1][2*x] = buffer[x];
	    mf->orig_cb[2*y+1][2*x+1] = buffer[x];
	}
    }

    for (y = 0; y < height / 4; y++) {			/* V */
	fread(buffer, 1, width/4, fpointer);
	for ( x = 0; x < width/4; x++ ) {
	    mf->orig_cr[2*y][2*x] = buffer[x];
	    mf->orig_cr[2*y][2*x+1] = buffer[x];
	    mf->orig_cr[2*y+1][2*x] = buffer[x];
	    mf->orig_cr[2*y+1][2*x+1] = buffer[x];
	}
    }
}


/*=====================*
 * INTERNAL PROCEDURES *
 *=====================*/

/*===========================================================================*
 *
 * ScanNextString
 *
 *	read a string from a input line, ignoring whitespace
 *
 * RETURNS:	pointer to position in input line after string
 *              NULL if all whitespace
 *              puts string in 'string'
 *
 * SIDE EFFECTS:    file stream munched a bit
 *
 *===========================================================================*/
static char *
ScanNextString(inputLine, string)
    char *inputLine;
    char *string;
{
    /* skip whitespace */
    while ( isspace(*inputLine) && (*inputLine != '\n') ) {
        inputLine++;
    }

    if ( *inputLine == '\n' ) {
        return NULL;
    }

    while ( (! isspace(*inputLine)) && (*inputLine != '\n') ) {
        *string = *inputLine;
	string++;
	inputLine++;
    }

    *string = '\0';

    return inputLine;
}
