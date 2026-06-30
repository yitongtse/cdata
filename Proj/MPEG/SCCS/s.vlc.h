h47353
s 00000/00000/00022
d D 2.1 00/08/21 11:04:27 ytse 2 1
c Before supporting Windows
e
s 00022/00000/00000
d D 1.1 99/10/29 15:58:16 yitong 1 0
c date and time created 99/10/29 15:58:16 by yitong
e
u
U
f e 0
t
T
I 1
#ifndef VLC_H
#define VLC_H


/* Variable Length Coder
*/
typedef struct
{
    int  size;			// Number of codewords
    int  *length;		// Codeword lengths
    uint *codeword;	        // Codeword patterns (right justified)
}
Vlc;


void loadVlc(Vlc *vlc, char *vlcTable);
void printVlc(Vlc *vlc);
void _putVlc(Writer *out, Vlc *vlc, int index);
void putVlc(Writer *out, Vlc *vlc, int index, char *label);

#endif

E 1
