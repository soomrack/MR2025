#include <stdio.h>
#include <math.h>

typedef int RUB;
typedef float PERCENT;

PERCENT inflation = 7;
PERCENT deposit_rate = 14;

struct Person {
    RUB bank_account;
    RUB deposit;
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
    RUB rent_fee;
    RUB flat_cost;
    
};

struct Person alice;
struct Person bob;


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

void alice_simulation()
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

////////////////////////////////////////////////

void bob_income(const int year, const int month)
{
    if(month == 9) {
        bob.income = bob.income * 1.07;  // Indexation
    }
        
    if(year == 2030 && month == 3) {
        bob.income *= 1.5;  // Promotion
    }
    
    bob.bank_account += bob.income ;
}


void bob_food()
{
    bob.bank_account -= bob.food;
    bob.food *= (1+(inflation/100)/12);
}

void bob_clothes()
{
    bob.bank_account -= bob.clothes;
    bob.clothes *= (1+(inflation/100)/12);
}

void bob_deposit(const int year, const int month)
{
    bob.bank_account -= bob.monthly_payment;
    bob.deposit += bob.monthly_payment;
    if (month == 7){
        bob.deposit *= (1+(deposit_rate)/100);
    }
    if (month == 8 && year == 2045){
        bob.bank_account += bob.deposit;
        bob.deposit = 0;
    }
}

void bob_tax()
{
    bob.bank_account -= bob.tax;
    bob.tax *= (1+(inflation/100)/12);
}

void bob_fuel()
{
    bob.bank_account -= bob.fuel;
    bob.fuel *= (1+(inflation/100)/12);
}

void bob_car_fix(const int month)
{
    if(month == 6) {
        bob.bank_account -= bob.car_fix;  
        bob.car_fix *= (1+(inflation/100));
    }
}

void bob_trip(const int month)
{
    if(month == 8) {
        bob.bank_account -= bob.trip;  
        bob.trip *= (1+(inflation/100));
    }
}

void bob_unexpected_costs()
{
    bob.bank_account -= bob.unexpected_costs;
    bob.unexpected_costs *= (1+(inflation/100)/12);
}

void bob_insurance(const int month)
{
    if (month == 4){
    bob.bank_account -= bob.insurance;
    bob.insurance *= (1+(inflation/100));
    }
}

void bob_flat_rent(const int month){
    bob.bank_account -= bob.rent_fee;
    if (month == 10){
        bob.rent_fee *= (1+(inflation/100));
    }
}

void bob_buying_flat(const int year, const int month){
    if (year == 2045 && month == 8)
    bob.bank_account -= bob.flat_cost * pow(1 + inflation/100, 19);  //учитывание подорожания квартиры за 20 лет
}

void bob_simulation()
{
    int year = 2025;
    int month = 9;
   
    while( !(year == 2045 && month == 9) ) {
        bob_income(year, month);
        bob_food();
        bob_clothes();
        bob_deposit(year, month);
        bob_tax();
        bob_fuel();
        bob_insurance(month);
        bob_car_fix(month);
        bob_trip(month);
        bob_unexpected_costs();
        bob_flat_rent(month);
        bob_buying_flat(year, month);
        
        
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

void print_bob_info()
{
    printf("Bob's capital = %d RUB\n", bob.bank_account);
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

void bob_init()
{
    bob.bank_account = 1000 * 1000;
    bob.monthly_payment = 125 * 1000;
    bob.income = 200 * 1000;
    bob.food = 30000;
    bob.clothes = 1000;
    bob.tax = (20000)/12;
    bob.fuel = 15000;
    bob.car_fix = 35000;
    bob.trip = 200*1000;
    bob.unexpected_costs = 10000;
    bob.insurance = 40000;
    bob.deposit = 1500 * 1000;
    bob.rent_fee = 60000;
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

   printf("Bob's flat = %.0f RUB\n", bob.flat_cost*inflation/100);
}