#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <fcntl.h> //open
#include <unistd.h>   // For write(), close()
#include <sys/stat.h> // For chmod()

//inspector    doua roluri
//manager

//A district directory has
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

// --> configuration file (district.cfg)
// storing at least a severity threshold =
// the minimum level that should trigger an escalation alert

//-->operation log (logged_district)
// recording every action performed on that district,
// with timestamp, declared role, and declared user name


void update_threshold(char *district_path, int new_threshold)
{

    char reports_path[256] ="";
    strcpy(reports_path, district_path);
    strcat(reports_path, "/reports.dat");

    struct stat st;
    //int flag=0;  // 1 if perrmisions are 0640

    if (stat(district_path, &st) == -1) {
        printf("Error: Could not access %s\n", district_path);
        return;
    }

    if ((st.st_mode & 0777) != 0640) {
        printf("Error: Can't update the threshold !  Prmissions for district.cfg are not 640.\n");
        return;
    }

    int fd = open(district_path, O_WRONLY | O_TRUNC);
    if (fd == -1) {
        printf("Error: Could not open %s for writing.\n", district_path);
        return;
    }

    char buffer[50];
    int length = snprintf(buffer, sizeof(buffer), "threshold=%d\n", new_threshold);

    write(fd, buffer, length);

    printf("Severity threshold successfully updated to %d.\n", new_threshold);

    close(fd);
}


void remove_report(char *district_path, int target_id)
 {
    char reports_path[256] ="";
    strcpy(reports_path, district_path);
    strcat(reports_path, "/reports.dat");

    int fd = open(reports_path, O_RDWR);
    if (fd == -1) {
        printf("Error: Could not open reports file.\n");
        return;
    }

    //Find the report
    REPORT r;
    off_t target_pos = -1; //store the exact byte location of the report
    while (read(fd, &r, sizeof(REPORT)) == sizeof(REPORT)) {
        if (r.id == target_id) {
            target_pos = lseek(fd, 0, SEEK_CUR) - sizeof(REPORT);  //the cursor is after the report so target_pos si the start of the report
            break;  //report found
        }
    }

    //report not found
    if (target_pos == -1) {
        printf("Report %d not found.\n", target_id);
        close(fd);
        return;
    }

    off_t write_pos = target_pos;                 // The position we are overwriting
    off_t read_pos = target_pos + sizeof(REPORT); // The report AFTER the deleted one

    while (1) {

        lseek(fd, read_pos, SEEK_SET);
        if (read(fd, &r, sizeof(REPORT)) <= 0) {
            break; // We reached the end of the file
        }

        // Go back and overwrite the old position
        lseek(fd, write_pos, SEEK_SET);
        write(fd, &r, sizeof(REPORT));

        // Move our trackers forward by one struct size
        write_pos += sizeof(REPORT);
        read_pos += sizeof(REPORT);

    }

    //Cutting the last report
    ftruncate(fd, write_pos);
    printf("Report %d successfully removed.\n", target_id);

    close(fd);
}

void list_report(REPORT r)
{
    printf("Report_id: %d\n", r.id);
    printf ("Inspector name %s\n", r.nume);
    printf("GPS: -latitude: %f  -longitude: %f\n" , r.latitude, r.longitude);
    printf("Issue category: %s\n", r.issue);
    printf("Severity level: %d\n", r.severity);
    printf("Timestamp: %s\n", ctime(&r.Timestamp));
    printf("Description: %s\n", r.Description);
}


void view(char *district_path, int target_id)
{
    char filepath[100];

    snprintf(filepath, sizeof(filepath), "%s/reports.dat", district_path);

    int fd = open(filepath, O_RDONLY);
    if (fd == -1) {
        printf("Error: Could not open reports file for district '%s'.\n", district_path);
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
        printf("Report ID %d not found in district '%s'.\n", target_id, district_path);
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






void list(char *district_path)
{
    char reports_path[256] ="";
    strcpy(reports_path, district_path);
    strcat(reports_path, "/reports.dat");

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




void add(char *district, char *nume_user)  //append a new report
{
    srand(time(NULL));
    REPORT nou;
    nou.id = rand()%1000;
    strcpy( nou.nume, nume_user);
    nou.latitude = 0;
    nou.longitude = 0;  //initialization on 0
    strcpy(nou.issue,"");
    nou.severity = 0;
    strcpy(nou.Description,"");
    nou.Timestamp = time(NULL);



    struct stat st;

    if (stat(district, &st) == 0) {
        if (!S_ISDIR(st.st_mode))  //it s not a directory
        {
            printf("Error '%s' is a file not a directory\n", district);
            exit(-1);
        }
    }
    else   //if the directory does not exist we make it + district.cfg file + logged_district file
    {
        if (mkdir(district, 0777) == -1)
        {
            perror("Error mkdir!");
            exit(-1);
        }
        char path[256] ="";
        strcpy(path,district);
        strcat(path, "/district.cfg");

        int configuration = open(path, O_RDWR |O_CREAT, 0664);//district.cfg file  storing severity threshold
        if(configuration == -1)
        {
            perror("Error opening config.cnf\n");
            exit(-1);
        }

       int severity = 3;
        write(configuration, &severity, sizeof(int));
        close(configuration);



    }


    char path[256] ="";
    strcpy(path, district);
    strcat(path, "/reports.dat");

    int reports = open(path, O_RDWR | O_APPEND | O_CREAT, 0664);    //reports.dat file
    if (reports == -1)
    {
        perror("Error opening reports.dat\n");
        return;
    }

    write(reports, &nou, sizeof(REPORT));
    close(reports);


}



int main(int argc, char* argv[])
{
    int role=0;  //r = 1 pentru inspector, r = 2 pentru manager
    char nume[20]="";  //user name
    //char district_id[20]= "";   //pentru functia de add

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
            if(strcmp(argv[5], "--add") == 0)
            {
                if (argc >= 7)
                {
                    add(argv[6], nume);       //argv[6] is id district or district name
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
                //argv[6] district id
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
                //argv[6] district id
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
                //argv[6] district id
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
                printf("Trebuie implementat\n");
            }


        }

        char path[256] = "";
        strcpy(path, argv[6] );
        strcat(path, "/logged_district");
       // printf("%s", path);

        int log = open(path, O_RDWR | O_CREAT, 0664 );
        if(log == -1)
        {
            perror("Error opening loggg\n");
            exit(-1);
        }

        struct stat st;

        if (stat(path, &st) == -1)
        {
            printf("Error: Could not access %s\n", path);
            exit(-1);
        }

        char operationlog[1024] = "";  //operation Time Role User
        strcat(operationlog, argv[5]);
        strcat(operationlog," ");
        strcat(operationlog, ctime(&st.st_mtime));
        strcat(operationlog, " ");
        if(role == 1)
            strcat (operationlog, "inspector");
        else
            if(role==2)
                strcat(operationlog, "manager");
        strcat(operationlog, " ");
        strcat( operationlog, argv[4]);
        strcat(operationlog, "\n");

        write(log, operationlog, strlen(operationlog));

        close(log);

    }

    return 0;
}
