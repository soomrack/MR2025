#include <stdio.h>


typedef int RUB;
typedef int USDT;


struct Person {
    RUB bank_account;
    RUB income;
    RUB food;
};

struct Person alice;


void alice_income(const int year, const int month)
{
    if(month == 10) {
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
}


void simulation()
{
    int year = 2025;
    int month = 9;

    while( !(year == 2045 && month == 9) ) {
        alice_income(year, month);
        alice_food();
        // alice_mortgage();
        // alice_car();
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
    printf("Alice capital = %d RUB\n", alice.bank_account);
}


void alice_init()
{
    alice.bank_account = 1000 * 1000;
    alice.income = 200 * 1000;
    alice.food = 30000;
}


int main()
{
    alice_init();
    
    simulation();

    print_alice_info();
}
