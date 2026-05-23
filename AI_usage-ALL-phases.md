Phase 1 usage
-For the AI-Assisted Condition Matching I used: Gemini

For the parse_condition function was given this promt:
"
i have to make a  c program  in the unix environment implementing a city issue reporting system, and in this program i work with records, each record containing at least:
Report ID (integer)
Inspector name (fixed-length string, provided as a --user argument)
GPS coordinates (latitude and longitude as floating-point numbers)
Issue category (fixed-length string, e.g. "road", "lighting", "flooding")
Severity level (integer: 1 = minor, 2 = moderate, 3 = critical)
Timestamp (time_t)
Description text (fixed-length string)
I want you to generate a function: int parse_condition(const char *input, char field, char *op, char *value) which splits a "field:operator:value string into its three parts
"
The AI generated a function assuming that the filed parameter will be a character (e.g., 's' for Severity, 'c' for Category) and for the op reads only to 2 character that are not a ":"

For the match_condition the promt was:
"
the fields from the record are a different type each, as you have seen in my previous message, now I want you to generate a function:  int match_condition(Report *r, const char *field, const char *op, const char *value);
which returns 1 if the record satisfies the condition and 0 otherwise.
"
The AI made a struct for the report and function codifing each filed as a letter and using a swich( for letter "l" the AI made a case for latitude only ), i wanted to modify the function a little and I gave this promt:

"
i want you to modify the function :
this is the structure of the record
typedef struct
{
    int id;   //report id
    char nume[20];  //inspercotr name
    float latitude, longitude;  //gps cords
    char issue[10]; //road, lighting, flooding
    int severity;  //1 = minor, 2 = moderate, 3 = critical
    time_t Timestamp;
    char Description[50];
} REPORT;
and this are the
Supported fields: severity, category, inspector, timestamp.
 Supported operators: ==, !=, <, <=, >, >=.
 "

the AI modified the function accordingly to my instructions.

-Besides the filter function AI was used for symlinks, to be more specific : how to make them and how to keep them updated.

Phase 2 usage
AI was used so the program can only end when it receives SIGINT ()

Phase 3 usage
AI was used
-to make sure that the pipes works okay. AI suggestedusing fflush(stdout)
-to calculete scores for unique users
