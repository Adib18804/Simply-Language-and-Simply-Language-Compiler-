#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int main() {
    int i = 0;
    double __rc = 0;
    double t0 = 0;
    double t1 = 0;
    double t2 = 0;
    i = 0;
    __rc = 5;
  L0:
    t0 = 0 < __rc;
    if (!(t0)) goto L1;
    t1 = i + 1;
    i = t1;
    printf("%g\n", (double)(i));
    t2 = __rc - 1;
    __rc = t2;
    goto L0;
  L1:
    return 0;
}
