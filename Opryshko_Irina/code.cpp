#include <stdio.h>
#include <windows.h>
#include <iostream>
#include <cmath>
using namespace std;

typedef long long int RUB;

struct Person {
    RUB bank_account;
    RUB income;
    RUB mortage;
    RUB mortgage_balance;     // Остаток долга по ипотеке
    double mortgage_rate;     // Годовая процентная ставка
    int mortage_term;        // Срок ипотеки в месяцах
    RUB monthly_payment;      // Ежемесячный платеж
    int mortgage_start_year;  // Год начала ипотеки
    int mortgage_start_month;
};

struct Person alice;
struct Person bob;

void alice_init() {
    cout << "Введите начальный счет Алис: " << "\n";
    cin >> alice.bank_account;

    cout << "Введите ежемесячный доход Алис: " << "\n";
    cin >> alice.income;

    cout << "Введите cумму, взятую в ипотеку Алис: " << "\n";
    cin >> alice.mortage;

    alice.monthly_payment = 50000;
    alice.mortgage_balance = alice.mortage - alice.bank_account;

    cout << "Ежемесячный платеж по ипотеке: " << alice.monthly_payment << " руб.\n";
}

void bob_init() {
    bob.bank_account = 2000000;
    cout << "Начальный счет Боба: " << bob.bank_account << "\n";

    bob.income = 100000;
    cout << "Ежемесячный доход Боба: " << bob.income << "\n";
}

void alice_print() {
    cout << "Банковский счет Алис = " << alice.bank_account << " руб.\n";
    cout << "ЗП Алис к концу периода = " << alice.income << " руб.\n";
    if (alice.mortgage_balance > 0) {
        cout << "Остаток по ипотеке: " << alice.mortgage_balance << " руб.\n";
    }
    else {
        cout << "Ипотека полностью погашена\n";
    }
}

void bob_print() {
    cout << "Банковский счет Боба = " << bob.bank_account << " руб.\n";
    cout << "ЗП Боба к концу периода = " << bob.income << " руб.\n";
}

void alice_income(const int year, const int month) {
    if (year == 2030 && month == 10) {
        alice.income = alice.income * 1.5; // Повышение
    }
    alice.bank_account += alice.income;
}


void alice_mortage(const int year, const int month) {
    if (alice.mortgage_balance <= 0) {
        return; // Ипотека уже погашена
    }


    RUB payment = 100000;

    alice.bank_account -= payment;

    alice.mortgage_balance -= payment;

    if (alice.mortgage_balance <= 0) {
        cout << "Ипотека погашена в " << year << " году, месяц " << month << "\n";
    }
}

void alice_debt() {
    if (alice.bank_account < 0) {
        cout << "Алис в долгах как в шелках. ";
    }
}

void bob_income(const int year, const int month) {
    if (year == 2029 && month == 1) {
        bob.income = bob.income * 1.5; // Повышение
    }
    bob.bank_account += bob.income;
}


void bob_rent(const int year, const int month) {
    float rent_payment = 60000;

    if (year >= 2040) {
        rent_payment *= 1.6;
    }

    bob.bank_account -= rent_payment;
}

void simulation() {
    int year = 2025;
    int month = 9;

    while (!(year == 2045 && month == 9)) {
        alice_income(year, month);
        bob_income(year, month);

        alice_mortage(year, month);
        bob_rent(year, month);      

        month++;
        if (month == 13) {
            year++;
            month = 1;
        }
    }

    alice_debt();
}

int main() {
    SetConsoleCP(1251);
    SetConsoleOutputCP(1251);

    alice_init();
    bob_init();

    simulation();

    alice_print();
    bob_print();

    return 0;
}
