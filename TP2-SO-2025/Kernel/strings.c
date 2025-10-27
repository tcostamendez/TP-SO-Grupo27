
#include "strings.h"
#include <stdint.h>
#include <stddef.h>

int strlen(const char *str) {
    int i = 0;
    while (str[i] != 0) {
		i++;
    }
    return i;
}

void my_strcpy(char *dest, const char *src) {
	int i = 0;
	while (src[i] != 0) {
		dest[i] = src[i];
		i++;
	}
	dest[i] = 0;
}

char * num_to_str(uint64_t num){
    if(num == 0){
        char * edge = mm_alloc(2);
		if(edge == NULL){
			return NULL;
		}
        edge[0] = '0';
        edge[1] = 0;
        return edge;
    }
    int len = 0;
    uint64_t aux = num;
    while(aux > 0){
        aux /= 10;
        len++;
    }
    len++; //para 0 del final
    char * ans = mm_alloc(len * sizeof(char));
    int i = 0;
    while(num > 0){
        aux = num%10;
        ans[len-i-2] = '0' + aux;
        num /= 10;
        i++;
    }
    ans[len-1] = 0;
    return ans;
}

void catenate(char * dest, const char * src){
	int i, j;
	i = j = 0;
	while(dest[i] != '\0'){
		i++;
	}
	while(src[j] != '\0'){
		dest[i++] = src[j++]; 
	}
	dest[i] = '\0';
}