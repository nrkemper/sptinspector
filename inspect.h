//
//  inspect.h
//  sptinspector
//
//  Created by Nicholas Kemp on 2020-11-10.
//

#ifndef __INSPECT_H__
#define __INSPECT_H__

#include <stdio.h>

typedef char            bool;
typedef unsigned char   byte;

#define true    1
#define false   0

#if (defined (__APPLE__) && defined (__MACH__)) ||\
        defined (macintosh) || defined (Macintosh)

#       define __inspctMacOSX           1

#elif defined (_WIN32) || defined (_WIN64)

#       define __inspctWindows          1

#elif defined (__linux) || defined (_linux_)

#       define __inspctLinux            1

#endif

#define SPRITE_HEADER_SIZE      23

typedef struct mmapped_file_s
{
        long    size;
        byte    *data;
        char    *fname;
        int     fd;
} mmapped_file_t;

typedef struct colour_s
{
        byte    r;
        byte    g;
        byte    b;
} colour_t;

typedef struct palette_entry_s
{
        byte    r;
        byte    g;
        byte    b;
} palette_entry_t;

typedef struct palette_s
{
        int                     nocolours;
        int                     maxcolours;
        palette_entry_t         *data;
} palette_t;

typedef struct sptfile_s
{
        char            stamp[3];
        int             width, height;
        int             nocolours;
        int             offset;
        int             size;
        palette_t       pallete;
        colour_t        *bmpdata;
        char*           path;
} sptfile_t;

/*
        System Dependent Functions
*/
void SYS_Printf (char *str, ...);
bool SYS_MMap (char *file, mmapped_file_t *ret);
bool SYS_MUnmap (mmapped_file_t *file);

/*
        Main Functions
 */
bool InspectFile (mmapped_file_t *src, sptfile_t *ret);
bool DumpFile (sptfile_t *file, FILE *stream);
void FreeSprite (sptfile_t *spt);

/*
        Miscellaneous Functions
*/
short ByteSwap2 (short n);
int ByteSwap4 (int n);
long ByteSwap8 (long n);
bool LittleEndian (void);

/*
        Palette Functions
*/
void P_InitPalette (palette_t *palette, int nocolours);
bool P_ColourExists (palette_t *palette, colour_t *colour);
int P_GetIndex (palette_t *palette, colour_t *colour);
bool P_CopyPalette (const palette_t *src, palette_t *des);
void P_AddColour (palette_t *palette, colour_t *colour);
void P_RemoveColour (palette_t *palette, colour_t *colour);
void P_RemoveColourAtIndex (palette_t *pallete, int index);
void P_PrintPalette (palette_t *palette);
void P_DestroyPalette (palette_t *palette);


#endif /* __INSPECT_H__ */
