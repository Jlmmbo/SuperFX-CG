#define ROM_FORMAT "\\\\fls0\\*.sfc"

void init_rom(char** rom_list, byte rom_index){
    char char_filename [100]= "\\\\fls0\\";
    strcat(char_filename, rom_list[rom_index]);

    Bfile_StrToName_ncpy(rom_file_name, char_filename, 49);
    int err = Bfile_OpenFile_OS(rom_file_name, READ, 0);
    if(err < 0) error_msg("erreur avec le path du ROM");
    else{
        rom_handle = err;
    }
}

void init_ram(){
    char char_filename[30]= "\\\\fls0\\SNES_ram.dat";
    Bfile_StrToName_ncpy(ram_file_name, char_filename, 29);
    size_t fsize = 65536;
    int err = Bfile_CreateEntry_OS(ram_file_name, CREATEMODE_FILE, &fsize);
    if(err < 0) error_msg("erreur d'allocation du RAM");
    int handle = Bfile_OpenFile_OS(ram_file_name, READ, 0);
    if(handle < 0) error_msg("erreur avec le path du RAM");
    // Bfile_CloseFile_OS(handle);
}

void fetch_ram_bank_fs(byte bank){
    bank &= 0x7F;//pour de-mirroirer
    rom_handle = Bfile_OpenFile_OS(ram_file_name, READ, 0);
    Bfile_ReadFile_OS(ram_handle, curr_ram_bank, 65536, bank * 0x8000);
    // Bfile_CloseFile_OS(ram_handle);
}

void write_ram_bank_fs(byte bank){
    bank &= 0x7F;//to un-mirror
    rom_handle = Bfile_OpenFile_OS(ram_file_name, WRITE, 0);
    Bfile_WriteFile_OS(ram_handle, curr_ram_bank, 65536);
    // Bfile_CloseFile_OS(ram_handle);
}

void get_rom_list_fs(char** rom_list, byte** rom_ptrs, byte* len){
    *len = 0;

    unsigned short* FoundFile = sys_malloc(64);
    int FindHandle;
    file_type_t fileinfo = {0, 0, 0, 0, 0, 0};

    unsigned short pathname[sizeof(ROM_FORMAT) * 2];
    Bfile_StrToName_ncpy(pathname, ROM_FORMAT, sizeof(ROM_FORMAT));

    int err = Bfile_FindFirst(pathname, &FindHandle, FoundFile, &fileinfo);//doesn't work with files larger than 2MB
    int i = 0;
    if (err == -16){
        error_msg(" pas de ROMs");
        rom_list = NULL;
        Bfile_FindClose(FindHandle);
        return;
    } else if (err == -1) {
        error_msg("mauvaise path");
        rom_list = NULL;
        Bfile_FindClose(FindHandle);
        return;
    } else if (err == -5) {
        error_msg("bad prefix");
        rom_list = NULL;
        Bfile_FindClose(FindHandle);
        return;
    } else if (err == -8) {
        error_msg("corrupted file?? Error");
        rom_list = NULL;
        Bfile_FindClose(FindHandle);
        return;
    } else if (err < 0) {
        error_msg("Path Error");
        error_msg(byte_to_str(err, tmp));
        rom_list = NULL;
        Bfile_FindClose(FindHandle);
        return;
    } else {
        Bfile_NameToStr_ncpy(rom_list[i], FoundFile, 0x10);
        rom_ptrs[i] = (byte*)fileinfo.address;
        i++;
        (*len)++;
    }

    while (1){
        int err = Bfile_FindNext(FindHandle, FoundFile, &fileinfo);
        if (err == -16){// no more files
            Bfile_FindClose(FindHandle);
            sys_free(FoundFile);
            return;
        } else if (err < 0){
            error_msg("path ERROR");
            sys_free(FoundFile);
            Bfile_FindClose(FindHandle);
            return;
        }
        if (err == 0){
            Bfile_NameToStr_ncpy(rom_list[i], FoundFile, 0x10);
            rom_ptrs[i] = (byte*)fileinfo.address;
            i++;
            (*len)++;
        }
    }
    sys_free(FoundFile);
    Bfile_FindClose(FindHandle);
}