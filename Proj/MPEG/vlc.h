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

