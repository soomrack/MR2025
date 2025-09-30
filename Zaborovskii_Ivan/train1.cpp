#include <iostream>
#include <math.h>

typedef int RUB;
typedef int USDT;

struct Person {
    RUB bank_account;
    RUB income;
    RUB food;
    RUB car;
    RUB clothes;
    RUB apartment_price;
    RUB monthly_pass;
    RUB mortgage_payment;
    RUB down_payment;
    RUB monthly_payment;
    RUB rent;
    int owner; // уже есть квартира или нет
};

struct Person alice;
struct Person bob;


void alice_init() {
    const int mortage_term = 20; // срок ипотеки
    const double mortage_rate = 0.217; // процентная ставка
    

    alice.bank_account = 1500*1000;
    alice.income = 160*1000;
    alice.food = 30*1000;
    alice.clothes = 16 * 1000;
    alice.monthly_pass = 3307; // проездной в метро на месяц
    alice.car = 9000; 
    alice.apartment_price = 7 * 1000 * 1000; 
    alice.down_payment = 1407 * 1000; // первоначальный взнос
    
    alice.monthly_payment = 103.08 * 1000; // ежемесячный платеж

    alice.bank_account -= alice.down_payment; 
    alice.owner = 1;
    

}

void bob_init() {
    
    

    bob.bank_account = 1500*1000;
    bob.income = 160*1000;
    bob.food = 30*1000;
    bob.clothes = 16 * 1000;
    bob.monthly_pass = 3307; // проездной в метро на месяц
    bob.car = 9000; 
    bob.apartment_price = 7 * 1000 * 1000; 
    bob.rent = 40 * 1000; // первоначальный взнос
    bob.owner = 0;
    

}

void inflation(const int year, const int month)
{
    if (month==1) {
        const double inflation_rate = 0.068;

        bob.apartment_price *= 1. + inflation_rate;
        bob.rent *= 1. + inflation_rate;
        bob.food *= 1. + inflation_rate;
        alice.food *= 1. + inflation_rate;
        bob.clothes *= 1. + inflation_rate;
        alice.clothes *= 1. + inflation_rate;
        bob.car *= 1. + inflation_rate;
        alice.car *= 1. + inflation_rate;
        bob.monthly_pass *= 1. + inflation_rate;
        alice.monthly_pass *= 1. + inflation_rate;
        
    }
}

void alice_food()
{
    alice.bank_account -= alice.food;
}

void alice_clothes()
{
    alice.bank_account -= alice.clothes;
}

void alice_car()
{
    alice.bank_account -= alice.car;
}

void alice_monthly_payment()
{
    alice.bank_account -= alice.monthly_payment;
}

void alice_monthly_pass()
{
    alice.bank_account -= alice.monthly_pass;
}

void alice_income(const int year, const int month)
 {
        if ( month == 10) {
            alice.income *= 1.07; //индексирование зп
        }
        
        if (year==2030 && month ==3) {
            alice.income*=1.5; //повышение
        }
        alice.bank_account += alice.income;

        
        
}

void bob_income(const int year, const int month)
 {
        if ( month == 10) {
            bob.income *= 1.07; //индексирование зп
        }
        
        if (year==2030 && month ==3) {
            bob.income*=1.5; //повышение
        }
        bob.bank_account += bob.income;

        
        
}

void bob_food()
{
    bob.bank_account -= bob.food;
}

void bob_clothes()
{
    bob.bank_account -= bob.clothes;
}

void bob_car()
{
    bob.bank_account -= bob.car;
}

void bob_rent()
{
    bob.bank_account -= bob.rent;
}

void bob_monthly_pass()
{
    bob.bank_account -= bob.monthly_pass;
}

void bob_dream_house (const int year, const int month) 
{
    if (!bob.owner && bob.bank_account >= bob.apartment_price) {
        bob.bank_account -= bob.apartment_price;
        bob.owner = 1;
        printf (" Боб купил квартиру в %d году, в %d-м месяце за %d рублей\n", year, month, bob.apartment_price); 
    }
}

void simulation()
{
    int year = 2025;
    int month = 9;

    while ( !(year == 2045 && month == 9) ) {

       inflation(year,month); 
       alice_income(year,month);
       alice_food();
       alice_clothes();
       alice_car();
       alice_monthly_payment();
       alice_monthly_pass();

        bob_income(year,month);
       bob_food();
       bob_clothes();
       bob_car();
       bob_rent();
       bob_monthly_pass();
       bob_dream_house(year,month);

        month++;
        if (month == 13) 
        {
            month =1;
            ++year;
        } 
    }
}

void print_alice_info() 
{
    printf("Alice capital = %d\n", alice.bank_account);
    printf("Alice owner = %d\n", alice.owner);
}

void print_bob_info() 
{
    printf("Bob capital = %d\n", bob.bank_account);
    printf("Bob owner = %d\n", bob.owner);

}

int main()
{
    bob_init();
    alice_init();
    simulation();
    print_alice_info();
    print_bob_info();
}