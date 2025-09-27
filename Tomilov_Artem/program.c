#include <stdio.h>

typedef long long int RUB;

struct Person {
    RUB money;
    RUB income;
    RUB rent;
    RUB food;
    RUB trip;
    RUB car_cost;
    RUB car_expense;
    RUB mortgage_payment;
    RUB flat_price_mortgage;
};

struct Person alice;
struct Person bob;

struct Bank {
    RUB bank_balance;
    double interest_rate;
    char * name;
};

struct Bank tinkoff = {0, 0.02, "Tinkoff"};
struct Bank sber = {0, 0.02, "Sberbank"};

void bob_init() {
    bob.money = 1000 * 1000; 
    bob.income = 199 * 1000; 
    bob.flat_price_mortgage = 25000 * 1000;
    bob.food = 25000;
    bob.trip = 150000;
    bob.mortgage_payment = 115000; 
}

void alice_init() {
    alice.money = 1000 * 1000;
    alice.income = 200 * 1000; 
    alice.rent = 75 * 1000;
    alice.food = 30000;
    alice.trip = 150 * 1000;
    alice.car_cost = 5000 * 1000; 
    alice.car_expense = 20 * 1000; 
}

/*
------------------------------------------------------------------------------------------------------
*/

void bob_print() {
    printf("============\n");
    printf("Bob money = %lld руб.\n", bob.money);
    printf("Bob Tinkoff deposit = %lld руб.\n", tinkoff.bank_balance);
    if (bob.flat_price_mortgage == 0) {
        printf("Bob paid off the mortgage. Success!\n");
    } else {
        printf("Bob hasn't paid off the mortgage yet.\n");
    }
    printf("============\n");
}

void alice_print() {
    printf("============\n");
    printf("Alice money = %lld руб.\n", alice.money);
    printf("Alice Sberbank deposit = %lld руб.\n", sber.bank_balance);
    printf("============\n");
}

/*
------------------------------------------------------------------------------------------------------
*/

void alice_income(const int year, const int month) {
    if (year == 2030 && month == 10) alice.income = (RUB)(alice.income * 1.5);
    alice.money += alice.income;
}

void alice_rent(const int year, const int month) {
    if (year > 2025 && month == 1) alice.rent = (RUB)(alice.rent * 1.05);
    alice.money -= alice.rent;
}

void alice_food(const int year, const int month) {
    if (year > 2025 && month == 1) alice.food = (RUB)(alice.food * 1.03);
    alice.money -= alice.food;
}

void alice_trip(const int year, const int month) {
    if (month == 8) alice.money -= alice.trip;
}

void alice_car(const int year, const int month) {
    if (month == 5 && year == 2034) alice.money -= alice.car_cost;
    if (year > 2034 || (year == 2034 && month > 5)) alice.money -= alice.car_expense;
}

/*
------------------------------------------------------------------------------------------------------
*/

void bob_income(const int year, const int month) {
    if (year == 2030 && month == 10) bob.income = (RUB)(bob.income * 1.5);
    bob.money += bob.income;
}

void bob_mortgage(const int year, const int month) {
    if (year >= 2025 && year <= 2045 && bob.flat_price_mortgage > 0) {
        RUB payment = bob.mortgage_payment;
        if (payment > bob.flat_price_mortgage) payment = bob.flat_price_mortgage;
        bob.flat_price_mortgage -= payment;
        bob.money -= payment;
    }
}

void bob_food(const int year, const int month) {
    if (year > 2025 && month == 1) bob.food = (RUB)(bob.food * 1.03);
    bob.money -= bob.food;
}

void bob_trip(const int year, const int month) {
    if (month == 8) bob.money -= bob.trip;
}

/*
------------------------------------------------------------------------------------------------------
*/

void bank_deposit(struct Person *p, struct Bank *b, double percent_of_money) {
    RUB amount = (RUB)(p->income * percent_of_money);
    if (p->money >= amount) {
        p->money -= amount;
        b->bank_balance += amount;
    } else {
        b->bank_balance += p->money;
        p->money = 0;
    }
}

void apply_bank_interest(struct Bank *b) {
    double monthly_rate = b->interest_rate / 12.0;
    b->bank_balance += (RUB)(b->bank_balance * monthly_rate);
}

/*
------------------------------------------------------------------------------------------------------
*/

void simulation() {
    int year = 2025;
    int month = 9;

    while (!(year == 2045 && month == 9)) {

        
        alice_income(year, month);
        alice_rent(year, month);
        alice_food(year, month);
        alice_car(year, month);
        alice_trip(year, month);

        // перевод на вклад, только если остаются деньги
        RUB min_alice = 0;
        RUB alice_deposit = (RUB)(alice.income * 0.3);
        if (alice.money - alice_deposit < min_alice) {
            if (alice.money > min_alice) {
                alice_deposit = alice.money - min_alice;
            } else {
                alice_deposit = 0;
            }
        } // не работает оно правда... грустнааа
        alice.money -= alice_deposit;
        sber.bank_balance += alice_deposit;
        apply_bank_interest(&sber);

    
        bob_income(year, month);
        bob_mortgage(year, month);
        bob_food(year, month);
        bob_trip(year, month);

        RUB min_bob = 0;
        RUB bob_deposit = (RUB)(bob.income * 0.1);
        if (bob.money - bob_deposit < min_bob) {
            if (bob.money > min_bob) {
                bob_deposit = bob.money - min_bob;
            } else {
                bob_deposit = 0;
            }
        }
        bob.money -= bob_deposit;
        tinkoff.bank_balance += bob_deposit;
        apply_bank_interest(&tinkoff);

        /* DEBUG
        if (month == 12) {
            printf("=== Year %d ===\n", year);
            printf("Alice money: %lld руб., Sber deposit: %lld руб.\n", alice.money, sber.bank_balance);
            printf("Bob money:   %lld руб., Tinkoff deposit: %lld руб., Mortgage remaining: %lld руб.\n",
                   bob.money, tinkoff.bank_balance, bob.flat_price_mortgage);
        } */

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
