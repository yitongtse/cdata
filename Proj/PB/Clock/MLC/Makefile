#############################################################################
# Copyright (c) 2001 by Cisco Systems, Inc.
# All rights reserved.
#############################################################################
#    File name : Makefile
#
#    Makefile for app/clk directory
#
#    Date      Who     Modification
#    --------  ---     ------------------------------------------------------
#    10-12-01  YWL     Created.
#
#############################################################################


# -------------------------------------------------------------------------- #
# Specified the root directory of the code tree
#
DIR_ROOT = ../..


# -------------------------------------------------------------------------- #
# Include the file contains common setup
#
MK_INC = ${DIR_ROOT}/proj/develop/Mk.inc
include ${MK_INC}


# -------------------------------------------------------------------------- #
# .H Include file used
#
H_FILES = \
	${DIR_COMMON}/dma.h \
	${DIR_COMMON}/int567.h \
	${DIR_COMMON}/mipsdsp.h \
	${DIR_COMMON}/mlcdef.h \
	${DIR_COMMON}/msg.h \
	${DIR_COMMON}/msgOutMdul.h \
	${DIR_COMMON}/regs.h \
	${DIR_INPUT}/cbinfo.h \
	clock.h \
	interpol.h \
	track.h \
	trkloop.h


# -------------------------------------------------------------------------- #
# Object file from .C source files
#
C_OBJS = \
	clock.o60 \
	interpol.o60 \
	track.o60 \
	trkloop.o60


# -------------------------------------------------------------------------- #
# Object file from .s62 source files
#
A_OBJS =


# -------------------------------------------------------------------------- #
# Archive files used
#
LIBRARY = clkMdul.a


# -------------------------------------------------------------------------- #
# Target
#
all : ${LIBRARY}

${A_OBJS} : $${@:.o60=.s62} ${H_FILES}

${C_OBJS} : $${@:.o60=.c} ${H_FILES}

${C_OBJS:.o60=.asm} : $${@:.asm=.c} ${H_FILES}

${C_OBJS:.o60=.lst} : $${@:.lst=.c} ${H_FILES}

${LIBRARY} : $@ ${C_OBJS} ${A_OBJS}
	${AR} $@ $?

clean:
	rm -f ${LIBRARY} ${C_OBJS} ${A_OBJS} ${C_OBJS:.o60=.lst} ${C_OBJS:.o60=.asm}
