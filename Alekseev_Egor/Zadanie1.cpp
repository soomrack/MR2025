#include <stdio.h>

typedef int RUB;

struct Person {
    RUB bank_account;
    RUB income;
    RUB food;
    RUB clothes;
    RUB monthly_payment;
    RUB tax;
    RUB fuel;
    RUB car_fix;
    
};

struct Person alice;


void alice_income(const int year, const int month)
{
    if(month == 9) {
        alice.income = alice.income * 1.05;  // Indexation
    }
        
    if(year == 2030 && month == 3) {
        alice.income *= 1.5;  // Promotion
    }
    
    alice.bank_account += alice.income ;
}


void alice_food()
{
    alice.bank_account -= alice.food;
}

void alice_clothes()
{
    alice.bank_account -= alice.clothes;
}

void alice_mortgage()
{
    alice.bank_account -= alice.monthly_payment;
}

void alice_tax()
{
    alice.bank_account -= alice.tax;
}

void alice_fuel()
{
    alice.bank_account -= alice.fuel;
}

void alice_car_fix(const int year, const int month)
{
    if(month == 6) {
        alice.bank_account -= alice.car_fix;  
    }
}

void simulation()
{
    int year = 2025;
    int month = 9;

    while( !(year == 2045 && month == 9) ) {
        alice_income(year, month);
        alice_food();
        alice_clothes();
        alice_mortgage();
        alice_tax();
        alice_fuel();
        alice_car_fix(year, month);
        // alice_trip();
        
        
        
        ++month;
        if(month == 13) {
            month = 1;
            ++year;
        }
    }
}


void print_alice_info()
{
    printf("Alice capital = %d RUR\n", alice.bank_account);
}


void alice_init()
{
    alice.bank_account = 1000 * 1000;
    alice.monthly_payment = 112 * 1000;
    alice.income = 200 * 1000;
    alice.food = 30000;
    alice.clothes = 1000;
    alice.tax = (20000)/12;
    alice.fuel = 10000;
    alice.car_fix = 30000;
}


int main()
{
    alice_init();
    
    simulation();

    print_alice_info();
}