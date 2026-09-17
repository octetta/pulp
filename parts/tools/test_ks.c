#include <stdio.h>
#include "../vendor/ksynth/ksynth.h"
int main() {
    ks_ctx *ctx = ks_create(64*1024*1024, 1000000000, 44100.0);
    K res1 = ks_eval(ctx, "X: !4096 % 4096", 15);
    K res2 = ks_eval(ctx, "X", 1);
    printf("res1=%d, res2=%d\n", res1->n, res2->n);
    return 0;
}
