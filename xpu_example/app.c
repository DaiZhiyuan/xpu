#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <time.h>

#include "arithmetic.h"

int main(int argc, char **argv)
{
    unsigned int op1, op2;

    srand((unsigned int)time(NULL));
    op1 = rand() % 100 + 1;
    op2 = rand() % 100 + 1;

    printf("This example demonstrates the use of the [arithmetic] library.\n");
    printf("[op1]  [action]  [op2]  [result]\n");
    printf("--------------------------------\n");
    printf("  %d      ADD      %d      %d\n", op1, op2, add(op1, op2));
    printf("  %d      MUL      %d      %d\n", op1, op2, mul(op1, op2));

    return 0;
}
