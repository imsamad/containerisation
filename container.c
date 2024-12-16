#define _GNU_SOURCE
#include <sched.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/mount.h>
#include <string.h>
#include <errno.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <fcntl.h>

#define STACK_SIZE (1024 * 1024) // Stack size for child process

// Function to create a Go Fibonacci script
void create_go_script() {
    const char *script_path = "/tmp/fibonacci.go";
    const char *script_content =
        "package main\n"
        "import \"fmt\"\n"
        "func fibonacci(n int) int {\n"
        "    if n <= 1 {\n"
        "        return n\n"
        "    }\n"
        "    return fibonacci(n-1) + fibonacci(n-2)\n"
        "}\n"
        "func main() {\n"
        "    fmt.Println(\"Fibonacci(10):\", fibonacci(10))\n"
        "}";

    int fd = open(script_path, O_WRONLY | O_CREAT | O_TRUNC, 0644);
    if (fd < 0) {
        perror("open");
        exit(1);
    }
    if (write(fd, script_content, strlen(script_content)) < 0) {
        perror("write");
        close(fd);
        exit(1);
    }
    close(fd);
    printf("Go script for Fibonacci created at %s\n", script_path);
}

// Function to set up Go runtime inside the container
void setup_go_runtime() {
    const char *go_install_script = "/tmp/go_install.sh";
    const char *install_content =
        "#!/bin/bash\n"
        "GO_VERSION=1.19.3\n"
        "wget https://golang.org/dl/go$GO_VERSION.linux-amd64.tar.gz\n"
        "tar -C /usr/local -xvzf go$GO_VERSION.linux-amd64.tar.gz\n"
        "export PATH=$PATH:/usr/local/go/bin\n"
        "echo \"Go runtime installed\"\n";

    // Write Go install script
    int fd = open(go_install_script, O_WRONLY | O_CREAT | O_TRUNC, 0755);
    if (fd < 0) {
        perror("open");
        exit(1);
    }
    if (write(fd, install_content, strlen(install_content)) < 0) {
        perror("write");
        close(fd);
        exit(1);
    }
    close(fd);

    // Execute Go install script
    const char *const args[] = {"/bin/bash", go_install_script, NULL};
    execvp("/bin/bash", args);

    // If execvp fails
    perror("execvp");
    exit(1);
}

// Function executed inside the new namespaces
int child_function(void *arg) {
    printf("Inside the container (namespaces)\n");

    // Set a new hostname in the UTS namespace
    if (sethostname("container-host", strlen("container-host")) != 0) {
        perror("sethostname");
        return 1;
    }
    printf("Hostname set to 'container-host'\n");

    // Mount a new /proc filesystem
    if (mount("proc", "/proc", "proc", 0, "") != 0) {
        perror("mount");
        return 1;
    }
    printf("/proc mounted inside the namespace\n");

    // Set up Go runtime inside the container
    setup_go_runtime();

    // Create Go Fibonacci script
    create_go_script();

    // Run the Go script using the Go runtime inside the container
    char *const args[] = {"go", "run", "/tmp/fibonacci.go", NULL};
    execvp("go", args);

    // If execvp fails
    perror("execvp");
    return 1;
}

int main() {
    char *stack;                    // Stack for child process
    char *stack_top;                // Top of the stack
    pid_t child_pid;

    // Allocate memory for the child's stack
    stack = malloc(STACK_SIZE);
    if (!stack) {
        perror("malloc");
        exit(1);
    }
    stack_top = stack + STACK_SIZE; // Stack grows downward

    printf("Starting container with PID, UTS, and Mount namespaces\n");

    // Create a new process in new namespaces
    child_pid = clone(child_function, stack_top,
                      CLONE_NEWPID | CLONE_NEWUTS | CLONE_NEWNS | SIGCHLD, NULL);

    if (child_pid == -1) {
        perror("clone");
        exit(1);
    }

    printf("Child process created with PID: %d\n", child_pid);

    // Wait for the child process to exit
    if (waitpid(child_pid, NULL, 0) == -1) {
        perror("waitpid");
        exit(1);
    }

    printf("Container process exited\n");

    // Free the stack
    free(stack);
    return 0;
}
