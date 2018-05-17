/*
Got inspired from https://stackoverflow.com/questions/19262851/what-is-the-rule-for-c-to-cast-between-short-and-int:


Anytime an integer type is being converted to a different integer type it falls to the classification in the standard known as the integer promotions and near all of them are defined (one of them by the implementation, but we'll get to that last; the spoiler is mentioned in general-comment above).

The general overview on value-qualification:

C99 6.3.1.1-p2

    If an int can represent all values of the original type (as restricted by the width, for a bit-field), the value is converted to an int; otherwise, it is converted to an unsigned int. These are called the integer promotions. All other types are unchanged by the integer promotions.

That said, lets look at your conversions. The signed-short to unsigned int is covered by the following, since the value being converted falls outside the unsigned int domain:

C99 6.3.1.3-p2

    Otherwise, if the new type is unsigned, the value is converted by repeatedly adding or subtracting one more than the maximum value that can be represented in the new type until the value is in the range of the new type.

Which basically means "add UINT_MAX+1". On your machine, UINT_MAX is 4294967295, therefore, this becomes

-1 + 4294967295 + 1 = 4294967295

Regarding your unsigned short to signed int conversion, that is covered by the regular value-quaified promotion. Specifically:

C99 6.3.1.3-p1

    When a value with integer type is converted to another integer type other than _Bool, if the value can be represented by the new type, it is unchanged.

In other words, because the value of your unsigned short falls within the coverable domain of signed int, there is nothing special done and the value is simply saved.

And finally, as mentioned in general-comment above, something special happens to your declaration of b

signed short b = 0xFFFF;

The 0xFFFF in this case is a signed integer. The decimal value is 65535. However, that value is not representable by a signed short so yet-another conversion happens, one that perhaps you weren't aware of:

C99 6.3.1.3-p3

    Otherwise, the new type is signed and the value cannot be represented in it; either the result is implementation-defined or an implementation-defined signal is raised.

In other words, your implementation chose to store it as (-1), but you cannot rely on that on a different implementation.

*/

#include <stdio.h>

int main() {
    short s = -1;
    //unsigned int u32Val = (unsigned int)s;
    //unsigned int u32Val = (unsigned)s;
    unsigned int u32Val = (unsigned short)s;

    printf("s = 0x%04hx\n", s);
    printf("u32Val = 0x%08x\n", u32Val);

    return 0;
}
