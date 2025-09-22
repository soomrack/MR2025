#include <stdio.h>
#include <windows.h>

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

struct Expanses alice_expenses;

struct Person alice;


void console_init() {
    SetConsoleCP(1251);
    SetConsoleOutputCP(1251);
}


void alice_init() {
    alice.bank_account = 100 * 1000;
    alice.income = 150 * 1000;
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


RUB alice_rent() {
    return alice_expenses.rent;
}


RUB alice_HCS(Year count_years, Month month) {
    RUB expens = alice_expenses.standart_HCS;
    float seasonal_tariffs[4] = { 2, 1.2, 1, 1.5 };
    enum Seasons {JAN=1, FEB, MAR, APR, MAY, JUN, JUL, AUG, SEP, OCT, NOV, DEC};
    Seasons this_season = static_cast<Seasons>(month);
    switch (month) {
        // winter
        case 1:
        case 2:
        case 12:
            expens *= seasonal_tariffs[0];
            break;
        // spring
        case 3:
        case 4:
        case 5:
            expens *= seasonal_tariffs[1];
            break;
        // summer
        case 6:
        case 7:
        case 8:
            expens *= seasonal_tariffs[2];
            break;
        // autuMonth
        case 9:
        case 10:
        case 11:
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
        return alice.income*2;
    }
    return alice.income;
}


void simulation(Year start_year, Month start_month, Year years_to_end) {
    Year year = start_year;
    Month month = start_month;
    while (!( year == start_year+years_to_end && month == start_month )) {

        alice.bank_account += alice_income(year - start_year, month);

        alice.bank_account -= alice_food(year - start_year, month);
        alice.bank_account -= alice_dress(year - start_year, month);
        alice.bank_account -= alice_trip(year - start_year, month);
        alice.bank_account -= alice_HCS(year - start_year, month);
        alice.bank_account -= alice_rent();
        
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

    simulation(START_YEAR, START_MONTH, COUNT_YEARS);

    alice_print_status();
}
