/*===========================================================================*
 * main.c								     *
 *									     *
 *	Main procedure							     *
 *									     *
 * EXPORTED PROCEDURES:							     *
 *	main								     *
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
 *  $Header: /n/picasso/users/keving/encode/src/RCS/main.c,v 1.3 1993/07/22 22:23:43 keving Exp keving $
 *  $Log: main.c,v $
 * Revision 1.3  1993/07/22  22:23:43  keving
 * nothing
 *
 * Revision 1.2  1993/06/30  20:06:09  keving
 * nothing
 *
 * Revision 1.1  1993/02/17  23:18:20  dwallach
 * Initial revision
 *
 */


/*==============*
 * HEADER FILES *
 *==============*/

#include "all.h"
#include "mtypes.h"
#include "mpeg.h"
#include "search.h"
#ifdef OS2
/* @@@ FAT 8.3 convention */
#include "prototyp.h"
#else
#include "prototypes.h"
#endif
#include "param.h"
#include "parallel.h"
#ifdef OS2
/* @@@ FAT 8.3 convention */
#include "readfram.h"
#else
#include "readframe.h"
#endif
#include "combine.h"
#include "frames.h"


int	main _ANSI_ARGS_((int argc, char **argv));

/*==================*
 * STATIC VARIABLES *
 *==================*/

static int	frameStart = -1;
static int	frameEnd;


/*==================*
 * GLOBAL VARIABLES *
 *==================*/

extern time_t IOtime;
int	whichGOP = -1;
boolean	childProcess = FALSE;
boolean	ioServer = FALSE;
boolean	outputServer = FALSE;
boolean	decodeServer = FALSE;
int	quietTime = 0;
boolean	frameSummary = TRUE;
boolean debugSockets = FALSE;
boolean debugMachines = FALSE;


/*===============================*
 * INTERNAL PROCEDURE prototypes *
 *===============================*/

static void Usage _ANSI_ARGS_((void));


/*=====================*
 * EXPORTED PROCEDURES *
 *=====================*/


/*===========================================================================*
 *
 * main
 *
 *	see man page.  run without arguments to see usage
 *
 * RETURNS:	0 if all is well; 1 on most if not all errors
 *
 *===========================================================================*/
int
main(argc, argv)
    int argc;
    char **argv;
{
    FILE *ofp = NULL;
    register int index;
    int	    function = ENCODE_FRAMES;
    int	    portNumber = 0;
    char    *hostName = NULL;
    int32   totalTime = -1;
    int	    maxMachines = 0x7fffffff;
    int	    outputFrames = 0;
    time_t  initTimeStart, initTimeEnd;
    time_t  framesTimeStart, framesTimeEnd;

    time(&initTimeStart);

    if ( argc == 1 ) {
	Usage();
    }

    SetStatFileName("");

    /* parse the arguments */
    index = 1;
    while ( index < argc-1 ) {
	if ( argv[index][0] != '-' ) {
	    Usage();
	}

	if ( strcmp(argv[index], "-stat") == 0 ) {
	    if ( index+1 < argc-1 ) {
		SetStatFileName(argv[index+1]);
		index += 2;
	    } else {
		Usage();
	    }
	} else if ( strcmp(argv[index], "-gop") == 0 ) {
	    if ( (function != ENCODE_FRAMES) || (frameStart != -1) ) {
		Usage();
	    }

	    if ( index+1 < argc-1 ) {
		whichGOP = atoi(argv[index+1]);
		index += 2;
	    } else {
		Usage();
	    }
	} else if ( strcmp(argv[index], "-frames") == 0 ) {
	    if ( (function != ENCODE_FRAMES) || (whichGOP != -1) ) {
		Usage();
	    }

	    if ( index+2 < argc-1 ) {
		frameStart = atoi(argv[index+1]);
		frameEnd = atoi(argv[index+2]);

		if ( (frameStart > frameEnd) || (frameStart < 0) ) {
		    fprintf(stderr, "ERROR:  bad frame numbers!\n");
		    Usage();
		}

		index += 3;
	    } else {
		Usage();
	    }
	} else if ( strcmp(argv[index], "-combine_gops") == 0 ) {
	    if ( (function != ENCODE_FRAMES) || (whichGOP != -1) || 
		 (frameStart != -1) ) {
		Usage();
	    }

	    function = COMBINE_GOPS;
	    index++;
	} else if ( strcmp(argv[index], "-combine_frames") == 0 ) {
	    if ( (function != ENCODE_FRAMES) || (whichGOP != -1) ||
		 (frameStart != -1) ) {
		Usage();
	    }

	    function = COMBINE_FRAMES;
	    index++;
	} else if ( strcmp(argv[index], "-child") == 0 ) {
	    if ( index+7 < argc-1 ) {
		hostName = argv[index+1];
		portNumber = atoi(argv[index+2]);
		ioPortNumber = atoi(argv[index+3]);
		combinePortNumber = atoi(argv[index+4]);
		decodePortNumber = atoi(argv[index+5]);
		machineNumber = atoi(argv[index+6]);
		remoteIO = atoi(argv[index+7]);

		IOhostName = hostName;
	    } else {
		Usage();
	    }

	    childProcess = TRUE;
	    index += 8;
	} else if ( strcmp(argv[index], "-io_server") == 0 ) {
	    if ( index+2 < argc-1 ) {
		hostName = argv[index+1];
		portNumber = atoi(argv[index+2]);
	    } else {
		Usage();
	    }

	    ioServer = TRUE;
	    index += 3;
	} else if ( strcmp(argv[index], "-output_server") == 0 ) {
	    if ( index+3 < argc-1 ) {
		hostName = argv[index+1];
		portNumber = atoi(argv[index+2]);
		outputFrames = atoi(argv[index+3]);
	    } else {
		Usage();
	    }

	    function = COMBINE_FRAMES;
	    outputServer = TRUE;
	    index += 4;
	} else if ( strcmp(argv[index], "-decode_server") == 0 ) {
	    if ( index+3 < argc-1 ) {
		hostName = argv[index+1];
		portNumber = atoi(argv[index+2]);
		outputFrames = atoi(argv[index+3]);
	    } else {
		Usage();
	    }

	    function = COMBINE_FRAMES;
	    decodeServer = TRUE;
	    index += 4;
	} else if ( strcmp(argv[index], "-nice") == 0 ) {
	    niceProcesses = TRUE;
	    index++;
	} else if ( strcmp(argv[index], "-max_machines") == 0 ) {
	    if ( index+1 < argc-1 ) {
		maxMachines = atoi(argv[index+1]);
	    } else {
		Usage();
	    }

	    index += 2;
	} else if ( strcmp(argv[index], "-quiet") == 0 ) {
	    if ( index+1 < argc-1 ) {
		quietTime = atoi(argv[index+1]);
	    } else {
		Usage();
	    }

	    index += 2;
	} else if ( strcmp(argv[index], "-no_frame_summary") == 0 ) {
	    if ( index < argc-1 ) {
		frameSummary = FALSE;
	    } else {
		Usage();
	    }

	    index++;
	} else if ( strcmp(argv[index], "-snr") == 0 ) {
	    printSNR = TRUE;
	    index++;
	} else if ( strcmp(argv[index], "-debug_sockets") == 0 ) {
	    debugSockets = TRUE;
	} else if ( strcmp(argv[index], "-debug_machines") == 0 ) {
	    debugMachines = TRUE;
	} else {
	    Usage();
	}
    }

    if ( ! ReadParamFile(argv[argc-1], function) ) {
	Usage();
    }

    if ( printSNR || (referenceFrame == DECODED_FRAME) ) {
	decodeRefFrames = TRUE;
    }

    numMachines = min(numMachines, maxMachines);

    Frame_Init();

#ifdef BLEAH
    time(&initTimeEnd);
    fprintf(stdout, "INIT TIME:  %d seconds\n",
	    initTimeEnd-initTimeStart);
    fflush(stdout);
#endif

    if ( ioServer ) {
	StartIOServer(numInputFiles, hostName, portNumber);
	return 0;
    } else if ( outputServer ) {
	StartCombineServer(outputFrames, outputFileName, hostName, portNumber);
	return 0;
    } else if ( decodeServer ) {
	StartDecodeServer(outputFrames, outputFileName, hostName, portNumber);
	return 0;
    }

    if ( (frameStart == -1) &&
	 ((numMachines == 0) || (function != ENCODE_FRAMES)) ) {
/* @@@ Open binary mode for OS/2, Andy Key */
#ifdef OS2
	if ( (ofp = fopen(outputFileName, "wb")) == NULL ) {
#else
	if ( (ofp = fopen(outputFileName, "w")) == NULL ) {
#endif
	    fprintf(stderr, "ERROR:  Could not open output file!\n");
	    exit(1);
	}
    }

    if ( ! childProcess ) {
	fprintf(stdout, "Output File:  %s\n", outputFileName);
	fprintf(stdout, "\n\n");
    }

    if ( function == ENCODE_FRAMES ) {
	if ( (numMachines == 0) || (frameStart != -1) ) {
	    time(&framesTimeStart);
	    totalTime = GenMPEGStream(whichGOP, frameStart, frameEnd,
				      numInputFiles, ofp,
				      outputFileName);
	    time(&framesTimeEnd);
	    if ( childProcess ) {
#ifdef BLEAH
		fprintf(stdout, "SCHEDULE:  MACHINE %d FRAMES %d-%d TIME %d-%d IOTIME %d\n",
			machineNumber, frameStart, frameEnd,
			framesTimeStart, framesTimeEnd,
			IOtime);
#endif
		fprintf(stdout, "%s:  FRAMES %d-%d (%d seconds)\n",
			getenv("HOST"), frameStart, frameEnd,
			framesTimeEnd-framesTimeStart);
		fflush(stdout);
	    }
	} else {
	    /* check if parameter file has absolute path */
	    if ( (argv[argc-1][0] != '/') && (argv[argc-1][0] != '~') ) {
		fprintf(stderr, "ERROR:  For parallel execution, please use absolute path for parameter file!\n");
		exit(1);
	    } else {
		StartMasterServer(numInputFiles, argv[argc-1], outputFileName);
	    }
	}
    } else if ( function == COMBINE_GOPS ) {
	GOPStoMPEG(numInputFiles, outputFileName, ofp);
    } else if ( function == COMBINE_FRAMES ) {
	FramesToMPEG(numInputFiles, outputFileName, ofp, FALSE);
    }

    if ( childProcess ) {
	while ( NotifyMasterDone(hostName, portNumber, machineNumber,
				 totalTime,
				 &frameStart, &frameEnd) ) {
	    /* do more frames */
	    time(&framesTimeStart);
	    totalTime = GenMPEGStream(-1, frameStart, frameEnd,
				      numInputFiles, NULL,
				      outputFileName);
	    time(&framesTimeEnd);
#ifdef BLEAH
	    fprintf(stdout, "SCHEDULE:  MACHINE %d FRAMES %d-%d TIME %d-%d IOTIME %d\n",
		    machineNumber, frameStart, frameEnd,
		    framesTimeStart, framesTimeEnd,
		    IOtime);
#endif
	    fprintf(stdout, "%s:  FRAMES %d-%d (%d seconds)\n",
		    getenv("HOST"), frameStart, frameEnd,
		    framesTimeEnd-framesTimeStart);
	    fflush(stdout);
	}
    }

    Frame_Exit();

    return 0;	/* all is well */
}


/*=====================*
 * INTERNAL PROCEDURES *
 *=====================*/

/*===========================================================================*
 *
 * Usage
 *
 *	prints out usage for the program
 *
 * RETURNS:	nothing
 *
 * SIDE EFFECTS:    none
 *
 *===========================================================================*/
static void
Usage()
{
    fprintf(stderr, "Usage:  mpeg_encode [options] param_file\n");
    fprintf(stderr, "Options:\n");
    fprintf(stderr, "\t-stat stat_file:  append stats to stat_file\n");
    fprintf(stderr, "\t-quiet n:  don't report remaining time for at least n seconds\n");
    fprintf(stderr, "\t-no_frame_summary:  suppress frame summary lines\n");
    fprintf(stderr, "\t-gop gop_num:  encode only the numbered GOP\n");
    fprintf(stderr, "\t-combine_gops:  combine GOP files instead of encode\n");
    fprintf(stderr, "\t-frames first_frame last_frame:  encode only the specified frames\n");
    fprintf(stderr, "\t-combine_frames:  combine frame files instead of encode\n");
    fprintf(stderr, "\t-nice:  run slave processes nicely\n");
    fprintf(stderr, "\t-max_machines num_machines:  use at most num_machines machines\n");
    fprintf(stderr, "\t-snr:  print signal-to-noise ratio\n");
    exit(1);

/* extended usage (used by parallel code; shouldn't be called by user):
    -child parallelHostName portNumber ioPortNumber combinePortNumber machineNumber remote
    -io_server parallelHostName portNumber
    
    (remote = 1 if need to use ioPortNumber)
 */
}
