#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int main() {
    int age = 0;
    double t0 = 0;
    age = 21;
    t0 = 1;
    if (!(t0)) goto L0;
    printf("%s\n", "Adult");
    goto L1;
  L0:
    printf("%s\n", "Minor");
  L1:
    return 0;
}
