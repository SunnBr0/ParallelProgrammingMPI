#include <mpi.h>
#include <iostream>
#include <vector>
#include <cmath>

using namespace std;

const int MASTER_RANK = 0; // Константа для обозначения мастер-процесса

inline void master_process(int num_processes, int n); // Функция для мастер-процесса
inline void slave_process(int rank, int num_processes, int n); // Функция для рабочих процессов
double series_sum(double x, double eps); // Функция для вычисления суммы ряда

int main(int argc, char** argv) {
    int rank, num_processes;
    int n = 5; // Количество точек для вычисления

    MPI_Init(&argc, &argv); // Инициализация MPI
    MPI_Comm_rank(MPI_COMM_WORLD, &rank); // Определение ранга текущего процесса
    MPI_Comm_size(MPI_COMM_WORLD, &num_processes); // Определение общего количества процессов

    // Проверка на корректное количество процессов
    if (num_processes < 2 || num_processes - 1 > n) {
        if (rank == 0) {
            cout << "Error: The number of required processes is less than two or" << endl;
            cout << "Error: Number of slave processes exceeds the number of points." << endl;
        }
        MPI_Finalize();
        return 1;
    }

    if (rank == MASTER_RANK) {
        master_process(num_processes, n); // Запуск мастер-процесса
    }
    else {
        slave_process(rank, num_processes, n); // Запуск рабочего процесса
    }

    MPI_Finalize(); // Завершение работы MPI
    return 0;
}

void master_process(int num_processes, int n) {
    double A = -1.0; // Начало диапазона
    double B = 1.0; // Конец диапазона
    double eps = 1e-3; // Точность вычислений
    double step = (B - A) / (n - 1); // Шаг между точками

    std::vector<double> points(n); // Вектор для хранения точек
    std::vector<double> global_results; // Вектор для хранения глобальных результатов

    // Заполнение вектора точек
    for (int i = 0; i < n; i++) {
        points[i] = A + i * step;
    }

    int points_per_proc = n / (num_processes - 1); // Количество точек на процесс
    int remainder = n % (num_processes - 1); // Остаток от деления

    std::vector<int> sendcounts(num_processes, 0); // Вектор для хранения количества точек для каждого процесса
    std::vector<int> displs(num_processes, 0); // Вектор для хранения смещений

    // Расчет количества точек для каждого процесса
    for (int i = 1; i < num_processes; i++) {
        sendcounts[i] = points_per_proc + (i <= remainder ? 1 : 0);
    }

    // Расчет смещений
    for (int i = 1; i < num_processes; i++) {
        displs[i] = displs[i - 1] + sendcounts[i - 1];
    }

    // Проверка на наличие процессов с нулевым количеством точек
    if (sendcounts[1] == 0) {
        cerr << "Error: Some processes have zero points assigned. Exiting." << endl;
        MPI_Abort(MPI_COMM_WORLD, 1);
    }

    std::vector<double> local_data(sendcounts[1]); // Вектор для хранения локальных данных

    // Рассылка точек рабочим процессам
    MPI_Scatterv(points.data(), sendcounts.data(), displs.data(), MPI_DOUBLE,
        local_data.data(), sendcounts[1], MPI_DOUBLE, MASTER_RANK, MPI_COMM_WORLD);

    // Рассылка значения eps всем процессам
    MPI_Bcast(&eps, 1, MPI_DOUBLE, MASTER_RANK, MPI_COMM_WORLD);

    std::vector<int> recvcounts(num_processes, 0); // Вектор для хранения количества данных для каждого процесса
    std::vector<int> recvdispls(num_processes, 0); // Вектор для хранения смещений

    // Расчет количества данных для каждого процесса
    for (int i = 1; i < num_processes; i++) {
        recvcounts[i] = sendcounts[i];
    }

    // Расчет смещений
    for (int i = 1; i < num_processes; i++) {
        recvdispls[i] = recvdispls[i - 1] + recvcounts[i - 1];
    }

    std::vector<double> gathered_data(n); // Вектор для хранения собранных данных
    // Сбор данных от рабочих процессов
    MPI_Gatherv(local_data.data(), sendcounts[1], MPI_DOUBLE, gathered_data.data(),
        recvcounts.data(), recvdispls.data(), MPI_DOUBLE, MASTER_RANK, MPI_COMM_WORLD);

    // Добавление собранных данных в глобальный вектор результатов
    for (double value : gathered_data) {
        global_results.push_back(value);
    }

    // Вывод глобальных результатов
    for (double num : global_results) {
        cout << " " << num;
    }
    cout << "; Master_glob_size2: " << global_results.size() << endl;

    // Вывод результатов вычисления суммы ряда и экспоненциальной функции
    cout << "Results of calculating the sum of a series and exponential function:\n";
    for (int i = 0; i < n; i++) {
        double exact = exp(-points[i] * points[i]);
        cout << "x = " << points[i] << ", Sum of a series = " << global_results[i]
            << ", exp(-x^2) = " << exact << "\n";
    }
}

void slave_process(int rank, int num_processes, int n) {
    std::vector<double> local_results; // Вектор для хранения локальных результатов
    double eps = 0; // Точность вычислений
    int points_per_proc = n / (num_processes - 1) + ((rank <= (n % (num_processes - 1))) ? 1 : 0); // Количество точек на процесс
    std::vector<double> local_data(points_per_proc); // Вектор для хранения локальных данных

    // Получение точек от мастер-процесса
    MPI_Scatterv(nullptr, nullptr, nullptr, MPI_DOUBLE,
        local_data.data(), points_per_proc, MPI_DOUBLE, MASTER_RANK, MPI_COMM_WORLD);

    // Получение значения eps от мастер-процесса
    MPI_Bcast(&eps, 1, MPI_DOUBLE, MASTER_RANK, MPI_COMM_WORLD);

    // Вычисление суммы ряда для каждой точки
    for (double num : local_data) {
        local_results.push_back(series_sum(num, eps));
    }

    // Вывод локальных результатов
    for (double num : local_results) {
        cout << " " << num;
    }
    cout << "; size: " << local_results.size() << "; Rank: " << rank << endl;

    // Отправка локальных результатов мастер-процессу
    MPI_Gatherv(local_results.data(), (int)local_results.size(), MPI_DOUBLE, nullptr, nullptr, nullptr, MPI_DOUBLE, MASTER_RANK, MPI_COMM_WORLD);
}

int factorial(int n) {
    if (n == 0 || n == 1) {
        return 1;
    }
    return n * factorial(n - 1); // Рекурсивное вычисление факториала
}

double series_sum(double x, double eps) {
    double sum = 0.0; // Сумма ряда
    double term = 1.0; // Текущий член ряда
    int n = 0; // Номер члена ряда
    while (fabs(term) > eps) {
        sum += term;
        n++;
        term = pow(-1, n) * pow(x, 2 * n) / factorial(n); // Вычисление следующего члена ряда
    }
    return sum;
}
