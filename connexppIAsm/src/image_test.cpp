#include "../include/api/wrapper/Image.h"
#include <string.h>
#include <stdio.h>
int main()
{
    unsigned char *buffer = new unsigned char[10 * 10];
    for(int i=0; i<100; i++)
    {
        buffer[i] = i;
    }
    
    Image *image = new Image(10, 10, buffer, LUMA_BYTES_PER_PIXEL);
    unsigned char line[14];
    memset(line, 99, 14);
    
    image->padImageLeft(2);
    image->dumpToConsole();
    
    printf("\n");
    
    image->padImageRight(2);
    image->dumpToConsole();
    
    printf("\n");
    
    image->padImageTop(2);
    image->dumpToConsole();
    
    printf("\n");
    
    image->padImageBottom(2);
    image->dumpToConsole();
    
    printf("\n");
    
    image->setLine(0, line);
    image->setColumn(12, line);
    image->dumpToConsole();
    
    printf("\n");
    
    Image *slice = image->getSlice(3, 3, 10, 10);
    slice->dumpToConsole();
    
    printf("\n");
    
    Image *slice2 = image->getSlice(10, 10, 5, 5);
    slice2->dumpToConsole();
    
    return 0;
}