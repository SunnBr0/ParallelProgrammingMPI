#include <mpi.h>
#include <iostream>
#include <cstring> // Для strcpy и strcat

using namespace std;

const int MASTER_RANK = 0;
const int MAX_STR_LEN = 6; // Длина исходной строки без дублирования

// Функция для дублирования строки с учетом триад
void duplicate_string(char* str) {
    char temp[MAX_STR_LEN * 2 + 1] = {0}; // Массив для строки после дублирования
    int triad_len = 3; // Длина триады
    int num_triads = MAX_STR_LEN / triad_len; // Количество триад

    for (int i = 0; i < num_triads; ++i) {
        // Дублируем текущую триаду
        strncat(temp, str + i * triad_len, triad_len);
        strncat(temp, str + i * triad_len, triad_len);
    }

    // Копируем результат обратно в исходную строку
    strncpy(str, temp, MAX_STR_LEN * 2 + 1);
}

inline void master_process(int num_processes, int count);
inline void slave_process(int rank, int count);

int main(int argc, char** argv) {
    int count = 1; // Мы передаем одну строку

    MPI_Init(&argc, &argv);
    int rank, num_processes;
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &num_processes);

    // Убедимся, что количество процессов хотя бы 2
    if (num_processes < 2) {
        if (rank == MASTER_RANK) {
            cerr << "Error: At least 2 processes are required (1 master + 1 slave)." << endl;
        }
        MPI_Finalize();
        return 1; // Завершаем программу с ошибкой
    }

    if (rank == MASTER_RANK) {
        master_process(num_processes, count);
    } else {
        slave_process(rank, count);
    }

    MPI_Finalize();
    return 0;
}

void master_process(int num_processes, int count) {
    // Строка для передачи
    char str[MAX_STR_LEN * 2 + 1] = "abcdef";  // Исходная строка

    cout << "Master process: created and duplicated string: " << str << endl;
    // Дублируем строку
    duplicate_string(str);

    // Создаем тип MPI для передачи строки
    int block_lengths[MAX_STR_LEN * 2];
    int displacements[MAX_STR_LEN * 2];
    for (int i = 0; i < MAX_STR_LEN * 2; ++i) {
        block_lengths[i] = 1; // Каждый блок имеет длину 1
        displacements[i] = i; // Смещения идут последовательно
    }

    MPI_Datatype str_type;
    MPI_Type_indexed(MAX_STR_LEN * 2, block_lengths, displacements, MPI_CHAR, &str_type);
    MPI_Type_commit(&str_type);

    // Отправляем строку процессу-слейву
    MPI_Send(str, 1, str_type, 1, 0, MPI_COMM_WORLD);
    cout << "Master process sent duplicated string." << endl;

    // Освобождаем тип
    MPI_Type_free(&str_type);
}

void slave_process(int rank, int count) {
    // Массив для получения строки
    char received_str[MAX_STR_LEN * 2 + 1] = {0};

    // Создаем тип MPI для получения строки
    int block_lengths[MAX_STR_LEN * 2];
    int displacements[MAX_STR_LEN * 2];
    for (int i = 0; i < MAX_STR_LEN * 2; ++i) {
        block_lengths[i] = 1; // Каждый блок имеет длину 1
        displacements[i] = i; // Смещения идут последовательно
    }

    MPI_Datatype str_type;
    MPI_Type_indexed(MAX_STR_LEN * 2, block_lengths, displacements, MPI_CHAR, &str_type);
    MPI_Type_commit(&str_type);

    // Получаем строку от мастер-процесса
    MPI_Recv(received_str, 1, str_type, MASTER_RANK, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);

    // Выводим полученную строку
    cout << "Slave process " << rank << " received string: " << received_str << endl;

    // Освобождаем тип
    MPI_Type_free(&str_type);
}
