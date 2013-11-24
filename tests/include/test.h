
#ifndef TEST_H
#define TEST_H

#include <cstdint>

#define PASS 0
#define FAIL -1

void initRand();
int32_t randPar(int32_t limit);
int32_t SumRedOfFirstXnumbers(uint32_t numbers, uint32_t start);
void eatRand(int times);

int RunAll(bool stress);

#endif // TEST_H


