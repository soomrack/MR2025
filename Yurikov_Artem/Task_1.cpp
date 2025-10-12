#include <stdio.h>
#include <math.h>

typedef int RUB;
typedef float PERCENT;

PERCENT inflation = 7;
PERCENT deposit_rate = 10;
PERCENT car_loss_rate = 5;

struct Person {
    RUB bank_account;
    RUB deposit;
    RUB income;
    RUB monthly_payment;
    RUB initial_capital;
    RUB rent_fee;

    RUB food;
    RUB clothes;
    RUB trip;
    RUB unexpected_costs;

    RUB fuel;
    RUB car_fix;
    RUB insurance;
    RUB tax;

    RUB flat_cost;
    RUB car_cost;
};

struct Person alice;
struct Person bob;


void alice_income(const int year, const int month)
{
    if(month == 9) {
        alice.income = alice.income * 1.07;
    }
        
    if(year == 2030 && month == 3) {
        alice.income *= 1.5;
    }
    
    alice.bank_account += alice.income;
}


void alice_food()
{
    alice.bank_account -= alice.food;
    alice.food *= (1 + (inflation / 100) / 12);
}


void alice_clothes()
{
    alice.bank_account -= alice.clothes;
    alice.clothes *= (1 + (inflation / 100) / 12);
}


void alice_mortgage()
{
    alice.bank_account -= alice.monthly_payment;
}


void alice_tax()
{
    alice.bank_account -= alice.tax;
    alice.tax *= (1 + (inflation / 100) / 12);
}


void alice_fuel()
{
    alice.bank_account -= alice.fuel;
    alice.fuel *= (1 + (inflation / 100) / 12);
}


void alice_car_fix(const int month)
{
    if(month == 3) {
        alice.bank_account -= alice.car_fix;  
        alice.car_fix *= (1 + (inflation / 100));
    }
}


void alice_trip(const int month)
{
    if(month == 6) {
        alice.bank_account -= alice.trip;  
        alice.trip *= (1 + (inflation / 100));
    }
}


void alice_unexpected_costs()
{
    alice.bank_account -= alice.unexpected_costs;
    alice.unexpected_costs *= (1 + (inflation / 100) / 12);
}


void alice_insurance(const int month)
{
    if (month == 4) {
        alice.bank_account -= alice.insurance;
        alice.insurance *= (1 + (inflation / 100));
    }
}


void alice_deposit(const int month)
{
   if (alice.bank_account > alice.initial_capital) {
        RUB extra_money = alice.bank_account - alice.initial_capital;
        alice.bank_account = alice.initial_capital;  
        alice.deposit += extra_money;  
    }

    if (month == 10) {
        alice.deposit *= (1 + deposit_rate / 100);
    }
}


void alice_flat_cost_increase(const int month)
{
    if (month == 7) {
        alice.flat_cost *= (1 + inflation / 100);
    }
}


void alice_car_cost_change(const int month)
{
    if (month == 7) {
        alice.car_cost *= (1 - car_loss_rate / 100);
        alice.car_cost *= (1 + inflation / 100);
    }
}


void alice_car(const int month)
{
    alice_car_fix(month);
    alice_insurance(month);
    alice_tax();
    alice_fuel();
    alice_car_cost_change(month);
}


void alice_flat(const int year, const int month)
{
    alice_mortgage();
    alice_flat_cost_increase(month);
}


void alice_household_expenses()
{
    alice_clothes();
    alice_food();
}


void alice_simulation()
{
    int current_year = 2025;
    int current_month = 9;

    while (!(current_year == 2045 && current_month == 9)) {
        alice_income(current_year, current_month);
        alice_car(current_month);
        alice_flat(current_year, current_month);
        alice_household_expenses();
        alice_trip(current_month);
        alice_unexpected_costs();
        alice_deposit(current_month);
     
        ++current_month;
        if (current_month == 13) {
            current_month = 1;
            ++current_year;
        }
    }
}


void bob_income(const int year, const int month)
{
    if(month == 9){
        bob.income = bob.income * 1.07;
    }
        
    if(year == 2030 && month == 3){
        bob.income *= 1.5;
    }
    
    bob.bank_account += bob.income;
}


void bob_food()
{
    bob.bank_account -= bob.food;
    bob.food *= (1 + (inflation / 100) / 12);
}


void bob_clothes()
{
    bob.bank_account -= bob.clothes;
    bob.clothes *= (1 + (inflation / 100) / 12);
}


void bob_deposit(const int month)
{
   if (bob.bank_account > bob.initial_capital) {
        RUB remaining_cash = bob.bank_account - bob.initial_capital;
        bob.bank_account = bob.initial_capital;  
        bob.deposit += remaining_cash;  
    }

    if (month == 10) {
        bob.deposit *= (1 + deposit_rate / 100);
    }
}


void bob_tax()
{
    bob.bank_account -= bob.tax;
    bob.tax *= (1 + (inflation / 100) / 12);
}


void bob_fuel()
{
    bob.bank_account -= bob.fuel;
    bob.fuel *= (1 + (inflation / 100) / 12);
}


void bob_car_fix(const int month)
{
    if(month == 3) {
        bob.bank_account -= bob.car_fix;  
        bob.car_fix *= (1 + (inflation / 100));
    }
}


void bob_trip(const int month)
{
    if(month == 6) {
        bob.bank_account -= bob.trip;  
        bob.trip *= (1 + (inflation / 100));
    }
}


void bob_unexpected_costs()
{
    bob.bank_account -= bob.unexpected_costs;
    bob.unexpected_costs *= (1 + (inflation / 100) / 12);
}


void bob_insurance(const int month)
{
    if (month == 4) {
        bob.bank_account -= bob.insurance;
        bob.insurance *= (1 + (inflation / 100));
    }
}


void bob_flat_rent(const int month)
{
    bob.bank_account -= bob.rent_fee;
    if (month == 11) {
        bob.rent_fee *= (1 + (inflation / 100));
    }
}


void bob_buying_flat(const int year, const int month)
{
    if (year == 2045 && month == 8) {
        bob.deposit -= bob.flat_cost;
    }
}


void bob_flat_cost_increase(const int month)
{
    if (month == 7) {
        bob.flat_cost *= (1 + inflation / 100);
    }
}


void bob_car_cost_change(const int month)
{
    if (month == 7) {
        bob.car_cost *= (1 - car_loss_rate / 100);
        bob.car_cost *= (1 + inflation / 100);
    }
}


void bob_car(const int month)
{
    bob_car_fix(month);
    bob_insurance(month);
    bob_tax();
    bob_fuel();
    bob_car_cost_change(month);
}


void bob_household_expenses()
{
    bob_clothes();
    bob_food();
}


void bob_flat(const int year, const int month)
{
    bob_flat_rent(month);
    bob_flat_cost_increase(month);
    bob_buying_flat(year, month);
}


void bob_simulation()
{
    int current_year = 2025;
    int current_month = 9;
   
    while( !(current_year == 2045 && current_month == 9) ) {
        bob_income(current_year, current_month);
        bob_trip(current_month);
        bob_unexpected_costs();
        bob_car(current_month);
        bob_household_expenses();
        bob_flat(current_year, current_month);

        bob_deposit(current_month);             
        
        ++current_month;
        if(current_month == 13){
            current_month = 1;
            ++current_year;
        }
    }
}


void print_alice_info()
{
    printf("Alice's funds = %d RUB\n", alice.bank_account);
    printf("Alice's deposit = %d RUB\n", alice.deposit);
    printf("Alice's flat cost = %d RUB\n", alice.flat_cost);
    printf("Alice's car cost = %d RUB\n", alice.car_cost);
    printf("\n");
}


void print_bob_info()
{
    printf("Bob's funds = %d RUB\n", bob.bank_account);
    printf("Bob's deposit = %d RUB\n", bob.deposit);
    printf("Bob's flat cost = %d RUB\n", bob.flat_cost);
    printf("Bob's car cost = %d RUB\n", bob.car_cost);
}


void alice_init()
{
    alice.bank_account = 1000 * 1000;
    alice.initial_capital = alice.bank_account;
    alice.monthly_payment = 125 * 1000;
    alice.income = 200 * 1000;
    alice.food = 30000;
    alice.clothes = 1000;
    alice.tax = 20000 / 12;
    alice.car_cost = 2500 * 1000;
    alice.fuel = 15000;
    alice.car_fix = 35000;
    alice.trip = 300 * 1000;
    alice.unexpected_costs = 10000;
    alice.insurance = 40000;
    alice.flat_cost = 8000 * 1000;
}


void bob_init()
{
    bob.bank_account = 1000 * 1000;
    bob.initial_capital = bob.bank_account;
    bob.monthly_payment = 125 * 1000;
    bob.income = 200 * 1000;
    bob.food = 35000;
    bob.clothes = 1000;
    bob.tax = 20000 / 12;
    bob.car_cost = 2000*1000;
    bob.fuel = 15000;
    bob.car_fix = 35000;
    bob.trip = 300 * 1000;
    bob.unexpected_costs = 10000;
    bob.insurance = 40000;
    bob.deposit = 1500 * 1000;
    bob.rent_fee = 80000;
    bob.flat_cost = 8000 * 1000;
}


int main()
{
    alice_init();
    alice_simulation();
    print_alice_info();

    bob_init();
    bob_simulation();
    print_bob_info();
    
    return 0;
}