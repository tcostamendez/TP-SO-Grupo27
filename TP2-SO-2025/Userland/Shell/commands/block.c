#include "stdio.h"
#include "libsys.h"
int _block(int argc, char* argv[]){
    if(argc != 2){
        perror("Usage: block <pid>");       
        return 1;
    }
    int pid = 0;
    sscanf(argv[1], "%d", &pid);
    if(pid <= 0){
        perror("Invalid pid");
        return 1;
    }
    blockProcess(pid);
    return 0;
}