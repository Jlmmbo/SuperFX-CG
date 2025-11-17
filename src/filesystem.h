/*typedef struct file_type_t{
  unsigned short id, type;
  unsigned long fsize, dsize;
  unsigned int property;
  unsigned long address;
}file_type_t;*/

void get_rom_list_fs(char ** rom_list){
    
    int FindHandle;
    char* FoundFile = NULL;
    file_type_t* fileinfo = NULL;
    
    int err = Bfile_FindFirst("\\\\fls0\\*.snb", &FindHandle, FoundFile, fileinfo);//.snb -> Super Nintendo Broken-up (it will use a .exe to split into multiple files to bypass the 2mb file limit)
    if (err == -16){
        PrintXY(1, 1, "No Roms", 0, TEXT_COLOR_BLACK);
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
    }

    Bfile_FindClose(FindHandle);
}