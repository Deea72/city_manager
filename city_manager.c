#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <fcntl.h> //open
#include <unistd.h>   // For write(), close(), symlink()
#include <sys/stat.h> // For chmod()
#include <sys/wait.h> // for wait()
#include <signal.h>

//inspector    doua roluri
//manager

//A district_id directory has
// --> report (reports.dat )
typedef struct
{
    int id;   //report id
    char nume[20];  //inspercotr name
    float latitude, longitude;  //gps cords
    char issue[10]; //road, lighting, flooding
    int severity;  //1 = minor, 2 = moderate, 3 = critical
    time_t Timestamp;
    char Description[50];
} REPORT;

int nrreports = 0;

// --> configuration file (district_id.cfg)
// storing at least a severity threshold =
// the minimum level that should trigger an escalation alert

//-->operation log (logged_district)
// recording every action performed on that district_id,
// with timestamp, declared role, and declared user name

void list_report(REPORT r)
{
    printf("Report_id: %d\n", r.id);
    printf ("Inspector name %s\n", r.nume);
    printf("GPS: -latitude: %f  -longitude: %f\n" , r.latitude, r.longitude);
    printf("Issue category: %s\n", r.issue);
    printf("Severity level: %d\n", r.severity);
    printf("Timestamp: %s", ctime(&r.Timestamp));  //time already has \n
    printf("Description: %s\n", r.Description);
    printf("\n");
}

/**
 * Splits a condition string (e.g., "s:>2") into its components.
 * 
 * param input The raw string to parse.
 * param field Pointer to store the field character.
 * param op    Pointer to store the operator string (should be size 3 to be safe).
 * param value Pointer to store the value string.
 * return      1 on success, 0 on failure.
 */
int parse_condition(const char *input, char *field, char *op, char *value) {
    if (input == NULL || field == NULL || op == NULL || value == NULL) {
        return 0;
    }

    // sscanf logic:
    // %c      : reads the first character (the field)
    // :       : matches the literal colon
    // %2[^:]  : reads up to 2 characters that are NOT a colon (the operator)
    // :       : matches the second literal colon
    // %s      : reads the remaining string (the value)
    
    int items = sscanf(input, "%c:%2[^:]:%s", field, op, value);

    // We expect exactly 3 items to be successfully matched
    if (items == 3) {
        return 1;
    }

    return 0;
}


int match_condition(REPORT *r, char field, const char *op, const char *value) {
    if (!r || !op || !value) return 0;

    switch (field) {
        // --- Severity (Integer) ---
        case 's': {
            int val = atoi(value);
            if (strcmp(op, "==") == 0) return r->severity == val;
            if (strcmp(op, "!=") == 0) return r->severity != val;
            if (strcmp(op, "<")  == 0) return r->severity <  val;
            if (strcmp(op, "<=") == 0) return r->severity <= val;
            if (strcmp(op, ">")  == 0) return r->severity >  val;
            if (strcmp(op, ">=") == 0) return r->severity >= val;
            break;
        }

        // --- Category/Issue (String) ---
        case 'c': {
            int cmp = strcmp(r->issue, value);
            if (strcmp(op, "==") == 0) return cmp == 0;
            if (strcmp(op, "!=") == 0) return cmp != 0;
            if (strcmp(op, "<")  == 0) return cmp <  0;
            if (strcmp(op, "<=") == 0) return cmp <= 0;
            if (strcmp(op, ">")  == 0) return cmp >  0;
            if (strcmp(op, ">=") == 0) return cmp >= 0;
            break;
        }

        // --- Inspector Name (String) ---
        case 'n': {
            int cmp = strcmp(r->nume, value);
            if (strcmp(op, "==") == 0) return cmp == 0;
            if (strcmp(op, "!=") == 0) return cmp != 0;
            if (strcmp(op, "<")  == 0) return cmp <  0;
            if (strcmp(op, "<=") == 0) return cmp <= 0;
            if (strcmp(op, ">")  == 0) return cmp >  0;
            if (strcmp(op, ">=") == 0) return cmp >= 0;
            break;
        }

        // --- Timestamp (time_t) ---
        case 't': {
            time_t val = (time_t)atoll(value);
            double diff = difftime(r->Timestamp, val);
            if (strcmp(op, "==") == 0) return diff == 0;
            if (strcmp(op, "!=") == 0) return diff != 0;
            if (strcmp(op, "<")  == 0) return diff <  0;
            if (strcmp(op, "<=") == 0) return diff <= 0;
            if (strcmp(op, ">")  == 0) return diff >  0;
            if (strcmp(op, ">=") == 0) return diff >= 0;
            break;
        }

        default:
            return 0;
    }
    return 0;
}

void filer(char *district_id, char *condition)
{
    char reports_path[256] ="";
    snprintf(reports_path, sizeof(reports_path),"%s/reports.dat", district_id);

    int freports = open(reports_path, O_RDWR);
    if (freports == -1) {
        printf("Error: Could not open reports file.\n");
        return;
    }

    char field;
    char o[3];
    char v[50];

    REPORT r;
    while (read(freports, &r, sizeof(REPORT)) == sizeof(REPORT))
    {
         if(parse_condition( condition, &field, o, v))
        {
           if((match_condition(&r, field, o, v))) // 1 for true condition
           {
                list_report(r);
           }
        }
    }

    close(freports);


}


void update_threshold(char *district_id, int new_threshold)
{
    char threshold_path[256] ="";
    snprintf(threshold_path, sizeof(threshold_path), "%s/district_id.cfg", district_id);

    struct stat st;

    if (stat(threshold_path, &st) == -1) {
        printf("Error: Could not access %s\n", threshold_path);
        return;
    }

    if ((st.st_mode & 0777) != 0640) {
        printf("Error: Can't update the threshold !  Prmissions for district_id.cfg are not 640.\n");
        return;
    }

    int f_cfg = open(threshold_path, O_WRONLY | O_TRUNC);
    if (f_cfg == -1) {
        printf("Error: Could not open %s for writing.\n", district_id);
        return;
    }

    char buffer[50];
    int length = snprintf(buffer, sizeof(buffer), "threshold=%d\n", new_threshold);

    write(f_cfg, buffer, length);

    printf("Severity threshold successfully updated to %d.\n", new_threshold);

    close(f_cfg);
}


void remove_district(char *district_id)
{
    char link_name[256] = "";
    snprintf(link_name, sizeof(link_name), "active_reports-%s", district_id);

    struct stat lst;
    if (lstat(link_name, &lst) == 0)
    {
        if (S_ISLNK(lst.st_mode))
        {

            unlink(link_name);
            printf("Old symlink removed.\n");   //delete the old ones to be up to date
        }
    }

    int pid;
    if( ( pid=fork() ) < 0)
    {
        perror("Error child process\n");
        exit(1);
    }
    if(pid==0)
    {

        //rm -rf <district_id_directory>
        execlp("rm", "rm", "-rf", district_id , NULL);
        printf("Codul a esuat\n");
        exit(0); // apel necesar pentru a se opri codul fiului astfel incat acesta sa nu execute si codul parintelui
    }

    int status;
    wait(&status);
}


void remove_report(char *district_id, int target_id)
 {
    char reports_path[256] ="";
    snprintf(reports_path, sizeof(reports_path), "%s/reports.dat", district_id);

    int f_rep = open(reports_path, O_RDWR);
    if (f_rep == -1) {
        printf("Error: Could not open reports file.\n");
        return;
    }

    //Find the report
    REPORT r;
    off_t target_pos = -1; //store the exact byte location of the report
    while (read(f_rep, &r, sizeof(REPORT)) == sizeof(REPORT)) {
        if (r.id == target_id) {
            target_pos = lseek(f_rep, 0, SEEK_CUR) - sizeof(REPORT);  //the cursor is after the report so target_pos si the start of the report
            break;  //report found
        }
    }

    //report not found
    if (target_pos == -1) {
        printf("Report %d not found.\n", target_id);
        close(f_rep);
        return;
    }

    off_t write_pos = target_pos;                 // The position we are overwriting
    off_t read_pos = target_pos + sizeof(REPORT); // The report AFTER the deleted one

    while (1) {

        lseek(f_rep, read_pos, SEEK_SET);
        if (read(f_rep, &r, sizeof(REPORT)) <= 0) {
            break; // We reached the end of the file
        }

        // Go back and overwrite the old position
        lseek(f_rep, write_pos, SEEK_SET);
        write(f_rep, &r, sizeof(REPORT));

        // Move our trackers forward by one struct size
        write_pos += sizeof(REPORT);
        read_pos += sizeof(REPORT);

    }

    //Cutting the last report
    ftruncate(f_rep, write_pos);
    printf("Report %d successfully removed.\n", target_id);

    close(f_rep);
}


void view(char *district_id, int target_id)
{
    char filepath[100];

    snprintf(filepath, sizeof(filepath), "%s/reports.dat", district_id);

    int fd = open(filepath, O_RDONLY);
    if (fd == -1) {
        printf("Error: Could not open reports file for district_id '%s'.\n", district_id);
        return;
    }

    REPORT r;
    int found = 0; // 1 - if we found the report
    while (read(fd, &r, sizeof(REPORT)) == sizeof(REPORT))
    {
        if (r.id == target_id) {
            printf("Report\n");
            list_report(r);
            found = 1;
            break; //report found
        }
    }

    //report not found
    if (found == 0) {
        printf("Report ID %d not found in district_id '%s'.\n", target_id, district_id);
    }


    close(fd);
}

void print_permissions(mode_t mode) {

    char perms[10] = "---------";

    //Owner permissions
    if (mode & S_IRUSR) perms[0] = 'r';
    if (mode & S_IWUSR) perms[1] = 'w';
    if (mode & S_IXUSR) perms[2] = 'x';

    //Group permissions
    if (mode & S_IRGRP) perms[3] = 'r';
    if (mode & S_IWGRP) perms[4] = 'w';
    if (mode & S_IXGRP) perms[5] = 'x';

    //Others permissions
    if (mode & S_IROTH) perms[6] = 'r';
    if (mode & S_IWOTH) perms[7] = 'w';
    if (mode & S_IXOTH) perms[8] = 'x';

    // Print the final string
    printf("%s", perms);
}






void list(char *district_id)
{
    char reports_path[256] ="";
    snprintf(reports_path, sizeof(reports_path), "%s/reports.dat", district_id);

    struct stat st;

    if (stat(reports_path, &st) == 0)
        print_permissions(st.st_mode);
    else
        printf("Error: Could not stat %s\n", reports_path);

    int fd = open(reports_path, O_RDONLY);
    if (fd == -1) {
        perror("Could not open reports.dat");
        return;
    }

    stat(reports_path, &st);
    printf("File size: %ld bytes | Last modified: %s", st.st_size, ctime(&st.st_mtime));

    REPORT r;
    while (read(fd, &r, sizeof(REPORT)) == sizeof(REPORT))
    {
        list_report(r);
    }

    close(fd);
}

int monitor_informed = 0; // o failed, 1 = success (Phase 2)
int f_log; //global so it can be accesed from add and main
struct stat st; //

void add(char *district_id, char *nume_user)  //append a new report
{
    REPORT nou;
    nou.id = rand()%1000;
    strcpy( nou.nume, nume_user);
    nou.latitude = 0;
    nou.longitude = 0;  //initialization on 0
    strcpy(nou.issue,"");
    nou.severity = rand()%3 + 1;
    strcpy(nou.Description,"");
    nou.Timestamp = time(NULL);



    struct stat st;

    if (stat(district_id, &st) == 0) {
        if (!S_ISDIR(st.st_mode))  //it s not a directory
        {
            printf("Error '%s' is a file not a directory\n", district_id);
            exit(-1);
        }
    }
    else   //if the directory does not exist we make it + district_id.cfg file + logged_district file
    {
        if (mkdir(district_id, 0777) == -1)
        {
            perror("Error mkdir!");
            exit(-1);
        }

        char path_cfg[256];
        snprintf(path_cfg, sizeof(path_cfg), "%s/district_id.cfg", district_id);

        int f_cfg = open(path_cfg, O_RDWR |O_CREAT, 0664);//district_id.cfg file  storing severity threshold
        if(f_cfg == -1)
        {
            perror("Error opening config.cnf\n");
            exit(-1);
        }


        int severity = rand()%3+1;
        write(f_cfg, &severity, sizeof(int));
        close(f_cfg);



    }


    char path_rep[256] ="";
    snprintf(path_rep, sizeof(path_rep), "%s/reports.dat", district_id);

    int f_reports = open(path_rep, O_RDWR | O_APPEND | O_CREAT, 0664);    //reports.dat file
    if (f_reports == -1)
    {
        perror("Error opening reports.dat\n");
        return;
    }

    write(f_reports, &nou, sizeof(REPORT));
    close(f_reports);

    char link_name[256] = "";
    snprintf(link_name, sizeof(link_name), "active_reports-%s", district_id);

    struct stat lst;
    if (lstat(link_name, &lst) == 0)
    {
        if (S_ISLNK(lst.st_mode))
        {

            unlink(link_name);
            //printf("Old symlink removed.\n");   //delete the old ones to be up to date
        }
    }

    if (lstat(link_name, &lst) == -1)
    {
        if (symlink(path_rep, link_name) == -1) {
            perror("Warning: Could not create symbolic link");
        }
    }

    //phase 2
    int f_pid = open(".monitor_pid", O_RDONLY);
    if(f_pid != -1)
    {
        char pid_buffer[32];
        ssize_t bytes_read = read(f_pid, pid_buffer, sizeof(pid_buffer) - 1);

        if (bytes_read > 0)
        {
            pid_buffer[bytes_read] = '\0';
            pid_t monitor_pid = (pid_t)atoi(pid_buffer);

            // kill(pid, signal) sends a signal to another process.
            if(monitor_pid > 0)
            {
                if (kill(monitor_pid, SIGUSR1) == 0)
                    monitor_informed = 1;
                else
                {
                    perror("FOund PID but could not send SIGUSR1\n");
                }
            }
        }
        close(f_pid);

        char monitor_inf[256];
        if(monitor_informed == 1)
        {
            snprintf( monitor_inf, sizeof(monitor_inf), "Monitor successfully notified\n");
        }
        else
        {
            snprintf( monitor_inf, sizeof(monitor_inf), "Failed to notify the monitor\n");

        }

        write(f_log, monitor_inf, strlen(monitor_inf));
    }
    else
    {
        printf("Notice: Monitor process is not currently active.\n");
    }






}



int main(int argc, char* argv[])
{
    srand(time(NULL));
    int role=0;  //r = 1 pentru inspector, r = 2 pentru manager
    char nume[20]="";  //user name
    //char district_id_id[20]= "";   //pentru functia de add

    if (argc < 5)
    {
        printf("Error: Not enough arguments provided.\n");
        return 1;
    }

    if(strcmp(argv[0], "./city_manager") == 0)  //suntem intr o comanda
    {
        if(strcmp(argv[1], "--role")==0)  //ne asteptam sa urmeze rolul
        {
            if (strcmp(argv[2], "inspector") == 0)
            {
                role=1;
            }

            if(strcmp(argv[2], "manager") == 0)
            {
                role=2;
            }
        }

        if (strcmp (argv[3], "--user") == 0)
        {
            strcpy(nume,argv[4]);
        }

        if(argv[5])
        {
            //writing in the log
            //----------------------
            char path_log[256] = "";
            snprintf(path_log, sizeof(path_log), "%s/logged_district.txt", argv[6]);


            f_log = open(path_log, O_RDWR | O_CREAT, 0664 );
            if(f_log == -1)
            {
                perror("Error opening loggg\n");
                exit(-1);
            }

            if (stat(path_log, &st) == -1)
            {
                printf("Error: Could not access %s\n", path_log);
                exit(-1);
            }
            //-----------------------------

            if(strcmp(argv[5], "--add") == 0)
            {
                if (argc >= 7)
                {
                    add(argv[6], nume);       //argv[6] is id district_id or district_id name
                } else
                {
                    perror("Error: Missing argument for --add\n");
                    exit(-1);
                }
            }

            if(strcmp(argv[5], "--list") == 0)
            {
                if (argc >= 7)
                {
                    list(argv[6]);
                } else
                {
                    perror("Error: Missing argument for --list\n");
                    exit(-1);
                }
            }

            if (strcmp(argv[5], "--view") == 0)
            {
                //argv[6] district_id id
                //argv[7] report id
                if (argc >= 8)
                {
                    int report_id = atoi(argv[7]);
                    view(argv[6], report_id);
                } else
                {
                    perror("Error: Missing argument for --view\n");
                    exit(-1);
                }

            }

            if(strcmp(argv[5], "--remove_report") == 0)
            {
                //argv[6] district_id id
                //argv[7] report id

                if (argc >= 8)
                {
                    if(role == 2)
                        remove_report(argv[6],atoi(argv[7]));
                    else
                        printf("YOU ARE NOT A MANAGER ! (you don't have persmision for this)\n");
                }
                else
                {
                    perror("Error: Missing argument for --view\n");
                    exit(-1);
                }



            }

            if(strcmp(argv[5], "--update_threshold") == 0)
            {
                //argv[6] district_id id
                //argv[7] value
                if (argc >= 8)
                {
                    if(role == 2)
                        update_threshold(argv[6],atoi(argv[7]));
                    else
                        printf("YOU ARE NOT A MANAGER ! (you don't have persmission for this)\n");
                }
                else
                {
                    perror("Error: Missing argument for --update_threshold\n");
                    exit(-1);
                }
            }

            if(strcmp(argv[5], "--filter") == 0)
            {
                if (argc >= 8) {
                    filer(argv[6], argv[7]); // argv[6] is district_id, argv[7] is condition
                } else {
                    printf("Error: --filter requires a district_id and a condition.\n");
                }
            }

            if (strcmp(argv[5], "--remove_district") == 0)   //PHASE 2
            {

                //argv[6]  district_id_id
                if (argc >= 7)
                {
                    if(role == 2)
                        remove_district(argv[6]);
                    else
                        printf("YOU ARE NOT A MANAGER ! (you don't have persmision for this)\n");
                }

            }


            char operationlog[1024] = "";  //operation Time Role User
            snprintf(operationlog, sizeof(operationlog), "%s %s %s %s\n", argv[5], ctime(&st.st_mtime), argv[2], argv[4]);


            write(f_log, operationlog, strlen(operationlog));

            close(f_log);

        }



    }

    return 0;
}
