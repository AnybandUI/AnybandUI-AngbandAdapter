#include "frontend.h"
#include <stdio.h>
int main(int argc, char **argv)
{
    if (argc != 3) return 1;
    if (frontend_run(argv[1], NULL, 0, NULL) != 72) return 2;
    if (frontend_run(argv[2], NULL, 0, NULL) != 72) return 3;
    if (frontend_run(NULL, NULL, 0, NULL) != 70) return 4;
    puts("PASS: incompatible build and table rejected before run; missing path rejected");
    return 0;
}
