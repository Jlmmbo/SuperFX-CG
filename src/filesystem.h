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
    Rom rom;
    rom.size = Bfile_GetFileSize_OS(handle);
    rom.raw = NULL;//just to get the compiler to shut up about using rom.raw uninitialized
    Bfile_ReadFile_OS(handle, rom.raw, rom.size, 0);
    return rom;
}

void get_rom_list_fs(char** rom_list, byte* len){
    *len = 0;
    
    int FindHandle;
    char* FoundFile = NULL;
    file_type_t* fileinfo = NULL;
    
    int err = Bfile_FindFirst("\\\\fls0\\*.sfc", &FindHandle, FoundFile, fileinfo);//if the file is larger that 2mb, too bad... :(
    if (err == -16){
        PrintXY(1, 1, "  No Roms", 0, TEXT_COLOR_BLACK);
        rom_list = NULL;
        Bfile_FindClose(FindHandle);
    }

    int i = 0;
    while (1){
        int err = Bfile_FindNext(FindHandle, FoundFile, fileinfo);
        if (err == -16){// no more files
            break;
        }
        rom_list[i] = FoundFile;
        i++;
        (*len)++;
    }

    Bfile_FindClose(FindHandle);
}