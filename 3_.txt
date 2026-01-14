#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

struct Matrix 
{
    size_t rows;
    size_t cols;
    double* data;
};
typedef struct Matrix Matrix;

enum MatrixExceptionLevel {ERROR, WARNING, INFO, DEBUG};

const Matrix MATRIX_NULL = {0, 0, NULL};


// Сообщение об ошибке
void matrix_exception(const enum MatrixExceptionLevel level, char *msg)
{
    if(level == ERROR) {
        printf("ERROR: %s", msg);
    }

    if(level == WARNING) {
        printf("WARNING: %s", msg);
    }
    
    if(level == INFO) {
        printf("INFO: %s", msg);
    }
}


Matrix matrix_alloc(const size_t rows, const size_t cols) 
{
    Matrix M;
    
    if (rows == 0 || cols == 0) {
        matrix_exception(INFO, "Матрица содержит 0 столбцов или строк");
        return (Matrix) {rows, cols, NULL};
    }
    
    size_t size = rows * cols;
    if (size / rows != cols) {
        matrix_exception(ERROR, "OVERFLOW: Переполнение количества элементов.");
        return MATRIX_NULL;
    }
    
    size_t size_in_bytes = size * sizeof(double);
    
    if (size_in_bytes / sizeof(double) != size) {
        matrix_exception(ERROR, "OVERFLOW: Переполнение выделенной памяти");
        return MATRIX_NULL;
    }
    
    M.data = malloc(rows * cols * sizeof(double));
    
    if (M.data == NULL) {
        matrix_exception(ERROR, "Сбой выделения памяти");
        return MATRIX_NULL;
    }
    
    M.rows = rows;
    M.cols = cols;
    return M;
}


void matrix_free(Matrix* M)  // Функция для освобождения памяти матрицы
{
    if (M == NULL){
        matrix_exception(ERROR, "Обращение к недопутимой области памяти");
        return;
    }
    
    free(M->data);
    *M = MATRIX_NULL;
}


// Нулевая матрица
void matrix_zero(const Matrix M)
{
    memset(M.data, 0, M.cols * M.rows * sizeof(double));
}


// Единичная матрица
Matrix matrix_identity(size_t size)
{
    Matrix M = matrix_alloc(size, size);

    matrix_zero(M);

    for (size_t idx = 0; idx < size; idx++) {
        M.data[idx * size + idx] = 1.0;
    }

    return M; 
}



void matrix_print(const Matrix M) // Функция для печати матрицы
{
    for (size_t row = 0; row < M.rows; row++) {
        for (size_t col = 0; col < M.cols; col++) {
            printf("%.2f ", M.data[row * M.cols + col]);
        }
        printf("\n");
    }
}


// C = A + B 
Matrix matrix_sum(const Matrix A, const Matrix B) // Сложение матриц
{
    if (A.rows != B.rows || A.cols != B.cols) {
        matrix_exception(WARNING, "Размеры матриц не подходят для сложения.\n");
        return MATRIX_NULL;
    }

    Matrix C = matrix_alloc(A.rows, A.cols);
    
    for (size_t idx = 0; idx < C.rows * C.cols; idx++) {
        C.data[idx] = A.data[idx] + B.data[idx];
    }
    return C;
}


// C = A - B 
Matrix matrix_subtract(const Matrix A, const Matrix B) // Вычитание матриц
{
    if (A.rows != B.rows || A.cols != B.cols) {
        matrix_exception(WARNING, "Размеры матриц не подходят для вычитания.\n");
        return MATRIX_NULL;
    }

    Matrix C = matrix_alloc(A.rows, A.cols);
    for (size_t idx = 0; idx < C.rows * C.cols; idx++) {
        C.data[idx] = A.data[idx] - B.data[idx];
    }
    return C;
}


// C = A * B
Matrix matrix_multiply(const Matrix A, const Matrix B) // Умножение матриц
{
    if (A.cols != B.rows) {
        matrix_exception(WARNING,"Число столбцов первой матрицы не равно числу строк второй матрицы.\n");
        return MATRIX_NULL;
    }

    Matrix C = matrix_alloc(A.rows, B.cols);
    
    for (size_t row = 0; row < C.rows; row++) {
        for (size_t col = 0; col < C.cols; col++) {
            C.data[row * B.cols + col] = 0;
            for (size_t idx = 0; idx < A.cols; idx++) {
                C.data[row * C.cols + col] += A.data[row * A.cols + idx] * B.data[idx * B.cols + col];
            }
        }
    }
    return C;
}


Matrix matrix_transpose(const Matrix A) // Транспонирование матрицы
{
    Matrix T = matrix_alloc(A.cols, A.rows);
    
    for (size_t row = 0; row < T.rows; row++) {
        for (size_t col = 0; col < T.cols; col++) {
            T.data[col * T.cols + row] = A.data[row * A.cols + col];
        }
    }
    return T;
}


Matrix matrix_power(const Matrix A, int power)  // Возведение матрицы в степень
{
    if (A.rows != A.cols) {
        matrix_exception(WARNING, "Матрица должна быть квадратной для возведения в степень.\n");
        return MATRIX_NULL;
    }
    
    Matrix result = matrix_identity(A.rows); // Создаем единичную матрицу

    for (int n = 0; n < power; n++) {
        Matrix temp = matrix_multiply(result, A);
        matrix_free(&result);
        result = temp;
    }

    return result;
}

// C = A * k
Matrix matrix_by_scalar(const Matrix A, double scalar) // Умножение матрицы на число
{
    Matrix C = matrix_alloc(A.rows, A.cols);
    
    for (size_t idx = 0; idx < C.rows * C.cols; idx++) {
        C.data[idx] = A.data[idx] * scalar;
    }
    return C;
}


double matrix_determinant(const Matrix A) // Определитель матрицы (для 2x2 и 3x3)
{
    if (A.rows != A.cols) {
        matrix_exception(WARNING, "Матрица должна быть квадратной для нахождения определителя.\n");
        return NAN;
    }
    
    if (A.rows == 1 && A.cols == 1) {
        return A.data[0];
    } 
    
    if (A.rows == 2 && A.cols == 2){
        return A.data[0] * A.data[3] - A.data[1] * A.data[2];
    }
    
    if (A.rows == 3 && A.cols == 3) {
        return A.data[0] * (A.data[4] * A.data[8] - A.data[5] * A.data[7]) -
               A.data[1] * (A.data[3] * A.data[8] - A.data[5] * A.data[6]) +
               A.data[2] * (A.data[3] * A.data[7] - A.data[4] * A.data[6]);
    }
    return 0; 
}


double factorial (const unsigned int f) 
{
    unsigned long long int res = 1;
    for (unsigned int idx = 1; idx <= f; idx++) {
        res *= idx;
    }

    return res;
}


// e ^ A
Matrix matrix_exponent(const Matrix A, const unsigned int num)
{
    if (A.rows != A.cols) {
        matrix_exception(WARNING, "Матрица должна быть квадратной для вычисления экспоненты");
        return MATRIX_NULL;
    }

    Matrix E = matrix_alloc(A.rows, A.cols);   
  
    if (E.data == NULL) {
        matrix_exception(ERROR, "Сбой выделения памяти");
        return MATRIX_NULL;
    }
    
    matrix_free(&E);
    E = matrix_identity(A.rows);

    if (num == 1) {
        return E;
    }
    
    for (size_t cur_num = 1; cur_num < num; ++cur_num) {
        Matrix tmp = matrix_power(A,cur_num);
        tmp = matrix_by_scalar(tmp, 1/factorial(cur_num));

        Matrix exp = matrix_sum(E, tmp);
        memcpy(E.data, exp.data, exp.rows * exp.cols * sizeof(double));
        
        matrix_free(&tmp);
        matrix_free(&exp);
        }
    
    return E;
}


int main() 
{
    Matrix A = matrix_alloc(3,3);
    Matrix B = matrix_alloc(3, 3);

    double data_A[9] = {3, 1, 7, 0, 5, 7, 2, 5, 8};
    double data_B[9] = {5, 0, 8, 1, 9, 6, 3, 2, 1};

    memcpy(A.data, data_A, 9 * sizeof(double));
    memcpy(B.data, data_B, 9 * sizeof(double));

    // Печать исходных матриц
    printf("Матрица A:\n");
    matrix_print(A);

    printf("Матрица B:\n");
    matrix_print(B);

    // Сложение
    Matrix C = matrix_sum(A, B);
    printf("Результат сложения:\n");
    matrix_print(C);

    // Вычитание
    Matrix D = matrix_subtract(A, B);
    printf("Результат вычитания:\n");
    matrix_print(D);

    // Умножение
    Matrix E = matrix_multiply(A, B);
    printf("Результат умножения:\n");
    matrix_print(E);

    // Транспонирование
    Matrix T = matrix_transpose(A);
    printf("Транспонированная матрица A:\n");
    matrix_print(T);

    // Возведение в степень
    int power = 3;
    Matrix F = matrix_power(A, power);
    printf("Матрица A в степени %d:\n", power);
    matrix_print(F);

    // Умножение на число
    double scalar = 5;
    Matrix G = matrix_by_scalar(A, scalar);
    printf("Матрица A умноженная на %2.f:\n", scalar);
    matrix_print(G);
    
    // Определитель
    printf("Определитель матрицы A: %2.f \n", matrix_determinant(A));
    
    //Матричная экспонента
    Matrix exponent_A = matrix_exponent(A, 3);
    printf("Матричная экспонента от A:\n");
    matrix_print(exponent_A);

    // Освобождение памяти
    matrix_free(&A);
    matrix_free(&B);
    matrix_free(&C);
    matrix_free(&D);
    matrix_free(&E);
    matrix_free(&T);
    matrix_free(&F);
    matrix_free(&G);
    matrix_free(&exponent_A);

    return 0;
}
