#include "utility.h"

char *to_string(int s) {
    static char buffer[20];
    int i = 0;
    int sign = 1;
    
    if(s < 0) {
        sign = -1;
        s = -s;
    }
    
    if(s == 0) {
        buffer[0] = '0';
        buffer[1] = '\0';
        return buffer;
    }
    
    while(s > 0) {
        buffer[i] = (s % 10) + '0';
        s = s / 10;
        i++;
    }
    
    if(sign == -1) {
        buffer[i] = '-';
        i++;
    }
    
    buffer[i] = '\0';
    
    int start = 0;
    int end = i - 1;
    while(start < end) {
        char temp = buffer[start];
        buffer[start] = buffer[end];
        buffer[end] = temp;
        start++;
        end--;
    }
    
    return buffer;
}