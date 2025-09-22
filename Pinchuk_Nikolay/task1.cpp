#include <stdio.h>
#include <windows.h>
#include <cmath>

typedef long int RUB;
typedef unsigned int Year;
typedef unsigned char Month;

// Задаваемые параметры
#define START_YEAR 2025
#define START_MONTH 9
#define COUNT_YEARS 20

struct Person {
    RUB income = 0;
    RUB bank_account = 0;
};

struct Expanses {
    RUB food = 0;
    RUB dress = 0;
    RUB trip = 0;
    RUB standart_HCS = 0;
    RUB rent = 0;
};


struct Mortgage {
    RUB amount_of_credit = 0;
    unsigned int count_of_month = 0;
    double monthly_loan_interest_rate = 0;
};


// --- Alice`s structs init ---
struct Person alice;

struct Expanses alice_expenses;

struct Mortgage alice_mortgage;


// --- Bob`s structs init ---
struct Person bob;

struct Expanses bob_expenses;


void console_init() {
    SetConsoleCP(1251);
    SetConsoleOutputCP(1251);
}

// --- Alice`s funcs ---

void alice_init() {
    alice.bank_account = 100 * 1000;
    alice.income = 150 * 1000;

    alice_expenses.food = 20 * 1000;
    alice_expenses.dress = 100 * 1000;
    alice_expenses.trip = 200 * 1000;
    alice_expenses.standart_HCS = 4000;
    alice_expenses.rent = 0;

    alice_mortgage.amount_of_credit = 10 * 1000 * 1000;
    alice_mortgage.count_of_month = 20 * 12;
    alice_mortgage.monthly_loan_interest_rate = 0.12 / 12;
}


void alice_print_status() {
    printf("Alice has %ld rubles in bank accaunt ", alice.bank_account);
    printf("with %ld rubles income.\n", alice.income);
}


RUB alice_food(const Year count_years, const Month month) {
    return alice_expenses.food;
}


RUB alice_dress(const Year count_years, const Month month) {
    RUB expens = 0;
    if (count_years % 2 == 0 && month == 8) {
        expens = alice_expenses.dress;
    }
    return expens;
}


RUB alice_trip(const Year count_years, const Month month) {
    RUB expens = 0;
    if (count_years % 4 == 0 && month == 7) {
        expens = alice_expenses.trip;
    }
    return expens;
}


RUB alice_mortgage_calc() {
    double i = alice_mortgage.monthly_loan_interest_rate;
    unsigned int n = alice_mortgage.count_of_month;

    double numerator = i * pow((1 + i), n);
    double denominator = pow((1 + i), n) - 1;

    return static_cast<RUB>(alice_mortgage.amount_of_credit * numerator / denominator);
}


RUB alice_HCS(Year count_years, Month month) {
    RUB expens = alice_expenses.standart_HCS;
    float seasonal_tariffs[4] = { 2, 1.2, 1, 1.5 };
    enum Seasons { JAN = 1, FEB, MAR, APR, MAY, JUN, JUL, AUG, SEPT, OCT, NOV, DEC };
    Seasons this_season = static_cast<Seasons>(month);

    switch (this_season) {
        // winter
    case JAN:
    case FEB:
    case DEC:
        expens *= seasonal_tariffs[0];
        break;
        // spring
    case MAR:
    case APR:
    case MAY:
        expens *= seasonal_tariffs[1];
        break;
        // summer
    case JUN:
    case JUL:
    case AUG:
        expens *= seasonal_tariffs[2];
        break;
        // autum
    case SEPT:
    case OCT:
    case NOV:
        expens *= seasonal_tariffs[3];
        break;
    }
    return expens;
}


RUB alice_income(const Year count_years, const Month month) {
    if (count_years == 5 && month == 3) {
        alice.income *= 2;
    }
    if (month == 12) {
        return alice.income * 2;
    }
    return alice.income;
}


// --- Bob`s funcs ---

void bob_init() {
    bob.bank_account = 100 * 1000;
    bob.income = 150 * 1000;

    bob_expenses.food = 20 * 1000;
    bob_expenses.dress = 100 * 1000;
    bob_expenses.trip = 200 * 1000;
    bob_expenses.standart_HCS = 4000;
    bob_expenses.rent = 45 * 1000;
}


RUB bob_rent() {
    return alice_expenses.rent;
}


void bob_print_status() {
    printf("Bob has %ld rubles in bank accaunt ", bob.bank_account);
    printf("with %ld rubles income.\n", bob.income);
}


RUB bob_dress(const Year count_years, const Month month) {
    RUB expens = 0;
    if (count_years % 2 == 0 && month == 8) {
        expens = bob_expenses.dress;
    }
    return expens;
}


RUB bob_trip(const Year count_years, const Month month) {
    RUB expens = 0;
    if (count_years % 4 == 0 && month == 7) {
        expens = bob_expenses.trip;
    }
    return expens;
}


RUB bob_food(const Year count_years, const Month month) {
    return bob_expenses.food;
}


RUB bob_HCS(Year count_years, Month month) {
    RUB expens = bob_expenses.standart_HCS;
    float seasonal_tariffs[4] = { 2, 1.2, 1, 1.5 };
    enum Seasons { JAN = 1, FEB, MAR, APR, MAY, JUN, JUL, AUG, SEPT, OCT, NOV, DEC };
    Seasons this_season = static_cast<Seasons>(month);

    switch (this_season) {
        // winter
    case JAN:
    case FEB:
    case DEC:
        expens *= seasonal_tariffs[0];
        break;
        // spring
    case MAR:
    case APR:
    case MAY:
        expens *= seasonal_tariffs[1];
        break;
        // summer
    case JUN:
    case JUL:
    case AUG:
        expens *= seasonal_tariffs[2];
        break;
        // autum
    case SEPT:
    case OCT:
    case NOV:
        expens *= seasonal_tariffs[3];
        break;
    }
    return expens;
}


RUB bob_income(const Year count_years, const Month month) {
    if (count_years == 5 && month == 3) {
        bob.income *= 2;
    }
    if (month == 12) {
        return bob.income * 2;
    }
    return bob.income;
}


void simulation(Year start_year, Month start_month, Year years_to_end) {
    Year year = start_year;
    Month month = start_month;
    while (!(year == start_year + years_to_end && month == start_month)) {

        alice.bank_account += alice_income(year - start_year, month);

        alice.bank_account -= alice_food(year - start_year, month);
        alice.bank_account -= alice_dress(year - start_year, month);
        alice.bank_account -= alice_trip(year - start_year, month);
        alice.bank_account -= alice_HCS(year - start_year, month);
        alice.bank_account -= alice_mortgage_calc();

        bob.bank_account += bob_income(year - start_year, month);

        bob.bank_account -= bob_food(year - start_year, month);
        bob.bank_account -= bob_dress(year - start_year, month);
        bob.bank_account -= bob_trip(year - start_year, month);
        bob.bank_account -= bob_HCS(year - start_year, month);
        bob.bank_account -= bob_rent();

        month++;
        if (month > 12) {
            year++;
            month = 1;
        }
    }
}


int main()
{
    console_init();

    alice_init();
    bob_init();

    simulation(START_YEAR, START_MONTH, COUNT_YEARS);

    alice_print_status();
    bob_print_status();
}
