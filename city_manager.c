#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct REPORT {
    char district_id[20];  //nume dupa add (ex add downtown) practic district_id

    int Report_id;
    char Inspector_name[50];
    float latitude;    //GPS coordinates
    float longitude;
    char issue_category[50];  //road lighting flooding
    int severity_level; //1= minor 2=moderate 3=critical
    time_t timestamp;
}REPORT;


int main (int argc, char *argv[])
{
    char role[20]= "";
    for (int i = 1; i < argc; i++)
    {
        if (strcmp(argv[i],"--role"))
            strcpy(role, argv[i+1]);

        if (strcmp(argv[i],"--add"))
        {
            REPORT *report= (REPORT*)malloc(sizeof(REPORT));
            if (strcmp(argv[i-2], "--user"))
                strcpy(report->Inspector_name, argv[i-1]);
            else
                strcpy(report->Inspector_name, "Inspector unknown");
            if (argv[i+1] !=NULL)
            {
                strcpy(report -> district_id, argv[i+1]);
            }
            else
            {
                perror("Comanda invalida, da ti un district id\n");
                exit(-1);

            }
    }



    }


}
