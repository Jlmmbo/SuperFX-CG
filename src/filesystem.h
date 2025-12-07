/*typedef struct file_type_t{
  unsigned short id, type;
  unsigned long fsize, dsize;
  unsigned int property;
  unsigned long address;
}file_type_t;*/

Rom load_rom_fs(char** rom_list, byte rom_index){//may possibly hang for files larger that 1mb
    unsigned short file_name[50];

    char char_filename [100]= "\\\\fls0\\";
    strcat(char_filename, rom_list[rom_index]);
    
    Bfile_StrToName_ncpy(file_name, char_filename, 49);

    int handle = Bfile_OpenFile_OS(file_name, 0, 0);
    if(handle < 0){
        error_msg("ROM not found");
        error_msg("There may be UB if you continue!");
        error_msg("Returning to Main Menu");

        return main_menu_ui();
    }
    Rom rom;
    rom.size = Bfile_GetFileSize_OS(handle);
    rom.raw = sys_malloc(sizeof(byte) * rom.size);
    Bfile_ReadFile_OS(handle, rom.raw, rom.size, 0);
    Bfile_CloseFile_OS(handle);
    return rom;
}

void get_rom_list_fs(char** rom_list, byte* len){
    *len = 0;
    
    int FindHandle;
    unsigned short* FoundFile = sys_malloc(sizeof(unsigned short) * 32);
    file_type_t fileinfo = {0, 0, 0, 0, 0, 0};

    unsigned short pathname[0x100];
    Bfile_StrToName_ncpy(pathname, "\\\\fls0\\*.sfc", 0x100);
    
    int err = Bfile_FindFirst(pathname, &FindHandle, FoundFile, &fileinfo);//if the file is larger that 2mb, too bad... :(
    int i = 0;
    if (err == -16){
        error_msg("  No ROMs");
        rom_list = NULL;
        Bfile_FindClose(FindHandle);
        return;
    } else if (err < 0) {
        error_msg("  Path Error");
        rom_list = NULL;
        Bfile_FindClose(FindHandle);
        return;
    } else {
        Bfile_NameToStr_ncpy(rom_list[i], FoundFile, 0x10);
        i++;
        (*len)++;
    }

    while (1){
        int err = Bfile_FindNext(FindHandle, FoundFile, &fileinfo);
        if (err == -16){// no more files
            Bfile_FindClose(FindHandle);
            return;
        } else if (err < 0){
            error_msg("  path ERROR");
            return;
        }
        if (err == 0){
            Bfile_NameToStr_ncpy(rom_list[i], FoundFile, 0x10);
            i++;
            (*len)++;
        }
    }
    Bfile_FindClose(FindHandle);
}