#include <cstddef> // Для std::size_t
#include <iostream> // Для вывода в консоль
#include <mpi.h> // Для использования MPI функций
#include <vector> // Для работы с std::vector
#include <algorithm> // Для std::max
#include <limits> // Для std::numeric_limits

// Функция для вывода содержимого вектора
void print_vector(const std::vector<double> &vec) {
  for (const double &item : vec) { // Проходим по каждому элементу вектора
    std::cout << item << " "; // Выводим текущий элемент
  }
  std::cout << std::endl; // Завершаем строку
}

// Координаторский процесс (ранк 0)
void coordinator_process(int size) {
  // Входной вектор X
  const std::vector<double> X = {1, 2, 3, 4, 5, 6, 7, 10,
                                 9, 10, 11, 12, 13, 14, 15, 16};
  if (X.size() % 2 != 0) {
    std::cerr << "Size vector be even\n";
    MPI_Abort(MPI_COMM_WORLD, 1);
    return;
  }

  const size_t N = X.size() / 2; // Длина половины вектора
  const std::vector<double> A(X.begin(), X.begin() + N); // Первая половина вектора X
  const std::vector<double> B(X.begin() + N, X.end()); // Вторая половина вектора X

  std::cout << "Vector X: ";
  print_vector(X); // Выводим вектор X

  std::cout << "Vector A: ";
  print_vector(A); // Выводим вектор A

  std::cout << "Vector B: ";
  print_vector(B); // Выводим вектор B

  MPI_Aint N_as_mpi = static_cast<MPI_Aint>(N); // Преобразуем размер N в тип MPI_Aint

  // Отправляем размер вектора всем воркерам
  for (int i = 1; i < size; ++i) {
    MPI_Send(&N_as_mpi, 1, MPI_AINT, i, 0, MPI_COMM_WORLD);
  }

  // Распределяем векторы A и B между всеми воркерами
  int chunk_size = N / (size - 1);
  for (int i = 1; i < size; ++i) {
    int start = (i - 1) * chunk_size;
    int end = (i == size - 1) ? N : start + chunk_size;
    MPI_Send(A.data() + start, end - start, MPI_DOUBLE, i, 1, MPI_COMM_WORLD);
    MPI_Send(B.data() + start, end - start, MPI_DOUBLE, i, 2, MPI_COMM_WORLD);
  }

  double local_max; // Переменная для хранения локального максимума
  double global_max = -std::numeric_limits<double>::infinity(); // Устанавливаем минимальное значение

  // Получаем локальные максимумы от всех воркеров
  for (int i = 1; i < size; ++i) {
    MPI_Recv(&local_max, 1, MPI_DOUBLE, i, 3, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
    global_max = std::max(global_max, local_max); // Находим глобальный максимум
  }

  // Выводим максимальное значение
  std::cout << "max A[i] and B[i]: " << global_max << std::endl;
}

// Воркерский процесс (ранк > 0)
void worker_process(int rank, int size) {
  MPI_Aint N_as_mpi; // Переменная для хранения размера вектора
  // Получаем размер вектора от координатора
  MPI_Recv(&N_as_mpi, 1, MPI_AINT, 0, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);

  size_t N = static_cast<size_t>(N_as_mpi); // Преобразуем тип MPI_Aint в std::size_t

  int chunk_size = N / (size - 1);
  int start = (rank - 1) * chunk_size;
  int end = (rank == size - 1) ? N : start + chunk_size;

  std::vector<double> A(end - start); // Вектор для первой половины X
  std::vector<double> B(end - start); // Вектор для второй половины X

  // Получаем векторы A и B от координатора
  MPI_Recv(A.data(), end - start, MPI_DOUBLE, 0, 1, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
  MPI_Recv(B.data(), end - start, MPI_DOUBLE, 0, 2, MPI_COMM_WORLD, MPI_STATUS_IGNORE);

  // Выводим данные, которые находятся в текущем процессе
  std::cout << "Process " << rank << " received A: ";
  print_vector(A);
  std::cout << "Process " << rank << " received B: ";
  print_vector(B);

  double local_max = -std::numeric_limits<double>::infinity(); // Устанавливаем минимальное значение
  // Находим локальный максимум для A[i] * B[i]
  for (size_t i = 0; i < A.size(); ++i) {
    local_max = std::max(local_max, A[i] * B[i]); // Вычисляем максимум покомпонентного произведения
  }

  // Отправляем локальный максимум координатору
  MPI_Send(&local_max, 1, MPI_DOUBLE, 0, 3, MPI_COMM_WORLD);
}

// Главная функция программы
int main(int argc, char **argv) {
  MPI_Init(&argc, &argv); // Инициализация MPI


  int rank, size;
  MPI_Comm_rank(MPI_COMM_WORLD, &rank); // Получаем текущий ранк процесса
  MPI_Comm_size(MPI_COMM_WORLD, &size); // Получаем общее количество процессов

  if (size < 2 || size % 2 != 0) { // Проверяем, что запущено четное количество процессов
    if (rank == 0) { // Только координатор выводит сообщение
      std::cerr << "You need an even number of processes\n";
    }
    MPI_Finalize(); // очищает все состояния, связанные с MPI
    return 1; // Завершаем программу с ошибкой
  }

  if (rank == 0) {
    coordinator_process(size); // Если ранк 0, запускаем координаторскую функцию
  } else {
    worker_process(rank, size); // Если ранк > 0, запускаем воркерскую функцию
  }

  MPI_Finalize(); // Завершаем MPI
  return 0; // Успешное завершение программы
}
