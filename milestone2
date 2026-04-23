/*
 * ============================================================
 *  Save My Semester — Academic Decision Support System
 *  Group 5: Maleeka Ramadan, Habiba Shouman, Salma Mohib
 * ============================================================
 *
 *  Features implemented:
 *   1.  Add assignment (name, subject, deadline, duration,
 *       difficulty, grade weight, progress)
 *   2.  Priority Score algorithm
 *   3.  View ranked task list (calendar-style table)
 *   4.  Overload Detection (weekly & daily)
 *   5.  What-If Simulator
 *   6.  Task Clock + Break Timer (console countdown)
 *   7.  Procrastination Pattern Tracker
 *   8.  Small Wins Mode
 *   9.  Mark task complete / partial progress update
 *  10.  Missed-task auto-reschedule
 *
 *  Compile:  g++ -std=c++17 -o save_my_semester save_my_semester.cpp
 *  Run:      ./save_my_semester
 * ============================================================
 */

#include <iostream>
#include <iomanip>
#include <string>
#include <vector>
#include <algorithm>
#include <chrono>
#include <thread>
#include <cmath>
#include <sstream>
#include <limits>

using namespace std;

// ─────────────────────────────────────────────
//  CONSTANTS
// ─────────────────────────────────────────────
const double SMALL_WIN_THRESHOLD   = 30.0;   // priority score ≤ this → small win
const int    DAILY_HOURS_AVAILABLE = 8;      // assumed productive hours / day
const int    WEEKLY_HOURS_LIMIT    = 40;     // overload threshold (hours/week)
const int    POMODORO_BREAK_MINS   = 5;      // short break every 25 min
const int    POMODORO_WORK_MINS    = 25;

// ─────────────────────────────────────────────
//  DATA STRUCTURE
// ─────────────────────────────────────────────
struct Assignment {
    int    id;
    string name;
    string subject;
    string deadline;        // stored as "YYYY-MM-DD"
    double durationHours;   // estimated hours to complete
    int    difficulty;      // 1 (easy) – 5 (very hard)
    double gradeWeight;     // percentage of final grade (0–100)
    double progress;        // 0–100 %
    bool   completed;
    int    delayCount;      // how many times rescheduled (procrastination tracker)
    double priorityScore;

    // Computed fields
    int daysUntilDeadline;
    double urgency;         // derived from days left
    double timePressure;    // derived from duration vs days left
};

// ─────────────────────────────────────────────
//  GLOBALS
// ─────────────────────────────────────────────
vector<Assignment> tasks;
int nextId = 1;

// ─────────────────────────────────────────────
//  HELPER: clear input buffer
// ─────────────────────────────────────────────
void clearInput() {
    cin.clear();
    cin.ignore(numeric_limits<streamsize>::max(), '\n');
}

// ─────────────────────────────────────────────
//  HELPER: safe integer input with range check
// ─────────────────────────────────────────────
int safeIntInput(const string& prompt, int lo, int hi) {
    int val;
    while (true) {
        cout << prompt;
        if (cin >> val && val >= lo && val <= hi) {
            clearInput();
            return val;
        }
        cout << "  ✗ Please enter a number between " << lo << " and " << hi << ".\n";
        clearInput();
    }
}

double safeDoubleInput(const string& prompt, double lo, double hi) {
    double val;
    while (true) {
        cout << prompt;
        if (cin >> val && val >= lo && val <= hi) {
            clearInput();
            return val;
        }
        cout << "  ✗ Please enter a value between " << lo << " and " << hi << ".\n";
        clearInput();
    }
}

// ─────────────────────────────────────────────
//  HELPER: parse "YYYY-MM-DD" → days from today
//  (simple integer arithmetic, no external libs)
// ─────────────────────────────────────────────
bool isLeapYear(int y) {
    return (y % 4 == 0 && y % 100 != 0) || (y % 400 == 0);
}
int daysInMonth(int m, int y) {
    int days[] = {31,28,31,30,31,30,31,31,30,31,30,31};
    if (m == 2 && isLeapYear(y)) return 29;
    return days[m - 1];
}
int dateToDayNumber(int y, int m, int d) {
    // Gregorian day count from year 1
    int total = 0;
    for (int yr = 1; yr < y; yr++)
        total += isLeapYear(yr) ? 366 : 365;
    for (int mo = 1; mo < m; mo++)
        total += daysInMonth(mo, y);
    total += d;
    return total;
}

// ─────────────────────────────────────────────
//  Get today's date from system clock
// ─────────────────────────────────────────────
void getTodayYMD(int &y, int &m, int &d) {
    time_t t = time(nullptr);
    tm* lt   = localtime(&t);
    y = lt->tm_year + 1900;
    m = lt->tm_mon  + 1;
    d = lt->tm_mday;
}

int daysUntil(const string& deadline) {
    // deadline format: YYYY-MM-DD
    int dy = stoi(deadline.substr(0, 4));
    int dm = stoi(deadline.substr(5, 2));
    int dd = stoi(deadline.substr(8, 2));
    int ty, tm2, td;
    getTodayYMD(ty, tm2, td);
    return dateToDayNumber(dy, dm, dd) - dateToDayNumber(ty, tm2, td);
}

// ─────────────────────────────────────────────
//  PRIORITY SCORE ALGORITHM
//  Score = (Urgency×4) + (GradeWeight×3) + (Difficulty×2)
//          + (TimePressure×2) − (Progress×2)
//  All sub-scores normalised to 0–10 scale before weighting.
// ─────────────────────────────────────────────
void computePriority(Assignment& a) {
    a.daysUntilDeadline = daysUntil(a.deadline);

    // Urgency (0–10): fewer days = higher urgency
    if      (a.daysUntilDeadline <= 0)  a.urgency = 10.0;
    else if (a.daysUntilDeadline <= 1)  a.urgency = 9.0;
    else if (a.daysUntilDeadline <= 2)  a.urgency = 8.0;
    else if (a.daysUntilDeadline <= 3)  a.urgency = 7.0;
    else if (a.daysUntilDeadline <= 5)  a.urgency = 6.0;
    else if (a.daysUntilDeadline <= 7)  a.urgency = 5.0;
    else if (a.daysUntilDeadline <= 10) a.urgency = 4.0;
    else if (a.daysUntilDeadline <= 14) a.urgency = 3.0;
    else if (a.daysUntilDeadline <= 21) a.urgency = 2.0;
    else                                a.urgency = 1.0;

    // Time Pressure (0–10): duration vs days available
    double hoursLeft = max(1.0, (double)a.daysUntilDeadline * DAILY_HOURS_AVAILABLE);
    double ratio     = a.durationHours / hoursLeft;
    a.timePressure   = min(10.0, ratio * 10.0);

    // Normalise inputs
    double normWeight      = a.gradeWeight / 10.0;          // 0–10
    double normDifficulty  = (double)a.difficulty * 2.0;    // 0–10
    double normProgress    = a.progress / 10.0;             // 0–10

    a.priorityScore = (a.urgency       * 4.0)
                    + (normWeight      * 3.0)
                    + (normDifficulty  * 2.0)
                    + (a.timePressure  * 2.0)
                    - (normProgress    * 2.0);

    // Clamp
    a.priorityScore = max(0.0, a.priorityScore);
}

void recomputeAll() {
    for (auto& t : tasks) computePriority(t);
}

// ─────────────────────────────────────────────
//  DISPLAY HELPERS
// ─────────────────────────────────────────────
string repeatChar(char c, int n) {
    return string(n, c);
}

void printHeader(const string& title) {
    cout << "\n" << repeatChar('=', 60) << "\n";
    cout << "  " << title << "\n";
    cout << repeatChar('=', 60) << "\n";
}

string diffLabel(int d) {
    switch(d) {
        case 1: return "Easy     ";
        case 2: return "Moderate ";
        case 3: return "Medium   ";
        case 4: return "Hard     ";
        case 5: return "Very Hard";
        default: return "Unknown  ";
    }
}

string urgencyLabel(double u) {
    if (u >= 9) return "🔴 CRITICAL";
    if (u >= 7) return "🟠 HIGH    ";
    if (u >= 5) return "🟡 MEDIUM  ";
    return             "🟢 LOW     ";
}

// ─────────────────────────────────────────────
//  FEATURE 1 — Add Assignment
// ─────────────────────────────────────────────
void addAssignment() {
    printHeader("ADD NEW ASSIGNMENT");

    Assignment a;
    a.id         = nextId++;
    a.completed  = false;
    a.delayCount = 0;

    clearInput();
    cout << "  Assignment name  : "; getline(cin, a.name);
    cout << "  Subject          : "; getline(cin, a.subject);

    // Deadline with basic validation
    while (true) {
        cout << "  Deadline (YYYY-MM-DD): "; getline(cin, a.deadline);
        if (a.deadline.size() == 10 && a.deadline[4] == '-' && a.deadline[7] == '-') break;
        cout << "  ✗ Invalid format. Try again.\n";
    }

    a.durationHours = safeDoubleInput("  Estimated hours to complete (0.5–200): ", 0.5, 200.0);
    a.difficulty    = safeIntInput   ("  Difficulty (1=Easy … 5=Very Hard): ", 1, 5);
    a.gradeWeight   = safeDoubleInput("  Grade weight (% of final grade, 0–100): ", 0.0, 100.0);
    a.progress      = safeDoubleInput("  Current progress % (0–100): ", 0.0, 100.0);

    computePriority(a);

    tasks.push_back(a);

    cout << "\n  ✔ Assignment added! Priority score: "
         << fixed << setprecision(1) << a.priorityScore << "\n";

    // Small Wins tip
    if (a.priorityScore <= SMALL_WIN_THRESHOLD) {
        cout << "  ⭐ This looks like a quick win — consider doing it soon to build momentum!\n";
    }
}

// ─────────────────────────────────────────────
//  FEATURE 2 — View Ranked Task List (Calendar)
// ─────────────────────────────────────────────
void viewRankedTasks() {
    if (tasks.empty()) { cout << "\n  No assignments added yet.\n"; return; }

    recomputeAll();

    // Sort by priority descending (exclude completed)
    vector<Assignment*> active, done;
    for (auto& t : tasks) {
        if (t.completed) done.push_back(&t);
        else             active.push_back(&t);
    }
    sort(active.begin(), active.end(),
         [](Assignment* a, Assignment* b){ return a->priorityScore > b->priorityScore; });

    printHeader("RANKED TASK CALENDAR");

    // Table header
    cout << left
         << setw(4)  << "#"
         << setw(22) << "Assignment"
         << setw(14) << "Subject"
         << setw(12) << "Deadline"
         << setw(7)  << "Hours"
         << setw(11) << "Difficulty"
         << setw(8)  << "Weight%"
         << setw(8)  << "Done%"
         << setw(8)  << "Score"
         << setw(12) << "Urgency"
         << "\n";
    cout << repeatChar('-', 106) << "\n";

    int rank = 1;
    for (auto* t : active) {
        cout << left
             << setw(4)  << rank++
             << setw(22) << t->name.substr(0, 20)
             << setw(14) << t->subject.substr(0, 12)
             << setw(12) << t->deadline
             << setw(7)  << fixed << setprecision(1) << t->durationHours
             << setw(11) << diffLabel(t->difficulty)
             << setw(8)  << fixed << setprecision(0) << t->gradeWeight
             << setw(8)  << fixed << setprecision(0) << t->progress
             << setw(8)  << fixed << setprecision(1) << t->priorityScore
             << setw(12) << urgencyLabel(t->urgency)
             << "\n";
    }

    if (!done.empty()) {
        cout << "\n  ✔ COMPLETED TASKS:\n";
        for (auto* t : done)
            cout << "    [DONE] " << t->name << " (" << t->subject << ")\n";
    }
}

// ─────────────────────────────────────────────
//  FEATURE 3 — Overload Detection
// ─────────────────────────────────────────────
void overloadDetection() {
    printHeader("OVERLOAD DETECTION");

    recomputeAll();

    double totalHoursThisWeek = 0;
    double totalHoursToday    = 0;
    int    ty, tm2, td;
    getTodayYMD(ty, tm2, td);
    string todayStr = to_string(ty) + "-"
                    + (tm2 < 10 ? "0" : "") + to_string(tm2) + "-"
                    + (td  < 10 ? "0" : "") + to_string(td);

    for (auto& t : tasks) {
        if (t.completed) continue;
        if (t.daysUntilDeadline <= 7 && t.daysUntilDeadline >= 0)
            totalHoursThisWeek += t.durationHours * (1.0 - t.progress / 100.0);
        if (t.deadline == todayStr)
            totalHoursToday    += t.durationHours * (1.0 - t.progress / 100.0);
    }

    cout << "  Hours of active work due this week : "
         << fixed << setprecision(1) << totalHoursThisWeek << " h\n";
    cout << "  Weekly capacity                    : "
         << WEEKLY_HOURS_LIMIT << " h\n";

    if (totalHoursThisWeek > WEEKLY_HOURS_LIMIT) {
        cout << "\n  🚨 OVERLOAD WARNING!\n";
        cout << "     You have " << fixed << setprecision(1)
             << (totalHoursThisWeek - WEEKLY_HOURS_LIMIT)
             << " extra hours this week.\n";
        cout << "  Suggestions:\n"
             << "    • Talk to professors about extensions\n"
             << "    • Focus on highest priority scores first\n"
             << "    • Use Small Wins Mode to build momentum\n";
    } else {
        cout << "\n  ✅ Workload looks manageable this week!\n";
    }

    if (totalHoursToday > DAILY_HOURS_AVAILABLE) {
        cout << "\n  ⚠  Today's tasks exceed your daily capacity ("
             << DAILY_HOURS_AVAILABLE << " h).\n"
             << "     Consider moving lower-priority tasks to tomorrow.\n";
    }
}

// ─────────────────────────────────────────────
//  FEATURE 4 — What-If Simulator
// ─────────────────────────────────────────────
void whatIfSimulator() {
    if (tasks.empty()) { cout << "\n  No assignments to simulate.\n"; return; }

    printHeader("WHAT-IF SIMULATOR");

    cout << "  Choose a scenario:\n"
         << "  1) What if I skip a task today?\n"
         << "  2) What if I only have N hours available tonight?\n"
         << "  3) What if I finish a specific task first?\n"
         << "  Enter choice: ";

    int choice = safeIntInput("", 1, 3);

    recomputeAll();

    if (choice == 1) {
        // List active tasks
        cout << "\n  Which task would you skip? (enter ID)\n";
        for (auto& t : tasks)
            if (!t.completed)
                cout << "    [" << t.id << "] " << t.name << " (score "
                     << fixed << setprecision(1) << t.priorityScore << ")\n";
        int skipId = safeIntInput("  Task ID: ", 1, nextId - 1);

        for (auto& t : tasks) {
            if (t.id == skipId) {
                t.delayCount++;
                cout << "\n  ⚠  Skipping '" << t.name << "' (delay count: " << t.delayCount << ").\n";
                if (t.daysUntilDeadline <= 2)
                    cout << "  🔴 DANGER: This is due in ≤2 days. Skipping is very risky!\n";
                else {
                    // Simulate rescheduling — add 1 day to deadline display
                    cout << "  The task will be carried over to tomorrow. "
                         << "New priority score (after +1 urgency day): ";
                    Assignment sim = t;
                    sim.daysUntilDeadline = max(0, sim.daysUntilDeadline - 1);
                    computePriority(sim);
                    cout << fixed << setprecision(1) << sim.priorityScore << "\n";
                    cout << "  Urgency shift: " << urgencyLabel(t.urgency)
                         << " → " << urgencyLabel(sim.urgency) << "\n";
                }
                break;
            }
        }

    } else if (choice == 2) {
        double hours = safeDoubleInput("  How many hours do you have tonight? (0.5–12): ", 0.5, 12.0);
        double used  = 0;
        cout << "\n  With " << hours << " hours, here's what you can realistically complete:\n";
        cout << "  " << repeatChar('-', 50) << "\n";

        // Sort by priority
        vector<Assignment*> sorted;
        for (auto& t : tasks) if (!t.completed) sorted.push_back(&t);
        sort(sorted.begin(), sorted.end(),
             [](Assignment* a, Assignment* b){ return a->priorityScore > b->priorityScore; });

        for (auto* t : sorted) {
            double remaining = t->durationHours * (1.0 - t->progress / 100.0);
            if (used + remaining <= hours) {
                cout << "  ✔ " << t->name << " (" << fixed << setprecision(1) << remaining << " h)\n";
                used += remaining;
            } else if (used < hours) {
                double partial = hours - used;
                int newProg = (int)(t->progress + (partial / t->durationHours) * 100.0);
                newProg = min(99, newProg);
                cout << "  ~ " << t->name << " — partial: ~" << newProg << "% done\n";
                used = hours;
            }
        }
        cout << "  Total hours used: " << fixed << setprecision(1) << used << " / " << hours << "\n";

    } else {
        // Finish a task first
        cout << "\n  Which task would you finish first? (enter ID)\n";
        for (auto& t : tasks)
            if (!t.completed)
                cout << "    [" << t.id << "] " << t.name << " (score "
                     << fixed << setprecision(1) << t.priorityScore << ")\n";
        int firstId = safeIntInput("  Task ID: ", 1, nextId - 1);

        cout << "\n  If you finish task #" << firstId << " first:\n";
        for (auto& t : tasks) {
            if (t.id == firstId) {
                Assignment sim = t;
                sim.progress = 100;
                computePriority(sim);
                cout << "  '" << t.name << "' priority drops to "
                     << fixed << setprecision(1) << sim.priorityScore
                     << " and will be removed from your active list.\n";
                cout << "  ⏱  Estimated time needed: "
                     << t.durationHours * (1.0 - t.progress / 100.0)
                     << " hours.\n";
            }
        }
        cout << "\n  Remaining tasks ranked after completing it:\n";
        vector<Assignment*> rest;
        for (auto& t : tasks)
            if (!t.completed && t.id != firstId) rest.push_back(&t);
        sort(rest.begin(), rest.end(),
             [](Assignment* a, Assignment* b){ return a->priorityScore > b->priorityScore; });
        int r = 1;
        for (auto* t : rest)
            cout << "  " << r++ << ". " << t->name
                 << " (score " << fixed << setprecision(1) << t->priorityScore << ")\n";
    }
}

// ─────────────────────────────────────────────
//  FEATURE 5 — Task Clock + Break Timer
// ─────────────────────────────────────────────
void taskClock() {
    if (tasks.empty()) { cout << "\n  No assignments available.\n"; return; }

    printHeader("TASK CLOCK + BREAK TIMER");

    cout << "  Select a task to start (enter ID):\n";
    for (auto& t : tasks)
        if (!t.completed)
            cout << "    [" << t.id << "] " << t.name
                 << " — est. " << t.durationHours << " h\n";

    int tid = safeIntInput("  Task ID: ", 1, nextId - 1);

    Assignment* selected = nullptr;
    for (auto& t : tasks)
        if (t.id == tid) { selected = &t; break; }

    if (!selected || selected->completed) {
        cout << "  Task not found or already completed.\n"; return;
    }

    double remaining = selected->durationHours * (1.0 - selected->progress / 100.0);
    int    totalMins = (int)(remaining * 60);
    int    pomodoros = totalMins / POMODORO_WORK_MINS;

    cout << "\n  Starting: " << selected->name << "\n";
    cout << "  Remaining work: " << fixed << setprecision(1)
         << remaining << " h  (" << totalMins << " min)\n";
    cout << "  Suggested Pomodoros: " << pomodoros << " × 25 min + "
         << (totalMins % POMODORO_WORK_MINS) << " min\n\n";

    cout << "  Press ENTER to simulate one Pomodoro session (25 min work → 5 min break)...\n";
    clearInput();
    cin.get();

    // Simulate countdown (abbreviated for console UX — every second = 1 "virtual minute")
    cout << "\n  ⏱  WORK SESSION (25 min)\n";
    for (int m = POMODORO_WORK_MINS; m >= 1; m--) {
        cout << "\r  Time remaining: " << setw(2) << m << " min   " << flush;
        this_thread::sleep_for(chrono::milliseconds(200)); // 200ms per "minute" in demo
    }
    cout << "\n  ✅ Work session complete! Take a " << POMODORO_BREAK_MINS << "-minute break.\n";

    this_thread::sleep_for(chrono::milliseconds(500));

    cout << "\n  ☕ BREAK (" << POMODORO_BREAK_MINS << " min)\n";
    for (int m = POMODORO_BREAK_MINS; m >= 1; m--) {
        cout << "\r  Break remaining: " << setw(2) << m << " min   " << flush;
        this_thread::sleep_for(chrono::milliseconds(200));
    }
    cout << "\n  🔔 Break over! Back to work.\n";

    // Update progress
    double sessionProgress = min(100.0, selected->progress + (POMODORO_WORK_MINS / (remaining * 60.0)) * 100.0);
    selected->progress = sessionProgress;
    computePriority(*selected);
    cout << "  Progress updated to " << fixed << setprecision(0) << selected->progress << "%.\n";
}

// ─────────────────────────────────────────────
//  FEATURE 6 — Procrastination Pattern Tracker
// ─────────────────────────────────────────────
void procrastinationTracker() {
    printHeader("PROCRASTINATION PATTERN TRACKER");

    bool found = false;
    for (auto& t : tasks) {
        if (!t.completed && t.delayCount > 0) {
            found = true;
            cout << "  ⚠  '" << t.name << "' has been delayed " << t.delayCount << " time(s).\n";
            if (t.delayCount >= 3)
                cout << "     🔴 Pattern detected! You keep pushing this task. "
                     << "Consider breaking it into smaller pieces.\n";
            else if (t.delayCount == 2)
                cout << "     🟠 You've delayed this twice already. Try scheduling a fixed time.\n";
            else
                cout << "     🟡 One delay noted. Watch out for recurring avoidance.\n";
        }
    }
    if (!found)
        cout << "  ✅ No procrastination patterns detected. Keep it up!\n";
}

// ─────────────────────────────────────────────
//  FEATURE 7 — Small Wins Mode
// ─────────────────────────────────────────────
void smallWinsMode() {
    printHeader("SMALL WINS MODE");
    cout << "  Feeling overwhelmed? Here are your quickest tasks to build momentum:\n\n";

    recomputeAll();

    vector<Assignment*> wins;
    for (auto& t : tasks)
        if (!t.completed && t.priorityScore <= SMALL_WIN_THRESHOLD)
            wins.push_back(&t);

    if (wins.empty()) {
        cout << "  No 'small win' tasks right now (all have high priority scores).\n";
        cout << "  Tip: Break your largest task into subtasks and add them separately.\n";
        return;
    }

    sort(wins.begin(), wins.end(),
         [](Assignment* a, Assignment* b){ return a->durationHours < b->durationHours; });

    int rank = 1;
    for (auto* t : wins) {
        double rem = t->durationHours * (1.0 - t->progress / 100.0);
        cout << "  " << rank++ << ". " << t->name
             << " (" << t->subject << ") — "
             << fixed << setprecision(1) << rem << " h remaining\n";
    }
}

// ─────────────────────────────────────────────
//  FEATURE 8 — Mark Task Complete / Update Progress
// ─────────────────────────────────────────────
void updateProgress() {
    if (tasks.empty()) { cout << "\n  No tasks found.\n"; return; }

    printHeader("UPDATE TASK PROGRESS");

    for (auto& t : tasks)
        if (!t.completed)
            cout << "  [" << t.id << "] " << t.name
                 << "  (progress: " << (int)t.progress << "%)\n";

    int tid = safeIntInput("  Task ID to update: ", 1, nextId - 1);

    for (auto& t : tasks) {
        if (t.id == tid) {
            if (t.completed) { cout << "  Already completed.\n"; return; }
            cout << "  1) Mark as 100% complete\n"
                 << "  2) Update progress %\n";
            int choice = safeIntInput("  Choice: ", 1, 2);
            if (choice == 1) {
                t.completed = true;
                t.progress  = 100;
                cout << "  🎉 '" << t.name << "' marked as complete!\n";
            } else {
                t.progress = safeDoubleInput("  New progress % (0–100): ", 0.0, 100.0);
                computePriority(t);
                cout << "  ✔ Progress updated. New priority score: "
                     << fixed << setprecision(1) << t.priorityScore << "\n";
            }
            return;
        }
    }
    cout << "  Task ID not found.\n";
}

// ─────────────────────────────────────────────
//  FEATURE 9 — Auto-Reschedule Missed Tasks
// ─────────────────────────────────────────────
void autoReschedule() {
    printHeader("AUTO-RESCHEDULE MISSED TASKS");

    recomputeAll();
    bool found = false;

    for (auto& t : tasks) {
        if (!t.completed && t.daysUntilDeadline < 0) {
            found = true;
            t.delayCount++;
            // Push deadline to today
            int ty, tm2, td;
            getTodayYMD(ty, tm2, td);
            t.deadline = to_string(ty) + "-"
                       + (tm2 < 10 ? "0" : "") + to_string(tm2) + "-"
                       + (td  < 10 ? "0" : "") + to_string(td);
            computePriority(t);
            cout << "  ⚠  '" << t.name << "' was overdue. Rescheduled to today.\n"
                 << "     Delay count: " << t.delayCount << "\n"
                 << "     New priority: " << fixed << setprecision(1) << t.priorityScore << "\n\n";
        }
    }
    if (!found) cout << "  ✅ No overdue tasks found.\n";
}

// ─────────────────────────────────────────────
//  BURNOUT / STRESS ESTIMATOR
// ─────────────────────────────────────────────
void burnoutReader() {
    printHeader("BURNOUT / STRESS ESTIMATOR");

    recomputeAll();

    double stressScore = 0;
    int    criticalCount = 0;
    for (auto& t : tasks) {
        if (!t.completed) {
            stressScore += t.priorityScore;
            if (t.urgency >= 8) criticalCount++;
        }
    }

    cout << "  Aggregate stress index: " << fixed << setprecision(1) << stressScore << "\n";
    cout << "  Critical tasks (due ≤2 days): " << criticalCount << "\n\n";

    if (stressScore > 200 || criticalCount >= 3) {
        cout << "  🔴 HIGH BURNOUT RISK\n"
             << "  Recommendations:\n"
             << "    • Prioritise sleep — productivity drops sharply without it.\n"
             << "    • Use the What-If Simulator to see what can be deferred.\n"
             << "    • Talk to your instructor about extensions for lower-weight tasks.\n"
             << "    • Try the Small Wins Mode to regain momentum.\n";
    } else if (stressScore > 100) {
        cout << "  🟠 MODERATE STRESS — manageable but stay on top of things.\n";
    } else {
        cout << "  🟢 LOW STRESS — you're doing great! Keep your schedule.\n";
    }
}

// ─────────────────────────────────────────────
//  MAIN MENU
// ─────────────────────────────────────────────
void printMenu() {
    cout << "\n";
    cout << "╔══════════════════════════════════════════════╗\n";
    cout << "║       💜  SAVE MY SEMESTER  💜               ║\n";
    cout << "║        Plan · Prioritize · Succeed           ║\n";
    cout << "╠══════════════════════════════════════════════╣\n";
    cout << "║  1.  Add Assignment                          ║\n";
    cout << "║  2.  View Ranked Task Calendar               ║\n";
    cout << "║  3.  Overload Detection                      ║\n";
    cout << "║  4.  What-If Simulator                       ║\n";
    cout << "║  5.  Task Clock + Break Timer                ║\n";
    cout << "║  6.  Procrastination Pattern Tracker         ║\n";
    cout << "║  7.  Small Wins Mode                         ║\n";
    cout << "║  8.  Update Task Progress / Mark Complete    ║\n";
    cout << "║  9.  Auto-Reschedule Missed Tasks            ║\n";
    cout << "║ 10.  Burnout / Stress Estimator              ║\n";
    cout << "║  0.  Exit                                    ║\n";
    cout << "╚══════════════════════════════════════════════╝\n";
    cout << "  Enter choice: ";
}

// ─────────────────────────────────────────────
//  DEMO: pre-load sample data so the app is
//  immediately usable during the presentation.
// ─────────────────────────────────────────────
void loadSampleData() {
    // Helper lambda
    auto addTask = [&](const string& name, const string& subj,
                       const string& dl, double dur, int diff,
                       double weight, double prog) {
        Assignment a;
        a.id           = nextId++;
        a.name         = name;
        a.subject      = subj;
        a.deadline     = dl;
        a.durationHours= dur;
        a.difficulty   = diff;
        a.gradeWeight  = weight;
        a.progress     = prog;
        a.completed    = false;
        a.delayCount   = 0;
        computePriority(a);
        tasks.push_back(a);
    };

    // Build dates relative to today
    int ty, tm2, td;
    getTodayYMD(ty, tm2, td);

    // Create deadline strings N days from today
    auto futureDate = [&](int daysAhead) -> string {
        int y = ty, m = tm2, d = td + daysAhead;
        while (d > daysInMonth(m, y)) { d -= daysInMonth(m, y); m++; if (m > 12) { m = 1; y++; } }
        ostringstream ss;
        ss << y << "-" << (m < 10 ? "0" : "") << m << "-" << (d < 10 ? "0" : "") << d;
        return ss.str();
    };

    addTask("Data Structures Assignment 3", "CS",       futureDate(2),  4.0, 4, 15, 30);
    addTask("Calculus Problem Set 5",       "MATH",     futureDate(4),  3.0, 3, 10, 60);
    addTask("History Essay Draft",          "HIST",     futureDate(7),  5.0, 3, 20,  0);
    addTask("Physics Lab Report",           "PHYS",     futureDate(1),  2.0, 2,  8, 80);
    addTask("English Reading Response",     "ENG",      futureDate(3),  1.5, 1,  5, 50);
    addTask("OOP Group Project Milestone",  "CS",       futureDate(10), 8.0, 5, 25,  0);
    addTask("Stats Quiz Prep",              "STAT",     futureDate(5),  2.0, 3, 12, 20);

    // Give one task some delay history for demo
    tasks[0].delayCount = 2;
}

// ─────────────────────────────────────────────
//  ENTRY POINT
// ─────────────────────────────────────────────
int main() {
    cout << "\n  Loading Save My Semester with sample data...\n";
    loadSampleData();
    cout << "  " << tasks.size() << " sample assignments loaded.\n";

    int choice;
    do {
        printMenu();
        if (!(cin >> choice)) {
            clearInput();
            choice = -1;
        }

        switch (choice) {
            case 1:  addAssignment();          break;
            case 2:  viewRankedTasks();        break;
            case 3:  overloadDetection();      break;
            case 4:  whatIfSimulator();        break;
            case 5:  taskClock();              break;
            case 6:  procrastinationTracker(); break;
            case 7:  smallWinsMode();          break;
            case 8:  updateProgress();         break;
            case 9:  autoReschedule();         break;
            case 10: burnoutReader();          break;
            case 0:
                cout << "\n  Goodbye! Stay on top of your work. 💜\n\n";
                break;
            default:
                cout << "  ✗ Invalid choice. Please enter 0–10.\n";
        }
    } while (choice != 0);

    return 0;
}
