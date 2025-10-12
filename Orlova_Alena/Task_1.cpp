#include <stdio.h>

typedef long long int RUB;

struct Person {
    RUB bank_account;
    RUB income;
    RUB rent_price;
    RUB food_price;
    RUB invested_money;
    RUB mortgage;
    RUB other_spendings;
    RUB flat_price;
    RUB all_spendings;
};

struct Person alice;
struct Person bob;

void alice_init() {
    alice.bank_account = 1000 * 1000;
    alice.income = 300 * 1000;
    alice.rent_price = 60 * 1000;
    alice.food_price = 25 * 1000;
    alice.other_spendings = 30 * 1000;
    alice.invested_money = alice.income - (alice.rent_price + alice.food_price + alice.other_spendings);
}

void bob_init() {
    bob.bank_account = 500 * 1000;
    bob.income = 200 * 1000;
    bob.flat_price = 20 * 1000 * 1000;
    bob.food_price = 25 * 1000;
    bob.other_spendings = 30 * 1000;
    bob.mortgage = 60 * 1000;
    bob.invested_money = bob.income - (bob.mortgage + bob.food_price + bob.other_spendings);
}

void alice_print() {
    printf("Alice bank account = %lld rub.\n", alice.bank_account);
}

void bob_print() {
    printf("Bob bank account = %lld rub.\n", bob.bank_account);
}

void bob_mortgage(const int current_year, const int current_month) {
    int start_year = 2025;
    int start_month = 9;
    int duration_years = 15;

    int years_passed = current_year - start_year;
    int total_months_passed = years_passed * 12 + (current_month - start_month);

    if (total_months_passed < duration_years * 12) {
        bob.bank_account -= bob.mortgage;
    }
}

void alice_income(const int year, const int month) {
    if (year == 2030 && month == 10) {
        alice.income *= 1.5;
    }
    alice.bank_account += alice.income;
    if (month == 12) {
        alice.bank_account += alice.income;
    }
}

void bob_income(const int year, const int month) {
    if (year == 2030 && month == 10) {
        bob.income *= 1.5;
    }
    bob.bank_account += bob.income;
}

void apply_inflation(RUB* cost, int year, int month) {
    if (month == 12) {
        *cost *= 1.08;
    }
}

void alice_rent(int year, int month) {
    alice.bank_account -= alice.rent_price;
    apply_inflation(&alice.rent_price, year, month);
}

void alice_food(int year, int month) {
    alice.bank_account -= alice.food_price;
    if (month == 5) {
        alice.bank_account -= 10000;
    }
    apply_inflation(&alice.food_price, year, month);
}

void bob_food(int year, int month) {
    bob.bank_account -= bob.food_price;
    if (month == 5) {
        bob.bank_account -= 20000;
    }
    apply_inflation(&bob.food_price, year, month);
}

void alice_deposit(int year, int month) {
    alice.bank_account += alice.invested_money * 0.18;
}

void bob_deposit(int year, int month) {
    bob.bank_account += bob.invested_money * 0.18;
}

void bob_trip(int year, int month) {
    if (month == 6) {
        bob.bank_account -= 250000;
    }

    if (year == 2035 && month == 12) {
        bob.bank_account -= 600000;
    }
}

void bob_other_spendings(int year, int month) {
    bob.bank_account -= bob.other_spendings;
    if (month == 6) {
        bob.bank_account -= 10000;
    }

    if (year == 2035 && month == 12) {
        bob.bank_account -= 250000;
    }

    if (year == 2035 && month == 12) {
        bob.bank_account -= 200000;
    }
    apply_inflation(&bob.other_spendings, year, month);
}

void alice_other_spendings(int year, int month) {
    alice.bank_account -= alice.other_spendings;
    if (month == 6) {
        alice.bank_account -= 20000;
    }

    if (year == 2035 && month == 12) {
        alice.bank_account -= 250000;
    }

    if (year == 2035 && month == 12) {
        alice.bank_account -= 200000;
    }
    apply_inflation(&alice.other_spendings, year, month);
}

void alice_trip(int year, int month) {
    if (month == 6) {
        alice.bank_account -= 250000;
    }

    if (year == 2035 && month == 12) {
        alice.bank_account -= 600000;
    }

    if (year == 2040 && month == 1) {
        alice.bank_account -= 800000;
    }
}

void bob_flat(int year, int month) {
    if (year == 2045 && month == 8)
        bob.bank_account += bob.flat_price * 2;
}

void run_simulation() {
    int current_year = 2025;
    int current_month = 9;

    while (!(current_year == 2045 && current_month == 9)) {
        bob_income(current_year, current_month);
        bob_mortgage(current_year, current_month);
        bob_food(current_year, current_month);
        bob_trip(current_year, current_month);
        bob_other_spendings(current_year, current_month);
        bob_deposit(current_year, current_month);
        bob_flat(current_year, current_month);

        alice_income(current_year, current_month);
        alice_rent(current_year, current_month);
        alice_food(current_year, current_month);
        alice_deposit(current_year, current_month);
        alice_trip(current_year, current_month);
        alice_other_spendings(current_year, current_month);

        current_month++;
        if (current_month == 13) {
            current_year++;
            current_month = 1;
        }
    }
}

int main() {
    alice_init();
    bob_init();

    run_simulation();

    alice_print();
    bob_print();

    return 0;
}
