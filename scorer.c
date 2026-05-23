#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <time.h>

typedef struct
{
    int id;   //report id
    char nume[20];  //inspector name
    float latitude, longitude;  //gps cords
    char issue[10]; //road, lighting, flooding
    int severity;  //1 = minor, 2 = moderate, 3 = critical
    time_t Timestamp;
    char Description[50];
} REPORT;

typedef struct {
    char name[20];
    int total_score;
} UserScore;

int main(int argc, char *argv[])
{
    if (argc < 2) {
        perror("Not enough arguments\n");
        exit(-1);
    }

    char *district_id = argv[1];
    char reports_path[256];
    snprintf(reports_path, sizeof(reports_path), "%s/reports.dat", district_id);

    // Open the district's binary file using low-level open()
    int f_rep = open(reports_path, O_RDONLY);
    if (f_rep == -1) {
        printf("Workload score report for district: %s ---\n", district_id);
        printf("  No reports filed in this district yet.\n");
        return 0;
    }

    UserScore scores[100];
    int unique_users = 0;
    REPORT r;

    while (read(f_rep, &r, sizeof(REPORT)) == sizeof(REPORT)) {
        int found = 0;

        for (int i = 0; i < unique_users; i++) {
            if (strcmp(scores[i].name, r.nume) == 0) {
                scores[i].total_score += r.severity; // Accumulate the severity levels
                found = 1;
                break;
            }
        }

        if (!found && unique_users < 100) {
            strcpy(scores[unique_users].name, r.nume);
            scores[unique_users].total_score = r.severity;
            unique_users++;
        }
    }
    close(f_rep);

    // Print the summary output that will be channeled through the pipe to city_hub
    printf("Workload score report for district: %s\n", district_id);
    for (int i = 0; i < unique_users; i++) {
        printf("  User: %s | Total Workload Score: %d\n", scores[i].name, scores[i].total_score);
    }

    return 0;
}
