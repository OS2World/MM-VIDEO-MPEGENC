--------------snibbel-snibbel-the-makefile-for-sysv-------------
# 
# Copyright (c) 1993 The Regents of the University of California.
# All rights reserved.
# 
# Permission to use, copy, modify, and distribute this software and its
# documentation for any purpose, without fee, and without written agreement is
# hereby granted, provided that the above copyright notice and the following
# two paragraphs appear in all copies of this software.
# 
# IN NO EVENT SHALL THE UNIVERSITY OF CALIFORNIA BE LIABLE TO ANY PARTY FOR
# DIRECT, INDIRECT, SPECIAL, INCIDENTAL, OR CONSEQUENTIAL DAMAGES ARISING OUT
# OF THE USE OF THIS SOFTWARE AND ITS DOCUMENTATION, EVEN IF THE UNIVERSITY OF
# CALIFORNIA HAS BEEN ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
# 
# THE UNIVERSITY OF CALIFORNIA SPECIFICALLY DISCLAIMS ANY WARRANTIES,
# INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY
# AND FITNESS FOR A PARTICULAR PURPOSE.  THE SOFTWARE PROVIDED HEREUNDER IS
# ON AN "AS IS" BASIS, AND THE UNIVERSITY OF CALIFORNIA HAS NO OBLIGATION TO
# PROVIDE MAINTENANCE, SUPPORT, UPDATES, ENHANCEMENTS, OR MODIFICATIONS.
# 

#  
#  $Header: /n/picasso/users/dwallach/vid2/mpeg_encode/RCS/Makefile,v 1.3 1993/01/18 10:20:02 dwallach Exp dwallach $
# 

#########################################################################
#
# standard

ST_DIR	 = 	/usr/X386

ST_FLAGS =	-I$(ST_DIR)/include \
		-L$(ST_DIR)/lib
ST_LIBS	 =	

CM_DIR   =	/n/picasso/project/mm/cms

#########################################################################
#
# tcl

TCL_FLAGS =	-I$(ST_DIR)/include/tcl
TCL_LIBS  = 	$(ST_DIR)/lib/libtcl.a

#########################################################################
#
# tk

TK_FLAGS =  \
		-DTK_LIBRARY=\"$(ST_DIR)/lib/tk\" \
		-I$(ST_DIR)/include/tk
TKA_LIBS  = 	$(ST_DIR)/lib/libtk.a 

#########################################################################
#
# x

X_DIR   = #	/usr/sww/X11

X_FLAGS = #	-I$(X_DIR)/include \
#		-L$(X_DIR)/lib

#X_LIBS  =	-lXvid -lXext -lX11_s
X_LIBS  =	-lXext -lX11 -lpt -linet
#                        ^^^^^^^^^^^----- they are important

SH_CFLAGS  =    $(GCFLAGS)\
		$(TCL_FLAGS) $(TK_FLAGS)\
		$(ST_FLAGS) $(X_FLAGS)

SH_LIBS	= 	$(TKA_LIBS) $(TCL_LIBS)\
		$(ST_LIBS) $(X_LIBS)\
		-ll -lm -lc_s -ltcl -lusleep
#                            ^^^^^^^----- well, thats my solution

##############################################################################
#
# Configurable items -- you want to make sure CC points at an ANSI
# compliant C compiler, and that INCLUDE and LIBDIRS specify what's
# necessary for you to include and link against the PBMPLUS libraries.
#
#

# PROFLAG = -pg
PROFLAG = 

PBMPLUS = ./pbmplus
INCLUDE = -I$(PBMPLUS) 
# LIBDIRS = -L$(PBMPLUS) -L/usr/sww/lib
LIBDIRS = -L$(PBMPLUS) -L/usr/sww/lib
LIBS = -lm
#CFLAGS = $(INCLUDE) -g -DHEINOUS_DEBUG_MODE -DFORCE_BIG_ENDIAN
# CFLAGS = $(INCLUDE)  -O -g
CFLAGS = $(INCLUDE) $(TCL_FLAGS) $(TK_FLAGS) -Iheaders -O -g -W -Wreturn-type -Wunused -Wmissing-prototypes $(PROFLAG) -DSYSV -D_POSIX_SOURCE
#                            ^^^^^^^^^^^^^^^^^^^^^^---- and I needed theme
CC = gcc
#CC = cc

PURIFY =

#
# Perl is necessary if you want to modify the Huffman RLE encoding table.
# Otherwise, you can use huff.c-dist and huff.h-dist
#
PERL = perl


# if you have an ANSI C Compiler, use the following line, otherwise
# uncomment the longer .c.o rule, also

.c.o:
	$(CC) $(CFLAGS) -c $<
ANSI2KNR=

#.c.o:
#	./ansi2knr $< tmpstupid.c
#	$(CC) $(CFLAGS) -c tmpstupid.c
#	mv tmpstupid.o $@
#	rm tmpstupid.c
#
#ANSI2KNR=ansi2knr

#
# End of configurable options.  Just type make and have fun.
##############################################################################

all: encode_sh blockppmtoyuv playone playskip ppmtoyuv xvidtoyuv xvideotojpeg

encode_sh: $(ANSI2KNR) shell.o file.o
	$(PURIFY) $(CC) $(SH_CFLAGS) -o $@ shell.o file.o $(LIBDIRS) $(SH_LIBS) $(LIBS)

blockppmtoyuv: blockppmtoyuv.c
	$(CC) -o blockppmtoyuv blockppmtoyuv.c

ppmtoyuv: ppmtoyuv.c
	$(CC) -o ppmtoyuv ppmtoyuv.c

playone: playone.c
	$(CC) -o playone playone.c -lX11 -lXvid -I/n/picasso/project/mm/xvideo/include

playskip: playskip.c
	$(CC) -o playskip playskip.c -lX11 -lXvid -I/n/picasso/project/mm/xvideo/include

xvideotojpeg: xvideotojpeg.c
	$(CC) -o xvideotojpeg xvideotojpeg.c -lX11 -lXvid -I/n/picasso/project/mm/xvideo/include

xvidtoyuv: xvidtoyuv.c
	$(CC) -o xvidtoyuv xvidtoyuv.c -lX11 -lXvid -I/n/picasso/project/mm/xvideo/include
	
ansi2knr: ansi2knr.c
	$(CC) -o $@ ansi2knr.c

wc:;		wc -l *.[ch] *.pl *.table
clean:;		rm -f *.o core *~ gmon.out
tags:;		ctags -t *.[ch]

