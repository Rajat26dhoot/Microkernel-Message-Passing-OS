#include <stdio.h>
#include <stdlib.h>
#include "tests.h"

int main(int argc, char *argv[]) {
    printf("\n");
    printf("╔═══════════════════════════════════════════════════════════╗\n");
    printf("║    MICROKERNEL MESSAGE PASSING OS                         ║\n");
    printf("╚═══════════════════════════════════════════════════════════╝\n");

    if (argc > 1) {
        int test_num = atoi(argv[1]);
        switch(test_num) {
            case 1: run_test1(); break;
            case 2: run_test2(); break;
            case 3: run_test3(); break;
            case 4: run_test4(); break;
            case 5: run_test5(); break;
            default:
                printf("\nInvalid test number. Use 1-5\n");
                return 1;
        }
    } else {
        run_test1();
        run_test2();
        run_test3();
        run_test4();
        run_test5();
    }

    printf("\n");
    printf("╔═══════════════════════════════════════════════════════════╗\n");
    printf("║   ALL TESTS COMPLETED SUCCESSFULLY                        ║\n");
    printf("╚═══════════════════════════════════════════════════════════╝\n\n");

    return 0;
}
