#include <mpi.h>
#include <iostream>
#include <vector>
#include <cstdlib>
#include <ctime>
#include <algorithm>
#include <limits>

using namespace std;

inline void print_matrix(const vector<vector<int>>& matrix);
inline void initialize_matrix_A(vector<vector<int>>& matrix, int n);
inline void initialize_matrix_B(vector<vector<int>>& matrix, int n);

int main(int argc, char* argv[]) {
    MPI_Init(&argc, &argv);

    int rank, size;
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);

    int n = size;  // Количество запущенных процессов
    vector<vector<int>> A(n, vector<int>(n));
    vector<vector<int>> B(n, vector<int>(n));

    // Создание виртуальной топологии "кольцо"
    MPI_Comm ring_comm;
    int dims[1] = { size };
    int periods[1] = { 1 };  // Периодическая топология
    MPI_Cart_create(MPI_COMM_WORLD, 1, dims, periods, 0, &ring_comm);

    int left, right;
    MPI_Cart_shift(ring_comm, 0, 1, &left, &right);

    initialize_matrix_A(A, n);
    if (rank == 0) {
        initialize_matrix_B(B, n);
    }

    if (rank == 0) {
        cout << "Matrix A: " << endl;
        print_matrix(A);
        cout << endl;

        cout << "Matrix B: " << endl;
        print_matrix(B);
        cout << endl;
    }

    // Передача матрицы B по кольцу
    vector<int> flat_B(n * n);
    if (rank == 0) {
        for (int i = 0; i < n; i++) {
            for (int j = 0; j < n; j++) {
                flat_B[i * n + j] = B[i][j];
            }
        }

        MPI_Send(flat_B.data(), n * n, MPI_INT, right, 0, ring_comm);
    }
    else {
        MPI_Recv(flat_B.data(), n * n, MPI_INT, left, 0, ring_comm, MPI_STATUS_IGNORE);
        if (rank != n - 1) {
            MPI_Send(flat_B.data(), n * n, MPI_INT, right, 0, ring_comm);
        }
    }

    // Вычисление результата
    vector<int> row = A[rank];
    int max_result = numeric_limits<int>::min();

    for (int j = 0; j < n; j++) {
        int sum = 0;
        for (int k = 0; k < n; k++) {
            sum += row[k] * flat_B[k * n + j];
        }
        max_result = max(max_result, sum);
    }

    if (rank == 0) {
        // Сбор результатов от всех процессов
        vector<int> all_results(n);
        all_results[0] = max_result;  // Результат от процесса 0

        // Получение результатов от остальных процессов
        for (int i = 1; i < n; i++) {
            MPI_Recv(&all_results[i], 1, MPI_INT, i, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
        }

        // Вывод результатов
        for (int i = 0; i < n; i++) {
            cout << "RANK[" << i << "]: Max result = " << all_results[i] << endl;
            cout << "It was calculated with the values:\n";
            for (int j = 0; j < n; j++) {
                cout << "A[" << i << "]: ";
                for (int num : A[i]) {
                    cout << num << " ";
                }
                cout << "and B[" << j << "]: ";
                for (int k = 0; k < n; k++) {
                    cout << B[k][j] << " ";
                }
                cout << endl;
            }
            cout << endl;
        }
    }
    else {
        // Отправка результата мастеру
        MPI_Send(&max_result, 1, MPI_INT, 0, 0, MPI_COMM_WORLD);
    }

    MPI_Comm_free(&ring_comm);
    MPI_Finalize();
    return 0;
}

void print_matrix(const vector<vector<int>>& matrix) {
    for (const auto& row : matrix) {
        for (int val : row) {
            cout << val << " ";
        }
        cout << endl;
    }
}

// Инициализация матрицы A
void initialize_matrix_A(vector<vector<int>>& matrix, int n) {
    for (int i = 0; i < n; i++) {
        for (int j = 0; j < n; j++) {
            matrix[i][j] = i + 1;  // Заполнение матрицы значениями i+1
        }
    }
}

// Инициализация матрицы B
void initialize_matrix_B(vector<vector<int>>& matrix, int n) {
    for (int i = 0; i < n; i++) {
        for (int j = 0; j < n; j++) {
            matrix[i][j] = i + 1 + j * n;
        }
    }
}
