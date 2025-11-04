#include "stdio.h"

int block(int argc, char* argv[]){
    if(argc != 2){
        //error
        return 1;
    }
    int pid = 0;
    sscanf(argv[1], "%d", &pid);
    if(pid <= 0){
        //error
        return 1;
    }
    
}