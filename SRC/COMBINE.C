/*===========================================================================*
 * combine.c								     *
 *									     *
 *	Procedures to combine frames or GOPS into an MPEG sequence	     *
 *									     *
 * EXPORTED PROCEDURES:							     *
 *	GOPStoMPEG							     *
 *	FramesToMPEG							     *
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
 *  $Header: /n/picasso/users/keving/encode/src/RCS/combine.c,v 1.1 1993/07/22 22:23:43 keving Exp keving $
 *  $Log: combine.c,v $
 * Revision 1.1  1993/07/22  22:23:43  keving
 * nothing
 *
 */


/*==============*
 * HEADER FILES *
 *==============*/

#include "all.h"
#include <time.h>
#include <errno.h>
#include "mtypes.h"
#include "frames.h"
#include "search.h"
#include "mpeg.h"
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
#include "mheaders.h"
#include "fsize.h"
#include "combine.h"


static int	currentGOP;


/*==================*
 * GLOBAL VARIABLES *
 *==================*/
extern int  yuvWidth, yuvHeight;
char	currentGOPPath[MAXPATHLEN];
char	currentFramePath[MAXPATHLEN];


/*===============================*
 * INTERNAL PROCEDURE prototypes *
 *===============================*/

static void	AppendFile _ANSI_ARGS_((FILE *outputFile, FILE *inputFile));


/*=====================*
 * EXPORTED PROCEDURES *
 *=====================*/

/*===========================================================================*
 *
 * GOPStoMPEG
 *
 *	convert some number of GOP files into a single MPEG sequence file
 *
 * RETURNS:	nothing
 *
 * SIDE EFFECTS:    none
 *
 *===========================================================================*/
void
GOPStoMPEG(numGOPS, outputFileName, outputFilePtr)
    int numGOPS;
    char *outputFileName;
    FILE *outputFilePtr;
{
    register int index;
    BitBucket *bb;
    char    fileName[1024];
    char    inputFileName[1024];
    FILE *inputFile;

    Fsize_Reset();
    Fsize_Note(0, yuvWidth, yuvHeight);
    SetBlocksPerSlice();
    
    bb = Bitio_New(outputFilePtr);

    Mhead_GenSequenceHeader(bb, Fsize_x, Fsize_y, /* pratio */ 1,
	       /* pict_rate */ -1, /* bit_rate */ -1,
	       /* buf_size */ -1, /*c_param_flag */ 1,
	       /* iq_matrix */ NULL, /* niq_matrix */ NULL,
	       /* ext_data */ NULL, /* ext_data_size */ 0,
	       /* user_data */ NULL, /* user_data_size */ 0);

    /* it's byte-padded, so we can dump it now */
    Bitio_Flush(bb);

    if ( numGOPS > 0 ) {
	for ( index = 0; index < numGOPS; index++ ) {
	    GetNthInputFileName(inputFileName, index);
	    sprintf(fileName, "%s/%s", currentGOPPath, inputFileName);

/* @@@ Open binary mode for OS/2, Andy Key */
#ifdef OS2
	    if ( (inputFile = fopen(fileName, "rb")) == NULL ) {
#else
	    if ( (inputFile = fopen(fileName, "r")) == NULL ) {
#endif
		fprintf(stderr, "ERROR:  Couldn't read:  %s\n", fileName);
		fflush(stderr);
		exit(1);
	    }
	
	    fprintf(stdout, "appending file:  %s\n", fileName);

	    AppendFile(outputFilePtr, inputFile);
	}
    } else {
	index = 0;
	while ( TRUE ) {
	    sprintf(fileName, "%s.gop.%d", outputFileName, index);

/* @@@ Open binary mode for OS/2, Andy Key */
#ifdef OS2
	    if ( (inputFile = fopen(fileName, "rb")) == NULL ) {
#else
	    if ( (inputFile = fopen(fileName, "r")) == NULL ) {
#endif
		break;
	    }

	    fprintf(stdout, "appending file:  %s\n", fileName);

	    AppendFile(outputFilePtr, inputFile);

	    index++;
	}
    }

    bb = Bitio_New(outputFilePtr);

    /* SEQUENCE END CODE */
    Mhead_GenSequenceEnder(bb);

    Bitio_Flush(bb);

    fclose(outputFilePtr);
}


/*===========================================================================*
 *
 * FramestoMPEG
 *
 *	convert some number of frame files into a single MPEG sequence file
 *
 *	if parallel == TRUE, then when appending a file, blocks until that
 *	file is actually ready
 *
 * RETURNS:	nothing
 *
 * SIDE EFFECTS:    none
 *
 *===========================================================================*/
void
FramesToMPEG(numFrames, outputFileName, outputFile, parallel)
    int numFrames;
    char *outputFileName;
    FILE *outputFile;
    boolean parallel;
{
    register int index;
    BitBucket *bb;
    char    fileName[1024];
    char    inputFileName[1024];
    FILE *inputFile;
    FrameTable *entry, *ptr;

    tc_hrs = 0;	tc_min = 0; tc_sec = 0; tc_pict = 0;

    Fsize_Reset();
    Fsize_Note(0, yuvWidth, yuvHeight);
    SetBlocksPerSlice();

    bb = Bitio_New(outputFile);
    Mhead_GenSequenceHeader(bb, Fsize_x, Fsize_y, /* pratio */ 1,
	       /* pict_rate */ -1, /* bit_rate */ -1,
	       /* buf_size */ -1, /*c_param_flag */ 1,
	       /* iq_matrix */ NULL, /* niq_matrix */ NULL,
	       /* ext_data */ NULL, /* ext_data_size */ 0,
	       /* user_data */ NULL, /* user_data_size */ 0);
    /* it's byte-padded, so we can dump it now */
    Bitio_Flush(bb);

    /* need to do these in the right order!!! */
    /* also need to add GOP headers */

    currentGOP = gopSize;
    totalFramesSent = 0;

    if ( numFrames > 0 ) {
	for ( index = 0; index < numFrames; index++ ) {
	    if ( FRAME_TYPE(index) == 'b' ) {
		continue;
	    }

	    if ( (FRAME_TYPE(index) == 'i') && (currentGOP >= gopSize) ) {
		int closed;

		/* first, check to see if closed GOP */
		if ( totalFramesSent == index ) {
		    closed = 1;
		} else {
		    closed = 0;
		}

		fprintf(stdout, "Creating new GOP (closed = %d) after %d frames\n",
			closed, currentGOP);

		/* new GOP */
		bb = Bitio_New(outputFile);
		Mhead_GenGOPHeader(bb, /* drop_frame_flag */ 0,
			   tc_hrs, tc_min, tc_sec, tc_pict,
			   closed, /* broken_link */ 0,
			   /* ext_data */ NULL, /* ext_data_size */ 0,
			   /* user_data */ NULL, /* user_data_size */ 0);
		Bitio_Flush(bb);
		SetGOPStartTime(index);
		
		currentGOP -= gopSize;
	    }

	    if ( parallel ) {
		WaitForOutputFile(index);
		sprintf(fileName, "%s.frame.%d", outputFileName, index);
	    } else {
		GetNthInputFileName(inputFileName, index);
		sprintf(fileName, "%s/%s", currentFramePath, inputFileName);
	    }

/* @@@ Open binary mode for OS/2, Andy Key */
#ifdef OS2
	    if ( (inputFile = fopen(fileName, "rb")) == NULL ) {
#else
	    if ( (inputFile = fopen(fileName, "r")) == NULL ) {
#endif
		fprintf(stderr, "ERROR:  Couldn't read:  %s\n", fileName);
		fflush(stderr);
		exit(1);
	    }

	    AppendFile(outputFile, inputFile);

	    currentGOP++;
	    IncrementTCTime();

	    if ( (index != 0) && ((index % framePatternLen) == 0) ) {
		entry = &(frameTable[framePatternLen]);
	    } else {
		entry = &(frameTable[index % framePatternLen]);
	    }

	    /* now, follow nextOutput and output B-frames */
	    ptr = entry->nextOutput;
	    while ( ptr != NULL ) {
		if ( parallel ) {
		    WaitForOutputFile(index - (entry->number - ptr->number));
		    sprintf(fileName, "%s.frame.%d", outputFileName,
			    index - (entry->number - ptr->number));
		} else {
		    GetNthInputFileName(inputFileName,
					index - (entry->number - ptr->number));
		    sprintf(fileName, "%s/%s", currentFramePath, inputFileName);
		}

/* @@@ Open binary mode for OS/2, Andy Key */
#ifdef OS2
		if ( (inputFile = fopen(fileName, "rb")) == NULL ) {
#else
		if ( (inputFile = fopen(fileName, "r")) == NULL ) {
#endif
		    fprintf(stderr, "ERROR:  Couldn't read:  %s\n", fileName);
		    exit(1);
		}

		AppendFile(outputFile, inputFile);
		    
		currentGOP++;
		IncrementTCTime();

		ptr = ptr->nextOutput;
	    }
	}
    } else {
	if ( parallel ) {
	    fprintf(stderr, "ERROR:  PARALLEL COMBINE WITH 0 FRAMES\n");
	    fprintf(stderr, "(please send bug report!)\n");
	    exit(1);
	}

	index = 0;
	while ( TRUE ) {
	    if ( FRAME_TYPE(index) == 'b' ) {
		index++;
		continue;
	    }

	    if ( (FRAME_TYPE(index) == 'i') && (currentGOP >= gopSize) ) {
		int closed;

		/* first, check to see if closed GOP */
		if ( totalFramesSent == index ) {
		    closed = 1;
		} else {
		    closed = 0;
		}

		fprintf(stdout, "Creating new GOP (closed = %d) before frame %d\n",
			closed, index);

		/* new GOP */
		bb = Bitio_New(outputFile);
		Mhead_GenGOPHeader(bb, /* drop_frame_flag */ 0,
			   tc_hrs, tc_min, tc_sec, tc_pict,
			   closed, /* broken_link */ 0,
			   /* ext_data */ NULL, /* ext_data_size */ 0,
			   /* user_data */ NULL, /* user_data_size */ 0);
		Bitio_Flush(bb);
		SetGOPStartTime(index);

		currentGOP -= gopSize;
	    }

	    sprintf(fileName, "%s.frame.%d", outputFileName, index);

/* @@@ Open binary mode for OS/2, Andy Key */
#ifdef OS2
	    if ( (inputFile = fopen(fileName, "rb")) == NULL ) {
#else
	    if ( (inputFile = fopen(fileName, "r")) == NULL ) {
#endif
		break;
	    }

	    AppendFile(outputFile, inputFile);

	    currentGOP++;
	    IncrementTCTime();

	    if ( (index != 0) && ((index % framePatternLen) == 0) ) {
		entry = &(frameTable[framePatternLen]);
	    } else {
		entry = &(frameTable[index % framePatternLen]);
	    }

	    /* now, follow nextOutput and output B-frames */
	    ptr = entry->nextOutput;
	    while ( ptr != NULL ) {
		sprintf(fileName, "%s.frame.%d", outputFileName,
			index - (entry->number - ptr->number));

/* @@@ Open binary mode for OS/2, Andy Key */
#ifdef OS2
		if ( (inputFile = fopen(fileName, "rb")) == NULL ) {
#else
		if ( (inputFile = fopen(fileName, "r")) == NULL ) {
#endif
		    fprintf(stderr, "ERROR:  Couldn't read:  %s\n", fileName);
		    fflush(stderr);
		    exit(1);
		}

		AppendFile(outputFile, inputFile);
		    
		currentGOP++;
		IncrementTCTime();

		ptr = ptr->nextOutput;
	    }

	    index++;
	}
    }

    fprintf(stdout, "Wrote %d frames\n", totalFramesSent);
    fflush(stdout);

    bb = Bitio_New(outputFile);

    /* SEQUENCE END CODE */
    Mhead_GenSequenceEnder(bb);

    Bitio_Flush(bb);

    fclose(outputFile);
}


/*=====================*
 * INTERNAL PROCEDURES *
 *=====================*/

/*===========================================================================*
 *
 * AppendFile
 *
 *	appends the output file with the contents of the given input file
 *
 * RETURNS:	nothing
 *
 * SIDE EFFECTS:    none
 *
 *===========================================================================*/
static void
AppendFile(outputFile, inputFile)
    FILE *outputFile;
    FILE *inputFile;
{
    uint8   *data[9999];
    int	    readItems;

    readItems = 9999;
    while ( readItems == 9999 ) {
	readItems = fread(data, sizeof(uint8), 9999, inputFile);
	if ( readItems > 0 ) {
	    fwrite(data, sizeof(uint8), readItems, outputFile);
	}
    }

    fclose(inputFile);
}


