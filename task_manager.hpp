// task_manager.hpp
#ifndef TASK_MANAGER_HPP
#define TASK_MANAGER_HPP

#include <string>

struct Date {
    int day;
    int month;
    int year;
};

struct TaskData {
    int code;
    std::string name;
    int priority;
    Date deadline;
    std::string category;
    bool isPinned = false;
};

struct Task;
typedef Task* TaskPtr;

struct StackNode;
typedef StackNode* StackPtr;

struct QueueNode;
typedef QueueNode* QueuePtr;

struct Task {
    TaskData data;
    TaskPtr prev;
    TaskPtr next;
};

struct StackNode {
    TaskData data;
    StackPtr next;
};

struct QueueNode {
    TaskData data;
    QueuePtr next;
};

extern TaskPtr head, tail;
extern StackPtr redoTop, doneTop;
extern QueuePtr queueFront, queueRear;
extern int taskCounter;
extern const std::string monthNames[13];

void createList();
TaskPtr createElement(std::string name, int priority, int day, int month, int year, std::string category, int code = -1, bool isPinned = false);
void addTask(const std::string& name, int priority, int day, int month, int year, const std::string& category, int code = -1, bool isPinned = false);
void deleteTask(int code);
void redo();
void finishTask(int code);
void showTasks();
void showDoneTasks();
void showMenu();

// Fitur tambahan
void showSortedTasksByName();
void showSortedTasksByDeadline();
void showSummary();
void showTasksByCategory(const std::string& category);
void showTodayTasks();
void editTask(int code);
void saveToFile(const std::string& filename = "tasks.txt");
void loadFromFile(const std::string& filename = "tasks.txt");
void checkDeadlineReminder();
void startFocusMode(int code);
void showMotivationalQuote();
void playFocusMusic();
void stopFocusMusic();
void logActivity(const std::string& activity);
void showLog();
void showTasksByEntryOrder();
void showTasksByPriority();
#endif // TASK_MANAGER_HPP
