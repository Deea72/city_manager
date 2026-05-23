#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h> // for wait()

int main()
{
    char command_line[512];

    while (1)
    {
        printf("city_hub> ");
        fflush(stdout);
        if (fgets(command_line, sizeof(command_line), stdin) == NULL) {
            break;
        }

        command_line[strcspn(command_line, "\n")] = '\0';

        if (strcmp(command_line, "exit") == 0)
        {
            printf("Finished\n");
            break;
        }
        else
        if (strcmp(command_line, "start_monitor") == 0)
        {
            printf("Background monitor starts\n");

            int monitor_pipe[2];

            if (pipe(monitor_pipe) == -1)
            {
                perror("Error: Failed to create pipe");
                continue;
            }

            pid_t hub_mon_pid = fork();

            if (hub_mon_pid == -1)
            {
                perror("Error: Failed to fork hub_mon");
                close(monitor_pipe[0]);
                close(monitor_pipe[1]);
                continue;
            }

            if (hub_mon_pid == 0) {


                //printf("Inside the child process\n");
                close(monitor_pipe[0]);

                pid_t monitor_pid = fork();

                if (monitor_pid == -1)
                {
                    perror("Error: hub_mon failed to fork monitor grandchild");
                    exit(1);
                }

                if (monitor_pid == 0)
                {

                    //printf("Inside the grandchild process\n");
                    dup2(monitor_pipe[1], STDOUT_FILENO);

                    close(monitor_pipe[1]);

                    execlp("./monitor_reports", "./monitor_reports", NULL);

                    perror("Error: hub_mon failed to execute ./monitor_reports");
                    close(score_pipe[0]); // Closing the pipes
                    close(score_pipe[1]);
                    exit(1);
                }

                close(monitor_pipe[1]);

                exit(0);

            }
            else
            {
                close(monitor_pipe[1]);

                char read_buffer[512];
                ssize_t bytes_read;

                bytes_read = read(monitor_pipe[0], read_buffer, sizeof(read_buffer) - 1);

                if (bytes_read > 0)
                {
                    read_buffer[bytes_read] = '\0';
                    printf("%s", read_buffer);
                    fflush(stdout);

                    if (strstr(read_buffer, "Error") != NULL)
                    {
                        printf("The background monitor has ended because another monitor is already running.\n");
                    }
                    else
                    {
                        printf("Monitor is running in the background.\n");
                    }
                }

                close(monitor_pipe[0]);
            }
        }
        else
        if (strncmp(command_line, "calculate_scores ", 17) == 0)   //calculate_scores has 17 char
        {

            char *args = command_line + 17;
            char *districts[50];
            int d_count = 0;

            char *p = strtok(args, " ");
            while (p != NULL && d_count < 50) {
                districts[d_count++] = p;
                p = strtok(NULL, " ");
            }

            if (d_count == 0) {
                printf("Error: Give a list of districts.\n");
                fflush(stdout);
                continue;
            }

            for (int i = 0; i < d_count; i++)
            {
                int score_pipe[2];
                if (pipe(score_pipe) == -1) {
                    perror("Error: Failed to create scorer pipeline");
                    continue;
                }

                pid_t score_pid = fork();

                if (score_pid == -1)
                {
                    perror("Error: Failed to fork scorer process");
                    close(score_pipe[0]); // Closing the pipes
                    close(score_pipe[1]);
                    continue;
                }

                if (score_pid == 0)
                {
                    close(score_pipe[0]); // Close unused read end

                    dup2(score_pipe[1], STDOUT_FILENO);
                    close(score_pipe[1]);

                    execlp("./scorer", "./scorer", districts[i], NULL);

                    perror("Error: Failed to execute external scorer binary");
                    close(score_pipe[0]); //CLosing the pipes
                    close(score_pipe[1]);
                    exit(1);
                }
                else
                {
                    close(score_pipe[1]);

                    char read_buffer[512];
                    ssize_t bytes_read;

                    // Read the text coming back from the scorer
                    while ((bytes_read = read(score_pipe[0], read_buffer, sizeof(read_buffer) - 1)) > 0) {
                        read_buffer[bytes_read] = '\0';
                        printf("%s", read_buffer);
                        fflush(stdout);
                    }

                    close(score_pipe[0]);
                    wait(NULL); // Wait for the child to finish
                }
            }


        }
        else
        if (strlen(command_line) > 0)
        {
            printf("Unknown Command\n");
        }
    }

    return 0;
}
