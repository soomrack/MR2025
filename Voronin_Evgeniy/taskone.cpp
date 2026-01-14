#include <iostream>
#include <math.h>


typedef int RUB;

struct Home {
    int has_flat;
    int rent;
    RUB flat_price;
    RUB month_payment;
    RUB first_payment;

};

struct Person {
    RUB bank_account;              
    RUB zarplata;                     
    RUB food;               
    RUB car;                   
    RUB cloth;                                    
    RUB gifts;                    
    RUB medicine;                     
    RUB pet;                    
    RUB wb;                                                       
    RUB internet;
    RUB chill;

    Home home;

};

const int initial_flat_price = 15 * 1000 * 1000;  
const double mortgage_percent = 0.12;
const int mortgage_period = 20;
const double first_payment_percent = 0.2;

struct Person alice;
struct Person bob;

void alice_zarplata(const int year, const int month) {
    if (month == 11) {
        alice.zarplata = alice.zarplata * 1.1;
    }
    if (year == 2030 && month == 1) {
        alice.zarplata = alice.zarplata * 1.5;
    }
    alice.bank_account += alice.zarplata;

}

void alice_internet(const int month)
{
    if (month == 1) {
        alice.internet = alice.internet * 1.05;
    }
    alice.bank_account -= alice.internet;
}

void alice_food(const int month)
{
    if (month == 1) {
        alice.food = alice.food * 1.05;
    }
    alice.bank_account -= alice.food;
}

void alice_car(const int month)
{
    if (month == 1) {
        alice.car = alice.car * 1.05;
    }
    alice.bank_account -= alice.car;
}

void alice_cloth(const int month)
{
    if (month == 1) {
        alice.cloth = alice.cloth * 1.05;
    }
    alice.bank_account -= alice.cloth;
}

void alice_gifts(const int month)
{
    if (month == 1) {
        alice.gifts = alice.gifts * 1.05;
    }
    alice.bank_account -= alice.gifts;
}

void alice_medicine(const int month)
{
    if (month == 1) {
        alice.medicine = alice.medicine * 1.05;
    }
    alice.bank_account -= alice.medicine;
}

void alice_pet(const int month)
{
    if (month == 1) {
        alice.pet = alice.pet * 1.05;
    }
    alice.bank_account -= alice.pet;
}

void alice_wb(const int month)
{
    if (month == 1) {
        alice.wb = alice.wb * 1.05;
    }
    alice.bank_account -= alice.wb;
}

void alice_chill(const int month)
{
    if (month == 1) {
        alice.chill = alice.chill * 1.05;
    }
    if (month == 8) {
        alice.bank_account -= alice.chill;
    }
    
}

void alice_mortgage()
{
    alice.bank_account -= alice.home.month_payment;
}                        

void bob_zarplata(const int year, const int month)
{
    if (month == 11) {
        bob.zarplata = bob.zarplata * 1.1;             
    }

    if (year == 2030 && month == 1) {
        bob.zarplata *= 1.5;          
    }

    bob.bank_account += bob.zarplata;
}

void bob_food(const int month)
{
    if (month == 1) {
        bob.food = bob.food * 1.05;
    }
    bob.bank_account -= bob.food;
}

void bob_internet(const int month)
{
    if (month == 1) {
        alice.internet = bob.internet * 1.05;
    }
    bob.bank_account -= bob.internet;
}

void bob_car(const int month)
{
    if (month == 1) {
        bob.car = bob.car * 1.05;
    }
    bob.bank_account -= bob.car;
}


void bob_cloth(const int month)
{
    if (month == 1) {
        bob.cloth = bob.cloth * 1.05;
    }
    bob.bank_account -= bob.cloth;
}

void bob_gifts(const int month)
{
    if (month == 1) {
        bob.gifts = bob.gifts * 1.05;
    }
    bob.bank_account -= bob.gifts;
}

void bob_medicine(const int month)
{
    if (month == 1) {
        bob.medicine = bob.medicine * 1.05;
    }
    bob.bank_account -= bob.medicine;
}

void bob_rent(const int month)
{
    if (month == 1) {
        bob.home.rent = bob.home.rent * 1.05;
    }
    bob.bank_account -= bob.home.rent;
}

void bob_pet(const int month)
{
    if (month == 1) {
        bob.pet = bob.pet * 1.05;
    }
    bob.bank_account -= bob.pet;
}

void bob_chill(const int month)
{
    if (month == 8) {
        bob.chill = bob.chill * 1.05;
    }
    if (month == 8) {
        bob.bank_account -= bob.chill;
    }
}

void bob_buy_flat(const int year, const int month)
{                                         
    if (!bob.home.has_flat && bob.bank_account >= bob.home.flat_price * 1) {
        bob.bank_account -= bob.home.flat_price;
        bob.home.has_flat = 1;
        printf("Bob kupil kvartiru v %d godu, month %d za %d rub\n", year, month, bob.home.flat_price);
        printf("--------------------------------------------------------------------------\n");
    }

}

RUB calculate_monthly_payment(RUB indicator, double annual, int years)
{
    double monthly_percent = annual / 12;
    int total_payments = years * 12;
    double payment = indicator * monthly_percent * pow(1. + monthly_percent, total_payments) /
        (pow(1. + monthly_percent, total_payments) - 1.);

    return (RUB)payment;
}


void action()
{
    int year = 2025;
    int month = 10;
    while (!(year == 2045 && month == 10)) {
        alice_zarplata(year, month);
        alice_car(month);
        alice_cloth(month);
        alice_food(month);
        alice_gifts(month);
        alice_medicine(month);
        alice_internet(month);
        alice_pet(month);
        alice_wb(month);
        alice_chill(month);
        alice_mortgage();

        bob_zarplata(year, month);
        bob_car(month);
        bob_cloth(month);
        bob_food(month);
        bob_gifts(month);
        bob_medicine(month);
        bob_internet(month);
        bob_pet(month);
        bob_chill(month);
        bob_buy_flat(year, month);

        ++month;
        if (month == 13) {
            month = 1;
            ++year;
        }
    }
}

void show_info()
{
    printf("Alice:\n\n");
    printf("Kapital Alice na 2045 sostavlaet %d rub\n", alice.bank_account);
    printf("Est svoya kvartira: %s\n", alice.home.has_flat ? "YES" : "NO");
    if (alice.home.has_flat) {
        printf("Stoimost kvartiri %d rub\n", alice.home.flat_price);
    }
    printf("Stoimost vsego imushestva %d rub\n\n", alice.bank_account + (alice.home.has_flat ? alice.home.flat_price : 0));
    printf("--------------------------------------------------------------------------\n\n");
    printf("Bob:\n\n");
    printf("Kapital Boba na 2045 sostavlaet %d rub\n", bob.bank_account);
    printf("Est svoya kvartira: %s\n", bob.home.has_flat ? "YES" : "NO");
    if (bob.home.has_flat) {
        printf("  Stoimost kvartiri %d rub\n", bob.home.flat_price);
    }
    printf("Stoimost vsego imushestva %d rub\n", bob.bank_account + (bob.home.has_flat ? bob.home.flat_price : 0));
}

void comparesment()
{
    printf("--------------------------------------------------------------------------\n");
    printf("Sravnim:\n");
    printf("Strategiya Boba vigodnee na %d rub", (bob.bank_account + (bob.home.has_flat ? bob.home.flat_price : 0)) - (alice.bank_account + (alice.home.has_flat ? alice.home.flat_price : 0)));
}

void init_alice()
{
    alice.bank_account = 1100 * 100;
    alice.zarplata = 2100 * 100;
    alice.food = 38000;
    alice.internet = 1000;
    alice.car = 10000;
    alice.cloth = 12000;
    alice.gifts = 2000;
    alice.medicine = 3000;
    alice.pet = 1500;
    alice.wb = 10000;
    alice.chill = 150000;
    alice.home.flat_price = initial_flat_price;
    alice.home.has_flat = 1;
    alice.home.first_payment = alice.home.flat_price * first_payment_percent;

    RUB mortgage_sum = alice.home.flat_price - alice.home.first_payment;
    alice.home.month_payment = calculate_monthly_payment(mortgage_sum, mortgage_percent, mortgage_period);
}

void init_bob()
{
    bob.bank_account = 110 * 1000;
    bob.zarplata = 220 * 1000;
    bob.food = 35000;
    bob.car = 15000;
    bob.cloth = 8000;
    bob.gifts = 4000;
    bob.medicine = 3000;
    bob.internet = 1000;
    bob.pet = 1500;
    bob.chill = 140000;
    bob.home.flat_price = initial_flat_price;
    bob.home.rent = 50000;
    bob.home.has_flat = 0;
    bob.home.first_payment = 0;
}


int main()
{
    init_alice();
    init_bob();

    action();

    show_info();
    comparesment();
    return 0;
}



