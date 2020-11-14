//
//  main.c
//  sptinspector
//
//  Created by Nicholas Kemp on 2020-11-10.
//

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "inspect.h"

#define MAXFILEPATH     100

char    filepath[MAXFILEPATH] = "";

bool ParseCommandLine (int argc, char *argv[])
{
        char    *ptr;
        int     i;
        
        for (i=1; i<argc; )
        {
                ptr = argv[i];
                
                switch (*ptr)
                {
                        case '-':
                        {
                                for (ptr++, i++; *ptr; ptr++)
                                {
                                        switch (*ptr)
                                        {
                                                case 'f':
                                                case 'F':
                                                        if (*filepath == '\0')
                                                                strncpy(filepath, argv[i], MAXFILEPATH);
                                                        else
                                                                SYS_Printf("ERROR: filename already supplied. Ignoring %s\n", argv[i]);
                                                        
                                                        i++;
                                                        break;
                                                        
                                                case 'g': //verify the file is a .spt file
                                                        SetFlag(FLG_VERIFY, true);
                                                        break;
                                                        
                                                case 'v': //verbose
                                                        SetFlag(FLG_VERBOSE, true);
                                                        break;
                                                        
                                                case 'V': //version
                                                        SetFlag(FLG_VERSION, true);
                                                        break;
                                        }
                                                
                                }
                        }
                                break;
                                
                        default:
                                
                                if (*filepath == '\0')
                                        strncpy(filepath, argv[i], MAXFILEPATH);
                                else
                                        SYS_Printf("ERROR: filepath already supplied. Ignoring %s\n", argv[i]);
                                
                                i++;
                                
                                break;
                }
        }
        return true;
}

int main(int argc, char *argv[])
{
        bool            version, verbose, verify, valid;
        
        memset(&sys, 0, sizeof(system_t));
        
        ParseCommandLine(argc, argv);
        
        version = GetFlag(FLG_VERSION);
        verbose = GetFlag(FLG_VERBOSE);
        verify = GetFlag(FLG_VERIFY);
        
        if (version)
        {
                PrintVersionNumber();
                exit(1);
        }
        
        if (*filepath == '\0')
        {
                SetErrno(ERR_NOFILE);
                SYS_Error("ERROR %d: did not supply a file.\n", ERR_NOFILE);
        }
        
        SYS_MMap(filepath, &sys.mmapped_file);
        
        if (verify)
        {
                valid = ValidSpriteFile(&sys.mmapped_file);
                SYS_MUnmap(&sys.mmapped_file);
                
                if (valid)
                {
                        if (verbose)
                                SYS_Printf("%s is a valid .spt file\n", filepath);
                        
                        exit(1);
                }
                else
                {
                        if (verbose)
                                SYS_Printf("%s isn't a valid .spt file\n", filepath);
                        
                        exit(0);
                }
        }
        
        InspectFile(&sys.mmapped_file, &sys.spt_file);
        DumpFile(&sys.spt_file, stdout);
        
        SYS_Shutdown(0);
        
        return 0;
}
