#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

int main() {
    int score = 0;
    double t0 = 0;
    double t1 = 0;
    score = 85;
    t0 = 0;
    if (!(t0)) goto L0;
    printf("%s\n", "Grade: A");
    goto L1;
  L0:
    t1 = score >= 75;
    if (!(t1)) goto L2;
    printf("%s\n", "Grade: B");
    goto L3;
  L2:
    printf("%s\n", "Grade: C");
  L3:
  L1:
    return 0;
}
