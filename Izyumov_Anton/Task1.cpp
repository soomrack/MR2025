#include <stdio.h>


typedef long long int RUB;


struct Person {
    RUB bank_account;
    RUB income;
    RUB cash;
    RUB pocket_cash;
    RUB spending;
    RUB car_spending;
    RUB food_spending;
    RUB mortgage_spending;
    RUB rent_spending;
    RUB house_cost;
    RUB trip_spending;
    RUB car_accident_cost;
    bool mortgage_closed;
};


struct Person alice;
struct Person bob;


void alice_init() {
    alice.bank_account = 3 * 1000 * 1000;
    alice.income = 200 * 1000;
    alice.cash = 0;
    alice.pocket_cash = 5000;
    alice.spending = 50000;
    alice.car_spending = 40000;
    alice.food_spending = 40000;
    alice.mortgage_spending = 100 * 1000;
    alice.trip_spending = 60000;
    alice.mortgage_closed = false;
}


void bob_init() { 
    bob.bank_account = 3 * 1000 * 1000;
    bob.income = 220 * 1000;
    bob.cash = 0;
    bob.pocket_cash = 5000;
    bob.spending = 50000;
    bob.car_spending = 40000;
    bob.food_spending = 40000;
    bob.rent_spending = 60000;
    bob.house_cost = 12 * 1000 * 1000;
    bob.trip_spending = 60000;
    bob.car_accident_cost = 100 * 1000;
}


void persons_print() {
    printf("Bob bank account = %lld rub.\n", bob.bank_account);
    printf("Alice bank account = %lld rub.\n", alice.bank_account);
}


void alice_income(const int year, const int month) {
    if (year == 2030 && month == 10) {
        alice.income *= 1.5; // Alice promotion
    }
    alice.cash += alice.income;

}


void alice_mortgage(const int year, const int month) {
    if (!alice.mortgage_closed) {
        if (year == 2025 && month == 9) {
            alice.bank_account -= 2 * 1000 * 1000; // Alice initial payment
        }
        else {
            alice.cash -= alice.mortgage_spending;
        }
        if (year == 2045 && month == 9) {
            alice.mortgage_closed = true;
        }
    }
}


void alice_spending(const int month) {  // Alice out-of-pocket expenses
    RUB spending = alice.spending;
    if (month == 2) {
        spending -= 7 * 1000;
    }
    if (month == 7) {
        spending += 5 * 1000;
    }
    if (month == 8) {
        spending += 2 * 1000;
    }
    alice.cash -= spending;
}


void alice_food() {
    alice.cash -= alice.food_spending;
}


void alice_car() {
        alice.cash -= alice.car_spending;
}


void alice_trip(const int month) {    // Alice annual trip
    if (month == 7) {
        alice.bank_account -= alice.trip_spending;
    }
}


void alice_deposit() {
    alice.bank_account += alice.cash - alice.pocket_cash;
    alice.cash = alice.pocket_cash;
}


///////////////////////////////////////////////////////////////////


void bob_income(const int year, const int month) {
    if (year == 2027 && month == 7) {
        bob.income *= 1.3; // Bob promotion
    }
    bob.cash += bob.income;
}


void bob_rent(const int year, const int month) {
    if (year == 2045 && month == 8) {
        bob.bank_account -= bob.house_cost;  // Bob buy a house
    }
    else {
        bob.cash -= bob.rent_spending;
    }
}


void bob_spending(const int month) {  // Bob out-of-pocket expenses
    RUB spending = bob.spending;
    if (month == 2) {
        spending -= 7 * 1000;
    }
    if (month == 7) {
        spending += 5 * 1000;
    }
    if (month == 8) {
        spending += 2 * 1000;
    }
    bob.cash -= spending;
}


void bob_food() {
    bob.cash -= bob.food_spending;
}


void bob_car(const int year, const int month) {
    if (year == 2034 && month == 7) {
        bob.bank_account -= bob.car_accident_cost; // Bob's car accident
    }
    bob.cash -= bob.car_spending;
}


void bob_trip(const int month) {  // Bob's annual trip
    if (month == 7) {
        bob.bank_account -= bob.trip_spending;
    }
}


void bob_deposit() {
    bob.bank_account += bob.cash - bob.pocket_cash;
    bob.cash = bob.pocket_cash;
}


void deposit_interest() {
    alice.bank_account *= 1.08;
    bob.bank_account *= 1.08;
}


void simulation() {
    int year = 2025;
    int month = 9;

    while (!(year == 2045 && month == 9)) {
        alice_income(year, month);
        alice_mortgage(year, month);
        alice_spending(month);
        alice_food();
        alice_car();
        alice_trip(month);
        alice_deposit();

        bob_income(year, month);
        bob_rent(year, month);
        bob_spending(month);
        bob_food();
        bob_car(year, month);
        bob_trip(month);
        bob_deposit();

        month++;
        if (month == 13) {
            deposit_interest();
            year++;
            month = 1;
        }
    }
}


int main() {
    alice_init();
    bob_init();

    simulation();

    persons_print();
}
