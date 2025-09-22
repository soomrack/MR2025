#include <stdio.h>
#include <math.h>

typedef long long int RUB;

struct Person {
    RUB bank_account;
    RUB income;
    RUB mortage;
    RUB rent;
    RUB car_spending;
    RUB food;
    RUB clothes;
    RUB trip;
};

struct Person alice;
struct Person bob;

void alice_init() {
    alice.bank_account = 100 * 1000;
    alice.income = 150 * 1000;
    alice.mortage = 5000 * 1000;
    alice.car_spending = 20 * 1000;
    alice.food = 30 * 1000;
    alice.clothes = 20 * 1000;
    alice.trip = 200 * 1000;
}

void bob_init() {
    bob.bank_account = 300 * 1000;
    bob.income = 200 * 1000;
    bob.rent = 80 * 1000;
    bob.car_spending = 25 * 1000;
    bob.food = 30 * 1000;
    bob.clothes = 10 * 1000;
    bob.trip = 250 * 1000;
}

void alice_print() {
    printf("Alice bank account = %lld руб.\n", alice.bank_account);
}

void bob_print() {
    printf("Bob bank account = %lld руб.\n", bob.bank_account);
}

void alice_income(const int year, const int month) {
    if (month == 10) {
        alice.income *= 1.05; //Indexation
    }
    if (year == 2035 && month == 10) {
        alice.income *= 1.4; //Promotion
    }
    alice.bank_account += alice.income;

}

void bob_income(const int year, const int month) {
    if (month == 10) {
        bob.income *= 1.05; //Indexation
    }
    if ((year == 2030 && month == 10) || (year == 2040 && month == 10)) {
        bob.income *= 1.25; //Promotion
    }
    bob.bank_account += bob.income;
}

void alice_spending(const int year, const int month) {
    RUB car_crash = 0 * 1000; //Driving skill issues
    if (year%5 == 0 && month == 12) {
        car_crash = 200 * 1000;
    }
    if (month == 1) {
        alice.car_spending *= 1.07; //Inflation
        alice.food *= 1.07;
        alice.clothes *= 1.07;
    }
    alice.bank_account -= alice.car_spending;
    alice.bank_account -= alice.food;
    alice.bank_account -= alice.clothes;
    alice.bank_account -= car_crash;
}

void bob_spending(const int year, const int month) {
    if (month == 1) {
        bob.car_spending *= 1.07; //Inflation
        bob.food *= 1.07;
        bob.clothes *= 1.07;
        bob.rent *= 1.07;
    }
    bob.bank_account -= bob.car_spending;
    bob.bank_account -= bob.food;
    bob.bank_account -= bob.clothes;
    bob.bank_account -= bob.rent;
}

void alice_trip() {
    static int months_after_trip_counter = 0;
    months_after_trip_counter++;
    if (months_after_trip_counter == 12) {
        alice.bank_account -= alice.trip;
        months_after_trip_counter = 0;
    }
}

void bob_trip() {
    static int months_after_trip_counter = 0;
    months_after_trip_counter++;
    if (months_after_trip_counter == 6) {
        bob.bank_account -= bob.trip;
        months_after_trip_counter = 0;
    }
}
    
void alice_mortage(const int year, const int month) {
    RUB debt_body;
    if (year == 2025 && month == 9) {
        debt_body = alice.mortage;
    }
    RUB monthly_payment = alice.mortage / 20 * 12;
    float annual_percentage = 12;
    float mounthly_persentage = annual_percentage / 12 / 100;
    double A = alice.mortage * (mounthly_persentage * pow(1 + mounthly_persentage, 240)) / (pow(1 + mounthly_persentage, 240) - 1);
    alice.bank_account -= (A);
}

void simulation() {
    int year = 2025;
    int month = 9;

    while( !(year == 2045 && month == 9)){
        alice_income(year, month);
        alice_mortage(year, month);
        alice_spending(year, month);
        alice_trip();

        bob_income(year, month);
        bob_spending(year, month);
        bob_trip();

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
}
