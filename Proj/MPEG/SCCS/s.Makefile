h29796
s 00043/00000/00135
d D 1.13 03/03/24 11:22:59 ytse 13 12
c Backup
e
s 00018/00003/00117
d D 1.12 02/05/24 12:27:22 ytse 12 11
c Update
e
s 00007/00004/00113
d D 1.11 02/03/21 13:28:41 ytse 11 10
c Backup for NEW2
e
s 00004/00002/00113
d D 1.10 02/03/07 16:40:44 ytse 10 9
c Update
e
s 00001/00001/00114
d D 1.9 02/03/04 15:38:24 ytse 9 8
c Update
e
s 00007/00002/00108
d D 1.8 02/02/28 13:41:57 ytse 8 7
c update
e
s 00007/00000/00103
d D 1.7 02/02/25 17:53:45 ytse 7 6
c Update
e
s 00004/00001/00099
d D 1.6 02/01/21 11:02:32 ytse 6 5
c Update
e
s 00003/00003/00097
d D 1.5 00/08/21 11:23:54 ytse 5 4
c Added support of Windows
e
s 00002/00002/00098
d D 1.4 00/02/14 17:09:39 ytse 4 3
c Turned on shared memory usage
e
s 00000/00001/00100
d D 1.3 00/01/14 14:58:22 ytse 3 2
c Update
e
s 00002/00002/00099
d D 1.2 99/11/01 16:15:34 yitong 2 1
c Backup
e
s 00101/00000/00000
d D 1.1 99/10/29 15:58:07 yitong 1 0
c date and time created 99/10/29 15:58:07 by yitong
e
u
U
f e 0
t
T
I 1
.KEEP_STATE:
.SUFFIXES: .C .c .o


D 2
CFLAGS = -g -Wimplicit
#CFLAGS = -g -O3 -Wimplicit
E 2
I 2
D 5
#CFLAGS = -g -Wimplicit
CFLAGS = -g -O3 -Wimplicit
E 5
I 5
D 12
#CFLAGS = -g -Wimplicit -DUNIX_ENV
D 6
CFLAGS = -g -O3 -Wimplicit -DUNIX_ENV
E 6
I 6
CFLAGS = -g -Wimplicit -DUNIX_ENV
E 12
I 12
CFLAGS = -g -Wimplicit -DUNIX
E 12
E 6
E 5
E 2

MPEG_FLAGS =		\
	-DIDCT_CLIP=0	\
	-DIMPRECISE=1


#  Suffix rules
#
.c.o:
	gcc -c $(CFLAGS) $<

.C.o:
	g++ -c $(CFLAGS) $<

.c:
	gcc $(CFLAGS) $(MPEG_FLAGS) -o $(@:%.o=%) $<


UTIL_OBJS = 		\
	block.o		\
	imageio.o	\
	infile.o	\
	outfile.o	\
	param.o		\
	reader.o	\
	stat.o		\
	vlc.o		\
	vld.o		\
	writer.o		

D 11
#	editor.o	\

E 11
MPEG_OBJS =		\
	decode.o	\
	encode.o	\
	fdct.o		\
	gethdr.o	\
	getmb.o		\
	idct.o		\
	info.o		\
	me.o		\
	mpeg2.o		\
	pes.o		\
	puthdr.o	\
	putmb.o		\
	reconst.o	\
	tp.o		\
	vlctbl.o

UTIL_PROGS =		\
	tsfilter	\
	tsmaker		\
	tsview

MPEG_PROGS =		\
D 11
	mpeg2enc	\
E 11
D 8
	mpeg2dec
E 8
I 8
	mpeg2dec	\
I 11
	mpeg2enc	\
	mpeg2cmp	\
E 11
	mpeg2rq
E 8


# Specific builds
#
all:	$(UTIL_PROGS) $(MPEG_PROGS)

util.a:	$(UTIL_OBJS)
	ar -ru util.a $(UTIL_OBJS)

mpeg.a: $(MPEG_OBJS)
	ar -ru mpeg.a $(MPEG_OBJS)


tsfilter: tsfilter.c util.a mpeg.a
	gcc $(CFLAGS) tsfilter.c -o tsfilter mpeg.a util.a

tsview: tsview.c util.a mpeg.a
	gcc $(CFLAGS) tsview.c -o tsview mpeg.a util.a

tsmaker: tsmaker.c util.a mpeg.a
	gcc $(CFLAGS) tsmaker.c -o tsmaker mpeg.a util.a


display.o: display.c
D 4
	gcc -g -I/usr/openwin/share/include -c display.c
E 4
I 4
D 5
	gcc -g -I/usr/openwin/share/include -DSH_MEM -c display.c
E 5
I 5
	gcc $(CFLAGS) -I/usr/openwin/share/include -DSH_MEM -c display.c
E 5
E 4

mpeg2dec: mpeg2dec.c display.o util.a mpeg.a
D 4
	gcc $(CFLAGS) $(MPEG_FLAGS) -L/usr/lib mpeg2dec.c -o mpeg2dec \
E 4
I 4
D 10
	gcc $(CFLAGS) $(MPEG_FLAGS) -DSH_MEM -L/usr/lib mpeg2dec.c -o mpeg2dec \
E 4
D 9
	    display.o mpeg.a util.a -lXext -lX11
E 9
I 9
	    display.o mpeg.a util.a -lm -lXext -lX11
E 10
I 10
	gcc $(CFLAGS) $(MPEG_FLAGS) -DSH_MEM \
D 11
	    -DDUMP_QSCALE=0 -DDUMP_COEF_MAP=0 -DMB_MSE=0 \
E 11
I 11
D 12
	    -DDUMP_QSCALE=1 -DDUMP_COEF_MAP=0 -DMB_MSE=0 \
E 12
E 11
	    -L/usr/lib mpeg2dec.c -o mpeg2dec display.o mpeg.a util.a \
	    -lm -lXext -lX11
E 10
E 9

I 12
mpeg2dec_new: mpeg2dec_new.c display.o util.a mpeg.a
	gcc $(CFLAGS) $(MPEG_FLAGS) -DSH_MEM \
	    -L/usr/lib mpeg2dec_new.c -o mpeg2dec_new display.o mpeg.a util.a \
	    -lm -lXext -lX11

mpeg2parse: mpeg2parse.c display.o util.a mpeg.a
	gcc $(CFLAGS) $(MPEG_FLAGS) -DSH_MEM \
	    -DDUMP_QSCALE=0 -DDUMP_COEF_MAP=0 -DMB_MSE=0 -DDUMP_TP_QS=0 \
	    -L/usr/lib mpeg2parse.c -o mpeg2parse display.o mpeg.a util.a \
	    -lm -lXext -lX11

E 12
mpeg2enc: mpeg2enc.c util.a mpeg.a
	gcc $(CFLAGS) $(MPEG_FLAGS) -L/usr/lib mpeg2enc.c -o mpeg2enc \
D 8
	    mpeg.a util.a -lXext -lX11
E 8
I 8
	    mpeg.a util.a
E 8

I 11
mpeg2cmp: mpeg2cmp.c util.a mpeg.a
	gcc $(CFLAGS) $(MPEG_FLAGS) -L/usr/lib mpeg2cmp.c -o mpeg2cmp \
	    mpeg.a util.a  -lm

E 11
I 8
mpeg2rq: mpeg2rq.c util.a mpeg.a
	gcc $(CFLAGS) $(MPEG_FLAGS) -L/usr/lib mpeg2rq.c -o mpeg2rq \
	    mpeg.a util.a

I 13
ipmpeg2dec: ipmpeg2dec.c display.o util.a mpeg.a
	gcc $(CFLAGS) $(MPEG_FLAGS) -DSH_MEM \
	    -L/usr/lib ipmpeg2dec.c -o ipmpeg2dec display.o mpeg.a util.a \
	    -lm -lXext -lX11

E 13
E 8
I 6
requant: requant.c util.a
	gcc $(CFLAGS) -L/usr/lib requant.c -o requant util.a

I 13
requant2: requant2.c util.a
	gcc $(CFLAGS) -L/usr/lib requant2.c -o requant2 util.a

E 13
I 7
coefMapCmp: coefMapCmp.c util.a
	gcc $(CFLAGS) -L/usr/lib coefMapCmp.c -o coefMapCmp util.a

tsparse: tsparse.c util.a mpeg.a
	gcc $(CFLAGS) -L/usr/lib tsparse.c -o tsparse mpeg.a util.a

I 12
tsparse2: tsparse2.c util.a mpeg.a
	gcc $(CFLAGS) -L/usr/lib tsparse2.c -o tsparse2 mpeg.a util.a
E 12

I 12
204to188: 204to188.c
	gcc $(CFLAGS) -L/usr/lib 204to188.c -o 204to188 util.a

I 13
statmux: statmux.c util.a
	gcc $(CFLAGS) -L/usr/lib statmux.c -o statmux util.a -lm
E 13

I 13
statmux1: statmux1.c util.a
	gcc $(CFLAGS) -L/usr/lib statmux1.c -o statmux1 util.a -lm

statmux2: statmux2.c util.a
	gcc $(CFLAGS) -L/usr/lib statmux2.c -o statmux2 util.a -lm

statmux3: statmux3.c util.a
	gcc $(CFLAGS) -L/usr/lib statmux3.c -o statmux3 util.a -lm

statmux4: statmux4.c util.a
	gcc $(CFLAGS) -L/usr/lib statmux4.c -o statmux4 util.a -lm

statmux5: statmux5.c util.a
	gcc $(CFLAGS) -L/usr/lib statmux5.c -o statmux5 util.a -lm

pcr: pcr.c util.a mpeg.a
	gcc $(CFLAGS) pcr.c -o pcr mpeg.a util.a

ipencap: ipencap.c util.a mpeg.a
	gcc $(CFLAGS) ipencap.c -o ipencap mpeg.a util.a

cbrMux: cbrMux.c util.a mpeg.a
	gcc $(CFLAGS) cbrMux.c -o cbrMux mpeg.a util.a

ipMux: ipMux.c util.a mpeg.a
	gcc $(CFLAGS) ipMux.c -o ipMux mpeg.a util.a

ipfilter: ipfilter.c util.a
	gcc $(CFLAGS) ipfilter.c -o ipfilter util.a

ineo: ineo.c util.a
	gcc $(CFLAGS) ineo.c -o ineo util.a

E 13
E 12
E 7
E 6
D 3

E 3
clean:
	rm -f *.o *.a

E 1
