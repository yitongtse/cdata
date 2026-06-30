h48128
s 00003/00000/00010
d D 2.2 00/08/21 11:23:55 ytse 4 3
c Added support of Windows
e
s 00000/00000/00010
d D 2.1 00/08/21 11:04:19 ytse 3 2
c Before supporting Windows
e
s 00001/00001/00009
d D 1.2 99/10/29 17:07:57 yitong 2 1
c minor changes
e
s 00010/00000/00000
d D 1.1 99/10/29 15:58:13 yitong 1 0
c date and time created 99/10/29 15:58:13 by yitong
e
u
U
f e 0
t
T
I 1
#ifndef DISPLAY_H
#define DISPLAY_H

void init_display(char *name, PicInfo* pic);
void init_dither();
D 2
void dither(unsigned char *src[], PicInfo *pic);
E 2
I 2
void dither(uchar *src[], PicInfo *pic);
E 2
void display_second_field();
I 4
/* Rui_B */
void exit_display();
/* Rui_E */
E 4

#endif

E 1
