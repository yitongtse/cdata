h55772
s 00002/00002/00066
d D 2.4 02/05/24 12:26:42 ytse 5 4
c Update
e
s 00021/00030/00047
d D 2.3 02/03/21 13:28:40 ytse 4 3
c Backup for NEW2
e
s 00040/00004/00037
d D 2.2 00/08/21 11:23:55 ytse 3 2
c Added support of Windows
e
s 00000/00000/00041
d D 2.1 00/08/21 11:04:21 ytse 2 1
c Before supporting Windows
e
s 00041/00000/00000
d D 1.1 99/10/29 15:58:09 yitong 1 0
c date and time created 99/10/29 15:58:09 by yitong
e
u
U
f e 0
t
T
I 1
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include "util.h"
#include "bitbuf.h"
#include "reader.h"
#include "infile.h"

D 3

E 3
I 3
D 4
// Rui_B
E 4
#include "def.h"
D 4
// RUi_E
E 4
I 4


E 4
E 3
void openInFile(InFile *inFile, char *filename, int bufSz)
{
D 3
    if ((inFile->fp = fopen(filename, "r")) == NULL) {
E 3
I 3
D 4
/* Rui: open using "rb" instead of "r" */
E 4
    if ((inFile->fp = fopen(filename, "rb")) == NULL) {
E 3
        printf("Error: Failed to open file %s for input\n", filename);
D 3
        exit(-1);
E 3
I 3
        exit(-4563);
E 3
    }

    inFile->bufSz = bufSz & 0xFFFFFFFc;  /* make sure it's integer # of words */
    if ((inFile->buf = (uint*)malloc(inFile->bufSz)) == NULL) {
        printf("Error: Failed to allocate buffer to file input\n");
D 3
        exit(-1);
E 3
I 3
        exit(-6783);
E 3
    }

    initReader(&inFile->rdr, readFile, inFile);
    setBitBuf(&inFile->rdr.bitBuf, 0, 0, 0);
}


int readFile(InFile *inFile)
{
    int sz = fread(inFile->buf, 1, inFile->bufSz, inFile->fp);
I 3

D 4
// Rui_B 
E 4
D 5
#ifndef UNIX_ENV
E 5
I 5
#ifndef UNIX
E 5
D 4

	int i;
	char tmp;
	union
	{
		char ch[4];
		uint ui;
	} data;

E 4
I 4
    int i;
    char tmp;
    union {
        char ch[4];
        uint ui;
    } data;
E 4
#endif
D 4
// Rui_E 
E 4

E 3
    setBitBuf(&inFile->rdr.bitBuf, inFile->buf, inFile->buf+(sz>>2), 32);
I 3
D 4
// Rui_B 
#ifndef UNIX_ENV
	/* for the big indian / small indian problem */
E 4

D 4
	for(i=0;i<sz>>2;i++)
	{
		data.ui = inFile->buf[i];
		tmp = data.ch[0];
		data.ch[0] = data.ch[3];
		data.ch[3] = tmp;
		tmp = data.ch[1];
		data.ch[1] = data.ch[2];
		data.ch[2] = tmp;
		inFile->buf[i] = data.ui;
	}

E 4
I 4
D 5
#ifndef UNIX_ENV
E 5
I 5
#ifndef UNIX
E 5
    /* for the big indian / small indian problem */
    for(i=0;i<sz>>2;i++) {
        data.ui = inFile->buf[i];
        tmp = data.ch[0];
        data.ch[0] = data.ch[3];
        data.ch[3] = tmp;
        tmp = data.ch[1];
        data.ch[1] = data.ch[2];
        data.ch[2] = tmp;
        inFile->buf[i] = data.ui;
    }
E 4
#endif
D 4
// Rui_E 
E 4
I 4

E 4
E 3
    return feof(inFile->fp)? EOF : 0;
}


void closeInFile(InFile *inFile)
{
    fclose(inFile->fp);
    free(inFile->buf);
}

E 1
