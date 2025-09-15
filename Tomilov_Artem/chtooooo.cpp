#include <stdio.h>

typedef long long int RUB;

struct Person {
    RUB bank_account;
    RUB income;
    RUB rent;
    RUB food;
    RUB trip;
    RUB car_cost, car_expense;
    RUB credit;
};

struct Person alice;
struct Person bob;

void bob_init() { // ipoteka
    bob.bank_account = 2500 * 1000; //первоначальный взнос
    bob.income = 200 * 1000; // зпка
    bob.credit = 130*1000;
    bob.food = 20000;
    bob.trip = 50 * 1000;
}

void bob_print() {
    printf("Bob bank account = %lld руб.\n", bob.bank_account);
}

void alice_init() {
    alice.bank_account = 1000 * 1000; //первоначальный взнос
    alice.income = 200 * 1000; // зпка
    alice.rent = 45000;
    alice.food = 20000;
    alice.trip = 100 * 1000;
    alice.car_cost = 3000 * 1000; // стоимость машины
    alice.car_expense = 13 * 1000; // траты на машину
}

void alice_print() {
    printf("Alice bank account = %lld руб.\n", alice.bank_account);
}

void alice_income(const int year, const int month) {
    if (year == 2030 && month == 10) {
        alice.income *= 1.5; // indexatia
    }
    alice.bank_account += alice.income;

}

void alice_spending(const int year, const int month) {
    if (year > 2025 && month == 1) {
        alice.rent = alice.rent* 1.05;
    }
    alice.bank_account -= alice.rent;
}

void alice_food(const int year, const int month) {
    if (year > 2025 && month == 1) {
        alice.food *= 1.03; 
    }
    alice.bank_account -= alice.food;
}

void alice_trip(const int year, const int month) {
    //путешествия каждый год
    if (month == 8) {
        alice.bank_account -= alice.trip;
    }
}

void alice_car(const int year, const int month) {
    if (month == 5 && year == 2034) { 
        alice.bank_account -= alice.car_cost; 
    }

    if (year > 2034 || (year > 2034 && month > 5) ) {
        alice.bank_account -= alice.car_expense; 
    }  
}

////////////////////////////////////////////////////////////////////////////// bobik

void bob_income(const int year, const int month) {
    if (year == 2030 && month == 10) {
        bob.income *= 1.5; // indexatia
    }
    bob.bank_account += bob.income;
}

void bob_spending(const int year, const int month) {
    if (year >= 2025 && year < 2045) {
        bob.bank_account -= bob.credit;
    }
}

void bob_food(const int year, const int month) {
    if (year > 2025 && month == 1) {
        bob.food *= 1.03; 
    }
    bob.bank_account -= bob.food;
}

void bob_trip(const int year, const int month) {
    //путешествия каждый год
    if (month == 8) {
        bob.bank_account -= bob.trip;
    }
}

void simulation() {
    int year = 2025;
    int month = 9;

    while( !(year == 2045 && month == 9)){
        alice_income(year, month);
        // alice_mortage();
        alice_spending(year, month);
        alice_food(year, month);
        alice_car(year, month);
        alice_trip(year, month);

        bob_income(year, month);
        bob_spending(year, month);
        bob_food(year, month);
        bob_trip(year, month);

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
