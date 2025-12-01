/*typedef struct file_type_t{
  unsigned short id, type;
  unsigned long fsize, dsize;
  unsigned int property;
  unsigned long address;
}file_type_t;*/

Rom load_rom_fs(char** rom_list, byte rom_index){//bad news: may not even work for files larger that 1mb
    unsigned short file_name[50];
    Bfile_StrToName_ncpy(file_name, rom_list[rom_index], 49);
    int handle = Bfile_OpenFile_OS(file_name, 0, 0);
    if(handle < 0) return test_rom;
    Rom rom;
    rom.size = Bfile_GetFileSize_OS(handle);
    rom.raw = NULL;//just to get the compiler to shut up about using rom.raw uninitialized
    Bfile_ReadFile_OS(handle, rom.raw, rom.size, 0);
    Bfile_CloseFile_OS(handle);
    return rom;
}

void get_rom_list_fs(char** rom_list, byte* len){
    *len = 0;
    
    int FindHandle;
    char* FoundFile = NULL;
    file_type_t* fileinfo = NULL;

    unsigned short pathname[0x100];
    Bfile_StrToName_ncpy(pathname, "\\\\fls0\\*.sfc", 0x50);
    
    int err = Bfile_FindFirst(pathname, &FindHandle, FoundFile, fileinfo);//if the file is larger that 2mb, too bad... :(
    Bdisp_AllClr_VRAM();
    PrintXY(1, 1, "  hi", 0, TEXT_COLOR_BLACK);
    GetKey(NULL);
    if (err == -5){
        Bdisp_AllClr_VRAM();
        PrintXY(1, 1, "  bad pref", 0, TEXT_COLOR_BLACK);
        GetKey(NULL);
        rom_list = NULL;
        Bfile_FindClose(FindHandle);
        return;
    }

    if (err == -1){
        Bdisp_AllClr_VRAM();
        PrintXY(1, 1, "  bad name", 0, TEXT_COLOR_BLACK);
        GetKey(NULL);
        rom_list = NULL;
        Bfile_FindClose(FindHandle);
        return;
    }

    if (err == -16){
        Bdisp_AllClr_VRAM();
        PrintXY(1, 1, "  No Roms", 0, TEXT_COLOR_BLACK);
        GetKey(NULL);
        rom_list = NULL;
        Bfile_FindClose(FindHandle);
        return;
    }

    if (err == 0){
        Bdisp_AllClr_VRAM();
        PrintXY(1, 1, "  found", 0, TEXT_COLOR_BLACK);
        GetKey(NULL);
        rom_list = NULL;
        Bfile_FindClose(FindHandle);
        return;
    }

    int i = 0;
    while (1){
        int err = Bfile_FindNext(FindHandle, FoundFile, fileinfo);
        if (err < 0){// no more files
            break;
        }
        rom_list[i] = FoundFile;
        i++;
        (*len)++;
    }

    Bfile_FindClose(FindHandle);

}