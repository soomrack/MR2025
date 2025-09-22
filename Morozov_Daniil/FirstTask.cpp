#include <stdio.h>

typedef long long int RUB;

struct Person {
    RUB bank_account;
    RUB income;
    RUB mortage;
    RUB spending;
    RUB food;
    RUB car_spending;
    RUB trip;
};

struct Person alice;
struct Person bob;

void alice_init() {
    alice.bank_account = 1000 * 1000;
    alice.income = 200 * 1000;
}

void bob_init() {
    bob.bank_account = 500 * 1000;
    bob.income = 100 * 1000;
}

void alice_print() {
    printf("Alice bank account = %lld руб.\n", alice.bank_account);
}

void alice_income(const int year, const int month) {
    if (year == 2030 && month == 10) {
        alice.income *= 1.5; //Promotion
    }
    alice.bank_account += alice.income;
 
}

void alice_mortage() {
    const RUB mortage = 70 * 1000;
    alice.bank_account -= mortage;
}

void alice_spending() {
    const RUB spending = 50 * 1000;
    alice.bank_account -= spending;
}

void alice_food(const int year, const int month) {
    RUB food = 20 * 1000;
    if (year == 2035 && month == 1) {
        food *= 2; //Crisis
    }
    alice.bank_account -= food;
}

void alice_car() {
    static int car_age = 0;
    car_age++;
    if (car_age == 10) {
        const RUB car_price = 500 * 1000;
        alice.bank_account -= car_price;
        car_age = 0;
    }
}

void alice_trip() {
    static int trip_counter = 0;
    trip_counter++;
    if (trip_counter == 6) {
        const RUB trip_price = 100 * 1000;
        alice.bank_account -= trip_price;
        trip_counter = 0;
    }
}

void simulation() {
    int year = 2025;
    int month = 9;

    while( !(year == 2045 && month == 9)){
        alice_income(year, month);
        // alice_mortage();
        // alice_spending();
        // alice_food(year, month);
        // alice_car();
        // alice_trip();

        month++;
        if (month == 13) {
            year++;
            month = 1;
        }
    }
}
int main() {
    alice_init();

    simulation();

    alice_print();
}
