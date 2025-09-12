#include <stdio.h>

typedef int RUB;
typedef int PERCENT;

PERCENT inflation = 7;

struct Person {
    RUB bank_account;
    RUB income;
    RUB food;
    RUB clothes;
    RUB monthly_payment;
    RUB tax;
    RUB fuel;
    RUB car_fix;
    RUB trip;
    RUB unexpected_costs;
    RUB insurance;
    
};

struct Person alice;


void alice_income(const int year, const int month)
{
    if(month == 9) {
        alice.income = alice.income * 1.07;  // Indexation
    }
        
    if(year == 2030 && month == 3) {
        alice.income *= 1.5;  // Promotion
    }
    
    alice.bank_account += alice.income ;
}


void alice_food()
{
    alice.bank_account -= alice.food;
    alice.food *= (1+(inflation/100)/12);
}

void alice_clothes()
{
    alice.bank_account -= alice.clothes;
    alice.clothes *= (1+(inflation/100)/12);
}

void alice_mortgage()
{
    alice.bank_account -= alice.monthly_payment;
}

void alice_tax()
{
    alice.bank_account -= alice.tax;
    alice.tax *= (1+(inflation/100)/12);
}

void alice_fuel()
{
    alice.bank_account -= alice.fuel;
    alice.fuel *= (1+(inflation/100)/12);
}

void alice_car_fix(const int month)
{
    if(month == 6) {
        alice.bank_account -= alice.car_fix;  
        alice.car_fix *= (1+(inflation/100));
    }
}

void alice_trip(const int month)
{
    if(month == 8) {
        alice.bank_account -= alice.trip;  
        alice.trip *= (1+(inflation/100));
    }
}

void alice_unexpected_costs()
{
    alice.bank_account -= alice.unexpected_costs;
    alice.unexpected_costs *= (1+(inflation/100)/12);
}

void alice_insurance(const int month)
{
    if (month == 4){
    alice.bank_account -= alice.insurance;
    alice.insurance *= (1+(inflation/100));
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
        alice_insurance(month);
        alice_car_fix(month);
        alice_trip(month);
        alice_unexpected_costs();
        
        
        ++month;
        if(month == 13) {
            month = 1;
            ++year;
        }
    }
}


void print_alice_info()
{
    printf("Alice capital = %d RUB\n", alice.bank_account);
}


void alice_init()
{
    alice.bank_account = 1000 * 1000;
    alice.monthly_payment = 125 * 1000;
    alice.income = 200 * 1000;
    alice.food = 30000;
    alice.clothes = 1000;
    alice.tax = (20000)/12;
    alice.fuel = 15000;
    alice.car_fix = 35000;
    alice.trip = 200*1000;
    alice.unexpected_costs = 10000;
    alice.insurance = 40000;
}


int main()
{
    alice_init();
    
    simulation();

    print_alice_info();
}