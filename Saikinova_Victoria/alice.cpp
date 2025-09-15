#include <stdio.h>

typedef long long int RUB;

struct person {
    RUB salary;
    RUB balance;
};

person alice;

void alice_init(RUB x, RUB y) {
    alice.salary = x;
    alice.balance = y;
}

void alice_print1() {
    printf("ALice took out a mortgage\n");
    printf("ALice has salary = %lld RUB\n", alice.salary);
    printf("ALice has balance = %lld RUB\n\n", alice.balance);
}

void alice_print2() {
    printf("ALice has balance = %lld RUB\n", alice.balance);
    if (alice.balance < 0) {
        printf("ALice starves(\n");
    }
    else {
        printf("ALice`s happy)\n");
    }
}

void alice_promotion(const int year, const int month) {
    if (year - 2025 == 5 && month == 9) {
        alice.salary += static_cast < RUB>(20*1000);
    }
}

void alice_income(const int year, const int month) {
    alice.balance += alice.salary;
    alice_promotion(year, month);
}

void calculat() {
    int month = 9;
    int year = 2025;
    while (!(month == 9 && year == 2045)) {
        alice_income(year,month);
        month ++;
        if (month == 13) {
            year++;
            month = 1;
        }
    }
}

int main() {
    alice_init(static_cast<RUB>(50*1000), static_cast<RUB>(150*1000));
    alice_print1();
    calculat();
    alice_print2();
}

// Запуск программы: CTRL+F5 или меню "Отладка" > "Запуск без отладки"
// Отладка программы: F5 или     меню "Отладка" > "Запустить отладку"

// Советы по началу работы 
//   1. В окне обозревателя решений можно добавлять файлы и управлять ими.
//   2. В окне Team Explorer можно подключиться к системе управления версиями.
//   3. В окне "Выходные данные" можно просматривать выходные данные сборки и другие сообщения.
//   4. В окне "Список ошибок" можно просматривать ошибки.
//   5. Последовательно выберите пункты меню "Проект" > "Добавить новый элемент", чтобы создать файлы кода, или "Проект" > "Добавить существующий элемент", чтобы добавить в проект существующие файлы кода.
//   6. Чтобы снова открыть этот проект позже, выберите пункты меню "Файл" > "Открыть" > "Проект" и выберите SLN-файл.
