#include <stdio.h>
#include <math.h>

typedef long long int RUB;

struct Person {
    RUB bank_account;
    RUB income;
    RUB spending;
    RUB food;
    RUB mortage;
    RUB car_price;
    RUB trip_price;
    RUB rent;
};

struct Person alice;
struct Person bob;

RUB calc_mortgage_payment(RUB principal, double annual_rate, int years) {
    int n = years * 12;                        // срок в месяцах
    double r = annual_rate / 12.0 / 100.0;     // месячная ставка

    // формула аннуитетного платежа
    double A = principal * (r * pow(1 + r, n)) / (pow(1 + r, n) - 1);

    return (RUB)A;
}

void alice_init() {
    alice.bank_account = 1000 * 1000;
    alice.income = 200 * 1000;
    alice.food = 20 * 1000;
    alice.spending = 50 * 1000;
    alice.mortage = calc_mortgage_payment(5 * 1000 * 1000, 10, 20); //5M, 10%, 20 years
    alice.car_price = 1 * 1000 * 1000;
    alice.trip_price = 200 * 1000;
}

void bob_init() {
    bob.bank_account = 500 * 1000;
    bob.income = 100 * 1000;
    bob.food = 15 * 1000;
    bob.spending = 20 * 1000;
    bob.rent = 25 * 1000;
    bob.car_price = 700 * 1000;
    bob.trip_price = 100 * 1000;
}

double inflation_rate = 0.07; // 7% годовая инфляция

void apply_inflation(struct Person *p) {
    p->income = (RUB)(p->income * (1 + inflation_rate));
    p->spending = (RUB)(p->spending * (1 + inflation_rate));
    p->food = (RUB)(p->food * (1 + inflation_rate));
    p->car_price = (RUB)(p->car_price * (1 + inflation_rate));
    p->trip_price = (RUB)(p->trip_price * (1 + inflation_rate));
    p->rent = (RUB)(p->rent * (1 + inflation_rate));
}

void alice_print() {
    printf("Alice bank account = %lld руб.\n", alice.bank_account);
}

void bob_print() {
    printf("Bob bank account = %lld руб.\n", bob.bank_account);
}

void alice_income(const int year, const int month) {
    if (month == 10) {
        alice.income *= 1.1; //Indexation
    }
    alice.bank_account += alice.income;
}

void bob_income(const int year, const int month) {
    if (month == 10) {
        bob.income *= 1.1; //Indexation
    }
    bob.bank_account += bob.income;

    if (year == 2030) {
        bob.income += 20 * 1000; //Promotion
    }
}

void alice_mortage() {
    alice.bank_account -= alice.mortage;
}

void alice_spending() {
    alice.bank_account -= alice.spending;
}

void bob_spending() {
    bob.bank_account -= bob.spending;
}

void alice_food(const int year, const int month) {
    if (year == 2035 && month == 1) {
        alice.food *= 2; //Crisis
    }
    alice.bank_account -= alice.food;
}

void bob_food(const int year, const int month) {
    if (year == 2035 && month == 1) {
        bob.food *= 2; //Crisis
    }
    bob.bank_account -= bob.food;
}

void alice_car() {
    static int car_months = 0;
    car_months++;
    if (car_months == 50) {
        alice.bank_account -= alice.car_price;
        car_months = 0;
    }
}

void bob_car() {
    static int car_months = 0;
    car_months++;
    if (car_months == 70) {
        bob.bank_account -= bob.car_price;
        car_months = 0;
    }
}

void alice_trip() {
    static int months_after_trip_counter = 0;
    months_after_trip_counter++;
    if (months_after_trip_counter == 12) {
        alice.bank_account -= alice.trip_price;
        months_after_trip_counter = 0;
    }
}

void bob_trip() {
    static int months_after_trip_counter = 0;
    months_after_trip_counter++;
    if (months_after_trip_counter == 6) {
        bob.bank_account -= bob.trip_price;
        months_after_trip_counter = 0;
    }
}


void bob_rent() {
    bob.bank_account -= bob.rent;
}

void simulation() {
    int year = 2025;
    int month = 9;

    while( !(year == 2045 && month == 9)){

         if (month == 1) {
            apply_inflation(&alice);
            apply_inflation(&bob);
        }

        alice_income(year, month);
        alice_mortage();
        alice_spending();
        alice_food(year, month);
        alice_car();
        alice_trip();

        bob_income(year, month);
        bob_spending();
        bob_food(year, month);
        bob_car();
        bob_trip();
        bob_rent();

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

    bob_print();
    alice_print();
}
