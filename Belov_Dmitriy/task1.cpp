#include <stdio.h>

typedef long long int RUB;

struct Person {
    RUB bank_account;
    RUB income;
};

Person alice;
Person bob;


//Блок 1 Инициализация персонажей

void alice_init() {
    alice.bank_account = 1000 * 1000;
    alice.income = 200 * 1000;
}

void bob_init() {
    bob.bank_account = 800 * 1000;
    bob.income = 200 * 1000;
}


// Блок 2 Доходы

void alice_income(const int year, const int month) {
    if (year == 2030 && month == 10) {
        alice.income *= 1.5; 
    }
    alice.bank_account += alice.income;
}

void bob_income(const int year, const int month) {
    if (year == 2032 && month == 5) {
        bob.income *= 1.3;
    }
    bob.bank_account += bob.income;
}


// Блок 3 Функции вывода

void alice_print() {
    printf("Alice bank account = %lld руб.\n", alice.bank_account);
}

void bob_print() {
    printf("Bob   bank account = %lld руб.\n", bob.bank_account);
}


// Блок 4 Симуляция 
void simulation() {
    int year = 2025;
    int month = 9;

    while (!(year == 2045 && month == 9)) {
        alice_income(year, month);
        bob_income(year, month);

        // TODO: добавить расходы (еда, одежда, коммунальные услуги)
        // TODO: добавить аренду для Bob
        // TODO: добавить ипотеку/кредит и проценты
        // TODO: добавить инфляцию

        month++;
        if (month == 13) {
            year++;
            month = 1;
        }
    }
}


int main() {
    alice_init();
    bob_init();

    simulation();

    alice_print();
    bob_print();

    return 0;
}
