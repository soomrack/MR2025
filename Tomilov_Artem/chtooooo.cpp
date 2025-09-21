#include <stdio.h>
#include <cmath>

typedef long long int RUB;

struct Person {
    RUB bank_account;
    RUB income;
    RUB rent;
    RUB food;
    RUB trip;
    RUB car_cost;
    RUB car_expense;
    RUB mortage;
    float interest_rate;
    RUB flat_price;
};

struct Person alice;
struct Person bob;

void bob_init() { // ipoteka
    bob.bank_account = 1000 * 1000; //первоначальный взнос 
    bob.income = 199 * 1000; // зпка
    bob.flat_price = 25000*1000;
    bob.interest_rate = 0.02;
    bob.food = 20000;
    bob.trip = 50 * 1000;
    bob.mortage = 115*1000;

}

void bob_print() {
    printf("Bob bank account = %lld руб.\n", bob.bank_account);
}

void alice_init() {
    alice.bank_account = 1000 * 1000; //первоначальный взнос
    alice.income = 200 * 1000; // зпка
    alice.rent = 100*1000;
    alice.food = 30000;
    alice.trip = 150 * 1000;
    alice.car_cost = 5000 * 1000; // стоимость машины
    alice.car_expense = 20 * 1000; // траты на машину
    alice.interest_rate = 0.02;
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

void alice_rent(const int year, const int month) {
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

    if (year > 2034 || (year == 2034 && month > 5) ) {
        alice.bank_account -= alice.car_expense; 
    }  
}

/*
------------------------------------------------------------------------------------
*/

void bob_income(const int year, const int month) {
    if (year == 2030 && month == 10) {
        bob.income *= 1.5; // indexatia
    }
    bob.bank_account += bob.income;
}

void bob_mortage(const int year, const int month) {
    if (year >= 2025 && year <= 2045) {
        bob.bank_account -= bob.mortage;
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


/*
--------------------------------------------------------------------------------------
*/

void apply_interest(Person &p) {
    double monthly_rate = p.interest_rate / 12;
    p.bank_account += static_cast<RUB>(p.bank_account * monthly_rate);
}

/*
--------------------------------------------------------------------------------------
*/

void simulation() {
    int year = 2025;
    int month = 9;

    while( !(year == 2045 && month == 9)){
        alice_income(year, month);
        alice_rent(year, month);
        alice_food(year, month);
        alice_car(year, month);
        alice_trip(year, month);
        apply_interest(alice);

        bob_income(year, month);
        bob_mortage(year, month);
        bob_food(year, month);
        bob_trip(year, month);
        apply_interest(bob);

        if (month == 12) {
            printf("=== year %d ===\n", year);
            printf("Alice bank account: %lld \n", alice.bank_account);
            printf("Bob bank account: %lld \n", bob.bank_account);
            printf("Bob mortage: %lld \n",(bob.flat_price - bob.bank_account > 0) ? (bob.flat_price - bob.bank_account) : 0);

        } // debug

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
