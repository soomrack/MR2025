#include <stdio.h>
#include <cmath>


typedef long int RUB;
typedef int USDT;


struct Person {
    RUB bank_account;
    RUB income;
    RUB expenses;
    RUB debt;
    RUB annuity_payment;
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


void alice_expenses(const int year, const int month)
{
    // coveres common expenses, such as car, trip, food, etc
    alice.bank_account -= alice.expenses;

    if(month == 9) {
        alice.expenses = alice.expenses * 1.07;  // Inflation
    }
}

void alice_mortgage(const int year, const int month)
{
    const float interest_rate = 0.10;
    if ( year == 2025 && month == 9) {
        const RUB flat_price = 20 * 1000 * 1000;
        const RUB first_payment = (RUB) flat_price * 0.04;
        // first payment
        alice.bank_account -= first_payment;
        alice.debt = flat_price - first_payment;

        const float monthly_interest_rate = interest_rate / 12;
        alice.annuity_payment = (RUB) round ( 
            alice.debt * (interest_rate * pow(1+monthly_interest_rate, 20*12) ) / ( pow(1+monthly_interest_rate, 20*12) - 1 ) 
        );
    }
    if (month == 8) {
        // annuity mortgage payment
        if (alice.debt > 0) {
            alice.bank_account -= alice.annuity_payment;
            alice.debt -= alice.annuity_payment;
        }
        if (alice.debt < 100) {
            // If debt is small, pay the rest of it.
            // If Alice paid too much (debt < 0), bank would return it.
            alice.bank_account -= alice.debt;
            alice.debt = 0;
        }
    }
    if (month == 9 && year != 2025 && alice.debt != 0) {
        alice.debt = (RUB) round ( 
            alice.debt * (1 + interest_rate)
        );
    }
}


void print_alice_info()
{
    printf("Alice capital = %d RUB\n", alice.bank_account);
}


void print_alice_info_more(const int year)
{
    printf("%d: Alice capital = %d RUB, debt %d RUB\n", year, alice.bank_account, alice.debt);
}


void simulation()
{
    int year = 2025;
    int month = 9;

    print_alice_info_more(year);
    while( !(year == 2045 && month == 9) ) {
        alice_income(year, month);
        alice_expenses(year, month);
        alice_mortgage(year, month);
        
        ++month;
        if(month == 13) {
            print_alice_info_more(year);
            month = 1;
            ++year;
        }
    }
}


void alice_init()
{
    alice.bank_account = 1000 * 1000;
    alice.income = 200 * 1000;
    alice.expenses = 40 * 1000;
    alice.debt = 0;
}


int main()
{
    alice_init();
    
    simulation();

    print_alice_info();
}
