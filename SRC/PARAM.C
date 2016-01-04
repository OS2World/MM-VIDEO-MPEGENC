/*===========================================================================*
 * param.c								     *
 *									     *
 *	Procedures to read in parameter file				     *
 *									     *
 * EXPORTED PROCEDURES:							     *
 *	ReadParamFile							     *
 *	GetNthInputFileName						     *
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
 *  $Header: /n/picasso/users/keving/encode/src/RCS/param.c,v 1.2 1993/07/22 22:23:43 keving Exp keving $
 *  $Log: param.c,v $
 * Revision 1.2  1993/07/22  22:23:43  keving
 * nothing
 *
 * Revision 1.1  1993/06/30  20:06:09  keving
 * nothing
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
#include "parallel.h"
#include "param.h"
#ifdef OS2
/* @@@ FAT 8.3 convention */
#include "readfram.h"
#else
#include "readframe.h"
#endif
#include "fsize.h"


/*===========*
 * CONSTANTS *
 *===========*/

#define INPUT_ENTRY_BLOCK_SIZE   128

#define FIRST_OPTION 0
#define OPTION_GOP  0
#define OPTION_PATTERN 1
#define OPTION_PIXEL 2
#define OPTION_PQSCALE 3
#define OPTION_OUTPUT 4
#define OPTION_RANGE 5
#define OPTION_PSEARCH_ALG 6
#define OPTION_IQSCALE 7
#define OPTION_INPUT_DIR 8
#define OPTION_INPUT_CONVERT 9
#define OPTION_INPUT 10
#define OPTION_BQSCALE 11
#define OPTION_BASE_FORMAT 12
#define OPTION_SPF 13
#define OPTION_BSEARCH_ALG 14
#define OPTION_REF_FRAME    15
#define LAST_OPTION 15
/* YUV_SIZE not required, so put after LAST_OPTION */
#define OPTION_IO_CONVERT   16
#define OPTION_SLAVE_CONVERT	17
#define OPTION_YUV_SIZE	18


/*=======================*
 * STRUCTURE DEFINITIONS *
 *=======================*/

typedef struct InputFileEntryStruct {
    char    left[256];
    char    right[256];
    boolean glob;	    /* if FALSE, left is complete name */
    int	    startID;
    int	    endID;
    int	    skip;
    int	    numPadding;	    /* -1 if there is none */
    int	    numFiles;
} InputFileEntry;


/*==================*
 * STATIC VARIABLES *
 *==================*/

static InputFileEntry **inputFileEntries;
static int numInputFileEntries = 0;
static int  maxInputFileEntries;


/*==================*
 * GLOBAL VARIABLES *
 *==================*/

extern char currentPath[MAXPATHLEN];
extern char currentGOPPath[MAXPATHLEN];
extern char currentFramePath[MAXPATHLEN];
char	outputFileName[256];
int numInputFiles = 0;
char inputConversion[1024];
char ioConversion[1024];
char slaveConversion[1024];
boolean optionSeen[OPTION_YUV_SIZE+1];
int numMachines;
char	machineName[MAX_MACHINES][256];
char	userName[MAX_MACHINES][256];
char	executable[MAX_MACHINES][1024];
char	remoteParamFile[MAX_MACHINES][1024];
boolean	remote[MAX_MACHINES];


/*===============================*
 * INTERNAL PROCEDURE prototypes *
 *===============================*/

static void	ReadInputFileNames _ANSI_ARGS_((FILE *fpointer,
						char *endInput));
static char	*SkipSpacesTabs _ANSI_ARGS_((char *start));
static void	ReadMachineNames _ANSI_ARGS_((FILE *fpointer));


/*=====================*
 * EXPORTED PROCEDURES *
 *=====================*/


/*===========================================================================*
 *
 * ReadParamFile
 *
 *	read the parameter file
 *	function is ENCODE_FRAMES, COMBINE_GOPS, or COMBINE_FRAMES, and
 *	    will slightly modify the procedure's behavior as to what it
 *	    is looking for in the parameter file
 *
 * RETURNS:	TRUE if the parameter file was read correctly; FALSE if not
 *
 * SIDE EFFECTS:    sets parameters accordingly, as well as machine info for
 *		    parallel execution and input file names
 *
 *===========================================================================*/
boolean
ReadParamFile(fileName, function)
    char *fileName;
    int function;
{
    FILE *fpointer;
    char    input[256];
    char    *charPtr;
    boolean yuvUsed = FALSE;
    static char *optionText[LAST_OPTION+1] = { "GOP", "PATTERN", "PIXEL", "PQSCALE",
	"OUTPUT", "RANGE", "PSEARCH_ALG", "IQSCALE", "INPUT_DIR",
	"INPUT_CONVERT", "INPUT", "BQSCALE", "BASE_FILE_FORMAT",
	"SLICES_PER_FRAME", "BSEARCH_ALG", "REFERENCE_FRAME" };
    register int index;

    if ( (fpointer = fopen(fileName, "r")) == NULL ) {
	fprintf(stderr, "Error:  Cannot open parameter file:  %s\n", fileName);
	return FALSE;
    }

    /* should set defaults */
    numInputFiles = 0;
    numMachines = 0;
    sprintf(currentPath, ".");
    sprintf(currentGOPPath, ".");
    sprintf(currentFramePath, ".");
    SetRemoteShell("rsh");

    switch(function) {
	case ENCODE_FRAMES:
	    for ( index = FIRST_OPTION; index <= LAST_OPTION; index++ ) {
		optionSeen[index] = FALSE;
	    }
	    optionSeen[OPTION_YUV_SIZE] = FALSE;
	    optionSeen[OPTION_IO_CONVERT] = FALSE;
	    optionSeen[OPTION_SLAVE_CONVERT] = FALSE;
	    break;
	case COMBINE_GOPS:
	    for ( index = FIRST_OPTION; index <= LAST_OPTION; index++ ) {
		optionSeen[index] = TRUE;
	    }

	    optionSeen[OPTION_YUV_SIZE] = FALSE;
	    optionSeen[OPTION_OUTPUT] = FALSE;
	    break;
	case COMBINE_FRAMES:
	    for ( index = FIRST_OPTION; index <= LAST_OPTION; index++ ) {
		optionSeen[index] = TRUE;
	    }

	    optionSeen[OPTION_GOP] = FALSE;
	    optionSeen[OPTION_OUTPUT] = FALSE;
	    optionSeen[OPTION_YUV_SIZE] = FALSE;
	    break;
    }

    while ( fgets(input, 256, fpointer) != NULL ) {
	if ( input[0] == '#' ) {	    /* skip comments */
	    continue;
	}

	input[strlen(input)-1] = '\0';	/* get rid of newline */

	switch(input[0]) {
	    case 'G':
		if ( strncmp(input, "GOP_SIZE", 8) == 0 ) {
		    charPtr = SkipSpacesTabs(&input[8]);
		    SetGOPSize(atoi(charPtr));
		    optionSeen[OPTION_GOP] = TRUE;
		} else if ( strncmp(input, "GOP_INPUT_DIR", 13) == 0 ) {
		    charPtr = SkipSpacesTabs(&input[13]);
		    strcpy(currentGOPPath, charPtr);
		} else if ( strncmp(input, "GOP_INPUT", 9) == 0 ) {
		    if ( function == COMBINE_GOPS ) {
			ReadInputFileNames(fpointer, "GOP_END_INPUT");
		    }
		}
		break;

	    case 'F':
		if ( strncmp(input, "FRAME_INPUT_DIR", 15) == 0 ) {
		    charPtr = SkipSpacesTabs(&input[15]);
		    strcpy(currentFramePath, charPtr);
		} else if ( strncmp(input, "FRAME_INPUT", 11) == 0 ) {
		    if ( function == COMBINE_FRAMES ) {
			ReadInputFileNames(fpointer, "FRAME_END_INPUT");
		    }
		} else if ( strncmp(input, "FORCE_I_ALIGN", 13) == 0 ) {
		    forceIalign = TRUE;
		}
		break;

	    case 'P':
		if ( strncmp(input, "PATTERN", 7) == 0 ) {
		    charPtr = SkipSpacesTabs(&input[7]);
		    SetFramePattern(charPtr);
		    optionSeen[OPTION_PATTERN] = TRUE;
		} else if ( strncmp(input, "PIXEL", 5) == 0 ) {
		    charPtr = SkipSpacesTabs(&input[5]);
		    SetPixelSearch(charPtr);
		    optionSeen[OPTION_PIXEL] = TRUE;
		} else if ( strncmp(input, "PQSCALE", 7) == 0 ) {
		    charPtr = SkipSpacesTabs(&input[7]);
		    SetPQScale(atoi(charPtr));
		    optionSeen[OPTION_PQSCALE] = TRUE;
		} else if ( strncmp(input, "PSEARCH_ALG", 11) == 0 ) {
		    charPtr = SkipSpacesTabs(&input[11]);
		    SetPSearchAlg(charPtr);
		    optionSeen[OPTION_PSEARCH_ALG] = TRUE;
		} else if ( strncmp(input, "PARALLEL_TEST_FRAMES", 20) == 0 ) {
		    charPtr = SkipSpacesTabs(&input[20]);
		    parallelTestFrames = atoi(charPtr);
		} else if ( strncmp(input, "PARALLEL_TIME_CHUNKS", 20) == 0 ) {
		    charPtr = SkipSpacesTabs(&input[20]);
		    parallelTimeChunks = atoi(charPtr);
		} else if ( strncmp(input, "PARALLEL_PERFECT", 16) == 0 ) {
		    SetParallelPerfect();
		} else if ( strncmp(input, "PARALLEL", 8) == 0 ) {
		    ReadMachineNames(fpointer);
		}
		break;

	    case 'O':
		if ( strncmp(input, "OUTPUT", 6) == 0 ) {
		    charPtr = SkipSpacesTabs(&input[6]);
		    if ( whichGOP == -1 ) {
			strcpy(outputFileName, charPtr);
		    } else {
			sprintf(outputFileName, "%s.gop.%d",
				charPtr, whichGOP);
		    }

		    optionSeen[OPTION_OUTPUT] = TRUE;
		}
		break;

	    case 'R':
		if ( strncmp(input, "RANGE", 5) == 0 ) {
		    charPtr = SkipSpacesTabs(&input[5]);
		    SetSearchRange(atoi(charPtr));
		    optionSeen[OPTION_RANGE] = TRUE;
		} else if ( strncmp(input, "REFERENCE_FRAME", 15) == 0 ) {
		    charPtr = SkipSpacesTabs(&input[15]);
		    SetReferenceFrameType(charPtr);
		    optionSeen[OPTION_REF_FRAME] = TRUE;
		} else if ( strncmp(input, "RSH", 3) == 0 ) {
		    charPtr = SkipSpacesTabs(&input[3]);
		    SetRemoteShell(charPtr);
		}

		break;

	    case 'S':
		if ( strncmp(input, "SLICES_PER_FRAME", 16) == 0 ) {
		    charPtr = SkipSpacesTabs(&input[16]);
		    SetSlicesPerFrame(atoi(charPtr));
		    optionSeen[OPTION_SPF] = TRUE;
		} else if ( strncmp(input, "SLAVE_CONVERT", 13) == 0 ) {
		    charPtr = SkipSpacesTabs(&input[13]);
		    strcpy(slaveConversion, charPtr);
		    optionSeen[OPTION_SLAVE_CONVERT] = TRUE;
		}

		break;

	    case 'I':
		if ( strncmp(input, "IQSCALE", 7) == 0 ) {
		    charPtr = SkipSpacesTabs(&input[7]);
		    SetIQScale(atoi(charPtr));
		    optionSeen[OPTION_IQSCALE] = TRUE;
		} else if ( strncmp(input, "INPUT_DIR", 9) == 0 ) {
		    charPtr = SkipSpacesTabs(&input[9]);
		    strcpy(currentPath, charPtr);
		    optionSeen[OPTION_INPUT_DIR] = TRUE;
		} else if ( strncmp(input, "INPUT_CONVERT", 13) == 0 ) {
		    charPtr = SkipSpacesTabs(&input[13]);
		    strcpy(inputConversion, charPtr);
		    optionSeen[OPTION_INPUT_CONVERT] = TRUE;
		} else if ( strcmp(input, "INPUT") == 0 ) {   /* yes, strcmp */
		    if ( function == ENCODE_FRAMES ) {
			ReadInputFileNames(fpointer, "END_INPUT");
			optionSeen[OPTION_INPUT] = TRUE;
		    }
		} else if ( strncmp(input, "IO_SERVER_CONVERT", 17) == 0 ) {
		    charPtr = SkipSpacesTabs(&input[17]);
		    strcpy(ioConversion, charPtr);
		    optionSeen[OPTION_IO_CONVERT] = TRUE;
		}

		break;

	    case 'B':
		if ( strncmp(input, "BQSCALE", 7) == 0 ) {
		    charPtr = SkipSpacesTabs(&input[7]);
		    SetBQScale(atoi(charPtr));
		    optionSeen[OPTION_BQSCALE] = TRUE;
		} else if ( strncmp(input, "BASE_FILE_FORMAT", 16) == 0 ) {
		    charPtr = SkipSpacesTabs(&input[16]);
		    SetFileFormat(charPtr);
		    if ( strcmp(charPtr, "YUV") == 0 ) {
		        yuvUsed = TRUE;
		    }
		    optionSeen[OPTION_BASE_FORMAT] = TRUE;
		} else if ( strncmp(input, "BSEARCH_ALG", 11) == 0 ) {
		    charPtr = SkipSpacesTabs(&input[11]);
		    SetBSearchAlg(charPtr);
		    optionSeen[OPTION_BSEARCH_ALG] = TRUE;
		}
		break;

	    case 'Y':
		if ( strncmp(input, "YUV_SIZE", 8) == 0 ) {
		    charPtr = SkipSpacesTabs(&input[8]);
		    sscanf(charPtr, "%dx%d", &yuvWidth, &yuvHeight);
		    realWidth = yuvWidth;
		    realHeight = yuvHeight;
		    Fsize_Validate(&yuvWidth, &yuvHeight);
		    optionSeen[OPTION_YUV_SIZE] = TRUE;
		}
		break;
	    default:
		break;
	}
    }

    fclose(fpointer);

    for ( index = FIRST_OPTION; index <= LAST_OPTION; index++ ) {
	if ( ! optionSeen[index] ) {
	    fprintf(stdout, "ERROR:  Missing option '%s'\n", optionText[index]);
	    exit(1);
	}
    }

    /* error checking */
    if ( yuvUsed && (! optionSeen[OPTION_YUV_SIZE]) ) {
        fprintf(stderr, "ERROR:  YUV format used but YUV_SIZE not given\n");
	exit(1);
    }

    if ( optionSeen[OPTION_IO_CONVERT] != optionSeen[OPTION_SLAVE_CONVERT] ) {
        fprintf(stderr, "ERROR:  must have either both IO_SERVER_CONVERT and SLAVE_CONVERT\n");
        fprintf(stderr, "        or neither\n");
	exit(1);
    }

    SetIOConvert(optionSeen[OPTION_IO_CONVERT]);

    SetFCode();

    if ( psearchAlg == PSEARCH_TWOLEVEL )
	SetPixelSearch("HALF");

    return TRUE;
}


/*===========================================================================*
 *
 * GetNthInputFileName
 *
 *	finds the nth input file name
 *
 * RETURNS:	name is placed in already allocated fileName string
 *
 * SIDE EFFECTS:    none
 *
 *===========================================================================*/
void
GetNthInputFileName(fileName, n)
    char *fileName;
    int n;
{
    static int	lastN = 0, lastMapN = 0, lastSoFar = 0;
    int	    mapN;
    register int index;
    int	    soFar;
    int	    loop;
    int	    numPadding;
    char    numBuffer[33];

    /* assumes n is within bounds 0...numInputFiles-1 */

    if ( n >= lastN ) {
	soFar = lastSoFar;
	index = lastMapN;
    } else {
	soFar = 0;
	index = 0;
    }

    while ( soFar + inputFileEntries[index]->numFiles <= n ) {
	soFar +=  inputFileEntries[index]->numFiles;
	index++;
    }

    mapN = index;

    index = inputFileEntries[mapN]->startID +
	    inputFileEntries[mapN]->skip*(n-soFar);

    numPadding = inputFileEntries[mapN]->numPadding;

    if ( numPadding != -1 ) {
	sprintf(numBuffer, "%32d", index);
	for ( loop = 32-numPadding; loop < 32; loop++ ) {
	    if ( numBuffer[loop] != ' ' ) {
		break;
	    } else {
		numBuffer[loop] = '0';
	    }
	}

	sprintf(fileName, "%s%s%s",
		inputFileEntries[mapN]->left,
		&numBuffer[32-numPadding],
		inputFileEntries[mapN]->right);
    } else {
	sprintf(fileName, "%s%d%s",
		inputFileEntries[mapN]->left,
		index,
		inputFileEntries[mapN]->right);
    }

    lastN = n;
    lastMapN = mapN;
    lastSoFar = soFar;
}


/*=====================*
 * INTERNAL PROCEDURES *
 *=====================*/

/*===========================================================================*
 *
 * ReadMachineNames
 *
 *	read a list of machine names for parallel execution
 *
 * RETURNS:	nothing
 *
 * SIDE EFFECTS:    machine info updated
 *
 *===========================================================================*/
static void
ReadMachineNames(fpointer)
    FILE *fpointer;
{
    char    input[256];
    char    *charPtr;

    while ( (fgets(input, 256, fpointer) != NULL) &&
	    (strncmp(input, "END_PARALLEL", 12) != 0) ) {
	if ( input[0] == '#' ) {
	    continue;
	}

	if ( strncmp(input, "REMOTE", 6) == 0 ) {
	    charPtr = SkipSpacesTabs(&input[6]);
	    remote[numMachines] = TRUE;

	    sscanf(charPtr, "%s %s %s %s", machineName[numMachines],
		   userName[numMachines], executable[numMachines],
		   remoteParamFile[numMachines]);
	} else {
	    remote[numMachines] = FALSE;

	    sscanf(input, "%s %s %s", machineName[numMachines],
		   userName[numMachines], executable[numMachines]);
	}

	numMachines++;
    }
}


/*===========================================================================*
 *
 * ReadInputFileNames
 *
 *	read a list of input file names
 *
 * RETURNS:	nothing
 *
 * SIDE EFFECTS:    info stored for retrieval using GetNthInputFileName
 *
 *===========================================================================*/
static void
ReadInputFileNames(fpointer, endInput)
    FILE *fpointer;
    char *endInput;
{
    char    input[256];
    char left[256], right[256];
    char *globPtr, *charPtr;
    char leftNumText[256], rightNumText[256];
    char    skipNumText[256];
    int	leftNum, rightNum;
    int	    skipNum;
    boolean padding;
    int	    numPadding = 0;
    int	    length;

    inputFileEntries = (InputFileEntry **) malloc(INPUT_ENTRY_BLOCK_SIZE*
						  sizeof(InputFileEntry *));
    maxInputFileEntries = INPUT_ENTRY_BLOCK_SIZE;

    length = strlen(endInput);

    /* read input files up until endInput */
    while ( (fgets(input, 256, fpointer) != NULL) &&
	    (strncmp(input, endInput, length) != 0) ) {
	if ( input[0] == '#' ) {
	    continue;
	}

	input[strlen(input)-1] = '\0';	/* get rid of newline */

	if ( numInputFileEntries == maxInputFileEntries ) {    /* more space! */
	    maxInputFileEntries += INPUT_ENTRY_BLOCK_SIZE;
	    inputFileEntries = realloc(inputFileEntries,
				       maxInputFileEntries*
				       sizeof(InputFileEntry *));
	}

	inputFileEntries[numInputFileEntries] = (InputFileEntry *)
						malloc(sizeof(InputFileEntry));

	if ( input[strlen(input)-1] == ']' ) {
	    inputFileEntries[numInputFileEntries]->glob = TRUE;

	    /* star expand */

	    globPtr = input;
	    charPtr = left;
	    /* copy left of '*' */
	    while ( (*globPtr != '\0') && (*globPtr != '*') ) {
		*charPtr = *globPtr;
		charPtr++;
		globPtr++;
	    }
	    *charPtr = '\0';

	    globPtr++;
	    charPtr = right;
	    /* copy right of '*' */
	    while ( (*globPtr != '\0') && (*globPtr != ' ') &&
		    (*globPtr != '\t') ) {
		*charPtr = *globPtr;
		charPtr++;
		globPtr++;
	    }
	    *charPtr = '\0';

	    globPtr = SkipSpacesTabs(globPtr);

	    if ( *globPtr != '[' ) {
		fprintf(stderr, "ERROR:  Invalid input file expansion expression (no '[')\n");
		exit(1);
	    }

	    globPtr++;
	    charPtr = leftNumText;
	    /* copy left number */
	    while ( isdigit(*globPtr) ) {
		*charPtr = *globPtr;
		charPtr++;
		globPtr++;
	    }
	    *charPtr = '\0';

	    if ( *globPtr != '-' ) {
		fprintf(stderr, "ERROR:  Invalid input file expansion expression (no '-')\n");
		exit(1);
	    }

	    globPtr++;
	    charPtr = rightNumText;
	    /* copy right number */
	    while ( isdigit(*globPtr) ) {
		*charPtr = *globPtr;
		charPtr++;
		globPtr++;
	    }
	    *charPtr = '\0';


	    if ( *globPtr != ']' ) {
		if ( *globPtr != '+' ) {
		    fprintf(stderr, "ERROR:  Invalid input file expansion expression (no ']')\n");
		    exit(1);
		}

		globPtr++;
		charPtr = skipNumText;
		/* copy skip number */
		while ( isdigit(*globPtr) ) {
		    *charPtr = *globPtr;
		    charPtr++;
		    globPtr++;
		}
		*charPtr = '\0';

		if ( *globPtr != ']' ) {
		    fprintf(stderr, "ERROR:  Invalid input file expansion expression (no ']')\n");
		    exit(1);
		}

		skipNum = atoi(skipNumText);
	    } else {
		skipNum = 1;
	    }

	    leftNum = atoi(leftNumText);
	    rightNum = atoi(rightNumText);

	    if ( (leftNumText[0] == '0') && (leftNumText[1] != '\0') ) {
		padding = TRUE;
		numPadding = strlen(leftNumText);
	    } else {
		padding = FALSE;
	    }

	    inputFileEntries[numInputFileEntries]->startID = leftNum;
	    inputFileEntries[numInputFileEntries]->endID = rightNum;
	    inputFileEntries[numInputFileEntries]->skip = skipNum;
	    inputFileEntries[numInputFileEntries]->numFiles = (rightNum-leftNum+1)/skipNum;
	    strcpy(inputFileEntries[numInputFileEntries]->left, left);
	    strcpy(inputFileEntries[numInputFileEntries]->right, right);
	    if ( padding ) {
		inputFileEntries[numInputFileEntries]->numPadding = numPadding;
	    } else {
		inputFileEntries[numInputFileEntries]->numPadding = -1;
	    }
	} else {
	    strcpy(inputFileEntries[numInputFileEntries]->left, input);
	    inputFileEntries[numInputFileEntries]->glob = FALSE;
	    inputFileEntries[numInputFileEntries]->numFiles = 1;
	}

	numInputFiles += inputFileEntries[numInputFileEntries]->numFiles;
	numInputFileEntries++;
    }
}


/*===========================================================================*
 *
 * SkipSpacesTabs
 *
 *	skip all spaces and tabs
 *
 * RETURNS:	point to next character not a space or tab
 *
 * SIDE EFFECTS:    none
 *
 *===========================================================================*/
static char *
SkipSpacesTabs(start)
    char *start;
{
    while ( (*start == ' ') || (*start == '\t') ) {
	start++;
    }

    return start;
}


