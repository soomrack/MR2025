#include <stdio.h>
#include <math.h>

typedef long int RUB;

struct Person {
    RUB account;
    RUB income;
    RUB foodcost;
    RUB car_cost;
    RUB car_expense;
    RUB trip;
    RUB apartment;
    RUB deposit;
    RUB deposit_min;
    RUB mortgage_payment; 
};

struct Person alice;
struct Person bob;


void alice_income(const int year, const int month)
{       if (month == 10) {
        alice.income = alice.income * 1.07;} // Indexation 

    if (year == 2030 && month == 3) {
        alice.income *= 1.5;  // Promotion
    }

    alice.account += alice.income;
}


void alice_foodcost(const int year, const int month) //Products spending
{
    if (year > 2025 && month == 1) {
        alice.foodcost *= 1.03;
    }
    alice.account -= alice.foodcost;
}


void alice_car(const int year, const int month) 
{
    if (month == 5 && year == 2034) { //Car cost
        alice.account -= alice.car_cost;
    }

    if (year > 2034 || (year == 2034 && month > 5)) { //Car service
        alice.account -= alice.car_expense;
    }
}


void alice_trip(const int year, const int month) { //Trips every year
    if (month == 8) {
        alice.account -= alice.trip;
    }
}


void alice_apartment(const int year, const int month)
{
    if (month == 1) {
        alice.apartment *= 1.1;
    }
}


void alice_mortgage(const int year, const int month)
{
    if (year >= 2025 && year <= 2045)
    //Mortgage payment at the beginning of the month
     {
        alice.account -= alice.mortgage_payment;
    }
}


void alice_deposit(const int month)
{
    {
        alice.deposit += (alice.account - 30 * 1000);
    }

    if (alice.deposit >= 3 * 1000 * 1000) {
        alice.deposit *= 1. + 0.11 / 12;
    }
    else {
        alice.deposit *= 1. + 0.1 / 12;
    }

    if (month == 1) {
        alice.deposit_min *= 1.07;
    }
}


void bob_income(const int year, const int month)
{
    if (month == 10) {
        bob.income = bob.income * 1.07;
    } // Indexation 

    if (year == 2030 && month == 3) {
        bob.income *= 1.5;  // Promotion
    }
    bob.account += bob.income;
}


void bob_foodcost(const int year, const int month) //Products spending
{
    if (year > 2025 && month == 1) {
        bob.foodcost *= 1.03;
    }
    bob.account -= bob.foodcost;
}


void bob_car(const int year, const int month)
{
    if (month == 5 && year == 2034) { //Car cost
        bob.account -= bob.car_cost;
    }

    if (year > 2034 || (year == 2034 && month > 5)) { //Car service
        bob.account -= bob.car_expense;
    }
}


void bob_trip(const int year, const int month) { //Trips every year
    if (month == 8) {
        bob.account -= bob.trip;
    }
}


void bob_apartment(const int year, const int month)
{
    if (month == 1) {
       bob.apartment *= 1.1;
    }
}


void bob_deposit(const int month)
{
    {
        bob.deposit += (bob.account - 30 * 1000);
    }

    if (bob.deposit >= 3 * 1000 * 1000) {
        bob.deposit *= 1. + 0.11 / 12;
    }
    else {
        bob.deposit *= 1. + 0.1 / 12;
    }

    if (month == 1) {
        bob.deposit_min *= 1.07;
    }
}


void simulation()
{
    int year = 2025;
    int month = 9;

    while (!(year == 2045 && month == 9)) {
        alice_income(year, month);
        alice_mortgage(year, month);
        alice_foodcost(year, month);
        alice_car(year, month);
        alice_trip(year,month);
        alice_apartment(year,month);
        alice_mortgage(year, month);
        alice_deposit(month);

        bob_income(year, month);
        bob_foodcost(year, month);
        bob_car(year, month);
        bob_trip(year, month);
        bob_apartment(year, month);
        bob_deposit(month);


        ++month;
        if (month == 13) {
            month = 1;
            ++year;
        }
    }
}


void print_alice_info()
{
    printf("Alice bank account = %d RUR\n", alice.account);
    printf("Alice capital = %d RUB\n", alice.account + alice.apartment + alice.deposit);
    printf("Alice deposit = %d RUB\n", alice.deposit);
    printf("Alice apartment = %d RUB\n\n", alice.apartment);
   
    printf("Bob capital = %d RUB\n", bob.account + bob.deposit);
    printf("Bob deposit = %d RUB\n", bob.deposit);
    printf("Bob bank account = %d RUB\n\n", bob.account);
}


void alice_init()
{
    RUB initial_payment = 100000;
    RUB mortgage_amount = alice.apartment - initial_payment;

    // Calculation of monthly payment (annuity)
    // Rate 8% per year, 10-year term = 120 months
    double annual_rate = 0.08;
    double monthly_rate = annual_rate / 12;
    int months = 120;

    double annuity_coef = (monthly_rate * pow(1 + monthly_rate, months)) /
        (pow(1 + monthly_rate, months) - 1);
    RUB monthly_payment = (RUB)round(mortgage_amount * annuity_coef); //Round off numbers

    alice.account = 1000 * 1000 - initial_payment; // After making the first payment
    alice.income = 200 * 1000;
    alice.apartment = 5000 * 9000;
    alice.foodcost = 30000;
    alice.car_cost = 4000*5000;
    alice.car_expense = 40*1000;
    alice.trip = 160 * 1000;
    alice.deposit = 0;
    alice.deposit_min = 3 * 1000 * 1000;
    alice.mortgage_payment = monthly_payment;
}


void bob_init()
{
    bob.account = 100 * 1000;
    bob.income = 130 * 1000;
    bob.apartment = 7000 * 5000;
    bob.foodcost = 25000;
    bob.car_cost = 6000 * 2000;
    bob.car_expense = 50 * 1000;
    bob.trip = 120 * 1000;
    bob.deposit = 0;
    bob.deposit_min = 3 * 1000 * 1000;
}


int main()
{
    alice_init();
    bob_init();
    simulation();
    print_alice_info();
}

