#include <stdio.h>
#include <float.h>
#include <time.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <conio.h> // For getch() on Windows
#include <windows.h> // For Sleep()

#ifdef __EMSCRIPTEN__
#include <emscripten.h>
EM_JS(void, flushInput, (), {
  if (window.flushInput) window.flushInput();
});
#else
void flushInput() {}
#endif

#ifndef ENABLE_VIRTUAL_TERMINAL_PROCESSING
#define ENABLE_VIRTUAL_TERMINAL_PROCESSING 0x0004
#endif

void setupConsole() {
#ifdef _WIN32
    HANDLE hOut = GetStdHandle(STD_OUTPUT_HANDLE);
    DWORD dwMode = 0;
    GetConsoleMode(hOut, &dwMode);
    dwMode |= ENABLE_VIRTUAL_TERMINAL_PROCESSING;
    SetConsoleMode(hOut, dwMode);
#endif
}

// ANSI color codes for UI
#define COLOR_RESET   "\x1b[0m"
#define COLOR_RED     "\x1b[31m"
#define COLOR_GREEN   "\x1b[32m"
#define COLOR_YELLOW  "\x1b[33m"
#define COLOR_CYAN    "\x1b[36m"
#define COLOR_BOLD    "\x1b[1m"

#define FILE_NAME "expenses.dat"
#define BUDGET_FILE_NAME "budgets.dat"

#pragma pack(push, 1)
struct Expense
{
    int day;
    int month;
    int year;
    char category[50];
    double amount;
    char note[200];
};
#pragma pack(pop)

#pragma pack(push, 1)
struct Budget
{
    int month;
    int year;
    double limit;
};
#pragma pack(pop)

// UI Box Helpers
void printBoxHeader(const char* title) {
    printf(COLOR_CYAN "================================================================================\n"); // 80
    int len = strlen(title);
    int padding = (79 - len) / 2;
    if (padding < 0) padding = 0;
    for(int i=0; i<padding; i++) printf(" ");
    printf(COLOR_BOLD "%s\n" COLOR_RESET COLOR_CYAN, title);
    printf("================================================================================\n" COLOR_RESET);
}

void printBoxFooter() {
    printf(COLOR_CYAN "================================================================================\n" COLOR_RESET);
}

void pauseScreen() {
    printf("\n" COLOR_YELLOW "Press any key to go to the main menu..." COLOR_RESET);
    getch();
}

void typewriter(const char* text, int speed) {
    int len = strlen(text);
    for (int i = 0; i < len; i++) {
        printf("%c", text[i]);
        Sleep(speed);
    }
}

void simulateLoading(const char* text) {
    printf(COLOR_CYAN "\n%s\n" COLOR_RESET, text);
    printf("[");
    for (int i = 0; i < 20; i++) {
        printf(COLOR_GREEN "\xDB" COLOR_RESET); // ASCII block 219
        Sleep(40);
    }
    printf(COLOR_GREEN "] 100%%\n" COLOR_RESET);
    Sleep(200);
}

void showSplashScreen() {
    printf("\033[2J\033[3J\033[H");
    char* frames[] = {
        "   [ $ ]   ",
        "   [ / ]   ",
        "   [ - ]   ",
        "   [ \\ ]   "
    };
    
    printf("\n\n\n\n\n");
    for (int i = 0; i < 15; i++) {
        printf("\r\t\t\t%s " COLOR_CYAN "L O A D I N G . . ." COLOR_RESET, frames[i % 4]);
        Sleep(100);
    }
    printf("\n\n");
}

void gap()
{
    printf("\n\n");
}
void line(){ printf("--------------------------------------------------------------------------------\n"); }

void instructions();
int menu();

void addExpense();
void manageExpenses();
void reportsMenu();
void monthlyReport();
void categoryReport();
void setMonthlyBudget();
void financialSummary();
void importFromCSV();
void exportToCSV();
void aiFinancialAdvisor();

int validateDate(int d,int m,int y);
double getBudgetLimit(int m, int y);
double getTotalSpent(int m, int y);




void trimWhitespace(char *str) {
    int start = 0;
    while (str[start] && isspace((unsigned char)str[start])) start++;
    if (start > 0) {
        int i = 0;
        while (str[start + i]) { str[i] = str[start + i]; i++; }
        str[i] = '\0';
    }
    int len = strlen(str);
    while (len > 0 && isspace((unsigned char)str[len - 1])) {
        str[len - 1] = '\0';
        len--;
    }
}

void capitalizeStr(char *str) {
    if (!str[0]) return;
    str[0] = toupper((unsigned char)str[0]);
    for(int i = 1; str[i]; i++) {
        str[i] = tolower((unsigned char)str[i]);
    }
}

int getValidInt(int *val, const char *prompt) {
    char buffer[100];
    while (1) {
        printf("%s", prompt);
        if (fgets(buffer, sizeof(buffer), stdin) != NULL) {
            buffer[strcspn(buffer, "\n")] = 0;
            trimWhitespace(buffer);
            if (strcasecmp(buffer, "a") == 0) return 0;

            long long value;
            char extra;
            int args = sscanf(buffer, "%lld %c", &value, &extra);
            if ((args == 1 || (args == 2 && isspace(extra))) && value <= 2147483647LL && value >= -2147483648LL) {
                *val = (int)value;
                return 1;
            }
        }
        printf(COLOR_RED "Invalid input! Please enter a number or 'a' to Abort: " COLOR_RESET);
    }
}

int getValidDouble(double *val, const char *prompt) {
    char buffer[100];
    while (1) {
        printf("%s", prompt);
        if (fgets(buffer, sizeof(buffer), stdin) != NULL) {
            buffer[strcspn(buffer, "\n")] = 0;
            trimWhitespace(buffer);
            if (strcasecmp(buffer, "a") == 0) return 0;

            char extra;
            int args = sscanf(buffer, "%lf %c", val, &extra);
            if (args == 1 || (args == 2 && isspace(extra))) {
                return 1;
            }
        }
        printf(COLOR_RED "Invalid input! Please enter a decimal or 'a' to Abort: " COLOR_RESET);
    }
}

int getValidDate(int *d, int *m, int *y) {
    char buffer[100];
    while (1) {
        printf("Enter date (day month year) [0 for Today, a to Abort]: ");
        if (fgets(buffer, sizeof(buffer), stdin) != NULL) {
            // Trim newline
            buffer[strcspn(buffer, "\n")] = 0;
            trimWhitespace(buffer);
            
            if (strcasecmp(buffer, "a") == 0) return 0;

            // Check for shortcut
            if (strcmp(buffer, "0") == 0 || strcasecmp(buffer, "t") == 0) {
                time_t now = time(NULL);
                now += (5 * 3600) + (30 * 60); // IST
                struct tm *info = gmtime(&now);
                *d = info->tm_mday;
                *m = info->tm_mon + 1;
                *y = info->tm_year + 1900;
                printf(COLOR_CYAN "Selected Today: %02d/%02d/%04d\n" COLOR_RESET, *d, *m, *y);
                return 1;
            }

            char extra;
            int args = sscanf(buffer, "%d %d %d %c", d, m, y, &extra);
            if (args == 3 || (args == 4 && isspace(extra))) {
                if (*y >= 1900 && *y <= 2100) {
                    if (validateDate(*d, *m, *y)) return 1;
                    else printf(COLOR_RED "Invalid date values! " COLOR_RESET);
                }
                else printf(COLOR_RED "Year must be between 1900 and 2100! " COLOR_RESET);
            }
        }
        printf(COLOR_RED "Invalid format! Try (day month year), '0', or 'a': " COLOR_RESET);
    }
}

int getValidMonthYear(int *m, int *y) {
    char buffer[100];
    while (1) {
        printf("Enter month year (e.g., 3 2026) [0 for current month, a to Abort]: ");
        if (fgets(buffer, sizeof(buffer), stdin) != NULL) {
            buffer[strcspn(buffer, "\n")] = 0;
            trimWhitespace(buffer);

            if (strcasecmp(buffer, "a") == 0) return 0;

            if (strcmp(buffer, "0") == 0 || strcasecmp(buffer, "t") == 0 || strcmp(buffer, ".") == 0) {
                time_t now = time(NULL);
                now += (5 * 3600) + (30 * 60); // IST
                struct tm *info = gmtime(&now);
                *m = info->tm_mon + 1;
                *y = info->tm_year + 1900;
                printf(COLOR_CYAN "Selected: %02d/%d\n" COLOR_RESET, *m, *y);
                return 1;
            }
            char extra;
            int args = sscanf(buffer, "%d %d %c", m, y, &extra);
            if (args == 2 || (args == 3 && isspace(extra))) {
                if (*m >= 1 && *m <= 12 && *y >= 1900 && *y <= 2100) {
                    return 1;
                }
                printf(COLOR_RED "Month must be 1-12 and year 1900-2100! " COLOR_RESET);
                continue;
            }
        }
        printf(COLOR_RED "Invalid format! Please enter (month year), '0', or 'a': " COLOR_RESET);
    }
}

int getFilterDate(int *d, int *m, int *y) {
    char buffer[100];
    while (1) {
        printf("\nEnter filter [0:Today / . for this month / <Year> / <Month Year> / <Day Month Year> / a:Abort]: ");
        if (fgets(buffer, sizeof(buffer), stdin) != NULL) {
            buffer[strcspn(buffer, "\n")] = 0;
            trimWhitespace(buffer);
            
            if (strcasecmp(buffer, "a") == 0) return -1; // Aborted
            
            // Shortcuts
            if (strcmp(buffer, "0") == 0) {
                time_t now = time(NULL);
                now += (5 * 3600) + (30 * 60); // IST
                struct tm *info = gmtime(&now);
                *d = info->tm_mday; *m = info->tm_mon + 1; *y = info->tm_year + 1900;
                printf(COLOR_CYAN "Selected Today: %02d/%02d/%04d\n" COLOR_RESET, *d, *m, *y);
                return 1;
            }
            if (strcmp(buffer, ".") == 0) {
                time_t now = time(NULL);
                now += (5 * 3600) + (30 * 60); // IST
                struct tm *info = gmtime(&now);
                *d = 0; *m = info->tm_mon + 1; *y = info->tm_year + 1900;
                printf(COLOR_CYAN "Selected Current Month: %02d/%04d\n" COLOR_RESET, *m, *y);
                return 1;
            }

            // Smart Detection
            int v1, v2, v3;
            char extra;
            int args = sscanf(buffer, "%d %d %d %c", &v1, &v2, &v3, &extra);
            
            if (args == 1) { // Year only: e.g., 2026
                if (v1 >= 1900 && v1 <= 2100) {
                    *d = 0; *m = 0; *y = v1;
                    printf(COLOR_CYAN "Selected Year: %04d\n" COLOR_RESET, *y);
                    return 1;
                }
            } else if (args == 2) { // Month/Year: e.g., 3 2026
                if (v1 >= 1 && v1 <= 12 && v2 >= 1900 && v2 <= 2100) {
                    *d = 0; *m = v1; *y = v2;
                    printf(COLOR_CYAN "Selected Month: %02d/%04d\n" COLOR_RESET, *m, *y);
                    return 1;
                }
            } else if (args == 3) { // Exact Date: 10 3 2026
                if (validateDate(v1, v2, v3)) {
                    *d = v1; *m = v2; *y = v3;
                    printf(COLOR_CYAN "Selected Date: %02d/%02d/%04d\n" COLOR_RESET, *d, *m, *y);
                    return 1;
                }
            }
        }
        printf(COLOR_RED "Invalid filter! Use '0' / '.' / '2026' / '3 2026' / '10 3 2026': " COLOR_RESET);
    }
}

int getValidString(char *target, int size, const char *prompt) {
    char buffer[512];
    while (1) {
        printf("%s", prompt);
        if (fgets(buffer, sizeof(buffer), stdin) != NULL) {
            buffer[strcspn(buffer, "\n")] = 0;
            trimWhitespace(buffer);
            if (strcasecmp(buffer, "a") == 0) return 0;
            
            strncpy(target, buffer, size - 1);
            target[size - 1] = '\0';
            return 1;
        }
    }
}

int main()
{
    srand((unsigned)time(NULL));
    setupConsole();
    showSplashScreen();
    instructions();

    while(1)
    {
        printf("\033[2J\033[3J\033[H");
        int choice = menu();
        
        printf("\033[2J\033[3J\033[H"); // Clear screen before showing feature

        switch(choice)
        {
            case 1: addExpense(); pauseScreen(); break;
            case 2: manageExpenses(); break; 
            case 3: setMonthlyBudget(); pauseScreen(); break;
            case 4: reportsMenu(); break;
            case 5: importFromCSV(); break;
            case 6: exportToCSV(); pauseScreen(); break;
            case 7: aiFinancialAdvisor(); break;
            case 8: exit(0);
        }
    }
}

void instructions()
{
    printf("\033[2J\033[3J\033[H");
    printBoxHeader("PERSONAL FINANCE MANAGEMENT SYSTEM");
    gap();

    typewriter("Date format -> day month year (e.g., 10 3 2026)\n\n", 5);
    
    typewriter("Categories  -> Food Travel Bills Shopping Other\n\n", 5);
    
    typewriter("Navigation  -> Use Up/Down Arrow Keys and Enter\n\n", 5);
    
    pauseScreen();
}

void getCurrentMonthYear(int *m, int *y) {
    time_t now = time(NULL);
    // Adjust for IST (UTC + 5:30)
    now += (5 * 3600) + (30 * 60);
    struct tm *info = gmtime(&now);
    *m = info->tm_mon + 1;
    *y = info->tm_year + 1900;
}

void showBudgetDashboard(int animate) {
    int m, y;
    getCurrentMonthYear(&m, &y);
    double spent = getTotalSpent(m, y);
    double budget = getBudgetLimit(m, y);

    if (budget <= 0) return;

    double percent = (spent / budget) * 100;
    int barWidth = 30;
    int barLen = (int)((percent / 100.0) * barWidth);
    if (barLen > barWidth) barLen = barWidth;

    printf("\n" COLOR_CYAN " %02d/%04d BUDGET DASHBOARD" COLOR_RESET "\n", m, y);
    if (animate) Sleep(40);
    printf(" Limit: $%.2f | Spent: $%.2f\n", budget, spent);
    if (animate) Sleep(40);
    printf(" [");
    for (int i = 0; i < barWidth; i++) {
        if (i < barLen) {
            if (percent > 100) printf(COLOR_RED "\xdb" COLOR_RESET);
            else printf(COLOR_GREEN "\xdb" COLOR_RESET);
        } else {
            printf(" ");
        }
    }
    printf("] %.1f%%\n", percent);
    if (animate) Sleep(40);

    if (spent > budget) {
        printf(COLOR_RED " !!! OVER BUDGET BY $%.2f !!!" COLOR_RESET "\n", spent - budget);
    } else {
        printf(COLOR_GREEN " Safe: $%.2f remaining" COLOR_RESET "\n", budget - spent);
    }
    if (animate) Sleep(40);
    printf("\n");
}

int menu()
{
    int highlight = 1;
    int num_choices = 8;
    int c = 0;
    
    char *options[] = {
        "Add Expense",
        "Manage Expenses (Search/Edit/Delete)",
        "Set Monthly Budget",
        "Reports & Analytics",
        "Import from CSV (Browse/Drop)",
        "Export to CSV",
        "AI Financial Advisor",
        "Exit"
    };

    int firstLoad = 1;
    while (1) {
        printf("\033[2J\033[3J\033[H");
        printBoxHeader("MAIN MENU");
        showBudgetDashboard(firstLoad);

        for (int i = 0; i < num_choices; i++) {
            if (highlight == i + 1) {
                printf("\x1b[43;30m" " > %2d. %-30s " "\x1b[0m" "\n", i + 1, options[i]);
            } else {
                printf("   %2d. %-30s  \n", i + 1, options[i]);
            }
            if (firstLoad) Sleep(40);
        }
        firstLoad = 0;
        printBoxFooter();
        printf(COLOR_CYAN " Use [" COLOR_YELLOW "Up" COLOR_CYAN "/" COLOR_YELLOW "Down" COLOR_CYAN "] Arrow Keys to navigate, and [" COLOR_YELLOW "Enter" COLOR_CYAN "] to select." COLOR_RESET "\n");

        c = getch();

        switch (c) {
            case 224: // Arrow key prefix
                c = getch();
                if (c == 72) { // Up arrow
                    highlight--;
                    if (highlight < 1) highlight = num_choices;
                } else if (c == 80) { // Down arrow
                    highlight++;
                    if (highlight > num_choices) highlight = 1;
                }
                break;
            case 13: // Enter key
                return highlight;
        }
    }
}

int validateDate(int d,int m,int y)
{
    if(m<1 || m>12) return 0;
    if(d<1) return 0;
    
    int daysInMonth[] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};
    if (m == 2 && ((y % 4 == 0 && y % 100 != 0) || (y % 400 == 0))) {
        daysInMonth[1] = 29;
    }
    
    if(d > daysInMonth[m-1]) return 0;
    
    return 1;
}

int isDuplicate(struct Expense *e) {
    struct Expense check;
    FILE *f = fopen(FILE_NAME, "rb");
    if (!f) return 0;
    while (fread(&check, sizeof(check), 1, f)) {
        if (check.day == e->day && check.month == e->month && check.year == e->year &&
            check.amount == e->amount && 
            strcmp(check.category, e->category) == 0 &&
            strcmp(check.note, e->note) == 0) {
            fclose(f);
            return 1;
        }
    }
    fclose(f);
    return 0;
}

void addExpense()
{
    struct Expense e;
    FILE *f=fopen(FILE_NAME,"ab");
    if (!f) {
        printf(COLOR_RED "Error: Could not open database for writing.\n" COLOR_RESET);
        return;
    }

    printBoxHeader("ADD EXPENSE");
    gap();

    if(!getValidDate(&e.day, &e.month, &e.year)) { fclose(f); return; }
    gap();

    if(!getValidString(e.category, sizeof(e.category), "Enter category (or 'a' to Abort): ")) { fclose(f); return; }
    capitalizeStr(e.category);
    if(strlen(e.category)==0) strcpy(e.category, "Unknown");
    gap();

    if(!getValidDouble(&e.amount, "Enter amount (or 'a' to Abort): ")) { fclose(f); return; }
    if (e.amount < 0) { 
        printf(COLOR_RED "Amount cannot be negative!\n" COLOR_RESET);
        fclose(f); return; 
    }
    gap();

    double budget = getBudgetLimit(e.month, e.year);
    if (budget > 0.0) {
        double spent = getTotalSpent(e.month, e.year);
        double newTotal = spent + e.amount;
        if (newTotal >= budget * 0.8) {
            printf(COLOR_YELLOW "[WARNING]: Adding this expense puts you at %.1f%% of your budget for %02d/%04d!\n" COLOR_RESET, (newTotal/budget)*100, e.month, e.year);
            printf("Current Limit: $%.2f | Current Spent: $%.2f | New Total: $%.2f\n", budget, spent, newTotal);
            printf("Do you still want to save this expense? (y/n, or 'a' to Abort): ");
            char confirm;
            {
                char buffer[100];
                fgets(buffer, sizeof(buffer), stdin);
                confirm = buffer[0];
            }
            if (confirm == 'a' || confirm == 'A') { fclose(f); return; }
            if (confirm != 'y' && confirm != 'Y') {
                printf(COLOR_RED "Expense not saved.\n" COLOR_RESET);
                fclose(f);
                return;
            }
            gap();
        }
    }

    if(!getValidString(e.note, sizeof(e.note), "Enter note (or 'a' to Abort): ")) { fclose(f); return; }
    gap();

    if (isDuplicate(&e)) {
        printf(COLOR_RED "Error: Duplicate expense detected! This entry already exists.\n" COLOR_RESET);
        fclose(f);
        return;
    }

    if (fwrite(&e,sizeof(e),1,f) != 1) {
        printf(COLOR_RED "Error: Failed to write expense to database.\n" COLOR_RESET);
        fclose(f);
        return;
    }
    fflush(f);

    fclose(f);

    printf(COLOR_GREEN "Expense added successfully.\n" COLOR_RESET);
}

int comparePtrByAmountDesc(const void *a, const void *b) {
    struct Expense *e1 = *(struct Expense **)a;
    struct Expense *e2 = *(struct Expense **)b;
    if (e1->amount < e2->amount) return 1;
    if (e1->amount > e2->amount) return -1;
    return 0;
}
int comparePtrByAmountAsc(const void *a, const void *b) {
    struct Expense *e1 = *(struct Expense **)a;
    struct Expense *e2 = *(struct Expense **)b;
    if (e1->amount > e2->amount) return 1;
    if (e1->amount < e2->amount) return -1;
    return 0;
}
int comparePtrByDateDesc(const void *a, const void *b) {
    struct Expense *e1 = *(struct Expense **)a;
    struct Expense *e2 = *(struct Expense **)b;
    if (e1->year != e2->year) return e2->year - e1->year;
    if (e1->month != e2->month) return e2->month - e1->month;
    return e2->day - e1->day;
}
int comparePtrByDateAsc(const void *a, const void *b) {
    struct Expense *e1 = *(struct Expense **)a;
    struct Expense *e2 = *(struct Expense **)b;
    if (e1->year != e2->year) return e1->year - e2->year;
    if (e1->month != e2->month) return e1->month - e2->month;
    return e1->day - e2->day;
}
int comparePtrByCategoryAsc(const void *a, const void *b) {
    struct Expense *e1 = *(struct Expense **)a;
    struct Expense *e2 = *(struct Expense **)b;
    char s1[50], s2[50];
    strcpy(s1, e1->category);
    strcpy(s2, e2->category);
    for(int i = 0; s1[i]; i++) s1[i] = tolower((unsigned char)s1[i]);
    for(int i = 0; s2[i]; i++) s2[i] = tolower((unsigned char)s2[i]);
    return strcmp(s1, s2);
}
int comparePtrByCategoryDesc(const void *a, const void *b) {
    return comparePtrByCategoryAsc(b, a);
}

void manageExpenses()
{
    int capacity = 100; // Initial capacity
    struct Expense *arr = (struct Expense*)malloc(capacity * sizeof(struct Expense));
    struct Expense **viewArr = (struct Expense**)malloc(capacity * sizeof(struct Expense*));
    
    if (!arr || !viewArr) {
        printf(COLOR_RED "Memory allocation failed!\n" COLOR_RESET);
        if(arr) free(arr);
        if(viewArr) free(viewArr);
        pauseScreen();
        return;
    }

    auto_load: ; // Label for reloading data after edit/delete
    struct Expense e;
    FILE *f=fopen(FILE_NAME,"rb");
    if(!f)
    {
        printf(COLOR_RED "No database found. Add some expenses first.\n" COLOR_RESET);
        if(arr) free(arr);
        if(viewArr) free(viewArr);
        pauseScreen();
        return;
    }

    int count=0;
    while(fread(&e,sizeof(e),1,f))
    {
        if (count >= capacity) {
            int new_capacity = capacity * 2;
            struct Expense* next_arr = (struct Expense*)realloc(arr, new_capacity * sizeof(struct Expense));
            struct Expense** next_viewArr = (struct Expense**)realloc(viewArr, new_capacity * sizeof(struct Expense*));
            
            if (!next_arr || !next_viewArr) {
                printf(COLOR_RED "Memory allocation failed during data load.\n" COLOR_RESET);
                if (next_arr) arr = next_arr;
                if (next_viewArr) viewArr = next_viewArr;
                fclose(f);
                free(arr);
                free(viewArr);
                pauseScreen();
                return;
            }
            arr = next_arr;
            viewArr = next_viewArr;
            capacity = new_capacity;
        }
        arr[count] = e;
        count++;
    }
    fclose(f);

    int itemsPerPage = 12; // Reduced slightly to fit more UI
    int currentPage = 1;
    int animate = 1;
    int sortBy = 0; 
    int sortDir = 1; 
    char cmd;
    
    char searchBuf[50] = {0};
    int searchLen = 0;
    int isSearching = 0;
    
    int filterD = 0, filterM = 0, filterY = 0;

    while (1) {
        int viewCount = 0;
        char lower_buf[50] = {0};
        for(int i=0; searchBuf[i]; i++) lower_buf[i] = tolower((unsigned char)searchBuf[i]);

        for(int i=0; i<count; i++) {
            int dateMatch = 1;
            if (filterY != 0) {
                if (filterM == 0) { if (arr[i].year != filterY) dateMatch = 0; }
                else if (filterD == 0) { if (arr[i].year != filterY || arr[i].month != filterM) dateMatch = 0; }
                else { if (arr[i].year != filterY || arr[i].month != filterM || arr[i].day != filterD) dateMatch = 0; }
            }
            char lower_cat[50] = {0};
            for(int j=0; arr[i].category[j]; j++) lower_cat[j] = tolower((unsigned char)arr[i].category[j]);
            char lower_note[200] = {0};
            for(int j=0; arr[i].note[j]; j++) lower_note[j] = tolower((unsigned char)arr[i].note[j]);
            
            int textMatch = (searchLen == 0 || strstr(lower_cat, lower_buf) != NULL || strstr(lower_note, lower_buf) != NULL);
            if(textMatch && dateMatch) {
                viewArr[viewCount] = &arr[i];
                viewCount++;
            }
        }

        int totalPages = (viewCount + itemsPerPage - 1) / itemsPerPage;
        if (totalPages == 0) totalPages = 1;
        if (currentPage > totalPages) currentPage = totalPages;

        printf("\033[2J\033[3J\033[H");
        printf(COLOR_CYAN "========== MANAGE EXPENSES (Page %d of %d) ==========\n" COLOR_RESET, currentPage, totalPages);
        
        if (isSearching) printf(COLOR_CYAN "Live Search Cat/Note: " COLOR_YELLOW "[ %s_ ]" COLOR_RESET "\n", searchBuf);
        else if (searchLen > 0 || filterY > 0) {
            printf(COLOR_CYAN "Active Filters: ");
            if (searchLen > 0) printf(COLOR_GREEN "Search='%s'  " COLOR_RESET, searchBuf);
            if (filterY > 0) {
                if (filterM == 0) printf(COLOR_GREEN "Year=%04d " COLOR_RESET, filterY);
                else if (filterD > 0) printf(COLOR_GREEN "Date=%02d/%02d/%04d " COLOR_RESET, filterD, filterM, filterY);
                else printf(COLOR_GREEN "Month=%02d/%04d " COLOR_RESET, filterM, filterY);
            }
            printf("\n");
        } else printf("No active filters.\n");
        gap();
        
        printf("%-4s  %-10s  %-16s  %-9s  %-32s\n", "IDX", "DATE", "CATEGORY", "AMOUNT", "NOTE");
        line();

        if (sortBy == 1) qsort(viewArr, viewCount, sizeof(struct Expense*), sortDir == 1 ? comparePtrByDateDesc : comparePtrByDateAsc);
        else if (sortBy == 2) qsort(viewArr, viewCount, sizeof(struct Expense*), sortDir == 1 ? comparePtrByCategoryDesc : comparePtrByCategoryAsc);
        else if (sortBy == 3) qsort(viewArr, viewCount, sizeof(struct Expense*), sortDir == 1 ? comparePtrByAmountDesc : comparePtrByAmountAsc);

        int start = (currentPage - 1) * itemsPerPage;
        int end = start + itemsPerPage;
        if (end > viewCount) end = viewCount;

        if (viewCount == 0) printf(COLOR_RED "No matching expenses found.\n" COLOR_RESET);
        else {
            for (int i = start; i < end; i++) {
                printf("[%02d]  %02d/%02d/%04d  %-16.16s  %9.2f  %-32.32s\n",
                       (i - start) + 1, viewArr[i]->day, viewArr[i]->month, viewArr[i]->year,
                       viewArr[i]->category, viewArr[i]->amount, viewArr[i]->note);
                if (animate) Sleep(40);
            }
        }
        animate = 0;

        if (isSearching) {
            printf("\n" COLOR_YELLOW "Filter Mode:" COLOR_CYAN " Type to search. Press [" COLOR_YELLOW "Enter" COLOR_CYAN "] to confirm." COLOR_RESET "\n");
            char c = getch();
            if (c == 13) { isSearching = 0; animate = 1; }
            else if (c == 8) { if (searchLen > 0) searchBuf[--searchLen] = '\0'; }
            else if (c >= 32 && c <= 126 && searchLen < 49) { searchBuf[searchLen++] = c; searchBuf[searchLen] = '\0'; }
            currentPage = 1;
        } else {
            printf("\n" COLOR_YELLOW "Nav: [N]ext, [P]rev | Sort: [D]ate, [C]at, [A]mount | [S]earch(text), [F]ind date, [X] Clear\n" COLOR_RESET);
            printf(COLOR_GREEN "Action: [E]dit Item, [R]emove Item | [Q]uit\n" COLOR_RESET);
            printf("Choice: ");
            cmd = getch();

            if (cmd == 'S' || cmd == 's') isSearching = 1;
            else if (cmd == 'F' || cmd == 'f') {
                int d, m, y;
                int res = getFilterDate(&d, &m, &y);
                if (res == 1) {
                    filterD = d; filterM = m; filterY = y;
                    currentPage = 1; animate = 1;
                }
            }
 else if (cmd == 'X' || cmd == 'x') {
                searchBuf[0] = '\0'; searchLen = 0; filterY = 0; filterM = 0; filterD = 0; animate = 1;
            } else if (cmd == 'N' || cmd == 'n') { if (currentPage < totalPages) { currentPage++; animate = 1; } }
            else if (cmd == 'P' || cmd == 'p') { if (currentPage > 1) { currentPage--; animate = 1; } }
            else if (cmd == 'D' || cmd == 'd') { if (sortBy == 1 && sortDir == 1) sortDir = -1; else { sortBy = 1; sortDir = 1; } animate = 1; }
            else if (cmd == 'C' || cmd == 'c') { if (sortBy == 2 && sortDir == -1) sortDir = 1; else { sortBy = 2; sortDir = -1; } animate = 1; }
            else if (cmd == 'A' || cmd == 'a') { if (sortBy == 3 && sortDir == 1) sortDir = -1; else { sortBy = 3; sortDir = 1; } animate = 1; }
            else if (cmd == 'Q' || cmd == 'q') break;
            else if (cmd == 'R' || cmd == 'r' || cmd == 'E' || cmd == 'e') {
                if (viewCount == 0) continue;
                int idxVal;
                if (!getValidInt(&idxVal, "Enter Index to Edit/Delete ('a' to cancel): ")) { animate = 1; continue; }
                if (idxVal > 0 && idxVal <= (end-start)) {
                    struct Expense *target = viewArr[start + idxVal - 1];
                    if (cmd == 'R' || cmd == 'r') {
                        // DELETE LOGIC
                        char tempname[32];
                        snprintf(tempname, sizeof(tempname), "temp_%d.dat", rand());
                        FILE *ft = fopen(FILE_NAME, "rb");
                        FILE *tmp = fopen(tempname, "wb");
                        if (ft && tmp) {
                            struct Expense ex;
                            int write_ok = 1;
                            while(fread(&ex, sizeof(ex), 1, ft)) {
                                if (memcmp(&ex, target, sizeof(ex)) == 0) continue;
                                if (fwrite(&ex, sizeof(ex), 1, tmp) != 1) { write_ok = 0; break; }
                            }
                            fclose(ft); ft = NULL;
                            fclose(tmp); tmp = NULL;

                            if (!write_ok) {
                                printf(COLOR_RED "Error: Write to temp file failed. Deletion aborted.\n" COLOR_RESET);
                                remove(tempname);
                            } else {
                                char backup[64];
                                snprintf(backup, sizeof(backup), "backup_%d.dat", rand());
                                if (rename(FILE_NAME, backup) == 0) {
                                    if (rename(tempname, FILE_NAME) == 0) {
                                        remove(backup);
                                        printf(COLOR_GREEN "Deleted successfully!\n" COLOR_RESET);
                                    } else {
                                        rename(backup, FILE_NAME);
                                        printf(COLOR_RED "Error: Deletion failed. Database restored from backup.\n" COLOR_RESET);
                                        remove(tempname);
                                    }
                                } else {
                                    printf(COLOR_RED "Error: Could not create backup. Deletion aborted.\n" COLOR_RESET);
                                    remove(tempname);
                                }
                            }
                        } else {
                            if(ft) fclose(ft);
                            if(tmp) fclose(tmp);
                            printf(COLOR_RED "Error: Could not access database or create temp file.\n" COLOR_RESET);
                        }
                        Sleep(1000);
                        goto auto_load;
                    } else {
                        // EDIT LOGIC
                        printf("\nEditing: %02d/%02d/%04d | %s | $%.2f\n", target->day, target->month, target->year, target->category, target->amount);
                        
                        double namt;
                        if (!getValidDouble(&namt, "New Amount [0 to keep, a to Abort]: ")) { animate = 1; continue; }
                        if (namt > 0) target->amount = namt;
                        
                        char ncat[50];
                        if (!getValidString(ncat, sizeof(ncat), "New Category [- to keep, a to Abort]: ")) { animate = 1; continue; }
                        if(strcmp(ncat,"-")!=0 && strlen(ncat)>0) { 
                            capitalizeStr(ncat); 
                            strncpy(target->category, ncat, sizeof(target->category) - 1); 
                            target->category[sizeof(target->category) - 1] = '\0';
                        }

                        char nnote[200];
                        if (!getValidString(nnote, sizeof(nnote), "New Note [- to keep, a to Abort]: ")) { animate = 1; continue; }
                        if(strcmp(nnote,"-")!=0 && strlen(nnote)>0) {
                            strncpy(target->note, nnote, sizeof(target->note) - 1);
                            target->note[sizeof(target->note) - 1] = '\0';
                        }

                        // Check if this edit created a duplicate
                        if (isDuplicate(target)) {
                            printf(COLOR_RED "Error: This edit would create a duplicate record! Reverting...\n" COLOR_RESET);
                            Sleep(2000);
                            goto auto_load; // Reload from disk to revert memory changes
                        }

                        // Safe edit-save: write to temp file, then backup-swap
                        char edit_temp[32];
                        snprintf(edit_temp, sizeof(edit_temp), "temp_edit_%d.dat", rand());
                        FILE *fout = fopen(edit_temp, "wb");
                        if (!fout) {
                            printf(COLOR_RED "Error: Could not create temp file for edit.\n" COLOR_RESET);
                            Sleep(1000);
                            goto auto_load;
                        }
                        int edit_ok = 1;
                        for (int wi = 0; wi < count; wi++) {
                            if (fwrite(&arr[wi], sizeof(struct Expense), 1, fout) != 1) {
                                edit_ok = 0;
                                break;
                            }
                        }
                        fclose(fout);

                        if (!edit_ok) {
                            printf(COLOR_RED "Error: Write failed during edit. Changes NOT saved.\n" COLOR_RESET);
                            remove(edit_temp);
                        } else {
                            char edit_backup[64];
                            snprintf(edit_backup, sizeof(edit_backup), "backup_e_%d.dat", rand());
                            if (rename(FILE_NAME, edit_backup) == 0) {
                                if (rename(edit_temp, FILE_NAME) == 0) {
                                    remove(edit_backup);
                                    printf(COLOR_GREEN "Updated successfully!\n" COLOR_RESET);
                                } else {
                                    rename(edit_backup, FILE_NAME);
                                    printf(COLOR_RED "Error: Edit swap failed. Database restored.\n" COLOR_RESET);
                                    remove(edit_temp);
                                }
                            } else {
                                printf(COLOR_RED "Error: Could not backup database for edit.\n" COLOR_RESET);
                                remove(edit_temp);
                            }
                        }
                        Sleep(1000);
                        goto auto_load;
                    }
                }
            }
        }
    }
    
    free(arr);
    free(viewArr);
}



void monthlyReport()
{
    struct Expense e;
    FILE *f=fopen(FILE_NAME,"rb");
    int m, y;
    double total=0;
    int count=0;
    double highest=-DBL_MAX;
    double lowest=DBL_MAX;
    struct Expense high, low;

    printBoxHeader("MONTHLY REPORT");
    gap();

    if(!f){ printf(COLOR_RED "No data available\n" COLOR_RESET); return; }

    if(!getValidMonthYear(&m, &y)) { fclose(f); return; }
    gap();

    // Pass 1: Gather Statistics
    while(fread(&e,sizeof(e),1,f)) {
        if(e.month==m && e.year==y) {
            total+=e.amount;
            count++;
            if(e.amount > highest) { highest = e.amount; high = e; }
            if(e.amount < lowest) { lowest = e.amount; low = e; }
        }
    }

    if(count == 0) {
        printf(COLOR_YELLOW "No transactions found for %02d/%04d.\n" COLOR_RESET, m, y);
        fclose(f);
        return;
    }

    // Print Statistics
    printf(COLOR_CYAN "--- SUMMARY FOR %02d/%04d ---\n" COLOR_RESET, m, y); Sleep(40);
    printf("Total Transactions : %d\n", count); Sleep(40);
    printf("Total Spent        : $%.2f\n", total); Sleep(40);
    printf("Average Spend      : $%.2f\n", total / count); Sleep(40);
    double budget = getBudgetLimit(m, y);
    if(budget > 0.0) {
        printf("Budget Limit       : $%.2f\n", budget); Sleep(40);
        if(total > budget) {
            printf(COLOR_RED "Budget Status      : EXCEEDED by $%.2f\n" COLOR_RESET, total - budget);
        } else {
            printf(COLOR_GREEN "Budget Status      : %.1f%% Used ($%.2f remaining)\n" COLOR_RESET, (total/budget)*100, budget - total);
        }
        Sleep(40);
    }

    // Category Breakdown for the Month
    gap();
    printf(COLOR_CYAN "--- CATEGORY BREAKDOWN ---\n" COLOR_RESET);
    line();
    
    int cat_cap = 10;
    char (*cats)[50] = malloc(cat_cap * sizeof(*cats));
    double *cat_tots = calloc(cat_cap, sizeof(double));
    int cat_cnt = 0;

    if (cats && cat_tots) {
        rewind(f);
        while(fread(&e, sizeof(e), 1, f)) {
            if(e.month == m && e.year == y) {
                int idx = -1;
                for(int i = 0; i < cat_cnt; i++) {
                    if(strcmp(cats[i], e.category) == 0) {
                        idx = i;
                        break;
                    }
                }
                if(idx == -1) {
                    if(cat_cnt >= cat_cap) {
                        int old_c = cat_cap;
                        cat_cap *= 2;
                        char (*t_cats)[50] = realloc(cats, cat_cap * sizeof(*cats));
                        double *t_tots = realloc(cat_tots, cat_cap * sizeof(double));
                        if (!t_cats || !t_tots) {
                            printf(COLOR_RED "Memory realloc failed\n" COLOR_RESET);
                            if (t_cats) cats = t_cats; // Update for free
                            if (t_tots) cat_tots = t_tots;
                            free(cats); free(cat_tots);
                            fclose(f);
                            return;
                        }
                        cats = t_cats;
                        cat_tots = t_tots;
                        for(int j=cat_cnt; j<cat_cap; j++) cat_tots[j] = 0;
                    }
                    strcpy(cats[cat_cnt], e.category);
                    cat_tots[cat_cnt] = e.amount;
                    cat_cnt++;
                } else {
                    cat_tots[idx] += e.amount;
                }
            }
        }
        for(int i = 0; i < cat_cnt; i++) {
            printf("%-20s: $%.2f\n", cats[i], cat_tots[i]); // Increased to 20
            Sleep(40);
        }
        free(cats);
        free(cat_tots);
    }


    gap();
    printf(COLOR_YELLOW "Highest Item:" COLOR_RESET " %02d/%02d/%04d | %-20.20s | $%.2f\n", high.day, high.month, high.year, high.category, high.amount);
    printf(COLOR_YELLOW "Lowest Item :" COLOR_RESET " %02d/%02d/%04d | %-20.20s | $%.2f\n", low.day, low.month, low.year, low.category, low.amount);
    gap();

    // Pass 2: Print Itemized Breakdown
    rewind(f);
    printf(COLOR_CYAN "--- ITEMIZED BREAKDOWN ---\n" COLOR_RESET);
    printf("%-12s  %-18.18s  %12s  %-32.32s\n","DATE","CATEGORY","AMOUNT","NOTE");
    line();

    while(fread(&e,sizeof(e),1,f)) {
        if(e.month==m && e.year==y) {
            printf("%02d/%02d/%04d  %-18.18s  %12.2f  %-32.32s\n",
                   e.day,e.month,e.year,e.category,e.amount,e.note);
            Sleep(40);
        }
    }

    fclose(f);
}

void categoryReport()
{
    struct Expense e;
    FILE *f=fopen(FILE_NAME,"rb");

    char category[50];
    double total=0;
    int count=0;
    double highest=-DBL_MAX;
    double lowest=DBL_MAX;
    struct Expense high, low;

    printBoxHeader("CATEGORY REPORT");
    gap();

    if(!f){ printf(COLOR_RED "No data available\n" COLOR_RESET); return; }

    if (!getValidString(category, sizeof(category), "Enter category (or 'a' to Abort): ")) { fclose(f); return; }
    gap();

    char lower_search[50] = {0};
    for(int i=0; category[i]; i++) lower_search[i] = tolower((unsigned char)category[i]);

    // Pass 1: Gather Statistics
    while(fread(&e,sizeof(e),1,f)) {
        char lower_cat[50] = {0};
        for(int i=0; e.category[i]; i++) lower_cat[i] = tolower((unsigned char)e.category[i]);

        if(strcmp(lower_cat, lower_search) == 0) {
            total+=e.amount;
            count++;
            if(e.amount > highest) { highest = e.amount; high = e; }
            if(e.amount < lowest) { lowest = e.amount; low = e; }
        }
    }

    if(count == 0) {
        printf(COLOR_YELLOW "No transactions found under category '%s'.\n" COLOR_RESET, category);
        fclose(f);
        return;
    }

    // Print Statistics
    printf(COLOR_CYAN "--- SUMMARY FOR '%s' ---\n" COLOR_RESET, category); Sleep(40);
    printf("Total Transactions : %d\n", count); Sleep(40);
    printf("Total Spent        : $%.2f\n", total); Sleep(40);
    printf("Average Spend      : $%.2f\n", total / count); Sleep(40);
    
    gap();
    printf(COLOR_YELLOW "Highest Item:" COLOR_RESET " %02d/%02d/%04d | $%.2f | Notice: %s\n", high.day, high.month, high.year, high.amount, high.note);
    printf(COLOR_YELLOW "Lowest Item :" COLOR_RESET " %02d/%02d/%04d | $%.2f | Notice: %s\n", low.day, low.month, low.year, low.amount, low.note);
    gap();

    // Pass 2: Print Itemized Breakdown
    rewind(f);
    printf(COLOR_CYAN "--- ITEMIZED BREAKDOWN ---\n" COLOR_RESET);
    printf("%-12s  %-18.18s  %12s  %-32.32s\n","DATE","CATEGORY","AMOUNT","NOTE");
    line();

    while(fread(&e,sizeof(e),1,f)) {
        char lower_cat[50] = {0};
        for(int i=0; e.category[i]; i++) lower_cat[i] = tolower((unsigned char)e.category[i]);

        if(strcmp(lower_cat, lower_search) == 0) {
            printf("%02d/%02d/%04d  %-18.18s  %12.2f  %-32.32s\n",
                   e.day,e.month,e.year,e.category,e.amount,e.note);
            Sleep(40);
        }
    }

    fclose(f);
}

double getBudgetLimit(int m, int y)
{
    struct Budget b;
    FILE *f = fopen(BUDGET_FILE_NAME, "rb");
    if (!f) return 0.0; // No budget set yet

    while (fread(&b, sizeof(b), 1, f))
    {
        if (b.month == m && b.year == y)
        {
            fclose(f);
            return b.limit;
        }
    }
    fclose(f);
    return 0.0;
}

double getTotalSpent(int m, int y)
{
    struct Expense e;
    FILE *f = fopen(FILE_NAME, "rb");
    if (!f) return 0.0;

    double total = 0.0;
    while (fread(&e, sizeof(e), 1, f))
    {
        if (e.month == m && e.year == y)
        {
            total += e.amount;
        }
    }
    fclose(f);
    return total;
}

void setMonthlyBudget()
{
    struct Budget b, temp_b;
    FILE *f = fopen(BUDGET_FILE_NAME, "rb");
    // If file doesn't exist, we'll create it via the temp-rename pattern below.
    // No need to fopen(..., "wb+") here as that truncates the live file.

    char tempname[32];
    snprintf(tempname, sizeof(tempname), "temp_budget_%d_%ld.dat", rand(), (long)time(NULL));
    FILE *temp = fopen(tempname, "wb");
    int m, y;
    double limit;
    int found = 0;

    printf(COLOR_CYAN "========== SET MONTHLY BUDGET ==========\n" COLOR_RESET);
    gap();

    if (!temp) {
        printf(COLOR_RED "File error: Could not create temporary file.\n" COLOR_RESET);
        if(f) fclose(f);
        return;
    }

    if (!getValidMonthYear(&m, &y)) { 
        if(f) fclose(f);
        if(temp) fclose(temp);
        remove(tempname);
        return; 
    }
    
    if (!getValidDouble(&limit, "Enter budget limit (0 to remove, 'a' to Abort): ")) {
        if(f) fclose(f);
        if(temp) fclose(temp);
        remove(tempname);
        return;
    }
    if (limit < 0) {
        printf(COLOR_RED "Budget limit cannot be negative!\n" COLOR_RESET);
        if(f) fclose(f); if(temp) fclose(temp); remove(tempname); return;
    }
    gap();

    if (f) {
        rewind(f);
        while (fread(&temp_b, sizeof(temp_b), 1, f))
        {
            if (temp_b.month == m && temp_b.year == y) {
                found = 1;
                if (limit == 0) continue; // Skip writing = Remove

                // Update existing budget
                temp_b.limit = limit;
            }
            if (fwrite(&temp_b, sizeof(temp_b), 1, temp) != 1) {
                printf(COLOR_RED "Error: Write failed during budget update.\n" COLOR_RESET);
                if(f) fclose(f); fclose(temp); remove(tempname);
                return;
            }
        }
    }

    if (!found && limit > 0) {
        // Add new budget
        b.month = m;
        b.year = y;
        b.limit = limit;
        if (fwrite(&b, sizeof(b), 1, temp) != 1) {
            printf(COLOR_RED "Error: Write failed while adding new budget.\n" COLOR_RESET);
            if(f) fclose(f); fclose(temp); remove(tempname);
            return;
        }
    }

    if (f) fclose(f);
    fclose(temp);

    char backup[64];
    snprintf(backup, sizeof(backup), "backup_b_%d.dat", rand());
    if (rename(BUDGET_FILE_NAME, backup) == 0) {
        if (rename(tempname, BUDGET_FILE_NAME) == 0) {
            remove(backup);
        } else {
            rename(backup, BUDGET_FILE_NAME);
            printf(COLOR_RED "Error: Could not update budget database. Restored from backup.\n" COLOR_RESET);
            remove(tempname);
            return;
        }
    } else {
        // If original didn't exist or rename failed, try direct rename (for new files)
        if (rename(tempname, BUDGET_FILE_NAME) != 0) {
            printf(COLOR_RED "Error: Could not save budget database.\n" COLOR_RESET);
            remove(tempname);
            return;
        }
    }

    if (limit == 0) {
        if (found) printf(COLOR_GREEN "Budget for %02d/%04d removed successfully.\n" COLOR_RESET, m, y);
        else printf(COLOR_YELLOW "No budget record found for %02d/%04d to remove.\n" COLOR_RESET, m, y);
    } else {
        printf(COLOR_GREEN "Budget for %02d/%04d set to $%.2f\n" COLOR_RESET, m, y, limit);
    }
}


void viewBudgets() {
    struct Budget b;
    FILE *f = fopen(BUDGET_FILE_NAME, "rb");
    printBoxHeader("ALL SAVED BUDGETS");
    gap();
    if (!f) {
        printf(COLOR_YELLOW "No budget data found.\n" COLOR_RESET);
        return;
    }
    printf("%-15s %-15s %-15s %-10s\n", "MONTH/YEAR", "LIMIT", "SPENT", "% USED");
    line();
    int count = 0;
    while (fread(&b, sizeof(b), 1, f)) {
        double spent = getTotalSpent(b.month, b.year);
        double percent = (b.limit > 0) ? (spent / b.limit) * 100.0 : 0;
        
        char *color = COLOR_GREEN;
        if (percent >= 100.0) color = COLOR_RED;
        else if (percent >= 50.0) color = COLOR_YELLOW;

        printf("%02d/%04d      $%10.2f %s$%10.2f   %5.1f%%%s\n", 
               b.month, b.year, b.limit, color, spent, percent, COLOR_RESET);
        Sleep(40);
        count++;
    }
    fclose(f);
    if (count == 0) printf(COLOR_YELLOW "No budget records saved.\n" COLOR_RESET);
}

void financialSummary()
{
    struct Expense e;
    FILE *f=fopen(FILE_NAME,"rb");

    printBoxHeader("FINANCIAL SUMMARY");
    gap();

    if(!f){ printf(COLOR_RED "Error: Could not open database.\n" COLOR_RESET); gap(); return; }

    double total=0;
    double highest=-DBL_MAX;
    double lowest=DBL_MAX;

    struct Expense high,low;

    int count=0;

    int cat_capacity = 10;
    char (*categories)[50] = malloc(cat_capacity * sizeof(*categories));
    double *totals = malloc(cat_capacity * sizeof(double));

    if (!categories || !totals) {
        printf(COLOR_RED "Memory alloc failed\n" COLOR_RESET);
        if(categories) free(categories);
        if(totals) free(totals);
        fclose(f);
        return;
    }
    
    // Initialize first block totals to 0
    for(int i=0; i<cat_capacity; i++) totals[i] = 0.0f;

    int cat_count=0;

    while(fread(&e,sizeof(e),1,f))
    {
        total+=e.amount;
        count++;

        if(e.amount>highest){ highest=e.amount; high=e; }
        if(e.amount<lowest){ lowest=e.amount; low=e; }

        int index=-1;
        for(int i=0;i<cat_count;i++) {
            if(strcmp(categories[i],e.category)==0) {
                index=i;
                break;
            }
        }

        if(index==-1) {
            if (cat_count >= cat_capacity) {
                int old_cap = cat_capacity;
                cat_capacity *= 2;
                char (*temp_cat)[50] = realloc(categories, cat_capacity * sizeof(*categories));
                double *temp_tot = realloc(totals, cat_capacity * sizeof(double));
                if (!temp_cat || !temp_tot) {
                    printf(COLOR_RED "Memory realloc failed\n" COLOR_RESET);
                    // Critical: categories/totals are only freed if realloc failed.
                    // If one succeeded, temp_cat/temp_tot points to it.
                    if (temp_cat) free(temp_cat); else free(categories);
                    if (temp_tot) free(temp_tot); else free(totals);
                    fclose(f);
                    return;
                }
                categories = temp_cat;
                totals = temp_tot;
                
                // Initialize new totals memory to 0
                for(int j=old_cap; j<cat_capacity; j++) totals[j] = 0.0f;
            }
            strcpy(categories[cat_count],e.category);
            totals[cat_count]=e.amount;
            cat_count++;
        } else {
            totals[index]+=e.amount;
        }
    }

    fclose(f);

    if(count==0){ 
        printf(COLOR_RED "No transactions\n" COLOR_RESET); 
        free(categories); free(totals); 
        return; 
    }

    double max_cat=0;
    int pos_cat=0;
    for(int i=0;i<cat_count;i++) {
        if(totals[i]>max_cat) {
            max_cat=totals[i];
            pos_cat=i;
        }
    }

    printf("Transactions : %d\n",count); Sleep(40);
    printf("Total spent  : $%.2f\n",total); Sleep(40);
    printf("Average      : $%.2f\n",total/count); Sleep(40);
    
    if(cat_count>0) {
        printf("Top category : %-20s ($%.2f)\n",categories[pos_cat],max_cat);
        Sleep(40);
    }

    // ASCII Chart Section
    gap();
    printf(COLOR_CYAN "--- CATEGORY SPENDING CHART ---\n" COLOR_RESET);
    line();
    for (int i = 0; i < cat_count; i++) {
        int bar_len = (max_cat > 0) ? (int)((totals[i] / max_cat) * 30) : 0;
        if (bar_len < 1 && totals[i] > 0) bar_len = 1; // Show at least one block if there is spending
        
        printf("%-20s | ", categories[i]); // Increased to 20
        for (int j = 0; j < bar_len; j++) printf("\xdb"); 
        printf(" $%.2f\n", totals[i]);
        Sleep(40);
    }

    gap();

    printf(COLOR_YELLOW "Highest Expense" COLOR_RESET "\n");
    printf("%-12s  %-18.18s  %12s  %-32.32s\n","DATE","CATEGORY","AMOUNT","NOTE");
    line();
    printf("%02d/%02d/%04d  %-18.18s  %12.2f  %-32.32s\n",
           high.day,high.month,high.year,
           high.category,high.amount,high.note);

    gap();

    printf(COLOR_YELLOW "Lowest Expense" COLOR_RESET "\n");
    printf("%-12s  %-18.18s  %12s  %-32.32s\n","DATE","CATEGORY","AMOUNT","NOTE");
    line();
    printf("%02d/%02d/%04d  %-18.18s  %12.2f  %-32.32s\n",
           low.day,low.month,low.year,
           low.category,low.amount,low.note);

    free(categories);
    free(totals);
}

void reportsMenu() {
    int highlight = 1;
    int num_choices = 5;
    int c = 0;
    char *options[] = {
        "Monthly Detailed Report",
        "Category-wise Analysis",
        "Financial Summary",
        "View All Saved Budgets",
        "Back to Main Menu"
    };

    while (1) {
        printf("\033[2J\033[3J\033[H");
        printBoxHeader("REPORTS & ANALYTICS");

        for (int i = 0; i < num_choices; i++) {
            if (highlight == i + 1) {
                printf("\x1b[43;30m" " > %2d. %-30s " "\x1b[0m" "\n", i + 1, options[i]);
            } else {
                printf("   %2d. %-30s  \n", i + 1, options[i]);
            }
        }
        printBoxFooter();
        printf(COLOR_CYAN " Use [" COLOR_YELLOW "Up" COLOR_CYAN "/" COLOR_YELLOW "Down" COLOR_CYAN "] to navigate, [" COLOR_YELLOW "Enter" COLOR_CYAN "] to select." COLOR_RESET "\n");

        c = getch();
        if (c == 224) {
            c = getch();
            if (c == 72) { highlight--; if (highlight < 1) highlight = num_choices; }
            else if (c == 80) { highlight++; if (highlight > num_choices) highlight = 1; }
        } else if (c == 13) {
            printf("\033[2J\033[3J\033[H"); 
            switch(highlight) {
                case 1: monthlyReport(); pauseScreen(); break;
                case 2: categoryReport(); pauseScreen(); break;
                case 3: financialSummary(); pauseScreen(); break;
                case 4: viewBudgets(); pauseScreen(); break;
                case 5: return;
            }
        }
    }
}





void escapeCSV(char *out, const char *in) {
    int i = 0, j = 0;
    out[j++] = '"';
    while(in[i]) {
        if(in[i] == '"') {
            out[j++] = '"'; 
            out[j++] = '"';
        } else if (in[i] == '\n' || in[i] == '\r') {
            out[j++] = ' ';
        } else {
            out[j++] = in[i];
        }
        i++;
    }
    out[j++] = '"';
    out[j] = '\0';
}

void exportToCSV()
{
    struct Expense e;
    FILE *f=fopen(FILE_NAME,"rb");
    if(!f)
    {
        printBoxHeader("EXPORT TO CSV");
        gap();
        printf(COLOR_RED "No data available to export.\n" COLOR_RESET);
        gap();
        return;
    }

    char tempname[32];
    snprintf(tempname, sizeof(tempname), "temp_csv_%d_%ld.csv", rand(), (long)time(NULL));
    FILE *csv=fopen(tempname,"w");
    if (!csv) {
        printf(COLOR_RED "Error: Could not create export file.\n" COLOR_RESET);
        fclose(f);
        return;
    }

    simulateLoading("Generating CSV File...");

    // Write CSV Header
    if (fprintf(csv, "Date,Category,Amount,Note\n") < 0) {
        printf(COLOR_RED "Error: Failed to write CSV header.\n" COLOR_RESET);
        fclose(f); fclose(csv); remove(tempname);
        return;
    }

    int count = 0;
    int csv_write_ok = 1;
    while(fread(&e,sizeof(e),1,f))
    {
        char escCat[200], escNote[600];
        escapeCSV(escCat, e.category);
        escapeCSV(escNote, e.note);
        if (fprintf(csv, "%02d/%02d/%04d,%s,%.2f,%s\n",
                e.day, e.month, e.year, escCat, e.amount, escNote) < 0) {
            csv_write_ok = 0;
            break;
        }
        count++;
    }

    fclose(f);
    fclose(csv);

    if (!csv_write_ok) {
        printf(COLOR_RED "Error: Failed writing CSV records. Export aborted.\n" COLOR_RESET);
        remove(tempname);
        return;
    }

    char backup[64];
    const char *exportName = "expenses_export.csv";
    snprintf(backup, sizeof(backup), "backup_export_%d.csv", rand());
    
    // Rename current to backup (if it exists)
    if (rename(exportName, backup) != 0) {
        // Probably doesn't exist, which is fine
    }
    
    if (rename(tempname, exportName) == 0) {
        remove(backup);
        printf(COLOR_GREEN "Successfully exported %d records to %s\n" COLOR_RESET, count, exportName);
    } else {
        rename(backup, exportName); // Restore original
        printf(COLOR_RED "Error: Export failed. Original file preserved.\n" COLOR_RESET);
        remove(tempname);
    }
}

#ifdef __EMSCRIPTEN__
EM_ASYNC_JS(int, call_js_import, (), {
    try {
        if (!window.triggerCsvImport) {
            console.error("CSV Import bridge: triggerCsvImport not found");
            return 0;
        }
        const result = await window.triggerCsvImport();
        return result ? 1 : 0;
    } catch (e) {
        console.error("CSV Import bridge error:", e);
        return 0;
    }
});

EM_ASYNC_JS(int, call_js_ai, (const char* summary, int mode), {
    try {
        if (!window.ext_callAI) {
            console.error("AI bridge: ext_callAI not found");
            return 0;
        }
        const result = await window.ext_callAI(summary, mode);
        return result ? 1 : 0;
    } catch (e) {
        console.error("AI bridge error:", e);
        return 0;
    }
});
#else
int call_js_import() { return 0; }
int call_js_ai(const char* s, int m) { return 0; }
#endif

void aiFinancialAdvisor() {
    printBoxHeader("AI FINANCIAL ADVISOR");
    gap();

    printf("Choose your AI connection mode:\n");
    printf(COLOR_CYAN " [1]" COLOR_RESET " Use Built-in Gemini API key\n");
    printf(COLOR_CYAN " [2]" COLOR_RESET " Use my own Gemini API Key\n");
    printf("\nChoice: ");
    
    int mode = 1;
    char buffer[100];
    if (fgets(buffer, sizeof(buffer), stdin) != NULL) {
        if (buffer[0] == '2') mode = 2;
    }
    gap();

    FILE* f = fopen(FILE_NAME, "rb");
    if (!f) {
        printf(COLOR_RED "Advisor Error: Database not found. Add expenses first.\n" COLOR_RESET);
        pauseScreen();
        return;
    }

    struct Expense e;
    char summary[2048] = "";
    int count = 0;

    // Get current time
    time_t t = time(NULL);
    struct tm* now = localtime(&t);
    
    int curM = now->tm_mon + 1;
    int curY = now->tm_year + 1900;

    while (fread(&e, sizeof(struct Expense), 1, f)) {
        if (e.month == curM && e.year == curY) {
            char line[256];
            snprintf(line, sizeof(line), "- %02d/%02d: %s $%.2f (%s)\n", 
                     e.day, e.month, e.category, e.amount, e.note);
            
            if (strlen(summary) + strlen(line) < sizeof(summary) - 50) {
                strcat(summary, line);
            }
            count++;
        }
    }
    fclose(f);

    if (count == 0) {
        printf(COLOR_YELLOW "You have no expenses recorded for this month (%02d/%04d).\n" COLOR_RESET, curM, curY);
        printf("The AI tip will be based on general financial wisdom.\n");
        snprintf(summary, sizeof(summary), "No expenses recorded in %02d/%04d.", curM, curY);
    }

    simulateLoading("Synchronizing with AI Brain...");

    if (!call_js_ai(summary, mode)) {
        printf(COLOR_RED "\n[ERROR]: AI connection stopped.\n" COLOR_RESET);
    }

    printf("\n");
    printf("\n");
    gap();
    flushInput();
    pauseScreen();
}

void importFromCSV() {
    printBoxHeader("FAST-IMPORT ENGINE");
    gap();
    
    printf(COLOR_CYAN "[1/3] Preparing Database..." COLOR_RESET "\n");
    fflush(stdout);
    { FILE *f = fopen(FILE_NAME, "ab"); if (f) fclose(f); }

    printf(COLOR_YELLOW "[2/3] Waiting for File Selection..." COLOR_RESET "\n");
    fflush(stdout);
    if (!call_js_import()) {
        printf(COLOR_RED "      Import portal closed.\n" COLOR_RESET);
        return;
    }

    printf(COLOR_CYAN "[3/3] Processing Data..." COLOR_RESET "\n");
    fflush(stdout);

    FILE *csv = fopen("/expenses_import.csv", "rb");
    if (!csv) {
        printf(COLOR_RED "      ERROR: Portal file access failed.\n" COLOR_RESET);
        pauseScreen();
        return;
    }

    fseek(csv, 0, SEEK_END);
    long size = ftell(csv);
    rewind(csv);

    if (size <= 0) {
        printf(COLOR_RED "      ERROR: Uploaded file is empty.\n" COLOR_RESET);
        fclose(csv);
        pauseScreen();
        return;
    }

    char *buf = malloc(size + 1);
    if (!buf) {
        printf(COLOR_RED "      ERROR: Out of memory.\n" COLOR_RESET);
        fclose(csv);
        pauseScreen();
        return;
    }

    long bytesRead = (long)fread(buf, 1, size, csv);
    buf[bytesRead] = '\0';
    fclose(csv);
    if (bytesRead < size) {
        printf(COLOR_RED "      ERROR: Partial read (%ld/%ld bytes). Import aborted.\n" COLOR_RESET, bytesRead, size);
        free(buf);
        pauseScreen();
        return;
    }
    remove("/expenses_import.csv");

    FILE *dat = fopen(FILE_NAME, "ab");
    if (!dat) {
        printf(COLOR_RED "      ERROR: Could not write to main database.\n" COLOR_RESET);
        free(buf);
        pauseScreen();
        return;
    }
    int imported = 0;
    char *line = strtok(buf, "\r\n");
    int first = 1;

    while (line != NULL) {
        if (first) { 
            printf("      Skipping header...\n");
            fflush(stdout);
            first = 0;
            line = strtok(NULL, "\r\n");
            continue; 
        }

        struct Expense e;
        memset(&e, 0, sizeof(e));
        char dStr[64]={0}, cStr[128]={0}, aStr[64]={0}, nStr[512]={0};
        
        // Manual Simple Split (assuming standard format)
        char *p = line;
        char *fields[4] = {dStr, cStr, aStr, nStr};
        int fSizes[4] = {64, 128, 64, 512};
        
        for (int f = 0; f < 4; f++) {
            int i = 0;
            if (*p == '"') {
                p++;
                while (*p && *p != '"' && i < fSizes[f]-1) fields[f][i++] = *p++;
                if (*p == '"') p++;
            } else {
                while (*p && *p != ',' && i < fSizes[f]-1) fields[f][i++] = *p++;
            }
            if (*p == ',') p++;
        }

        // Validate and Save
        if (sscanf(dStr, "%d/%d/%d", &e.day, &e.month, &e.year) == 3 ||
            sscanf(dStr, "%d-%d-%d", &e.day, &e.month, &e.year) == 3) {
            
            if (validateDate(e.day, e.month, e.year)) {
                strncpy(e.category, cStr, sizeof(e.category)-1);
                e.category[sizeof(e.category)-1] = '\0';
                capitalizeStr(e.category);
                e.amount = atof(aStr);
                strncpy(e.note, nStr, sizeof(e.note)-1);
                e.note[sizeof(e.note)-1] = '\0';

                if (e.amount > 0) {
                    if (isDuplicate(&e)) {
                        printf(COLOR_RED "      [ERROR]: Duplicate detected! Skipping: %02d/%02d/%04d, %s, $%.2f\n" COLOR_RESET, 
                               e.day, e.month, e.year, e.category, e.amount);
                        fflush(stdout);
                    } else {
                        if (fwrite(&e, sizeof(e), 1, dat) != 1) {
                            printf(COLOR_RED "      [ERROR]: Write to database failed at record %d. Stopping import.\n" COLOR_RESET, imported + 1);
                            fflush(stdout);
                            break;
                        }
                        fflush(dat); // Flush so isDuplicate can see it immediately
                        imported++;
                        printf("      + Imported record %d from Date: %s\n", imported, dStr);
                        fflush(stdout);
                    }
                }
            }
        }
        
        line = strtok(NULL, "\r\n");
    }

    fclose(dat);
    free(buf);

    printf(COLOR_GREEN "\nDONE! Successfully imported %d expenses.\n" COLOR_RESET, imported);
    fflush(stdout);
    pauseScreen();
}
