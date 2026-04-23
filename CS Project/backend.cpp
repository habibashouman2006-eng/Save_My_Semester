#include <arpa/inet.h>
#include <chrono>
#include <ctime>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <limits>
#include <netinet/in.h>
#include <sstream>
#include <string>
#include <sys/socket.h>
#include <sys/types.h>
#include <thread>
#include <unistd.h>
#include <vector>

using namespace std;

const double SMALL_WIN_THRESHOLD   = 30.0;
const int    DAILY_HOURS_AVAILABLE = 8;
const int    WEEKLY_HOURS_LIMIT    = 40;

struct Assignment {
    int    id;
    string name;
    string subject;
    string deadline;
    double durationHours;
    int    difficulty;
    double gradeWeight;
    double progress;
    bool   completed;
    int    delayCount;
    double priorityScore;
    int    daysUntilDeadline;
    double urgency;
    double timePressure;
};

vector<Assignment> tasks;
int nextId = 1;

bool isLeapYear(int y) {
    return (y % 4 == 0 && y % 100 != 0) || (y % 400 == 0);
}

int daysInMonth(int m, int y) {
    int days[] = {31,28,31,30,31,30,31,31,30,31,30,31};
    if (m == 2 && isLeapYear(y)) return 29;
    return days[m - 1];
}

int dateToDayNumber(int y, int m, int d) {
    int total = 0;
    for (int yr = 1; yr < y; yr++)
        total += isLeapYear(yr) ? 366 : 365;
    for (int mo = 1; mo < m; mo++)
        total += daysInMonth(mo, y);
    total += d;
    return total;
}

void getTodayYMD(int &y, int &m, int &d) {
    time_t t = time(nullptr);
    tm* lt   = localtime(&t);
    y = lt->tm_year + 1900;
    m = lt->tm_mon + 1;
    d = lt->tm_mday;
}

int daysUntil(const string& deadline) {
    int dy = stoi(deadline.substr(0, 4));
    int dm = stoi(deadline.substr(5, 2));
    int dd = stoi(deadline.substr(8, 2));
    int ty, tm2, td;
    getTodayYMD(ty, tm2, td);
    return dateToDayNumber(dy, dm, dd) - dateToDayNumber(ty, tm2, td);
}

void computePriority(Assignment& a) {
    a.daysUntilDeadline = daysUntil(a.deadline);
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
    double hoursLeft = max(1.0, (double)a.daysUntilDeadline * DAILY_HOURS_AVAILABLE);
    double ratio = a.durationHours / hoursLeft;
    a.timePressure = min(10.0, ratio * 10.0);
    double normWeight = a.gradeWeight / 10.0;
    double normDifficulty = (double)a.difficulty * 2.0;
    double normProgress = a.progress / 10.0;
    a.priorityScore = (a.urgency * 4.0)
                    + (normWeight * 3.0)
                    + (normDifficulty * 2.0)
                    + (a.timePressure * 2.0)
                    - (normProgress * 2.0);
    a.priorityScore = max(0.0, a.priorityScore);
}

void recomputeAll() {
    for (auto& t : tasks) computePriority(t);
}

string readFile(const string& path) {
    ifstream file(path);
    if (!file.is_open()) return "";
    string content((istreambuf_iterator<char>(file)), istreambuf_iterator<char>());
    return content;
}

string escapeJson(const string& value) {
    string out;
    for (char c : value) {
        switch (c) {
            case '\\': out += "\\\\"; break;
            case '"': out += "\\\""; break;
            case '\n': out += "\\n"; break;
            case '\r': out += "\\r"; break;
            case '\t': out += "\\t"; break;
            default: out += c;
        }
    }
    return out;
}

string toJsonString(const string& key, const string& value) {
    return "\"" + key + "\": \"" + escapeJson(value) + "\"";
}

string toJsonNumber(const string& key, double value) {
    ostringstream ss;
    ss << "\"" << key << "\": " << fixed << setprecision(1) << value;
    return ss.str();
}

string toJsonInt(const string& key, int value) {
    ostringstream ss;
    ss << "\"" << key << "\": " << value;
    return ss.str();
}

string assignmentToJson(const Assignment& a) {
    ostringstream ss;
    ss << "{"
       << toJsonInt("id", a.id) << ","
       << toJsonString("name", a.name) << ","
       << toJsonString("subject", a.subject) << ","
       << toJsonString("deadline", a.deadline) << ","
       << toJsonNumber("durationHours", a.durationHours) << ","
       << toJsonInt("difficulty", a.difficulty) << ","
       << toJsonNumber("gradeWeight", a.gradeWeight) << ","
       << toJsonNumber("progress", a.progress) << ","
       << toJsonInt("completed", a.completed ? 1 : 0) << ","
       << toJsonInt("delayCount", a.delayCount) << ","
       << toJsonNumber("priorityScore", a.priorityScore) << ","
       << toJsonInt("daysUntilDeadline", a.daysUntilDeadline) << ","
       << toJsonNumber("urgency", a.urgency) << ","
       << toJsonNumber("timePressure", a.timePressure)
       << "}";
    return ss.str();
}

string tasksToJson() {
    recomputeAll();
    vector<Assignment*> sorted;
    for (auto& t : tasks) sorted.push_back(&t);
    sort(sorted.begin(), sorted.end(),
         [](Assignment* a, Assignment* b){ return a->priorityScore > b->priorityScore; });
    ostringstream ss;
    ss << "[";
    bool first = true;
    for (auto* t : sorted) {
        if (!first) ss << ",";
        ss << assignmentToJson(*t);
        first = false;
    }
    ss << "]";
    return ss.str();
}

string getJsonValue(const string& text, const string& key) {
    string needle = "\"" + key + "\"";
    size_t pos = text.find(needle);
    if (pos == string::npos) return "";
    pos = text.find(':', pos);
    if (pos == string::npos) return "";
    pos++;
    while (pos < text.size() && isspace((unsigned char)text[pos])) pos++;
    if (pos >= text.size()) return "";
    if (text[pos] == '"') {
        pos++;
        size_t end = text.find('"', pos);
        if (end == string::npos) return "";
        return text.substr(pos, end - pos);
    } else if (text.substr(pos, 4) == "true") {
        return "true";
    } else if (text.substr(pos, 5) == "false") {
        return "false";
    } else {
        // number
        size_t end = pos;
        while (end < text.size() && (isdigit((unsigned char)text[end]) || text[end] == '.' || text[end] == '-' || text[end] == '+')) end++;
        return text.substr(pos, end - pos);
    }
}

int getJsonInt(const string& text, const string& key, int fallback = 0) {
    string value = getJsonValue(text, key);
    if (value.empty()) return fallback;
    return stoi(value);
}

double getJsonDouble(const string& text, const string& key, double fallback = 0.0) {
    string value = getJsonValue(text, key);
    if (value.empty()) return fallback;
    return stod(value);
}

bool getJsonBool(const string& text, const string& key, bool fallback = false) {
    string value = getJsonValue(text, key);
    if (value.empty()) return fallback;
    return value == "true" || value == "1";
}

void addSampleData() {
    auto addTask = [&](const string& name, const string& subj,
                       const string& dl, double dur, int diff,
                       double weight, double prog) {
        Assignment a;
        a.id = nextId++;
        a.name = name;
        a.subject = subj;
        a.deadline = dl;
        a.durationHours = dur;
        a.difficulty = diff;
        a.gradeWeight = weight;
        a.progress = prog;
        a.completed = false;
        a.delayCount = 0;
        computePriority(a);
        tasks.push_back(a);
    };
    int ty, tm2, td;
    getTodayYMD(ty, tm2, td);
    auto futureDate = [&](int daysAhead) {
        int y = ty, m = tm2, d = td + daysAhead;
        while (d > daysInMonth(m, y)) {
            d -= daysInMonth(m, y);
            m++;
            if (m > 12) { m = 1; y++; }
        }
        ostringstream ss;
        ss << y << "-" << setw(2) << setfill('0') << m << "-" << setw(2) << setfill('0') << d;
        return ss.str();
    };
    addTask("Data Structures Assignment 3", "CS",       futureDate(2),  4.0, 4, 15, 30);
    addTask("Calculus Problem Set 5",       "MATH",     futureDate(4),  3.0, 3, 10, 60);
    addTask("History Essay Draft",          "HIST",     futureDate(7),  5.0, 3, 20,  0);
    addTask("Physics Lab Report",           "PHYS",     futureDate(1),  2.0, 2,  8, 80);
    addTask("English Reading Response",     "ENG",      futureDate(3),  1.5, 1,  5, 50);
    addTask("OOP Group Project Milestone",  "CS",       futureDate(10), 8.0, 5, 25,  0);
    addTask("Stats Quiz Prep",              "STAT",     futureDate(5),  2.0, 3, 12, 20);
    if (!tasks.empty()) tasks[0].delayCount = 2;
}

string jsonStatus(const string& status, const string& message) {
    return "{\"status\": \"" + status + "\", \"message\": \"" + escapeJson(message) + "\"}";
}

string handleApiRequest(const string& method, const string& path, const string& body) {
    if (path == "/api/tasks") {
        return tasksToJson();
    }
    if (path == "/api/add" && method == "POST") {
        Assignment a;
        a.id = nextId++;
        a.name = getJsonValue(body, "name");
        a.subject = getJsonValue(body, "subject");
        a.deadline = getJsonValue(body, "deadline");
        a.durationHours = getJsonDouble(body, "durationHours", 1.0);
        a.difficulty = getJsonInt(body, "difficulty", 1);
        a.gradeWeight = getJsonDouble(body, "gradeWeight", 0.0);
        a.progress = getJsonDouble(body, "progress", 0.0);
        a.completed = false;
        a.delayCount = 0;
        computePriority(a);
        tasks.push_back(a);
        return jsonStatus("ok", "Assignment added successfully.");
    }
    if (path == "/api/update" && method == "POST") {
        int id = getJsonInt(body, "id", -1);
        double progress = getJsonDouble(body, "progress", -1.0);
        bool completed = getJsonBool(body, "completed", false);
        for (auto& t : tasks) {
            if (t.id == id) {
                if (progress >= 0) t.progress = progress;
                if (completed) {
                    t.completed = true;
                    t.progress = 100.0;
                }
                computePriority(t);
                return jsonStatus("ok", "Task updated.");
            }
        }
        return jsonStatus("error", "Task ID not found.");
    }
    if (path == "/api/overload") {
        recomputeAll();
        double totalHoursThisWeek = 0;
        double totalHoursToday = 0;
        int ty, tm2, td;
        getTodayYMD(ty, tm2, td);
        ostringstream ss;
        string todayStr = to_string(ty) + "-" + (tm2 < 10 ? "0" : "") + to_string(tm2) + "-" + (td < 10 ? "0" : "") + to_string(td);
        for (auto& t : tasks) {
            if (t.completed) continue;
            if (t.daysUntilDeadline <= 7 && t.daysUntilDeadline >= 0)
                totalHoursThisWeek += t.durationHours * (1.0 - t.progress / 100.0);
            if (t.deadline == todayStr)
                totalHoursToday += t.durationHours * (1.0 - t.progress / 100.0);
        }
        string explanation = "This week: " + to_string((int)totalHoursThisWeek) + " h / " + to_string(WEEKLY_HOURS_LIMIT) + " h limit. Today: " + to_string((int)totalHoursToday) + " h / " + to_string(DAILY_HOURS_AVAILABLE) + " h capacity. If over, consider deferring tasks or seeking extensions.";
        ss << "{"
           << toJsonNumber("weeklyHours", totalHoursThisWeek) << ","
           << toJsonNumber("weeklyLimit", WEEKLY_HOURS_LIMIT) << ","
           << toJsonNumber("todayHours", totalHoursToday) << ","
           << toJsonNumber("dailyCapacity", DAILY_HOURS_AVAILABLE) << ","
           << toJsonString("explanation", explanation)
           << "}";
        return ss.str();
    }
    if (path == "/api/small-wins") {
        recomputeAll();
        ostringstream ss;
        ss << "{"
           << "\"wins\": [";
        bool first = true;
        for (auto& t : tasks) {
            if (!t.completed && t.priorityScore <= SMALL_WIN_THRESHOLD) {
                if (!first) ss << ",";
                ss << assignmentToJson(t);
                first = false;
            }
        }
        ss << "],"
           << toJsonString("explanation", "Small wins are tasks with priority score ≤ 30. These are quick to complete and can build momentum by giving you a sense of accomplishment.")
           << "}";
        return ss.str();
    }
    if (path == "/api/burnout") {
        recomputeAll();
        double stressScore = 0;
        int criticalCount = 0;
        for (auto& t : tasks) {
            if (!t.completed) {
                stressScore += t.priorityScore;
                if (t.urgency >= 8) criticalCount++;
            }
        }
        double stressPercent = min(100.0, (stressScore / 200.0) * 100.0);
        ostringstream ss;
        ss << "{"
           << toJsonNumber("stressPercent", stressPercent) << ","
           << toJsonInt("criticalCount", criticalCount) << ","
           << toJsonNumber("totalScore", stressScore) << ","
           << toJsonString("explanation", "Your stress level is " + to_string((int)stressPercent) + "% based on your total priority score (" + to_string((int)stressScore) + ") and " + to_string(criticalCount) + " critical tasks (due in ≤2 days). High stress may indicate burnout risk.")
           << "}";
        return ss.str();
    }
    if (path == "/api/delete" && method == "POST") {
        int id = getJsonInt(body, "id", -1);
        auto it = remove_if(tasks.begin(), tasks.end(), [id](const Assignment& t){ return t.id == id; });
        if (it != tasks.end()) {
            tasks.erase(it, tasks.end());
            return jsonStatus("ok", "Task deleted successfully.");
        }
        return jsonStatus("error", "Task ID not found.");
    }
    return jsonStatus("error", "Unknown endpoint.");
}

pair<string, string> parseRequestLine(const string& request) {
    istringstream ss(request);
    string method, uri, version;
    ss >> method >> uri >> version;
    return {method, uri};
}

string makeHttpResponse(const string& status, const string& mimeType, const string& body) {
    ostringstream ss;
    ss << "HTTP/1.1 " << status << "\r\n"
       << "Content-Type: " << mimeType << "; charset=UTF-8\r\n"
       << "Content-Length: " << body.size() << "\r\n"
       << "Connection: close\r\n"
       << "\r\n"
       << body;
    return ss.str();
}

string readHttpRequest(int clientFd) {
    string request;
    char buffer[4096];
    ssize_t received;
    while ((received = recv(clientFd, buffer, sizeof(buffer), 0)) > 0) {
        request.append(buffer, received);
        if (request.find("\r\n\r\n") != string::npos) break;
    }
    if (request.empty()) return request;
    size_t headerEnd = request.find("\r\n\r\n");
    if (headerEnd == string::npos) return request;
    string headers = request.substr(0, headerEnd);
    size_t contentLengthPos = headers.find("Content-Length:");
    if (contentLengthPos != string::npos) {
        contentLengthPos += 15;
        while (contentLengthPos < headers.size() && isspace((unsigned char)headers[contentLengthPos])) contentLengthPos++;
        size_t lineEnd = headers.find("\r\n", contentLengthPos);
        string lenStr = headers.substr(contentLengthPos, lineEnd - contentLengthPos);
        int contentLength = stoi(lenStr);
        size_t totalLength = headerEnd + 4 + contentLength;
        while ((int)request.size() < totalLength && (received = recv(clientFd, buffer, sizeof(buffer), 0)) > 0) {
            request.append(buffer, received);
        }
    }
    return request;
}

void serveClient(int clientFd) {
    string request = readHttpRequest(clientFd);
    if (request.empty()) {
        close(clientFd);
        return;
    }
    auto [method, uri] = parseRequestLine(request);
    string path = uri;
    size_t qpos = path.find('?');
    if (qpos != string::npos) path = path.substr(0, qpos);
    string body;
    size_t bodyPos = request.find("\r\n\r\n");
    if (bodyPos != string::npos) {
        body = request.substr(bodyPos + 4);
    }
    string response;
    if (path == "/" || path == "/index.html") {
        string html = readFile("www/index.html");
        if (html.empty()) html = "<h1>Cannot load interface</h1>";
        response = makeHttpResponse("200 OK", "text/html", html);
    } else if (path == "/style.css") {
        string css = readFile("www/style.css");
        if (css.empty()) css = "body { font-family: sans-serif; }";
        response = makeHttpResponse("200 OK", "text/css", css);
    } else if (path == "/script.js") {
        string js = readFile("www/script.js");
        if (js.empty()) js = "console.error('Missing script');";
        response = makeHttpResponse("200 OK", "application/javascript", js);
    } else if (path.rfind("/api/", 0) == 0) {
        string result = handleApiRequest(method, path, body);
        response = makeHttpResponse("200 OK", "application/json", result);
    } else {
        string msg = "<h1>404 Not Found</h1>";
        response = makeHttpResponse("404 Not Found", "text/html", msg);
    }
    send(clientFd, response.c_str(), response.size(), 0);
    close(clientFd);
}

int main() {
    tasks.clear();
    nextId = 1;
    int serverFd = socket(AF_INET, SOCK_STREAM, 0);
    if (serverFd < 0) {
        cerr << "Socket creation failed\n";
        return 1;
    }
    int opt = 1;
    setsockopt(serverFd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
    sockaddr_in serverAddr{};
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_addr.s_addr = INADDR_ANY;
    serverAddr.sin_port = htons(8080);
    if (::bind(serverFd, (sockaddr*)&serverAddr, sizeof(serverAddr)) < 0) {
        cerr << "Bind failed. Is port 8080 in use?\n";
        close(serverFd);
        return 1;
    }
    if (::listen(serverFd, 10) < 0) {
        cerr << "Listen failed\n";
        close(serverFd);
        return 1;
    }
    cout << "Save My Semester GUI backend running at http://localhost:8080/\n";
    while (true) {
        sockaddr_in clientAddr{};
        socklen_t clientLen = sizeof(clientAddr);
        int clientFd = accept(serverFd, (sockaddr*)&clientAddr, &clientLen);
        if (clientFd < 0) continue;
        thread(serveClient, clientFd).detach();
    }
    close(serverFd);
    return 0;
}
