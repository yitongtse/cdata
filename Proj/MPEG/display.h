#ifndef DISPLAY_H
#define DISPLAY_H

void init_display(char *name, PicInfo* pic);
void init_dither();
void dither(uchar *src[], PicInfo *pic);
void display_second_field();
/* Rui_B */
void exit_display();
/* Rui_E */

#endif

