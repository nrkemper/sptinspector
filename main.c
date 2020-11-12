//
//  main.c
//  sptinspector
//
//  Created by Nicholas Kemp on 2020-11-10.
//

#include <stdio.h>
#include <string.h>
#include "inspect.h"

char    filepath[100] = "/Users/nicholaskemp/Desktop/black_white.spt";

bool ParseCommandLine (int argc, const char *argv[])
{
        return true;
}

int main(int argc, const char * argv[])
{
        mmapped_file_t  mapped_file;
        sptfile_t       sprite_file;
        memset (&sprite_file, 0, sizeof (sptfile_t));
        ParseCommandLine(argc, argv);
        if (!SYS_MMap(filepath, &mapped_file))
                return -1;
        
        if (!InspectFile(&mapped_file, &sprite_file))
        {
                SYS_MUnmap(&mapped_file);
                return -1;
        }
        
        DumpFile(&sprite_file, stdout);
        FreeSprite(&sprite_file);
        SYS_MUnmap(&mapped_file);
        
        return 0;
}
