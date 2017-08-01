#ifndef TEST_H
#define TEST_H

#include <cstdint>
#include <string.h>

#define PASS 0
#define FAIL -1

void initRand();
int32_t randPar(int32_t limit);
int32_t SumRedOfFirstXnumbers(uint32_t numbers, uint32_t start);
void eatRand(int times);

int RunAll(bool stress);



// Alex: adding these for debugging purposes in the Opincaa simulator
/*
void PrintDebugString(char *str) {
    for (int i = 0; i < strlen(str); i+=2) {
        PRINTCHARS((str[i + 1] << 8) + str[i]);
    }
}
*/
#define PrintDebugMessage(aStr) \
    for (int idxPrint = 0; idxPrint < strlen(aStr); idxPrint += 2) { \
        if (aStr[idxPrint + 1] == 0) { \
            PRINTCHARS((' ' << 8) | aStr[idxPrint]); \
        } \
        else { \
            PRINTCHARS((aStr[idxPrint] << 8) | (aStr[idxPrint + 1] == 0 ? '\r' : aStr[idxPrint + 1])); \
        } \
    }
    //PRINTCHARS((aStr[idxPrint] << 8) | (aStr[idxPrint + 1] == 0 ? '\r' : aStr[idxPrint + 1]));

// I use the stringizing operator - see info at https://msdn.microsoft.com/en-us/library/09dwwt6y.aspx
#define PrintRegDebug(regIdx) \
    PrintDebugMessage("Print reg " #regIdx ":\n"); \
    PRINTREG(regIdx);


#endif // TEST_H
