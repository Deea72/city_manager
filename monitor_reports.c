#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <fcntl.h>  //O_RDWR
#include <string.h>

volatile sig_atomic_t got_newrep = 0;

void sigusr1_handler ( int sig)
{
    got_newrep = 1;
}

volatile sig_atomic_t got_sigint = 0;

void sigint_handler(int sig)
{
    got_sigint = 1;
}

int main()
{

    int f_check = open(".monitor_pid", O_RDONLY);
    if (f_check != -1)  //if i can open it means that is old
    {
        char old_pid_str[32] = {0};
        read(f_check, old_pid_str, sizeof(old_pid_str) - 1);
        close(f_check);

        char err_msg[128];
        snprintf(err_msg, sizeof(err_msg), "Error : Another monitor is already active with PID: %s\n", old_pid_str);

        write(STDOUT_FILENO, err_msg, strlen(err_msg));

        exit(1);
    }

    //if it does not exist we make it
    int f_mpid = open(".monitor_pid", O_WRONLY | O_CREAT | O_TRUNC, 0664);
    if(f_mpid == -1)
    {
        perror("Error creating .monitor_pid file\n");
        exit(-1);
    }

    pid_t main_pid = getpid();
    char pid_str[32];
    int len = snprintf(pid_str, sizeof(pid_str), "%d", main_pid);
    write(f_mpid, pid_str, len);
    close(f_mpid);

    struct sigaction sa_sigint;                      //for sigint
    sa_sigint.sa_handler = sigint_handler;
    sigemptyset(&sa_sigint.sa_mask);
    sa_sigint.sa_flags = 0;

    struct sigaction sa_sigusr1;                     //for sigusr1
    sa_sigusr1.sa_handler = sigusr1_handler;
    sigemptyset(&sa_sigusr1.sa_mask);
    sa_sigusr1.sa_flags = 0;

    if (sigaction(SIGINT, &sa_sigint, NULL) == -1) {
        perror("Error setting up SIGINT");
        exit(-1);
    }

    if (sigaction(SIGUSR1, &sa_sigusr1, NULL) == -1) {
        perror("Error setting up SIGUSR1");
        exit(-1);
    }

    printf("Monitor process started with PID: %d\n", main_pid);
    fflush(stdout);

    while (got_sigint == 0) {

        if (got_newrep == 1) {
            printf("Notification: A new report has been added to a district!\n");
            fflush(stdout);
            got_newrep = 0; // Reset the flag
        }
        sleep(1);
    }

    printf("\nSIGINT received\n");

    // Second, use unlink to safely delete the file (or symlink)
    if (unlink(".monitor_pid") == 0) {
        printf("Successfully deleted .monitor.pid\n");
    } else {
        perror("Error deleting file during cleanup");
        exit(-1);
    }

    printf("Cleanup complete\n");

}
