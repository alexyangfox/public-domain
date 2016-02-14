#ifndef TGASAVE_H
#define TGASAVE_H


enum TGA_Code {
    TGA_OK,
    TGA_ERRCREATE,
    TGA_ERRWRITE
};


typedef struct {
    int r, g, b;
} TGA_Color;


void TGA_SetImageID(char *s);
void TGA_SetAuthor(char *s);
void TGA_SetSoftwareID(char *s);
void TGA_SetAspect(int width, int height);
int  TGA_Save(
         char *filename,
         int width, int height,
         void (*getpixel)(int x, int y, TGA_Color *col)
     );


#endif
