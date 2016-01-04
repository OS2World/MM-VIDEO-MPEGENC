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

/**********************************************************************/
/*
 *  The variable below is the default prompt, stored in the "prompt"
 *  variable
 */

#define PROMPT  "grabsh: "
/**********************************************************************/

#include "tk.h"

#include "all.h"

#include <sys/time.h>
#include <sys/param.h>
#include <string.h>


/* from frames.h */
#define USE_MAD	    0
#define USE_MSE	    1

#define USE_HALF    0
#define USE_FULL    1

/**********************************************************************/

/*
 * Command used to initialize wish:
 */

char initCmd[] = "source $tk_library/wish.tcl";

Tk_Window w;			/* NULL means window has been deleted. */
Tk_TimerToken timeToken = 0;
int idleHandler = 0;
Tcl_Interp *interp;
int x, y;
Tcl_CmdBuf buffer;
int tty;
extern int Tk_SquareCmd _ANSI_ARGS_((ClientData clientData,
	Tcl_Interp *interp, int argc, char **argv));
extern int ListDirectory _ANSI_ARGS_((ClientData nulldata, Tcl_Interp *interp,
			 int argc, char **argv));
extern int ChangeDirectory _ANSI_ARGS_((ClientData nulldata, Tcl_Interp *interp,
			 int argc, char **argv));
extern int SetBrowseGlob _ANSI_ARGS_((ClientData nulldata, Tcl_Interp *interp,
		   int argc, char **argv));


/*
 * ENCODING PARAMETERS
 *
 */
int numInputFiles = 0;
char *inputFiles[1024];	    /* should change */
char currentPath[MAXPATHLEN];

static char	outputFileName[256];
static char	inputConversion[1024];
static char	pixelSearch[1024];
static char	pattern[1024];
static int IQScale, PQScale, BQScale;
static int searchRange;
static char	Psearch[1024];
static char	Bsearch[1024];
static char	conversion[1024];
static char	format[1024];
static int	gopSize;
static int	spf;
static int	height, width;


int SetOutputFileName (nulldata, interp, argc, argv)
ClientData nulldata;
Tcl_Interp *interp;
int argc;
char **argv;
{
    if (argc == 2 )
    {
	strcpy(outputFileName, argv[1]);

	numInputFiles = 0;

	return TCL_OK;
    }

	Tcl_AppendResult (interp, 
            "wrong args: should be \"", argv[0]," filename\"", (char *) NULL);
	return TCL_ERROR;
}


int SetConversion (nulldata, interp, argc, argv)
ClientData nulldata;
Tcl_Interp *interp;
int argc;
char **argv;
{
    if (argc == 2 )
    {
	strcpy(conversion, argv[1]);
	return TCL_OK;
    }

	Tcl_AppendResult (interp, 
            "wrong args: should be \"", argv[0]," filename\"", (char *) NULL);
	return TCL_ERROR;
}


int SetGOPSize (nulldata, interp, argc, argv)
ClientData nulldata;
Tcl_Interp *interp;
int argc;
char **argv;
{
    if (argc == 2 )
    {
	gopSize = atoi(argv[1]);
	return TCL_OK;
    }

	Tcl_AppendResult (interp, 
            "wrong args: should be \"", argv[0]," filename\"", (char *) NULL);
	return TCL_ERROR;
}


int SetSPF (nulldata, interp, argc, argv)
ClientData nulldata;
Tcl_Interp *interp;
int argc;
char **argv;
{
    if (argc == 2 )
    {
	spf = atoi(argv[1]);
	return TCL_OK;
    }

	Tcl_AppendResult (interp, 
            "wrong args: should be \"", argv[0]," filename\"", (char *) NULL);
	return TCL_ERROR;
}


int SetDimensions (nulldata, interp, argc, argv)
ClientData nulldata;
Tcl_Interp *interp;
int argc;
char **argv;
{
    if (argc == 3 )
    {
	width = atoi(argv[1]);
	height = atoi(argv[2]);
	return TCL_OK;
    }

	Tcl_AppendResult (interp, 
            "wrong args: should be \"", argv[0]," filename\"", (char *) NULL);
	return TCL_ERROR;
}


int SetPatternString (nulldata, interp, argc, argv)
ClientData nulldata;
Tcl_Interp *interp;
int argc;
char **argv;
{
    if (argc == 2 )
    {
	strcpy(pattern, argv[1]);

	return TCL_OK;
    }

	Tcl_AppendResult (interp, 
            "wrong args: should be \"", argv[0]," filename\"", (char *) NULL);
	return TCL_ERROR;
}


int EncodeCmd (nulldata, interp, argc, argv)
ClientData nulldata;
Tcl_Interp *interp;
int argc;
char **argv;
{
    if (argc == 1 )
    {
	FILE *fpointer;
	register int index;

	fpointer = fopen("tcl.param", "w");
	    fprintf(fpointer, "# mpeg_encode parameter file created by tcl interface\n");
	    fprintf(fpointer, "PATTERN\t%s\n", pattern);
	    fprintf(fpointer, "OUTPUT\t%s\n", outputFileName);
	    fprintf(fpointer, "BASE_FILE_FORMAT\t%s\n", format);
	    fprintf(fpointer, "GOP_SIZE\t%d\n", gopSize);
	    fprintf(fpointer, "SLICES_PER_FRAME\t%d\n", spf);
	    fprintf(fpointer, "PIXEL\t%s\n", pixelSearch);
	    fprintf(fpointer, "RANGE\t%d\n", searchRange);
	    fprintf(fpointer, "PSEARCH_ALG\t%s\n", Psearch);
	    fprintf(fpointer, "BSEARCH_ALG\t%s\n", Bsearch);
	    fprintf(fpointer, "IQSCALE\t%d\n", IQScale);
	    fprintf(fpointer, "PQSCALE\t%d\n", PQScale);
	    fprintf(fpointer, "BQSCALE\t%d\n", BQScale);
	    fprintf(fpointer, "YUV_SIZE\t%dx%d\n", width, height);
	    fprintf(fpointer, "INPUT_CONVERT\t%s\n", conversion);
	    fprintf(fpointer, "INPUT_DIR\t%s\n", currentPath);
	    fprintf(fpointer, "INPUT\n");
	    for ( index = 0; index < numInputFiles; index++ )
		fprintf(fpointer, "%s\n", inputFiles[index]);
	    fprintf(fpointer, "END_INPUT\n");
	    fprintf(fpointer, "REFERENCE_FRAME ORIGINAL\n");
	fclose(fpointer);

	system("mpeg_encode tcl.param");

	return TCL_OK;
    }

	Tcl_AppendResult (interp, 
            "wrong args: should be \"", argv[0]," num\"", (char *) NULL);
	return TCL_ERROR;
}


int SetPixelSearchCmd (nulldata, interp, argc, argv)
ClientData nulldata;
Tcl_Interp *interp;
int argc;
char **argv;
{
    if (argc == 2 )
    {
	strcpy(pixelSearch, argv[1]);

	return TCL_OK;
    }

	Tcl_AppendResult (interp, 
            "wrong args: should be \"", argv[0]," type\"", (char *) NULL);
	return TCL_ERROR;
}



int SetSearchRangeCmd (nulldata, interp, argc, argv)
ClientData nulldata;
Tcl_Interp *interp;
int argc;
char **argv;
{
    if (argc == 2 )
    {
	searchRange = atoi(argv[1]);

	return TCL_OK;
    }

	Tcl_AppendResult (interp, 
            "wrong args: should be \"", argv[0]," num\"", (char *) NULL);
	return TCL_ERROR;
}


int SetPSearchCmd (nulldata, interp, argc, argv)
ClientData nulldata;
Tcl_Interp *interp;
int argc;
char **argv;
{
    if (argc == 2 )
    {
	strcpy(Psearch, argv[1]);

	return TCL_OK;
    }

	Tcl_AppendResult (interp, 
            "wrong args: should be \"", argv[0]," num\"", (char *) NULL);
	return TCL_ERROR;
}


int SetFormatCmd (nulldata, interp, argc, argv)
ClientData nulldata;
Tcl_Interp *interp;
int argc;
char **argv;
{
    if (argc == 2 )
    {
	strcpy(format, argv[1]);

	return TCL_OK;
    }

	Tcl_AppendResult (interp, 
            "wrong args: should be \"", argv[0]," num\"", (char *) NULL);
	return TCL_ERROR;
}


int SetBSearchCmd (nulldata, interp, argc, argv)
ClientData nulldata;
Tcl_Interp *interp;
int argc;
char **argv;
{
    if (argc == 2 )
    {
	strcpy(Bsearch, argv[1]);

	return TCL_OK;
    }

	Tcl_AppendResult (interp, 
            "wrong args: should be \"", argv[0]," num\"", (char *) NULL);
	return TCL_ERROR;
}


int SetQScaleCmd (nulldata, interp, argc, argv)
ClientData nulldata;
Tcl_Interp *interp;
int argc;
char **argv;
{
    if (argc == 4 )
    {
	IQScale = atoi(argv[1]);
	PQScale = atoi(argv[2]);
	BQScale = atoi(argv[3]);

	return TCL_OK;
    }

	Tcl_AppendResult (interp, 
            "wrong args: should be \"", argv[0]," 3nums\"", (char *) NULL);
	return TCL_ERROR;
}


int NextInputFile (nulldata, interp, argc, argv)
ClientData nulldata;
Tcl_Interp *interp;
int argc;
char **argv;
{
    if (argc == 2 )
    {
	inputFiles[numInputFiles] = (char *) malloc(256*sizeof(char));

	strcpy(inputFiles[numInputFiles], argv[1]);
	numInputFiles++;

	return TCL_OK;
    }

	Tcl_AppendResult (interp, 
            "wrong args: should be \"", argv[0]," num\"", (char *) NULL);
	return TCL_ERROR;
}


int PlayOutput (nulldata, interp, argc, argv)
ClientData nulldata;
Tcl_Interp *interp;
int argc;
char **argv;
{
    char command[256];

    /* ignore argc, argv */
    sprintf(command, "mpeg_play %s", outputFileName);
    fprintf(stdout, "DOING:  %s\n", command);

    system(command);

    return TCL_OK;
}


/*
 * Information for testing out command-line options:
 */

int synchronize = 0;
char *fileName = NULL;
char *name = NULL;
char *display = NULL;
char *geometry = NULL;

Tk_ArgvInfo argTable[] = {
    {"-file", TK_ARGV_STRING, (char *) NULL, (char *) &fileName,
	"File from which to read commands"},
    {"-geometry", TK_ARGV_STRING, (char *) NULL, (char *) &geometry,
	"Initial geometry for window"},
    {"-display", TK_ARGV_STRING, (char *) NULL, (char *) &display,
	"Display to use"},
    {"-name", TK_ARGV_STRING, (char *) NULL, (char *) &name,
	"Name to use for application"},
    {"-sync", TK_ARGV_CONSTANT, (char *) 1, (char *) &synchronize,
	"Use synchronous mode for display server"},
    {(char *) NULL, TK_ARGV_END, (char *) NULL, (char *) NULL,
	(char *) NULL}
};

void StdinProc(clientData, mask)
    ClientData clientData;		/* Not used. */
    int mask;
{
    char line[200];
    static int gotPartial = 0;
    char *cmd;
    int result;

    if (mask & TK_READABLE) {
	if (fgets(line, 200, stdin) == NULL) {
	    if (!gotPartial) {
		if (tty) {
		    Tcl_Eval(interp, "destroy .", 0, (char **) NULL);
		    exit(0);
		} else {
		    Tk_DeleteFileHandler(0);
		}
		return;
	    } else {
		line[0] = 0;
	    }
	}
	cmd = Tcl_AssembleCmd(buffer, line);
	if (cmd == NULL) {
	    gotPartial = 1;
	    return;
	}
	gotPartial = 0;
	result = Tcl_RecordAndEval(interp, cmd, 0);
	if (*interp->result != 0) {
	    if ((result != TCL_OK) || (tty)) {
		printf("%s\n", interp->result);
	    }
	}
	if (tty) {
	    printf(Tcl_GetVar (interp, "prompt", TCL_GLOBAL_ONLY));
	    fflush(stdout);
	}
    }
}

static void StructureProc(clientData, eventPtr)
    ClientData clientData;	/* Information about window. */
    XEvent *eventPtr;		/* Information about event. */
{
    if (eventPtr->type == DestroyNotify) {
	w = NULL;
    }
}

/*
 * Procedure to map initial window.  This is invoked as a do-when-idle
 * handler.  Wait for all other when-idle handlers to be processed
 * before mapping the window, so that the window's correct geometry
 * has been determined.
 */

static void DelayedMap(clientData)
    ClientData clientData;	/* Not used. */
{

    while (Tk_DoOneEvent(1) != 0) {
	/* Empty loop body. */
    }
    if (w == NULL) {
	return;
    }
    Tk_MapWindow(w);
}


/************************************************************************/

int main(argc, argv)
    int argc;
    char **argv;
{
    char *args, *p, *msg;
    char buf[20];
    int result;
    Tk_3DBorder border;

    strcpy(inputConversion, "*");

    ResetPath();	/* reset directory for browsing */

    interp = Tcl_CreateInterp();

#ifdef TCL_MEM_DEBUG
    Tcl_InitMemory(interp);
#endif

    if (Tk_ParseArgv(interp, (Tk_Window) NULL, &argc, argv, argTable, 0)
	    != TCL_OK) {
	fprintf(stderr, "%s\n", interp->result);
	exit(1);
    }
    if (name == NULL) {
	if (fileName != NULL) {
	    p = fileName;
	} else {
	    p = argv[0];
	}
	name = strrchr(p, '/');
	if (name != NULL) {
	    name++;
	} else {
	    name = p;
	}
    }

    w = Tk_CreateMainWindow(interp, display, name);
    if (w == NULL) {
	fprintf(stderr, "%s\n", interp->result);
	exit(1);
    }
    Tk_SetClass(w, "Tk");
    Tk_CreateEventHandler(w, StructureNotifyMask, StructureProc,
	    (ClientData) NULL);
    Tk_DoWhenIdle(DelayedMap, (ClientData) NULL);
    tty = isatty(0);

    args = Tcl_Merge(argc-1, argv+1);
    Tcl_SetVar(interp, "argv", args, TCL_GLOBAL_ONLY);
    ckfree(args);
    sprintf(buf, "%d", argc-1);
    Tcl_SetVar(interp, "argc", buf, TCL_GLOBAL_ONLY);
    Tcl_SetVar (interp, "prompt", PROMPT, TCL_GLOBAL_ONLY);

    if (synchronize) {
	XSynchronize(Tk_Display(w), True);
    }
    Tk_GeometryRequest(w, 200, 200);
    border = Tk_Get3DBorder(interp, w, None, "#4eee94");
    if (border == NULL) {
	Tcl_SetResult(interp, (char *) NULL, TCL_STATIC);
	Tk_SetWindowBackground(w, WhitePixelOfScreen(Tk_Screen(w)));
    } else {
	Tk_SetBackgroundFromBorder(w, border);
    }
    XSetForeground(Tk_Display(w), DefaultGCOfScreen(Tk_Screen(w)),
	    BlackPixelOfScreen(Tk_Screen(w)));

    Tcl_CreateCommand (interp, "SetOutputFileName", SetOutputFileName, (ClientData) 0,
	(void (*) ()) NULL);

    Tcl_CreateCommand (interp, "SetConversion", SetConversion, (ClientData) 0,
	(void (*) ()) NULL);

    Tcl_CreateCommand (interp, "SetBrowseGlob", SetBrowseGlob, (ClientData) 0,
	(void (*) ()) NULL);

    Tcl_CreateCommand (interp, "SetFormat", SetFormatCmd, (ClientData) 0,
	(void (*) ()) NULL);

    Tcl_CreateCommand (interp, "SetPSearch", SetPSearchCmd, (ClientData) 0,
	(void (*) ()) NULL);

    Tcl_CreateCommand (interp, "SetGOPSize", SetGOPSize, (ClientData) 0,
	(void (*) ()) NULL);

    Tcl_CreateCommand (interp, "SetSPF", SetSPF, (ClientData) 0,
	(void (*) ()) NULL);

    Tcl_CreateCommand (interp, "SetDimensions", SetDimensions, (ClientData) 0,
	(void (*) ()) NULL);

    Tcl_CreateCommand (interp, "SetBSearch", SetBSearchCmd, (ClientData) 0,
	(void (*) ()) NULL);

    Tcl_CreateCommand (interp, "SetPatternString", SetPatternString, (ClientData) 0,
	(void (*) ()) NULL);

    Tcl_CreateCommand (interp, "SetSearchRange", SetSearchRangeCmd, (ClientData) 0,
	(void (*) ()) NULL);

    Tcl_CreateCommand (interp, "SetQScale", SetQScaleCmd, (ClientData) 0,
	(void (*) ()) NULL);

    Tcl_CreateCommand (interp, "encode", EncodeCmd, (ClientData) 0,
	(void (*) ()) NULL);

    Tcl_CreateCommand (interp, "SetPixelSearch", SetPixelSearchCmd,
		       (ClientData) 0, (void (*) ()) NULL);

    Tcl_CreateCommand (interp, "PlayOutput", PlayOutput, (ClientData) 0,
	(void (*) ()) NULL);

    Tcl_CreateCommand (interp, "NextInputFile", NextInputFile, (ClientData) 0,
	(void (*) ()) NULL);

    Tcl_CreateCommand (interp, "ListDirectory", ListDirectory, (ClientData) 0,
	(void (*) ()) NULL);

    Tcl_CreateCommand (interp, "ChangeDirectory", ChangeDirectory, (ClientData) 0,
	(void (*) ()) NULL);

    if (geometry != NULL) {
	Tcl_SetVar(interp, "geometry", geometry, TCL_GLOBAL_ONLY);
    }

    result = Tcl_Eval(interp, initCmd, 0, (char **) NULL);
    if (result != TCL_OK) {
	goto error;
    }

    if (fileName != NULL) {
	result = Tcl_VarEval(interp, "source ", fileName, (char *) NULL);
	if (result != TCL_OK) {
	    goto error;
	}
	tty = 0;
    } else {
	tty = isatty(0);
	Tk_CreateFileHandler(0, TK_READABLE, StdinProc, (ClientData) 0);
	if (tty) {
	    printf(Tcl_GetVar (interp, "prompt", TCL_GLOBAL_ONLY));
	}
    }

    fflush(stdout);
    buffer = Tcl_CreateCmdBuf();
    (void) Tcl_Eval(interp, "update", 0, (char **) NULL);

    Tk_MainLoop();

    Tcl_DeleteInterp(interp);
    Tcl_DeleteCmdBuf(buffer);
    exit(0);

error:
    msg = Tcl_GetVar(interp, "errorInfo", TCL_GLOBAL_ONLY);
    if (msg == NULL) {
	msg = interp->result;
    }
    fprintf(stderr, "%s\n", msg);
    Tcl_Eval(interp, "destroy .", 0, (char **) NULL);
    exit(1);
    return 0;
}
