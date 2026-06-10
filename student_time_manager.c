#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <ctype.h>

#ifdef _WIN32
#include <windows.h>
#ifndef ENABLE_VIRTUAL_TERMINAL_PROCESSING
#define ENABLE_VIRTUAL_TERMINAL_PROCESSING 0x0004
#endif
void enable_ansi() {
    HANDLE hOut = GetStdHandle(STD_OUTPUT_HANDLE);
    if (hOut == INVALID_HANDLE_VALUE) return;
    DWORD dwMode = 0;
    if (!GetConsoleMode(hOut, &dwMode)) return;
    dwMode |= ENABLE_VIRTUAL_TERMINAL_PROCESSING;
    SetConsoleMode(hOut, dwMode);
}
#else
void enable_ansi() {}
#endif

// Max Limits
#define MAX_TASKS 100
#define MAX_SCHEDULE_BLOCKS 50
#define MAX_STUDY_LOGS 500
#define MAX_STR_LEN 100

// ANSI colors for styling
#define ANSI_COLOR_RED     "\033[1;31m"
#define ANSI_COLOR_GREEN   "\033[1;32m"
#define ANSI_COLOR_YELLOW  "\033[1;33m"
#define ANSI_COLOR_BLUE    "\033[1;34m"
#define ANSI_COLOR_MAGENTA "\033[1;35m"
#define ANSI_COLOR_CYAN    "\033[1;36m"
#define ANSI_COLOR_RESET   "\033[0m"
#define ANSI_BOLD          "\033[1m"

// Data Structures
typedef struct {
    int id;
    char title[MAX_STR_LEN];
    char course[MAX_STR_LEN];
    char due_date[MAX_STR_LEN]; // YYYY-MM-DD
    int priority;               // 0: LOW, 1: MEDIUM, 2: HIGH
    int status;                 // 0: PENDING, 1: IN_PROGRESS, 2: COMPLETED
} Task;

typedef struct {
    int day;                    // 0: Mon, 1: Tue, 2: Wed, 3: Thu, 4: Fri, 5: Sat, 6: Sun
    char start_time[MAX_STR_LEN]; // HH:MM
    char end_time[MAX_STR_LEN];   // HH:MM
    char course[MAX_STR_LEN];
    char location[MAX_STR_LEN];
} ScheduleBlock;

typedef struct {
    char course[MAX_STR_LEN];
    char date[MAX_STR_LEN];     // YYYY-MM-DD
    int duration_mins;
} StudyLog;

typedef struct {
    char course[MAX_STR_LEN];
    int total_mins;
} CourseTotal;

// Global States
Task tasks[MAX_TASKS];
int task_count = 0;

ScheduleBlock schedule[MAX_SCHEDULE_BLOCKS];
int schedule_count = 0;

StudyLog study_logs[MAX_STUDY_LOGS];
int study_log_count = 0;

// Function Declarations
void print_header(const char* title);
void get_today_date(char* date_str);
int validate_date(const char* date);
int validate_time(const char* time_str);
int case_insensitive_match(const char* s1, const char* s2);

void read_string(char* buffer, int max_len);
int read_int();

void split_line(char* line, char fields[][MAX_STR_LEN], int max_fields);
void load_tasks();
void save_tasks();
void load_schedule();
void save_schedule();
void load_study_logs();
void save_study_logs();
void load_all_data();

void display_dashboard();
void task_menu();
void schedule_menu();
void pomodoro_menu();
void display_statistics();

// Helper Functions
void print_header(const char* title) {
    printf("\n%s================================================================================%s\n", ANSI_COLOR_CYAN, ANSI_COLOR_RESET);
    printf("%s   %s%s\n", ANSI_BOLD, title, ANSI_COLOR_RESET);
    printf("%s================================================================================%s\n", ANSI_COLOR_CYAN, ANSI_COLOR_RESET);
}

void get_today_date(char* date_str) {
    time_t t = time(NULL);
    struct tm tm = *localtime(&t);
    sprintf(date_str, "%04d-%02d-%02d", tm.tm_year + 1900, tm.tm_mon + 1, tm.tm_mday);
}

int validate_date(const char* date) {
    if (strlen(date) != 10) return 0;
    if (date[4] != '-' || date[7] != '-') return 0;
    for (int i = 0; i < 10; i++) {
        if (i == 4 || i == 7) continue;
        if (date[i] < '0' || date[i] > '9') return 0;
    }
    int y, m, d;
    if (sscanf(date, "%d-%d-%d", &y, &m, &d) != 3) return 0;
    if (y < 2000 || y > 2100) return 0;
    if (m < 1 || m > 12) return 0;
    if (d < 1 || d > 31) return 0;
    return 1;
}

int validate_time(const char* time_str) {
    if (strlen(time_str) != 5) return 0;
    if (time_str[2] != ':') return 0;
    for (int i = 0; i < 5; i++) {
        if (i == 2) continue;
        if (time_str[i] < '0' || time_str[i] > '9') return 0;
    }
    int h, m;
    if (sscanf(time_str, "%d:%d", &h, &m) != 2) return 0;
    if (h < 0 || h > 23) return 0;
    if (m < 0 || m > 59) return 0;
    return 1;
}

int case_insensitive_match(const char* s1, const char* s2) {
    while (*s1 && *s2) {
        if (tolower((unsigned char)*s1) != tolower((unsigned char)*s2)) {
            return 0;
        }
        s1++;
        s2++;
    }
    return tolower((unsigned char)*s1) == tolower((unsigned char)*s2);
}

void read_string(char* buffer, int max_len) {
    if (fgets(buffer, max_len, stdin) != NULL) {
        buffer[strcspn(buffer, "\n")] = '\0';
        buffer[strcspn(buffer, "\r")] = '\0';
    } else {
        buffer[0] = '\0';
    }
}

int read_int() {
    char buf[32];
    read_string(buf, sizeof(buf));
    return atoi(buf);
}

void split_line(char* line, char fields[][MAX_STR_LEN], int max_fields) {
    int f = 0;
    char* start = line;
    char* p = line;
    while (*p && f < max_fields) {
        if (*p == '|') {
            *p = '\0';
            strncpy(fields[f], start, MAX_STR_LEN - 1);
            fields[f][MAX_STR_LEN - 1] = '\0';
            f++;
            start = p + 1;
        }
        p++;
    }
    if (f < max_fields) {
        strncpy(fields[f], start, MAX_STR_LEN - 1);
        fields[f][MAX_STR_LEN - 1] = '\0';
        f++;
    }
    while (f < max_fields) {
        fields[f][0] = '\0';
        f++;
    }
}

// Persistence Methods
void load_tasks() {
    FILE* file = fopen("tasks.txt", "r");
    if (!file) return;
    task_count = 0;
    char line[512];
    while (fgets(line, sizeof(line), file) && task_count < MAX_TASKS) {
        line[strcspn(line, "\r\n")] = '\0';
        if (strlen(line) == 0) continue;
        
        char fields[6][MAX_STR_LEN];
        split_line(line, fields, 6);
        
        Task t;
        t.id = atoi(fields[0]);
        strncpy(t.title, fields[1], MAX_STR_LEN - 1);
        t.title[MAX_STR_LEN - 1] = '\0';
        strncpy(t.course, fields[2], MAX_STR_LEN - 1);
        t.course[MAX_STR_LEN - 1] = '\0';
        strncpy(t.due_date, fields[3], MAX_STR_LEN - 1);
        t.due_date[MAX_STR_LEN - 1] = '\0';
        t.priority = atoi(fields[4]);
        t.status = atoi(fields[5]);
        
        tasks[task_count++] = t;
    }
    fclose(file);
}

void save_tasks() {
    FILE* file = fopen("tasks.txt", "w");
    if (!file) return;
    for (int i = 0; i < task_count; i++) {
        fprintf(file, "%d|%s|%s|%s|%d|%d\n",
                tasks[i].id, tasks[i].title, tasks[i].course,
                tasks[i].due_date, tasks[i].priority, tasks[i].status);
    }
    fclose(file);
}

void load_schedule() {
    FILE* file = fopen("schedule.txt", "r");
    if (!file) return;
    schedule_count = 0;
    char line[512];
    while (fgets(line, sizeof(line), file) && schedule_count < MAX_SCHEDULE_BLOCKS) {
        line[strcspn(line, "\r\n")] = '\0';
        if (strlen(line) == 0) continue;
        
        char fields[5][MAX_STR_LEN];
        split_line(line, fields, 5);
        
        ScheduleBlock sb;
        sb.day = atoi(fields[0]);
        strncpy(sb.start_time, fields[1], MAX_STR_LEN - 1);
        sb.start_time[MAX_STR_LEN - 1] = '\0';
        strncpy(sb.end_time, fields[2], MAX_STR_LEN - 1);
        sb.end_time[MAX_STR_LEN - 1] = '\0';
        strncpy(sb.course, fields[3], MAX_STR_LEN - 1);
        sb.course[MAX_STR_LEN - 1] = '\0';
        strncpy(sb.location, fields[4], MAX_STR_LEN - 1);
        sb.location[MAX_STR_LEN - 1] = '\0';
        
        schedule[schedule_count++] = sb;
    }
    fclose(file);
}

void save_schedule() {
    FILE* file = fopen("schedule.txt", "w");
    if (!file) return;
    for (int i = 0; i < schedule_count; i++) {
        fprintf(file, "%d|%s|%s|%s|%s\n",
                schedule[i].day, schedule[i].start_time, schedule[i].end_time,
                schedule[i].course, schedule[i].location);
    }
    fclose(file);
}

void load_study_logs() {
    FILE* file = fopen("study_logs.txt", "r");
    if (!file) return;
    study_log_count = 0;
    char line[512];
    while (fgets(line, sizeof(line), file) && study_log_count < MAX_STUDY_LOGS) {
        line[strcspn(line, "\r\n")] = '\0';
        if (strlen(line) == 0) continue;
        
        char fields[3][MAX_STR_LEN];
        split_line(line, fields, 3);
        
        StudyLog sl;
        strncpy(sl.course, fields[0], MAX_STR_LEN - 1);
        sl.course[MAX_STR_LEN - 1] = '\0';
        strncpy(sl.date, fields[1], MAX_STR_LEN - 1);
        sl.date[MAX_STR_LEN - 1] = '\0';
        sl.duration_mins = atoi(fields[2]);
        
        study_logs[study_log_count++] = sl;
    }
    fclose(file);
}

void save_study_logs() {
    FILE* file = fopen("study_logs.txt", "w");
    if (!file) return;
    for (int i = 0; i < study_log_count; i++) {
        fprintf(file, "%s|%s|%d\n",
                study_logs[i].course, study_logs[i].date, study_logs[i].duration_mins);
    }
    fclose(file);
}

void load_all_data() {
    load_tasks();
    load_schedule();
    load_study_logs();
}

// Menu and Dashboard Implementations
void display_dashboard() {
    int pending = 0;
    int high_priority = 0;
    
    char today[11];
    get_today_date(today);
    
    for (int i = 0; i < task_count; i++) {
        if (tasks[i].status != 2) {
            pending++;
            if (tasks[i].priority == 2) {
                high_priority++;
            }
        }
    }
    
    int study_today = 0;
    for (int i = 0; i < study_log_count; i++) {
        if (strcmp(study_logs[i].date, today) == 0) {
            study_today += study_logs[i].duration_mins;
        }
    }
    
    printf("\n%s┌──────────────────────────────────────────────────┐%s\n", ANSI_COLOR_BLUE, ANSI_COLOR_RESET);
    printf("%s│                %sSTUDENT DASHBOARD                 %s│%s\n", ANSI_COLOR_BLUE, ANSI_BOLD, ANSI_COLOR_BLUE, ANSI_COLOR_RESET);
    printf("%s├──────────────────────────────────────────────────┤%s\n", ANSI_COLOR_BLUE, ANSI_COLOR_RESET);
    printf("%s│ %sPending Tasks:%s %-3d  │ %sHigh Priority:%s %-3d │ %sStudy Today:%s %-3d m │%s\n",
           ANSI_COLOR_BLUE,
           ANSI_COLOR_YELLOW, ANSI_COLOR_BLUE, pending,
           ANSI_COLOR_RED, ANSI_COLOR_BLUE, high_priority,
           ANSI_COLOR_GREEN, ANSI_COLOR_BLUE, study_today,
           ANSI_COLOR_RESET);
    printf("%s└──────────────────────────────────────────────────┘%s\n", ANSI_COLOR_BLUE, ANSI_COLOR_RESET);
}

// Task Manager Logic
int compare_tasks(const void* a, const void* b) {
    Task* ta = (Task*)a;
    Task* tb = (Task*)b;
    if (ta->status != tb->status) {
        return ta->status - tb->status;
    }
    if (ta->priority != tb->priority) {
        return tb->priority - ta->priority;
    }
    return strcmp(ta->due_date, tb->due_date);
}

void list_tasks() {
    if (task_count == 0) {
        printf("\n%sNo tasks found! Add some tasks to get started.%s\n", ANSI_COLOR_YELLOW, ANSI_COLOR_RESET);
        return;
    }
    
    qsort(tasks, task_count, sizeof(Task), compare_tasks);
    
    printf("\n%s%-4s | %-20s | %-10s | %-10s | %-8s | %-12s%s\n",
           ANSI_BOLD, "ID", "Task Title", "Course", "Due Date", "Priority", "Status", ANSI_COLOR_RESET);
    printf("-------------------------------------------------------------------------------\n");
    for (int i = 0; i < task_count; i++) {
        printf("%-4d | %-20.20s | %-10.10s | %-10s | ",
               tasks[i].id, tasks[i].title, tasks[i].course, tasks[i].due_date);
        
        if (tasks[i].priority == 2) printf("%sHIGH%s     | ", ANSI_COLOR_RED, ANSI_COLOR_RESET);
        else if (tasks[i].priority == 1) printf("%sMED%s      | ", ANSI_COLOR_YELLOW, ANSI_COLOR_RESET);
        else printf("%sLOW%s      | ", ANSI_COLOR_GREEN, ANSI_COLOR_RESET);
        
        if (tasks[i].status == 2) printf("%sCOMPLETED%s\n", ANSI_COLOR_GREEN, ANSI_COLOR_RESET);
        else if (tasks[i].status == 1) printf("%sIN PROGRESS%s\n", ANSI_COLOR_YELLOW, ANSI_COLOR_RESET);
        else printf("%sPENDING%s\n", ANSI_COLOR_RED, ANSI_COLOR_RESET);
    }
    printf("\n");
}

void add_task() {
    if (task_count >= MAX_TASKS) {
        printf("%sError: Task list is full!%s\n", ANSI_COLOR_RED, ANSI_COLOR_RESET);
        return;
    }
    Task t;
    
    int max_id = 0;
    for (int i = 0; i < task_count; i++) {
        if (tasks[i].id > max_id) max_id = tasks[i].id;
    }
    t.id = max_id + 1;
    
    printf("Enter task title: ");
    read_string(t.title, MAX_STR_LEN);
    if (strlen(t.title) == 0) {
        printf("%sTitle cannot be empty.%s\n", ANSI_COLOR_RED, ANSI_COLOR_RESET);
        return;
    }
    
    printf("Enter course/subject: ");
    read_string(t.course, MAX_STR_LEN);
    if (strlen(t.course) == 0) {
        strcpy(t.course, "General");
    }
    
    printf("Enter due date (YYYY-MM-DD) [Press Enter for today]: ");
    char date_buf[MAX_STR_LEN];
    read_string(date_buf, MAX_STR_LEN);
    if (strlen(date_buf) == 0) {
        get_today_date(t.due_date);
    } else {
        while (!validate_date(date_buf)) {
            printf("%sInvalid date format. Enter as YYYY-MM-DD: %s", ANSI_COLOR_RED, ANSI_COLOR_RESET);
            read_string(date_buf, MAX_STR_LEN);
        }
        strcpy(t.due_date, date_buf);
    }
    
    printf("Enter priority (0 = LOW, 1 = MEDIUM, 2 = HIGH) [default 1]: ");
    char pri_buf[10];
    read_string(pri_buf, sizeof(pri_buf));
    if (strlen(pri_buf) == 0) {
        t.priority = 1;
    } else {
        t.priority = atoi(pri_buf);
        if (t.priority < 0 || t.priority > 2) t.priority = 1;
    }
    
    t.status = 0; // PENDING
    
    tasks[task_count++] = t;
    save_tasks();
    printf("%sTask added successfully! ID: %d%s\n", ANSI_COLOR_GREEN, t.id, ANSI_COLOR_RESET);
}

void update_task_status() {
    list_tasks();
    if (task_count == 0) return;
    
    printf("Enter Task ID to update: ");
    int id = read_int();
    
    int found = -1;
    for (int i = 0; i < task_count; i++) {
        if (tasks[i].id == id) {
            found = i;
            break;
        }
    }
    
    if (found != -1) {
        printf("Select status (0 = PENDING, 1 = IN PROGRESS, 2 = COMPLETED): ");
        int choice = read_int();
        if (choice >= 0 && choice <= 2) {
            tasks[found].status = choice;
            save_tasks();
            printf("%sTask status updated!%s\n", ANSI_COLOR_GREEN, ANSI_COLOR_RESET);
        } else {
            printf("%sInvalid choice.%s\n", ANSI_COLOR_RED, ANSI_COLOR_RESET);
        }
    } else {
        printf("%sTask ID not found.%s\n", ANSI_COLOR_RED, ANSI_COLOR_RESET);
    }
}

void delete_task() {
    list_tasks();
    if (task_count == 0) return;
    
    printf("Enter Task ID to delete: ");
    int id = read_int();
    
    int found = -1;
    for (int i = 0; i < task_count; i++) {
        if (tasks[i].id == id) {
            found = i;
            break;
        }
    }
    
    if (found != -1) {
        for (int i = found; i < task_count - 1; i++) {
            tasks[i] = tasks[i + 1];
        }
        task_count--;
        save_tasks();
        printf("%sTask deleted successfully.%s\n", ANSI_COLOR_GREEN, ANSI_COLOR_RESET);
    } else {
        printf("%sTask ID not found.%s\n", ANSI_COLOR_RED, ANSI_COLOR_RESET);
    }
}

void task_menu() {
    while (1) {
        print_header("TASK MANAGER");
        printf("1. View Tasks (Sorted by status/priority)\n");
        printf("2. Add New Task\n");
        printf("3. Update Task Status\n");
        printf("4. Delete Task\n");
        printf("5. Back to Main Menu\n");
        printf("Choose choice: ");
        int choice = read_int();
        
        switch (choice) {
            case 1: list_tasks(); break;
            case 2: add_task(); break;
            case 3: update_task_status(); break;
            case 4: delete_task(); break;
            case 5: return;
            default: printf("%sInvalid option. Please try again.%s\n", ANSI_COLOR_RED, ANSI_COLOR_RESET);
        }
    }
}

// Weekly Schedule Logic
const char* get_day_name(int d) {
    switch(d) {
        case 0: return "Monday";
        case 1: return "Tuesday";
        case 2: return "Wednesday";
        case 3: return "Thursday";
        case 4: return "Friday";
        case 5: return "Saturday";
        case 6: return "Sunday";
        default: return "Unknown";
    }
}

int compare_schedule(const void* a, const void* b) {
    ScheduleBlock* sa = (ScheduleBlock*)a;
    ScheduleBlock* sb = (ScheduleBlock*)b;
    if (sa->day != sb->day) {
        return sa->day - sb->day;
    }
    return strcmp(sa->start_time, sb->start_time);
}

void list_schedule() {
    if (schedule_count == 0) {
        printf("\n%sYour schedule is empty. Add classes or study blocks!%s\n", ANSI_COLOR_YELLOW, ANSI_COLOR_RESET);
        return;
    }
    
    qsort(schedule, schedule_count, sizeof(ScheduleBlock), compare_schedule);
    
    printf("\n%s%-3s | %-12s | %-15s | %-20s | %-20s%s\n",
           ANSI_BOLD, "Idx", "Day", "Time Window", "Course/Activity", "Location/Link", ANSI_COLOR_RESET);
    printf("----------------------------------------------------------------------------------------\n");
    for (int i = 0; i < schedule_count; i++) {
        char time_window[32];
        sprintf(time_window, "%s - %s", schedule[i].start_time, schedule[i].end_time);
        printf("%-3d | %-12s | %-15s | %-20.20s | %-20.20s\n",
               i + 1, get_day_name(schedule[i].day), time_window,
               schedule[i].course, schedule[i].location);
    }
    printf("\n");
}

void add_schedule() {
    if (schedule_count >= MAX_SCHEDULE_BLOCKS) {
        printf("%sError: Schedule limit reached!%s\n", ANSI_COLOR_RED, ANSI_COLOR_RESET);
        return;
    }
    ScheduleBlock sb;
    
    printf("Select Day (0=Mon, 1=Tue, 2=Wed, 3=Thu, 4=Fri, 5=Sat, 6=Sun): ");
    sb.day = read_int();
    if (sb.day < 0 || sb.day > 6) {
        printf("%sInvalid day index.%s\n", ANSI_COLOR_RED, ANSI_COLOR_RESET);
        return;
    }
    
    printf("Enter Start Time (HH:MM): ");
    read_string(sb.start_time, MAX_STR_LEN);
    while (!validate_time(sb.start_time)) {
        printf("%sInvalid format. Enter as HH:MM: %s", ANSI_COLOR_RED, ANSI_COLOR_RESET);
        read_string(sb.start_time, MAX_STR_LEN);
    }
    
    printf("Enter End Time (HH:MM): ");
    read_string(sb.end_time, MAX_STR_LEN);
    while (!validate_time(sb.end_time)) {
        printf("%sInvalid format. Enter as HH:MM: %s", ANSI_COLOR_RED, ANSI_COLOR_RESET);
        read_string(sb.end_time, MAX_STR_LEN);
    }
    
    printf("Enter Course/Activity: ");
    read_string(sb.course, MAX_STR_LEN);
    if (strlen(sb.course) == 0) {
        printf("%sActivity name cannot be empty.%s\n", ANSI_COLOR_RED, ANSI_COLOR_RESET);
        return;
    }
    
    printf("Enter Location/Link [default: N/A]: ");
    read_string(sb.location, MAX_STR_LEN);
    if (strlen(sb.location) == 0) {
        strcpy(sb.location, "N/A");
    }
    
    schedule[schedule_count++] = sb;
    save_schedule();
    printf("%sSchedule block added successfully!%s\n", ANSI_COLOR_GREEN, ANSI_COLOR_RESET);
}

void remove_schedule() {
    list_schedule();
    if (schedule_count == 0) return;
    
    printf("Enter index (1 to %d) to remove: ", schedule_count);
    int idx = read_int();
    if (idx < 1 || idx > schedule_count) {
        printf("%sInvalid index.%s\n", ANSI_COLOR_RED, ANSI_COLOR_RESET);
        return;
    }
    
    int actual_idx = idx - 1;
    for (int i = actual_idx; i < schedule_count - 1; i++) {
        schedule[i] = schedule[i + 1];
    }
    schedule_count--;
    save_schedule();
    printf("%sSchedule block removed successfully.%s\n", ANSI_COLOR_GREEN, ANSI_COLOR_RESET);
}

void schedule_menu() {
    while (1) {
        print_header("WEEKLY TIMETABLE");
        printf("1. View Schedule\n");
        printf("2. Add Class / Block\n");
        printf("3. Remove Class / Block\n");
        printf("4. Back to Main Menu\n");
        printf("Choose choice: ");
        int choice = read_int();
        
        switch (choice) {
            case 1: list_schedule(); break;
            case 2: add_schedule(); break;
            case 3: remove_schedule(); break;
            case 4: return;
            default: printf("%sInvalid option. Please try again.%s\n", ANSI_COLOR_RED, ANSI_COLOR_RESET);
        }
    }
}

// Pomodoro Module
void draw_timer_ui(const char* label, int elapsed, int total) {
    int remaining = total - elapsed;
    int bar_width = 30;
    float ratio = (float)elapsed / total;
    int filled = (int)(ratio * bar_width);
    
    printf("\r\033[K%s%s:%s [", ANSI_BOLD, label, ANSI_COLOR_RESET);
    for (int i = 0; i < bar_width; i++) {
        if (i < filled) {
            printf("%s█%s", ANSI_COLOR_GREEN, ANSI_COLOR_RESET);
        } else {
            printf("░");
        }
    }
    printf("] %3d%% | %02d:%02d remaining", (int)(ratio * 100), remaining / 60, remaining % 60);
    fflush(stdout);
}

void run_timer(const char* label, int minutes, const char* course_log) {
    int total_seconds = minutes * 60;
    int tick_ms = 1000;
    
    printf("\n%s--- %s TIMER STARTED ---%s\n", ANSI_COLOR_YELLOW, label, ANSI_COLOR_RESET);
    printf("Speed Settings:\n");
    printf("1. Normal Mode (1 second = 1 second)\n");
    printf("2. Simulation Mode (1 millisecond = 1 second, for speed testing)\n");
    printf("Choose setting [1]: ");
    char speed_buf[10];
    read_string(speed_buf, sizeof(speed_buf));
    if (atoi(speed_buf) == 2) {
        tick_ms = 1;
        printf("Simulating timer...\n");
    } else {
        printf("Timer active. Press Ctrl+C if you need to terminate the session.\n");
    }
    
    for (int elapsed = 0; elapsed <= total_seconds; elapsed++) {
        draw_timer_ui(label, elapsed, total_seconds);
        if (elapsed < total_seconds) {
            #ifdef _WIN32
            Sleep(tick_ms);
            #else
            struct timespec req = {tick_ms / 1000, (tick_ms % 1000) * 1000000L};
            nanosleep(&req, NULL);
            #endif
        }
    }
    printf("\n\n%s🎉 session completed!%s\n", ANSI_COLOR_GREEN, ANSI_COLOR_RESET);
    
    if (course_log && strlen(course_log) > 0) {
        if (study_log_count < MAX_STUDY_LOGS) {
            StudyLog sl;
            strncpy(sl.course, course_log, MAX_STR_LEN - 1);
            sl.course[MAX_STR_LEN - 1] = '\0';
            get_today_date(sl.date);
            sl.duration_mins = minutes;
            
            study_logs[study_log_count++] = sl;
            save_study_logs();
            printf("%sLogged %d minutes of study for '%s'!%s\n", ANSI_COLOR_GREEN, minutes, course_log, ANSI_COLOR_RESET);
        } else {
            printf("%sWarning: Study logs database is full. Session not recorded.%s\n", ANSI_COLOR_RED, ANSI_COLOR_RESET);
        }
    }
}

void pomodoro_menu() {
    print_header("POMODORO / STUDY TIMER");
    printf("1. Start Pomodoro Study Session (25 min)\n");
    printf("2. Start Short Break (5 min)\n");
    printf("3. Start Long Break (15 min)\n");
    printf("4. Start Custom Study Session\n");
    printf("5. Back to Main Menu\n");
    printf("Choose option: ");
    int choice = read_int();
    
    char course[MAX_STR_LEN] = "";
    
    switch(choice) {
        case 1:
            printf("Enter course name to study: ");
            read_string(course, MAX_STR_LEN);
            if (strlen(course) == 0) strcpy(course, "Self-Study");
            run_timer("Study Session", 25, course);
            break;
        case 2:
            run_timer("Short Break", 5, NULL);
            break;
        case 3:
            run_timer("Long Break", 15, NULL);
            break;
        case 4:
            printf("Enter session length in minutes: ");
            int mins = read_int();
            if (mins <= 0) {
                printf("%sInvalid duration.%s\n", ANSI_COLOR_RED, ANSI_COLOR_RESET);
                break;
            }
            printf("Enter course name: ");
            read_string(course, MAX_STR_LEN);
            if (strlen(course) == 0) strcpy(course, "Self-Study");
            run_timer("Custom Study Session", mins, course);
            break;
        case 5:
        default:
            return;
    }
}

// Statistics Module
void display_statistics() {
    print_header("STUDY STATISTICS & ANALYTICS");
    
    if (study_log_count == 0) {
        printf("%sNo study logs recorded. Log study hours by running the Pomodoro timer!%s\n", ANSI_COLOR_YELLOW, ANSI_COLOR_RESET);
        return;
    }
    
    CourseTotal totals[MAX_STUDY_LOGS];
    int total_courses = 0;
    int grand_total = 0;
    
    for (int i = 0; i < study_log_count; i++) {
        grand_total += study_logs[i].duration_mins;
        
        int found = -1;
        for (int j = 0; j < total_courses; j++) {
            if (case_insensitive_match(totals[j].course, study_logs[i].course)) {
                found = j;
                break;
            }
        }
        
        if (found != -1) {
            totals[found].total_mins += study_logs[i].duration_mins;
        } else {
            strncpy(totals[total_courses].course, study_logs[i].course, MAX_STR_LEN - 1);
            totals[total_courses].course[MAX_STR_LEN - 1] = '\0';
            totals[total_courses].total_mins = study_logs[i].duration_mins;
            total_courses++;
        }
    }
    
    printf("%sTotal Study Time Logged: %s%d hours, %d minutes (%d mins total)%s\n\n",
           ANSI_BOLD, ANSI_COLOR_GREEN, grand_total / 60, grand_total % 60, grand_total, ANSI_COLOR_RESET);
    
    printf("%sStudy hours breakdown by Course:%s\n", ANSI_BOLD, ANSI_COLOR_RESET);
    printf("----------------------------------------------------\n");
    
    int max_mins = 0;
    for (int i = 0; i < total_courses; i++) {
        if (totals[i].total_mins > max_mins) {
            max_mins = totals[i].total_mins;
        }
    }
    
    for (int i = 0; i < total_courses; i++) {
        printf("%-15.15s : ", totals[i].course);
        
        int bar_len = 0;
        if (max_mins > 0) {
            bar_len = (int)(((float)totals[i].total_mins / max_mins) * 20);
        }
        
        printf("%s", ANSI_COLOR_BLUE);
        for (int b = 0; b < bar_len; b++) {
            printf("█");
        }
        printf("%s", ANSI_COLOR_RESET);
        
        for (int b = bar_len; b < 22; b++) {
            printf(" ");
        }
        
        printf("(%d mins)\n", totals[i].total_mins);
    }
    printf("\n");
}

int main() {
    enable_ansi();
    load_all_data();
    
    while (1) {
        print_header("STUDENT TIME MANAGER");
        display_dashboard();
        
        printf("\nMain Menu Options:\n");
        printf("1. Task Manager (Homework, assignment track)\n");
        printf("2. Weekly Timetable (Classes schedule)\n");
        printf("3. Pomodoro Timer (Focus study session)\n");
        printf("4. Study Analytics & Statistics\n");
        printf("5. Save & Exit\n");
        printf("Enter your choice: ");
        int choice = read_int();
        
        switch (choice) {
            case 1: task_menu(); break;
            case 2: schedule_menu(); break;
            case 3: pomodoro_menu(); break;
            case 4: display_statistics(); break;
            case 5:
                printf("\n%sSaving data... Goodbye! Stay productive!%s\n", ANSI_COLOR_GREEN, ANSI_COLOR_RESET);
                save_tasks();
                save_schedule();
                save_study_logs();
                return 0;
            default:
                printf("%sInvalid choice. Please enter 1-5.%s\n", ANSI_COLOR_RED, ANSI_COLOR_RESET);
        }
    }
    return 0;
}
