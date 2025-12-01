#include <iostream>
#include <vector>
#include <chrono>
#include <atomic>
#include <sstream>
#include <random>
#include <windows.h> 

const int DISTANCE = 50;
const int TURTLES_COUNT = 10;

CRITICAL_SECTION race_cs;
std::atomic<bool> winner_found = false;

std::vector<int> turtle_positions(TURTLES_COUNT, 0);
std::vector<bool> turtle_is_finished(TURTLES_COUNT, false);

void print_race_state() {
    std::cout << "--- Состояние трассы ---\n";
    std::stringstream track_line;

    for (int j = 0; j <= DISTANCE; ++j) {
        bool turtle_present = false;

        for (int i = 0; i < TURTLES_COUNT; ++i) {
            if (turtle_positions[i] == j && turtle_positions[i] < DISTANCE) {
                track_line << (i + 1);
                turtle_present = true;
                break;
            }
        }

        if (!turtle_present) {
            if (j == DISTANCE) {
                track_line << "|";
            }
            else {
                track_line << "0";
            }
        }
    }

    std::cout << track_line.str() << "\n";

    for (int i = 0; i < TURTLES_COUNT; ++i) {
        if (turtle_is_finished[i]) {
            std::cout << "Черепашка " << (i + 1) << " финишировала.\n";
        }
    }

    if (winner_found) {
        std::cout << "!!! ПОБЕДИТЕЛЬ ОПРЕДЕЛЕН !!!\n";
    }
    std::cout << "--------------------------\n\n";
}

DWORD WINAPI run_turtle(LPVOID lpParam) {
    int* data = static_cast<int*>(lpParam);
    int turtle_index = data[0];
    int turtle_id = data[1];

    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> step_dist(0, 2);
    std::uniform_int_distribution<> sleep_dist(1000, 3000);

    int position = 0;

    while (position < DISTANCE) {

        DWORD sleep_time_ms = sleep_dist(gen);
        Sleep(sleep_time_ms);

        int step = step_dist(gen);
        position += step;

        if (position > DISTANCE) {
            position = DISTANCE;
        }

        EnterCriticalSection(&race_cs);

        turtle_positions[turtle_index] = position;

        if (position == DISTANCE) {
            turtle_is_finished[turtle_index] = true;

            if (!winner_found) {
                winner_found = true;
                std::cout << "\n!!! ЧЕРЕПАШКА " << turtle_id << " ПОБЕДИЛА !!!\n";
            }
        }

        print_race_state();

        LeaveCriticalSection(&race_cs);
    }

    return 0;
}

int main() {
    InitializeCriticalSection(&race_cs);

    setlocale(LC_ALL, "");
    std::cout << "=== НАЧАЛО ГОНКИ ===\n\n";

    std::vector<HANDLE> threads_handles;
    std::vector<int*> thread_data;

    for (int i = 0; i < TURTLES_COUNT; ++i) {

        int* data = new int[2];
        data[0] = i;
        data[1] = i + 1;
        thread_data.push_back(data);

        HANDLE hThread = CreateThread(NULL, 0, run_turtle, data, 0, NULL);

        if (hThread != NULL) {
            threads_handles.push_back(hThread);
        }
        else {
            std::cerr << "Ошибка при создании потока " << i + 1 << "\n";
        }
    }

    WaitForMultipleObjects(
        threads_handles.size(),
        threads_handles.data(),
        TRUE,
        INFINITE
    );

    for (HANDLE h : threads_handles) {
        CloseHandle(h);
    }
    for (int* data : thread_data) {
        delete[] data;
    }
    DeleteCriticalSection(&race_cs);

    std::cout << "\n=== ВСЕ ЧЕРЕПАШКИ ФИНИШИРОВАЛИ. ГОНКА ОКОНЧЕНА. ===\n";

    return 0;
}