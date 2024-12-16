#define _GNU_SOURCE

#include<stdio.h>
#include<stdlib.h>
#include <unistd.h>
#include <sched.h>
#include <signal.h>

int child_function(void *args) {
    int onemb = 1 << 20;
    char *stack = malloc(onemb * 4);

    if (!stack) {
        printf("Error while allocating 2mb");
        perror("malloc");
    }

    printf("Child process: %s\n",(char *)args);
    return 0;
}

int main() {
    int onemb = 1 << 20;

    char *stack = malloc(onemb);

    if (!stack) {
        perror("malloc");
        return -1;
    }

    char *message = "Helo world";

    int child = clone(child_function, stack + onemb,SIGCHLD, message);

    if (child == -1) {
        perror("clone");
        exit(1);
    }

    printf("Parent process\n");
    // sleep(0); // Allow the child to complete
    free(stack);
    return 1;
}
