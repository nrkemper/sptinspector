//
//  inspect.c
//  sptinspector
//
//  Created by Nicholas Kemp on 2020-11-10.
//
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>

#include "inspect.h"

system_t        sys;

/*
        System Dependent Functions
*/
#if defined (__inspctLinux) || defined (__inspctMacOSX)

#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>

//FIXME: make console???
void SYS_Printf (char *str, ...)
{
        va_list         args;
        
        va_start (args, str);
        vprintf (str, args);
        va_end (args);
}

int SYS_MMap (char *file, mmapped_file_t *ret)
{
        struct stat     sb;
        
        if (!ret || !file || *file == '\0')
        {
                SetErrno(ERR_NULLPTR);
                SYS_Error("ERROR %d: supplied null pointer while trying to mmap\n", ERR_NULLPTR);
        }
        
        ret->fd = open (file, O_RDONLY);
        if (ret->fd == -1)
        {
                SetErrno(ERR_OPEN);
                SYS_Error ("ERROR %d: unable to open %s\n", ERR_OPEN, file);
        }
        
        if (stat (file, &sb) == -1)
        {
                SetErrno(ERR_STAT);
                SYS_Error("ERROR %d: unable to fstat file %s\n", ERR_STAT, file);
        }
        
        ret->size = sb.st_size;
        
        ret->data = mmap(NULL, ret->size, PROT_READ, MAP_PRIVATE, ret->fd, 0);
        if (ret->data == (void *)-1)
        {
                SetErrno(ERR_MMAP);
                SYS_Error("ERROR %d: unable to map file %s\n", ERR_MMAP, file);
        }
        
        ret->path = (char *)malloc (strlen (file) + 1);
        if (!ret->path)
        {
                SetErrno(ERR_MALLOC);
                SYS_Error("ERROR %d: unable to allocate memory for file name %s\n", ERR_MALLOC, file);
        }
        
        strcpy (ret->path, file);
        close(ret->fd);
        
        return true;
}

bool SYS_MUnmap (mmapped_file_t *file)
{
        munmap (file->data, file->size);
        free(file->path);
        memset(file, 0, sizeof (mmapped_file_t));
        
        return true;
}

void SYS_Error (char *str, ...)
{
        va_list         args;
        
        va_start(args, str);
        vprintf(str, args);
        va_end(args);
        
        SYS_Shutdown(sys.errno);
}

void SYS_Shutdown (int exitcode)
{
        SYS_MUnmap(&sys.mmapped_file);
        FreeSprite(&sys.spt_file);
        
        exit (sys.errno);
}

#elif defined (__inspctWindows)

//FIXME: implement windows code here

#elif defined 

//FIXME: implement other OS code here

#endif

/*
        Main Functions
 */
bool ValidSpriteFile (mmapped_file_t *file)
{
        byte    *data;
        char    *fname;
        
        if (!file || !file->path)
        {
                SetErrno(ERR_NULLPTR);
                SYS_Error("ERROR %d: null pointer when trying to validate .spt file\n", ERR_NULLPTR);
        }
        
        data = file->data;
        fname = file->path;
        
        if (strcmp(fname + strlen (fname) - 4, ".spt") != 0)
                return false;
        
        if (*data != 'S' ||
            *(data + 1) != 'P' ||
            *(data + 2) != 'T')
                return false;
        
        return true;
}

bool InspectFile (mmapped_file_t *src, sptfile_t *ret)
{
        byte            *data;
        int             i, x, offset, nbytes;
        colour_t        tmp;
        bool            valid;
        
        if (!src || !ret)
        {
                SetErrno(ERR_NULLPTR);
                SYS_Error("ERROR %d: null pointer while inspecting file\n", ERR_NULLPTR);
        }
        
        valid = ValidSpriteFile(src);
        if (!valid)
        {
                SetErrno(ERR_INVALIDSPT);
                SYS_Error("ERROR %d: invalid .spt file.\n", ERR_INVALIDSPT);
        }
        
        data = src->data;
        
        ret->stamp[0] = *data;
        ret->stamp[1] = *(data + 1);
        ret->stamp[2] = *(data + 2);
        ret->width = *(int *)(data + 3);
        ret->height = *(int *)(data + 7);
        ret->nocolours = *(int *)(data + 11);
        ret->offset = *(int *)(data + 15);
        ret->size = *(int *)(data + 19);
        
        if (!LittleEndian ())
        {
                ret->width = ByteSwap4(ret->width);
                ret->height = ByteSwap4(ret->height);
                ret->nocolours = ByteSwap4(ret->nocolours);
                ret->offset = ByteSwap4(ret->offset);
                ret->size = ByteSwap4(ret->size);
        }
        
        ret->pallete.data = (palette_entry_t *)malloc (sizeof (palette_entry_t) *                                                               ret->nocolours);
        if (!ret->pallete.data)
        {
                SetErrno(ERR_MALLOC);
                SYS_Error("ERROR %d: unable to allocate memory for the palette\n", ERR_MALLOC);
        }
        
        //get colour table
        offset = SPRITE_HEADER_SIZE;
        nbytes = ret->nocolours * 3;
        
        for (i=0; i<nbytes; i+=3)
        {
                /*if (*(data + offset + i) == EOF ||
                    *(data + offset + i + 1) == EOF ||
                    *(data + offset + i + 2) == EOF)
                {//reached EOF before processing all the data
                        SYS_Printf("ERROR: reached EOF before extracting colour table\n");
                        free (ret->pallete.data);
                        memset(ret, 0, sizeof(sptfile_t));
                        
                        return false;
                }*/
                
                tmp.r = *(data + offset + i);
                tmp.g = *(data + offset + i + 1);
                tmp.b = *(data + offset + i + 2);
                
                if (P_ColourExists(&ret->pallete, &tmp))
                {//duplicate colour in palette
                        SetErrno(ERR_DUPCOL);
                        SYS_Error("ERROR %d: colour (%c, %c, %c) exists in palette\n", ERR_DUPCOL, tmp.r, tmp.g, tmp.b);
                }
                
                P_AddColour(&ret->pallete, &tmp);
        }
        
        //get bmpdata
        offset = ret->offset;
        nbytes = ret->width * ret->height * 3;
        
        ret->bmpdata = (colour_t *)malloc (nbytes * sizeof (colour_t));
        if (!ret->bmpdata)
        {
                SetErrno(ERR_MALLOC);
                SYS_Error("ERROR %d: unable to allocate memory for bitmap data\n", ERR_MALLOC);
        }
        
        for (i=0, x=0; i<nbytes; i+=3, x++)
        {
                /*if (*(data + offset + i) == EOF ||
                    *(data + offset + i + 1) == EOF ||
                    *(data + offset + i + 2) == EOF)
                { // if reached EOF before parsing all the data
                        SYS_Printf("ERROR: reached EOF before extracting bmp data\n");
                        FreeSprite(ret);
                        
                        return false;
                }*/
                
                ret->bmpdata[x].r = *(data + offset + i);
                ret->bmpdata[x].g = *(data + offset + i + 1);
                ret->bmpdata[x].b = *(data + offset + i + 2);
        }
        
        ret->path = (char *)malloc (strlen(src->path) + 1);
        if (!ret->path)
        {
                SetErrno(ERR_MALLOC);
                SYS_Error("ERROR %d: unable to allocate memory for name\n", ERR_MALLOC);
        }
        
        strcpy (ret->path, src->path);
        return true;
}

void FreeSprite (sptfile_t *spt)
{
        if (!spt)
                return;
        
        if (spt->bmpdata)
                free (spt->bmpdata);
        
        if (spt->pallete.data)
                free (spt->pallete.data);
        
        if (spt->path)
                free (spt->path);
        
        memset(spt, 0, sizeof (sptfile_t));
}

bool DumpFile (sptfile_t *file, FILE *stream)
{
        int     i, x, y;
        
        if(!file)
        {
                SetErrno(ERR_NULLPTR);
                SYS_Error("ERROR %d: null pointer when trying to dump file\n", ERR_NULLPTR);
        }
        
        fprintf (stream, "Filepath:     %s\n", file->path);
        fprintf (stream, "Stamp:        %c%c%c\n", file->stamp[0], file->stamp[1],                                                      file->stamp[2]);
        
        fprintf (stream, "Width:        %d\n", file->width);
        fprintf (stream, "Height:       %d\n", file->height);
        fprintf (stream, "No. Colours:  %d\n", file->nocolours);
        fprintf (stream, "Offset:       %d\n", file->offset);
        fprintf (stream, "Size:         %d\n\n", file->size);
        
        if (GetFlag(FLG_VERBOSE))
        {
                fprintf (stream, "Colour Table\n");
                fprintf (stream, "------------\n\n");
                
                for (i=0; i<file->nocolours; i++)
                {
                        fprintf (stream, "Index:        %d\n", i);
                        fprintf (stream, "      R:      %x\n", file->pallete.data[i].r);
                        fprintf (stream, "      G:      %x\n", file->pallete.data[i].g);
                        fprintf (stream, "      B:      %x\n\n", file->pallete.data[i].b);
                }
                
                fprintf (stream, "Bitmap Data\n");
                fprintf (stream, "------------\n\n");
                
                for (y=0; y<file->height * file->width; y+=file->width)
                {
                        for (x=0; x<file->width; x++)
                        {
                                fprintf (stream, "Index:        %d\n", y + x);
                                fprintf (stream, "      R:      %x\n", file->bmpdata[y + x].r);
                                fprintf (stream, "      G:      %x\n", file->bmpdata[y + x].g);
                                fprintf (stream, "      G:      %x\n\n", file->bmpdata[y + x].b);
                        }
                }
        }
        return true;
}

void PrintVersionNumber (void)
{
        SYS_Printf("---Version %.2f---\n", VERSION);
}

/*
        Flag Functions
 */
void SetFlag (int flag, bool val)
{
        switch (flag)
        {
                case FLG_VERSION:
                        sys.flg_version = val;
                        break;
                        
                case FLG_VERBOSE:
                        sys.flg_verbose = val;
                        break;
                        
                case FLG_VERIFY:
                        sys.flg_verify = val;
                        break;
                        
                default:
                        SYS_Printf("ERROR: invalid flag %d\n", flag);
                        break;
        }
}

bool GetFlag (int flag)
{
        switch (flag)
        {
                case FLG_VERSION:
                        return sys.flg_version;
                        break;
                
                case FLG_VERBOSE:
                        return sys.flg_verbose;
                        break;
                
                case FLG_VERIFY:
                        return sys.flg_verify;
                        break;
                        
                default:
                        SYS_Printf("ERROR: inavlid flag %d\n", flag);
                        return false;
                        break;
        }
}


bool SetErrno (error_t errno)
{
        switch (errno)
        {
                case ERR_MALLOC:
                case ERR_MMAP:
                case ERR_NULLPTR:
                case ERR_OPEN:
                case ERR_STAT:
                case ERR_DUPCOL:
                case ERR_INVALIDSPT:
                case ERR_NOFILE:
                        sys.errno = errno;
                        return true;
                        break;
                        
                default:
                        return false;
                        break;
        }
}

error_t GetErrno (void)
{
        return sys.errno;
}

/*
        Miscellaneous Functions
 */
short ByteSwap2 (short n)
{
        return (n & 0xff00) >> 8 || (n & 0x00ff) << 8;
}

int ByteSwap4 (int n)
{
        return (n & 0xff000000 >> 24) || (n & 0x00ff0000) >> 8 || (n & 0x0000ff00) << 8 || (n & 0x000000ff) << 24;
}

long ByteSwap8 (long n)
{
        long            data;
        
        ((byte *)&data)[0] = ((byte *)&n)[7];
        ((byte *)&data)[1] = ((byte *)&n)[6];
        ((byte *)&data)[2] = ((byte *)&n)[5];
        ((byte *)&data)[3] = ((byte *)&n)[4];
        ((byte *)&data)[4] = ((byte *)&n)[3];
        ((byte *)&data)[5] = ((byte *)&n)[2];
        ((byte *)&data)[6] = ((byte *)&n)[1];
        ((byte *)&data)[7] = ((byte *)&n)[0];
        
        return data;
}

bool LittleEndian (void)
{
        long            x=1;
        char            *y = (char *)&x;
        
        if (*y)
                return true;
        
        return false;
}

/*
        Palette Functions
*/

#define PALETTE_INCREASE_SIZE           20

void P_InitPalette (palette_t *palette, int nocolours)
{
        palette->nocolours = 0;
        palette->maxcolours = nocolours;
        palette->data = malloc (sizeof (palette_t) * nocolours);
}

bool P_ColourExists (palette_t *pallete, colour_t *colour)
{
        int     i;
        
        for (i=0; i<pallete->nocolours; i++)
        {
                if (colour->r == pallete->data[i].r &&
                    colour->g == pallete->data[i].g &&
                    colour->b == pallete->data[i].b)
                        return true;
        }
        
        return false;
}

int P_GetIndex (palette_t *palette, colour_t *colour)
{
        int     i;
        
        if (!palette || !colour)
                return -1;
        
        for (i=0; i<palette->nocolours; i++)
        {
                if (palette->data[i].r == colour->r &&
                    palette->data[i].g == colour->g &&
                    palette->data[i].b == colour->b)
                        return i;
        }
        
        return -1;
}

bool P_CopyPalette (const palette_t *src, palette_t *des)
{
        int     i;
        
        if (!src || !des)
                return false;
        
        if (!src->maxcolours) //uninitialized src palette
                return false;
        
        if (!des->maxcolours) // uninitialized des palette
                P_InitPalette (des, src->maxcolours);
        
        if (src->nocolours > des->maxcolours)
        {
                P_DestroyPalette (des);
                P_InitPalette(des, src->maxcolours);
        }
        
        for (i=0; i<src->nocolours; i++)
        {
                des->data[i].r = src->data[i].r;
                des->data[i].g = src->data[i].g;
                des->data[i].b = src->data[i].b;
        }
        
        des->nocolours = src->nocolours;
        des->maxcolours = src->maxcolours;
        
        return true;
}

void P_AddColour (palette_t *palette, colour_t *colour)
{
        palette_t       tmp;
        int             index;
        
        if (!palette || !colour)
                return;
        
        if (palette->nocolours >= palette->maxcolours)
        {
                int     newmax = palette->maxcolours + PALETTE_INCREASE_SIZE;
                
                P_InitPalette (&tmp, newmax);
                P_CopyPalette (palette, &tmp);
                P_DestroyPalette (palette);
                P_InitPalette (palette, newmax);
                P_CopyPalette (&tmp, palette);
                P_DestroyPalette (&tmp);
        }
        
        index = palette->nocolours;
        palette->data[index].r = colour->r;
        palette->data[index].g = colour->g;
        palette->data[index].b = colour->b;
        palette->nocolours++;
}

void P_RemoveColour (palette_t *palette, colour_t *colour)
{
        if (P_ColourExists(palette, colour))
        {
                int     index;
                
                index = P_GetIndex (palette, colour);
                P_RemoveColourAtIndex (palette, index);
        }
}

void P_RemoveColourAtIndex (palette_t *palette, int index)
{
        int     i;
        
        if (!palette)
                return;
        
        if (index > palette->nocolours)
                return;
        
        for (i=index; i<palette->nocolours - 1; i++)
        {
                palette->data[i].r = palette->data[i + 1].r;
                palette->data[i].g = palette->data[i + 1].g;
                palette->data[i].b = palette->data[i + 1].b;
        }
        
        palette->nocolours--;
}

void P_DestroyPalette (palette_t *palette)
{
        if (!palette)
                return;
        
        if (palette->data)
                free (palette->data);
        
        memset (palette, 0, sizeof (palette_t));
}

void P_PrintPalette (palette_t *palette)
{
        int     i;
        
        for (i=0; i<palette->nocolours; i++)
        {
                printf ("---------------------------\n");
                printf ("Index:                 %d\n", i);
                printf ("       Red:            %d\n", palette->data[i].r);
                printf ("       Green:          %d\n", palette->data[i].g);
                printf ("       Blue:           %d\n", palette->data[i].b);
                printf ("---------------------------\n\n");
        }
}
