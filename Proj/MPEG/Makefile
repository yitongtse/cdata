.KEEP_STATE:
.SUFFIXES: .C .c .o


CFLAGS = -g -Wimplicit -DUNIX

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
	tsview		\
	ipfilter	\
	ineo


MPEG_PROGS =		\
	mpeg2dec	\
	mpeg2enc	\
	mpeg2cmp	\
	mpeg2rq		\
        208to188        \
	ipmpeg2dec


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
	gcc $(CFLAGS) -I/usr/openwin/share/include -DSH_MEM -c display.c

mpeg2dec: mpeg2dec.c display.o util.a mpeg.a
	gcc $(CFLAGS) $(MPEG_FLAGS) -DSH_MEM \
	    -L/usr/lib mpeg2dec.c -o mpeg2dec display.o mpeg.a util.a \
	    -lm -lXext -lX11

mpeg2dec_new: mpeg2dec_new.c display.o util.a mpeg.a
	gcc $(CFLAGS) $(MPEG_FLAGS) -DSH_MEM \
	    -L/usr/lib mpeg2dec_new.c -o mpeg2dec_new display.o mpeg.a util.a \
	    -lm -lXext -lX11

mpeg2parse: mpeg2parse.c display.o util.a mpeg.a
	gcc $(CFLAGS) $(MPEG_FLAGS) -DSH_MEM \
	    -DDUMP_QSCALE=0 -DDUMP_COEF_MAP=0 -DMB_MSE=0 -DDUMP_TP_QS=0 \
	    -L/usr/lib mpeg2parse.c -o mpeg2parse display.o mpeg.a util.a \
	    -lm -lXext -lX11

mpeg2enc: mpeg2enc.c util.a mpeg.a
	gcc $(CFLAGS) $(MPEG_FLAGS) -L/usr/lib mpeg2enc.c -o mpeg2enc \
	    mpeg.a util.a

mpeg2cmp: mpeg2cmp.c util.a mpeg.a
	gcc $(CFLAGS) $(MPEG_FLAGS) -L/usr/lib mpeg2cmp.c -o mpeg2cmp \
	    mpeg.a util.a  -lm

mpeg2rq: mpeg2rq.c util.a mpeg.a
	gcc $(CFLAGS) $(MPEG_FLAGS) -L/usr/lib mpeg2rq.c -o mpeg2rq \
	    mpeg.a util.a

ipmpeg2dec: ipmpeg2dec.c display.o util.a mpeg.a
	gcc $(CFLAGS) $(MPEG_FLAGS) -DSH_MEM \
	    -L/usr/lib ipmpeg2dec.c -o ipmpeg2dec display.o mpeg.a util.a \
	    -lm -lXext -lX11

requant: requant.c util.a
	gcc $(CFLAGS) -L/usr/lib requant.c -o requant util.a

requant2: requant2.c util.a
	gcc $(CFLAGS) -L/usr/lib requant2.c -o requant2 util.a

coefMapCmp: coefMapCmp.c util.a
	gcc $(CFLAGS) -L/usr/lib coefMapCmp.c -o coefMapCmp util.a

tsparse: tsparse.c util.a mpeg.a
	gcc $(CFLAGS) -L/usr/lib tsparse.c -o tsparse mpeg.a util.a

tsparse2: tsparse2.c util.a mpeg.a
	gcc $(CFLAGS) -L/usr/lib tsparse2.c -o tsparse2 mpeg.a util.a

204to188: 204to188.c
	gcc $(CFLAGS) -L/usr/lib 204to188.c -o 204to188 util.a

statmux: statmux.c util.a
	gcc $(CFLAGS) -L/usr/lib statmux.c -o statmux util.a -lm

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
	gcc $(CFLAGS) ineo.c -o ineo util.a -lm

pcrAnaly: pcrAnaly.c util.a
	gcc $(CFLAGS) pcrAnaly.c -o pcrAnaly util.a -lm

comcast: comcast.c util.a
	gcc $(CFLAGS) comcast.c -o comcast util.a -lm

pcap2ts: pcap2ts.c util.a
	gcc $(CFLAGS) pcap2ts.c -o pcap2ts util.a

clean:
	rm -f *.o *.a

