#include "task_manager.hpp" // Header file deklarasi struktur dan fungsi
#include <iostream>         // Untuk input/output
#include <iomanip>          // Untuk manipulasi output (setw, setfill)
#include <ctime>            // Untuk waktu dan tanggal
#include <fstream>          // Untuk file I/O
#include <sstream>          // Untuk parsing string
#include <chrono>           // Untuk timer/fokus mode
#include <thread>           // Untuk multithreading (fokus mode)
#include <atomic>           // Untuk variabel atomik (fokus mode)
#include <cstdlib>          // Untuk fungsi system()

using namespace std;

// Global Pointer untuk struktur data utama
TaskPtr head, tail;             // Linked list tugas
StackPtr redoTop, doneTop;      // Stack untuk redo dan tugas selesai
QueuePtr queueFront, queueRear; // Queue untuk antrian tugas
int taskCounter = 1;            // Counter kode tugas otomatis

// Nama bulan untuk tampilan deadline
const string monthNames[13] = {
    "", "Januari", "Februari", "Maret", "April", "Mei", "Juni",
    "Juli", "Agustus", "September", "Oktober", "November", "Desember"
};

// Array quotes motivasi
const string quotes[] = {
    "Keep going, you're getting there!",
    "Success is the sum of small efforts repeated.",
    "One task at a time, one step at a time.",
    "You’re doing great. Keep it up!",
    "Stay focused and never give up.",
    "Progress, not perfection.",
    "Every step counts. Keep moving!",
    "Setiap hari adalah kesempatan baru untuk berkembang.",
    "Jangan menyerah, hasil besar dimulai dari langkah kecil.",
    "Kerja keras hari ini adalah kesuksesan esok hari.",
    "Fokus pada tujuan, bukan hambatan.",
    "Kamu lebih kuat dari yang kamu kira.",
    "Tantangan adalah peluang untuk belajar.",
    "Semangat! Selesaikan satu tugas lagi.",
    "Kesuksesan dimulai dari keberanian mencoba.",
    "Jangan tunda, lakukan sekarang juga.",
    "Kegagalan hanyalah keberhasilan yang tertunda.",
    "Percaya pada proses, nikmati setiap langkahnya.",
    "Setiap tugas yang selesai mendekatkanmu ke impian.",
    "Kamu bisa! Jangan ragu untuk melangkah.",
    "Waktu terbaik untuk memulai adalah sekarang.",
    "Jadilah versi terbaik dari dirimu hari ini.",
    "Tugas berat akan terasa ringan jika dikerjakan bersama semangat.",
    "Satu tugas selesai, satu beban berkurang.",
    "Tetap konsisten, hasil tidak akan mengkhianati usaha."
};
const int quoteCount = sizeof(quotes) / sizeof(quotes[0]); // Jumlah quote

void playFocusMusic() {
    system("afplay music/focus.mp3 &");  // Mainkan musik fokus di background
}

void stopFocusMusic() {
    system("killall afplay");  // Hentikan semua pemutaran afplay
}

// Inisialisasi semua struktur data
void createList() {
    head = tail = nullptr;
    redoTop = doneTop = nullptr;
    queueFront = queueRear = nullptr;
    taskCounter = 1;
}

// Membuat node tugas baru
TaskPtr createElement(string name, int priority, int day, int month, int year, string category, int code, bool isPinned) {
    TaskPtr pBaru = new Task;  
    if (code == -1) { // Jika kode -1, generate otomatis
        pBaru->data.code = taskCounter++;
    } else {    // Jika kode diberikan
        pBaru->data.code = code;    // Gunakan kode yang diberikan
        if (code >= taskCounter) taskCounter = code + 1;    // Update counter jika kode lebih besar
    }
    pBaru->data.name = name; 
    pBaru->data.priority = priority;   
    pBaru->data.deadline = {day, month, year};
    pBaru->data.category = category;
    pBaru->data.isPinned = isPinned;
    pBaru->prev = pBaru->next = nullptr;
    return pBaru;
}

// Menambah tugas ke linked list (urut prioritas)
void addTask(const string& name, int priority, int day, int month, int year, const string& category, int code, bool isPinned) {
    TaskPtr pBaru = createElement(name, priority, day, month, year, category, code, isPinned ); // Buat node tugas baru

    if (!head) head = tail = pBaru; // List kosong
    else {  // List sudah ada, cari posisi berdasarkan prioritas
        TaskPtr pBantu = head;
        while (pBantu && pBantu->data.priority <= priority) pBantu = pBantu->next;  // Cari posisi yang tepat

        if (!pBantu) { // Tambah di akhir
            tail->next = pBaru;
            pBaru->prev = tail;
            tail = pBaru;
        } else if (pBantu == head) { // Tambah di awal
            pBaru->next = head;
            head->prev = pBaru;
            head = pBaru;
        } else { // Tambah di tengah
            pBaru->next = pBantu;
            pBaru->prev = pBantu->prev;
            pBantu->prev->next = pBaru;
            pBantu->prev = pBaru;
        }
    }

    // Tambahkan ke queue untuk antrian tugas
    QueuePtr qBaru = new QueueNode{pBaru->data, nullptr};   // Buat node queue baru
    if (!queueRear) queueFront = queueRear = qBaru; // Jika queue kosong, set front dan rear
    else queueRear = queueRear->next = qBaru;   // Tambah di akhir queue

    logActivity("Tambah tugas: " + name); // Catat log
    cout << "\n\033[1;32mTugas \"" << name << "\" ditambahkan dengan kode " << pBaru->data.code << ".\033[0m\n";
}

// Menghapus tugas berdasarkan kode
void deleteTask(int code) {
    TaskPtr pBantu = head;
    while (pBantu) {    // Cari tugas dengan kode yang sesuai
        if (pBantu->data.code == code) {    // Jika ditemukan
            StackPtr pBaru = new StackNode{pBantu->data, redoTop}; // Simpan ke stack redo
            redoTop = pBaru;    // Pointer redoTop menunjuk ke pBaru

            if (pBantu->prev) pBantu->prev->next = pBantu->next;    // Update prev node
            else head = pBantu->next; head->prev = nullptr;   // Jika ini tugas pertama, update head

            if (pBantu->next) pBantu->next->prev = pBantu->prev;    // Update next node
            else tail = pBantu->prev; tail->next = nullptr; // Jika ini tugas terakhir, update tail

            logActivity("Hapus tugas: " + pBantu->data.name + " (kode " + to_string(code) + ")");
            delete pBantu;
            cout << "\033[1;33mTugas dengan kode " << code << " dihapus dan dapat diredo.\033[0m\n";
            return;
        }
        pBantu = pBantu->next;  // Lanjut ke tugas berikutnya
    }
    cout << "\033[1;31mTugas tidak ditemukan.\033[0m\n";
}

// Tandai tugas selesai (pindah ke stack done)
void finishTask(int code) {
    TaskPtr pBantu = head; 
    while (pBantu) {
        if (pBantu->data.code == code) {
            StackPtr pSelesai = new StackNode{pBantu->data, doneTop}; // Simpan ke stack done
            doneTop = pSelesai;

            if (pBantu->prev) pBantu->prev->next = pBantu->next;    // Update prev
            else head = pBantu->next; head->prev = nullptr;   //  Jika ini tugas pertama, update head

            if (pBantu->next) pBantu->next->prev = pBantu->prev;    // Update next
            else tail = pBantu->prev; tail->next = nullptr; // Jika ini tugas terakhir, update tail

            delete pBantu;

            logActivity("Selesai tugas: " + pBantu->data.name + " (kode " + to_string(code) + ")");
            cout << "\033[1;32mTugas dengan kode " << code << " telah diselesaikan.\033[0m\n";
            return;
        }
        pBantu = pBantu->next;
    }
    cout << "\033[1;31mTugas tidak ditemukan.\033[0m\n";
}

// Redo tugas yang dihapus (ambil dari stack redo)
void redo() {
    if (!redoTop) {
        cout << "\033[1;31mTidak ada tugas untuk diredo.\033[0m\n";
        return;
    }
    StackPtr pBaru = redoTop;
    redoTop = redoTop->next;
    logActivity("Redo tugas: " + pBaru->data.name + " (kode " + to_string(pBaru->data.code) + ")");
    addTask(pBaru->data.name, pBaru->data.priority, pBaru->data.deadline.day, pBaru->data.deadline.month, pBaru->data.deadline.year, pBaru->data.category, pBaru->data.code);
    delete pBaru;
}

// Tampilkan tugas yang deadline hari ini
void showTodayTasks() {
    time_t t = time(nullptr);
    tm* now = localtime(&t);
    int d = now->tm_mday;
    int m = now->tm_mon + 1;
    int y = now->tm_year + 1900;
    bool found = false;
    for (TaskPtr pBantu = head; pBantu; pBantu = pBantu->next) {
        if (pBantu->data.deadline.day == d && pBantu->data.deadline.month == m && pBantu->data.deadline.year == y) {
            if (!found) cout << "\n\033[1;36mTugas yang deadline hari ini:\033[0m\n";
            found = true;
            cout << "- " << pBantu->data.name << " (Kode: " << pBantu->data.code << ")\n";
        }
    }
    if (!found) cout << "\033[1;33mTidak ada tugas untuk hari ini.\033[0m\n";
}

// Tampilkan semua tugas (pinned dulu, lalu biasa)
void showTasks() {
    if (!head) {
        cout << "\033[1;33mTidak ada tugas.\033[0m\n";
        return;
    }

    cout << "\n\033[1;34mDaftar Tugas:\033[0m\n";
    cout << left << setw(6) << "Kode"
         << setw(30) << "Nama"
         << setw(10) << "Prioritas"
         << setw(25) << "Deadline"
         << "Kategori\n";
    cout << string(90, '-') << endl;

    // Tampilkan tugas PINNED dulu
    for (TaskPtr p = head; p; p = p->next) {
        if (p->data.isPinned) {
            string namaTugas = "[PINNED] " + p->data.name;
            cout << left << setw(6) << p->data.code
                 << setw(30) << namaTugas
                 << setw(10) << p->data.priority
                 << setw(25) << (to_string(p->data.deadline.day) + " " + monthNames[p->data.deadline.month] + " " + to_string(p->data.deadline.year))
                 << p->data.category << endl;
        }
    }

    // Lalu tugas biasa
    for (TaskPtr p = head; p; p = p->next) {
        if (!p->data.isPinned) {
            cout << left << setw(6) << p->data.code
                 << setw(30) << p->data.name
                 << setw(10) << p->data.priority
                 << setw(25) << (to_string(p->data.deadline.day) + " " + monthNames[p->data.deadline.month] + " " + to_string(p->data.deadline.year))
                 << p->data.category << endl;
        }
    }
}

// Tampilkan tugas yang sudah selesai (stack done)
void showDoneTasks() {
    if (!doneTop) {
        cout << "\033[1;33mBelum ada tugas selesai.\033[0m\n";
        return;
    }
    StackPtr pBantu = doneTop;
    cout << "\n\033[1;32mTugas yang telah diselesaikan:\033[0m\n";
    while (pBantu) {
        cout << "- " << pBantu->data.name << " [Prioritas: " << pBantu->data.priority << "]\n";
        pBantu = pBantu->next;
    }
}

// Urutkan tugas berdasarkan prioritas (bubble sort, tukar data)
void showTasksByPriority() {
    for (TaskPtr i = head; i; i = i->next) {
        for (TaskPtr j = i->next; j; j = j->next) {
            if (j->data.priority < i->data.priority) {
                TaskData temp = i->data;
                i->data = j->data;
                j->data = temp;
            }
        }
    }
    showTasks(); // langsung panggil
}


// Tampilkan tugas berdasarkan urutan masuk (queue)
void showTasksByEntryOrder() {
    if (!queueFront) {
        cout << "\033[1;33mBelum ada tugas yang dimasukkan.\033[0m\n";
        return;
    }

    QueuePtr pBantu = queueFront;
    cout << "\n\033[1;36mTugas Berdasarkan Urutan Masuk:\033[0m\n";
    cout << left << setw(6) << "Kode" 
         << setw(25) << "Nama" 
         << setw(10) << "Prioritas" 
         << setw(25) << "Deadline" 
         << "Kategori\n";
    cout << string(80, '-') << endl;

    while (pBantu) {    // Iterasi melalui queue
        const TaskData& t = pBantu->data;   // Ambil data tugas
        cout << left << setw(6) << t.code   // Tampilkan kode tugas
             << setw(25) << t.name
             << setw(10) << t.priority
             << setw(25) << (to_string(t.deadline.day) + " " + monthNames[t.deadline.month] + " " + to_string(t.deadline.year))
             << t.category << endl;
        pBantu = pBantu->next;
    }
}

// Urutkan tugas berdasarkan nama (bubble sort, tukar data)
void showSortedTasksByName() {
    for (TaskPtr i = head; i; i = i->next) {
        for (TaskPtr j = i->next; j; j = j->next) {
            if (j->data.name < i->data.name) {
                TaskData temp = i->data;
                i->data = j->data;
                j->data = temp;
            }
        }
    }
    showTasks();
}

// Urutkan tugas berdasarkan deadline (bubble sort, tukar data)
void showSortedTasksByDeadline() {
    for (TaskPtr i = head; i; i = i->next) {
        for (TaskPtr j = i->next; j; j = j->next) {
            Date a = i->data.deadline;
            Date b = j->data.deadline;
            if (b.year < a.year || (b.year == a.year && b.month < a.month) || (b.year == a.year && b.month == a.month && b.day < a.day)) {
                TaskData temp = i->data;
                i->data = j->data;
                j->data = temp;
            }
        }
    }
    showTasks();
}

// Tampilkan ringkasan statistik tugas
void showSummary() {
    int total = 0, done = 0, pinned = 0, nearDeadline = 0;

    time_t now = time(0);
    tm* today = localtime(&now);

    for (TaskPtr p = head; p; p = p->next) {
        total++;
        if (p->data.isPinned) pinned++;

        tm deadline = {};
        deadline.tm_mday = p->data.deadline.day;
        deadline.tm_mon  = p->data.deadline.month - 1;
        deadline.tm_year = p->data.deadline.year - 1900;
        time_t deadlineTime = mktime(&deadline);
        double diff = difftime(deadlineTime, now) / (60 * 60 * 24);
        if (diff >= 0 && diff <= 1) nearDeadline++;
    }

    for (StackPtr p = doneTop; p; p = p->next) done++;

    int totalAll = total + done;
    int percent = totalAll ? (done * 100 / totalAll) : 0;
    int barLen = 20;
    int filled = percent * barLen / 100;

    string bar = "[";
    for (int i = 0; i < barLen; ++i)
        bar += (i < filled) ? "█" : "-";
    bar += "]";

    // Tampilan simetris dan sejajar
    cout << "\n\033[1;36m┌─────────────── RINGKASAN TUGAS ───────────────┐\033[0m\n";
    cout << "│ 🗂️  Total Tugas    : " << setw(3) << totalAll << "   📌  Pinned    :  " << setw(2) << pinned << " │\n";
    cout << "│ ✅  Tugas Selesai : " << setw(3) << done     << "   ⏳  Mendesak  :  " << setw(2) << nearDeadline << " │\n";
    cout << "│ ❗  Belum Selesai : " << setw(3) << total    << "                       │\n";
    cout << "│ 📈  Progress      : " << bar << " " << setw(1) << percent << "% │\n";
    cout << "\033[1;36m└───────────────────────────────────────────────┘\033[0m\n";
}

// Tampilkan tugas berdasarkan kategori
void showTasksByCategory(const string& category) {
    bool found = false;
    for (TaskPtr p = head; p; p = p->next) {
        if (p->data.category == category) {
            if (!found) cout << "\n\033[1;36mTugas dalam kategori '" << category << "':\033[0m\n";
            found = true;
            cout << "- " << p->data.name << " (Kode: " << p->data.code << ")\n";
        }
    }
    if (!found) cout << "\033[1;33mTidak ada tugas dengan kategori tersebut.\033[0m\n";
}

// Edit data tugas berdasarkan kode
void editTask(int code) {
    TaskPtr pBantu = head;
    while (pBantu) {
        if (pBantu->data.code == code) {
            cout << "Ubah nama tugas: "; getline(cin, pBantu->data.name);
            cout << "Ubah prioritas (1–5): "; cin >> pBantu->data.priority;
            cout << "Ubah deadline (dd mm yyyy): "; cin >> pBantu->data.deadline.day >> pBantu->data.deadline.month >> pBantu->data.deadline.year; cin.ignore();
            cout << "Ubah kategori: "; getline(cin, pBantu->data.category);
            cout << "\033[1;32mTugas berhasil diubah.\033[0m\n";
            return;
        }
        pBantu = pBantu->next;
        logActivity("Edit tugas: " + pBantu->data.name + " (kode " + to_string(code) + ")");
    }
    cout << "\033[1;31mTugas tidak ditemukan.\033[0m\n";
}

// Reminder tugas yang deadline hari ini/besok
void checkDeadlineReminder() {
    time_t now = time(nullptr);
    tm* localNow = localtime(&now);
    int today = localNow->tm_mday;
    int month = localNow->tm_mon + 1;
    int year = localNow->tm_year + 1900;

    cout << "\n\033[1;33m[🔔 Reminder Tugas Mendekati Deadline]\033[0m\n";
    bool found = false;

    for (TaskPtr pBantu = head; pBantu; pBantu = pBantu->next) {
        int d = pBantu->data.deadline.day;
        int m = pBantu->data.deadline.month;
        int y = pBantu->data.deadline.year;

        if (d == today && m == month && y == year) {
            found = true;
            cout << "- " << pBantu->data.name << " (Hari ini!)\n";
        }
        else if (d == today + 1 && m == month && y == year) {
            found = true;
            cout << "- " << pBantu->data.name << " (Besok!)\n";
        }
    }

    if (!found) cout << "Tidak ada tugas mendekati deadline.\n";
}

// Mode fokus: timer + musik + deteksi ENTER
void startFocusMode(int code) {
    TaskPtr pBantu = head;
    while (pBantu && pBantu->data.code != code) pBantu = pBantu->next;

    if (!pBantu) {
        cout << "\033[1;31mTugas tidak ditemukan.\033[0m\n";
        return;
    }

    int duration;
    cout << "\n⏱️ Masukkan durasi fokus (dalam menit): ";
    cin >> duration;
    cin.ignore(); // Bersihkan newline agar tidak terbaca cin.get()

    cout << "\n\033[1;35m=== MODE FOKUS ===\033[0m\n";
    cout << "🎯 Tugas     : " << pBantu->data.name << "\n";
    cout << "📌 Kategori : " << pBantu->data.category << "\n";
    cout << "🔥 Prioritas: " << pBantu->data.priority << "\n";
    cout << "🗓️ Deadline : " << pBantu->data.deadline.day << " "
         << monthNames[pBantu->data.deadline.month] << " "
         << pBantu->data.deadline.year << "\n";

    cout << "\n⏱️ Fokus dimulai (" << duration << ":00 menit)\n";
    cout << "\033[1;33m(→ Tekan ENTER kapan saja untuk keluar dari mode fokus)\033[0m\n";
    logActivity("Fokus mode mulai: " + pBantu->data.name + " (kode " + to_string(code) + ")");
    playFocusMusic(); // Mainkan musik fokus

    std::atomic<bool> exitFocus(false);

    // Thread untuk deteksi ENTER
    std::thread inputThread([&]() {
        cin.get();
        exitFocus = true;
    });

    for (int m = duration - 1; m >= 0 && !exitFocus; --m) {
        for (int s = 59; s >= 0 && !exitFocus; --s) {
            printf("\r⏳ Waktu tersisa: %02d:%02d ", m, s);
            fflush(stdout);
            std::this_thread::sleep_for(std::chrono::seconds(1));
        }
    }

    exitFocus = true;
    inputThread.detach(); // Stop thread input

    stopFocusMusic(); // Hentikan musik fokus
    logActivity("Fokus mode selesai: " + pBantu->data.name);

    cout << "\n\n\033[1;32m✅ Mode fokus selesai atau dihentikan. Good job!\033[0m\n";
}

// Catat aktivitas ke file log
void logActivity(const string& message) {
    ofstream log("data/log.txt", ios::app);  // append mode
    time_t now = time(0);
    tm* local = localtime(&now);

    log << "[" << setfill('0') 
        << setw(2) << local->tm_mday << "/"
        << setw(2) << local->tm_mon + 1 << "/"
        << 1900 + local->tm_year << " "
        << setw(2) << local->tm_hour << ":"
        << setw(2) << local->tm_min << "] "
        << message << "\n";
    log.close();
}

// Tampilkan isi file log aktivitas
void showLog() {
    ifstream log("data/log.txt");
    if (!log) {
        cout << "Belum ada log aktivitas.\n";
        return;
    }

    cout << "\n\033[1;36m=== RIWAYAT AKTIVITAS ===\033[0m\n";
    string line;
    while (getline(log, line)) {
        cout << line << endl;
    }
    log.close();
}

// Simpan semua tugas ke file
void saveToFile(const string& filename) {
    ofstream file("data/"+filename);
    if (!file.is_open()) {
        cout << "\033[1;31mGagal menyimpan file.\033[0m\n";
        return;
    }

    for (TaskPtr pBantu = head; pBantu; pBantu = pBantu->next) {
        file << pBantu->data.code << "|"
             << pBantu->data.name << "|"
             << pBantu->data.priority << "|"
             << pBantu->data.deadline.day << "|"
             << pBantu->data.deadline.month << "|"
             << pBantu->data.deadline.year << "|"
             << pBantu->data.category << "|"
             << pBantu->data.isPinned << '\n';
    }

    file.close();
    cout << "\033[1;32mTugas berhasil disimpan ke file.\033[0m\n";
}

// Load tugas dari file
void loadFromFile(const string& filename) {
    ifstream file("data/" + filename);
    if (!file.is_open()) return;

    string line;
    while (getline(file, line)) {
        stringstream ss(line);
        string name, category, temp;
        int code, priority, day, month, year;
        char sep;
        bool isPinned = false;

        getline(ss, line, '|'); code = stoi(line);
        getline(ss, name, '|');
        getline(ss, line, '|'); priority = stoi(line);
        getline(ss, line, '|'); day = stoi(line);
        getline(ss, line, '|'); month = stoi(line);
        getline(ss, line, '|'); year = stoi(line);
        getline(ss, category, '|');
        getline(ss, temp);  // read pinned flag if available
        if (!temp.empty()) isPinned = (temp == "1");

        addTask(name, priority, day, month, year, category, code, isPinned);
    }

    file.close();
    cout << "\033[1;32mData berhasil dimuat dari file.\033[0m\n";
}

// Tampilkan quote motivasi random
void showMotivationalQuote() {
    srand(time(0));
    int index = rand() % quoteCount;
    cout << "\n\033[1;36m🎯 " << quotes[index] << "\033[0m\n";
}

// Menu utama aplikasi
void showMenu() {
    string nama, kategori;
    int prioritas, day, month, year, code, pilihan;

    while (true) {
        cout << "\n\033[1;35m╔══════════════════════════════════════╗\033[0m\n";
        cout <<   "\033[1;35m║          📋 TASK MANAGER CLI         ║\033[0m\n";
        cout <<   "\033[1;35m╚══════════════════════════════════════╝\033[0m\n";

        cout << "\033[1;36m1.\033[0m ➕ Tambah Tugas\n";
        cout << "\033[1;36m2.\033[0m 📂 Lihat Tugas\n";
        cout << "\033[1;36m3.\033[0m 🛠️  Kelola Tugas\n";
        cout << "\033[1;36m4.\033[0m 📊 Statistik & Filter\n";
        cout << "\033[1;36m5.\033[0m 🧘 Fokus Mode\n";
        cout << "\033[1;36m6.\033[0m 🕓 Riwayat Aktivitas\n";
        cout << "\033[1;31m0.\033[0m ❌ Keluar\n";
        cout << "👉 Pilih menu: "; cin >> pilihan; cin.ignore();

        switch (pilihan) {
            case 1:
                cout << "\n\033[1;33m== Tambah Tugas ==\033[0m\n";
                cout << "📝 Nama tugas: "; getline(cin, nama);
                cout << "🎯 Prioritas (1–5): "; cin >> prioritas;
                cout << "📅 Deadline (dd mm yyyy): "; cin >> day >> month >> year; cin.ignore();
                cout << "📁 Kategori: "; getline(cin, kategori);
                addTask(nama, prioritas, day, month, year, kategori);
                break;

            case 2:
                cout << "\n\033[1;36m== Tampilkan Tugas ==\033[0m\n";
                cout << "1. Semua Tugas\n2. Tugas Hari Ini\n3. Tugas Selesai\n";
                cout << "👉 Pilih opsi: "; cin >> pilihan; cin.ignore();
                switch (pilihan) {
                    case 1: showTasksByPriority(); break;
                    case 2: showTodayTasks(); break;
                    case 3: showDoneTasks(); break;
                    default: cout << "\033[1;31m⚠️  Pilihan tidak valid.\033[0m\n";
                }
                break;

            case 3:
                cout << "\n\033[1;36m== Kelola Tugas ==\033[0m\n";
                cout << "1. ✏️  Edit Tugas\n2. 🗑️  Hapus Tugas\n3. ✅ Tandai Selesai\n4. 🔁 Redo Tugas\n5. 📌 Pin / Unpin\n";
                cout << "👉 Pilih aksi: "; cin >> pilihan;

                if (pilihan == 1) {
                    cout << "Kode tugas yang ingin diedit: "; cin >> code; cin.ignore();
                    editTask(code);
                } else if (pilihan == 2) {
                    cout << "Kode tugas yang ingin dihapus: "; cin >> code;
                    deleteTask(code);
                } else if (pilihan == 3) {
                    cout << "Kode tugas yang selesai: "; cin >> code;
                    finishTask(code);
                } else if (pilihan == 4) {
                    redo();
                } else if (pilihan == 5) {
                    cout << "Masukkan kode tugas yang ingin dipin/unpin: ";
                    cin >> code;
                    bool found = false;
                    for (TaskPtr p = head; p; p = p->next) {
                        if (p->data.code == code) {
                            p->data.isPinned = !p->data.isPinned;
                            cout << (p->data.isPinned ? "\033[1;32m📌 Tugas dipinned.\033[0m\n" : "\033[1;33m📍 Tugas di-unpin.\033[0m\n");
                            found = true;
                            break;
                        }
                    }
                    if (!found) cout << "\033[1;31m⚠️  Tugas tidak ditemukan.\033[0m\n";
                } else {
                    cout << "\033[1;31m⚠️  Pilihan tidak valid.\033[0m\n";
                }
                break;

            case 4:
                cout << "\n\033[1;36m== Statistik & Filter ==\033[0m\n";
                cout << "1. 🔤 Urut Nama\n2. 📅 Urut Deadline\n3. 📊 Ringkasan\n4. 🔎 Filter Kategori\n5. 📥 Urutan Masuk\n";
                cout << "👉 Pilih filter: "; cin >> pilihan; cin.ignore();
                switch (pilihan) {
                    case 1: showSortedTasksByName(); break;
                    case 2: showSortedTasksByDeadline(); break;
                    case 3: showSummary(); break;
                    case 4:
                        cout << "Masukkan kategori: "; getline(cin, kategori);
                        showTasksByCategory(kategori); break;
                    case 5: showTasksByEntryOrder(); break;
                    default: cout << "\033[1;31m⚠️  Pilihan tidak valid.\033[0m\n";
                }
                break;

            case 5:
                cout << "Masukkan kode tugas untuk masuk Mode Fokus: ";
                cin >> code; cin.ignore();
                startFocusMode(code);
                break;

            case 6:
                showLog();
                break;

            case 0:
                saveToFile();
                cout << "\n\033[1;36m📁 Data disimpan. Terima kasih telah menggunakan Task Manager!\033[0m\n";
                return;

            default:
                cout << "\033[1;31m⚠️  Pilihan tidak valid.\033[0m\n";
        }
    }
}
