#include <stdio.h>

typedef long long int RUB;

struct Person {
    RUB bank_account;
    RUB income;

    RUB food;
    RUB clothes;
    RUB utilities;
    RUB rent; // у Alice = 0, у Bob > 0
    RUB flat_price;
};

Person alice;
Person bob;


// TODO: добавить стоимость квартиры для результатов сравнения

//Блок 1 Инициализация персонажей

void alice_init() {
    alice.bank_account = 1000 * 1000;
    alice.income       = 200 * 1000;

    alice.food      = 20 * 1000;
    alice.clothes   = 10 * 1000;
    alice.utilities = 15 * 1000;
    alice.rent      = 0; // Alice живёт в своей квартире
    alice.flat_price = 6 * 1000 * 1000;// Стоимость квартиры 
}

void bob_init() {
    bob.bank_account = 800 * 1000;
    bob.income       = 200 * 1000;

    bob.food      = 20 * 1000;
    bob.clothes   = 10 * 1000;
    bob.utilities = 15 * 1000;
    bob.rent      = 20 * 1000; // Bob снимает квартиру
    bob.flat_price = 0; // у него квартиры в собственности нет
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


// Блок 3 Расходы


void monthly_expenses(Person *p) {
    p->bank_account -= (p->food + p->clothes + p->utilities);
}

void pay_rent(Person *p) {
    p->bank_account -= p->rent;
}


// Блок 3 Функции вывода

void alice_print() {
    printf("Alice bank account = %lld руб.\n", alice.bank_account);
}

void bob_print() {
    printf("Bob   bank account = %lld руб.\n", bob.bank_account);
}
// TODO: при сравнении итогов учитывать стоимость квартиры у Alice

// Блок 4 Симуляция 

void simulation() {
    int year = 2025;
    int month = 9;

    while (!(year == 2045 && month == 9)) {
        // TODO: добавить ипотеку и кредит
        // TODO: добавить инфляцию
        
        // доходы
        alice_income(year, month);
        bob_income(year, month);

        // расходы
        monthly_expenses(&alice);
        monthly_expenses(&bob);

        // аренда (только у Bob ненулевая)
        pay_rent(&bob);

        // шаг времени
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
