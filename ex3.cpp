#include <iostream>
#include <mpi.h>
#include <vector>
#include <cstdlib>
#include <ctime>

using namespace std;

const int MASTER_RANK = 0;

inline void master_process(int num_processes, int n);
inline void slave_process(int rank, int n);
inline void min_fibonacci_opFunc(int* in, int* inout, int* len, MPI_Datatype* dtype);
inline int find_min_fibonacci_greater_than(int num);

int main(int argc, char** argv) {
    // Инициализация переменных для хранения ранга и количества процессов
    int rank, num_processes;
    // Количество элементов для обработки
    int n = 5;

    // Инициализация MPI
    MPI_Init(&argc, &argv);
    // Получение ранга текущего процесса
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    // Получение общего количества процессов
    MPI_Comm_size(MPI_COMM_WORLD, &num_processes);

    // Проверка, что количество процессов не меньше двух
    if (num_processes < 2) {
        if (rank == MASTER_RANK) {
            cout << "Error: The number of required processes is less than two" << endl;
        }
        MPI_Finalize();
        return 1;
    }

    // Запуск master-процесса
    if (rank == MASTER_RANK) {
        master_process(num_processes, n);
    }
    // Запуск slave-процессов
    else {
        slave_process(rank, n);
    }

    // Завершение работы MPI
    MPI_Finalize();
    return 0;
}

// Функция для master-процесса
void master_process(int num_processes, int n) {
    // Инициализация результата большим значением
    vector<int> result(n, INT_MAX);

    // Создание пользовательской операции для нахождения минимального числа Фибоначчи
    MPI_Op min_fibonacci_op;
    MPI_Op_create((MPI_User_function*)min_fibonacci_opFunc, 1, &min_fibonacci_op);

    // Сбор результатов от всех процессов
    MPI_Reduce(MPI_IN_PLACE, result.data(), n, MPI_INT, min_fibonacci_op, MASTER_RANK, MPI_COMM_WORLD);

    // Вывод результата
    cout << "Result: ";
    for (int i = 0; i < n; ++i) {
        cout << result[i] << " ";
    }
    cout << endl;

    // Освобождение пользовательской операции
    MPI_Op_free(&min_fibonacci_op);
}

// Функция для slave-процессов
void slave_process(int rank, int n) {
    // Инициализация генератора случайных чисел
    srand((int)time(0) + rank);

    // Генерация случайных чисел
    vector<int> data(n);
    for (int i = 0; i < n; ++i) {
        data[i] = rand() % 101 - 50;
    }

    // Нахождение минимального числа Фибоначчи для каждого элемента
    vector<int> fibonacci_results(n);
    for (int i = 0; i < n; ++i) {
        fibonacci_results[i] = find_min_fibonacci_greater_than(data[i]);
    }

    // Вывод сгенерированных чисел и результатов
    cout << "Process " << rank << " generated numbers: ";
    for (int i = 0; i < n; ++i) {
        cout << data[i] << " ";
    }
    cout << "\nProcess " << rank << " Fibonacci results: ";
    for (int i = 0; i < n; ++i) {
        cout << fibonacci_results[i] << " ";
    }
    cout << endl;

    // Отправка результатов master-процессу
    MPI_Reduce(fibonacci_results.data(), nullptr, n, MPI_INT, MPI_MIN, MASTER_RANK, MPI_COMM_WORLD);
}

// Пользовательская операция для нахождения минимального числа Фибоначчи
void min_fibonacci_opFunc(int* in, int* inout, int* len, MPI_Datatype* dtype) {
    for (int i = 0; i < *len; ++i) {
        inout[i] = min(inout[i], in[i]);
    }
}

// Функция для нахождения минимального числа Фибоначчи, превосходящего заданное значение
int find_min_fibonacci_greater_than(int num) {
    if (num <= 0) return 0;

    int a = 1, b = 1;
    while (b <= num) {
        int next = a + b;
        a = b;
        b = next;
    }
    return b;
}
