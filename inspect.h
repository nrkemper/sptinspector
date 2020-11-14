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

typedef enum flag_e { FLG_VERBOSE, FLG_VERSION, FLG_VERIFY } flag_t;
typedef enum error_e {
        ERR_NULL = 3000,
        ERR_NOFILE,
        ERR_MALLOC,
        ERR_MMAP,
        ERR_OPEN,
        ERR_STAT,
        ERR_INVALIDSPT,
        ERR_DUPCOL
} error_t;

#if (defined (__APPLE__) && defined (__MACH__)) ||\
        defined (macintosh) || defined (Macintosh)

#       define __inspctMacOSX           1

#elif defined (_WIN32) || defined (_WIN64)

#       define __inspctWindows          1

#elif defined (__linux) || defined (_linux_)

#       define __inspctLinux            1

#endif

#define VERSION                 0.9
#define SPRITE_HEADER_SIZE      23

typedef struct mmapped_file_s
{
        long    size;
        byte    *data;
        char    *path;
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

typedef struct system_s
{
        sptfile_t       spt_file;
        mmapped_file_t  mmapped_file;
        int             errno;
        bool            flg_verbose;
        bool            flg_verify;
        bool            flg_version;
} system_t;//system critical resources

extern system_t         sys;

/*
        System Dependent Functions
*/
void SYS_Printf (char *str, ...);
int SYS_MMap (char *file, mmapped_file_t *ret);
bool SYS_MUnmap (mmapped_file_t *file);
void SYS_Error (char *str, ...);
void SYS_Shutdown (int exitcode);

/*
        Main Functions
 */
bool ValidSpriteFile (mmapped_file_t *file);
bool InspectFile (mmapped_file_t *src, sptfile_t *ret);
bool DumpFile (sptfile_t *file, FILE *stream);
void PrintVersionNumber (void);
void FreeSprite (sptfile_t *spt);

/*
        Flag Functions
 */
void SetFlag (int flag, bool val);
bool GetFlag (int flag);
bool SetErrno (error_t errno);
error_t GetErrno (void);

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
