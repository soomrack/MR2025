#include <stdio.h>
#include <cmath>


typedef long int RUB;
typedef int USDT;


struct Person {
    // TODO: отдельная структура Home home, туда аренду, ипотеку и расходы. Машина, долг аналогично.
    RUB bank_account;
    RUB income;
    RUB expenses;
    RUB annuity_payment;
    RUB rent;
    RUB min_balance; // minimal balance during month for depositing
};

struct Person alice;
struct Person bob;


RUB inflated_flat_price = 20 * 1000 * 1000; // this price will later be inflated


// common functions ////////////////////////////////////////////////////////////


void deposit(Person &person /*, const int year, const int month */) {
    // assume person keeps all of money on deposit
    float interest_rate = 0.05;
    if (person.bank_account > 3 * 1000 * 1000) { // increase interest rate if person has a lot of money
        interest_rate += 0.01;
    }
    float monthly_interest_rate = pow(1+interest_rate, 1.0/12) - 1; // e.g. 5% yearly will result in 0.00407 monthly

    if (person.min_balance > 0) {
        person.bank_account += person.min_balance * monthly_interest_rate;
        // person.bank_account *= 1.0+monthly_interest_rate;
    }
}


////////////////////////////////////////////////////////////////////////////////


void alice_income(const int year, const int month)
{
    if(month == 10) {
        alice.income = alice.income * 1.02;  // Indexation
    }
        
    if(year == 2030 && month == 3) {
        alice.income *= 1.5;  // Promotion
    }
    
    alice.bank_account += alice.income ;
}


void alice_expenses(const int year, const int month)
{
    // covers common expenses, such as car, trip, food, etc
    alice.bank_account -= alice.expenses;
    alice.min_balance -= alice.expenses;

    if(month == 9) {
        alice.expenses = alice.expenses * 1.07;  // Inflation
    }

    // TODO: разделить расходы: машина (стоимость (падает), бензин, налог (ежегодно)), путешествия отдельно. В конце добавить машину к капиталу.
}


void alice_cat(const int year, const int month)
{
    if ((year == 2026 && month >= 4) || (year == 2035 && month <=2) || (2026 < year && year < 2035)) {
        const RUB cat_expenses = 5000; // TODO: инфляция
        alice.bank_account -= cat_expenses;
        alice.min_balance -= cat_expenses;
    }
}


void alice_mortgage(const int year, const int month)
{
    const float interest_rate = 0.06;
    if ( year == 2025 && month == 9) {
        const RUB flat_price = 20 * 1000 * 1000;
        const RUB first_payment = 4000 * 1000;
        // (RUB) flat_price * 0.2;
        // first payment
        alice.bank_account -= first_payment;

        const float monthly_interest_rate = interest_rate / 12;
        alice.annuity_payment = (RUB) round ( 
            // alice.debt * (monthly_interest_rate * pow(1+monthly_interest_rate, 20*12) ) / ( pow(1+monthly_interest_rate, 20*12) - 1 ) 
            (flat_price - first_payment) * monthly_interest_rate / ( 1 - pow(1+monthly_interest_rate, -20*12) )
        );
        // printf("Annuity payment %d RUB\n", alice.annuity_payment); //dbg
    }
    
    // monthly annuity mortgage payment
    alice.bank_account -= alice.annuity_payment;
    alice.min_balance -= alice.annuity_payment; // TODO: не учёл если сначала зачислили + отдельная функция на снятие средств
}


void print_alice_info()
{   
    printf("Alice capital = %d RUB\n", alice.bank_account);
}


void print_alice_info_more(const int year)
{
    printf("%d: Alice capital = %d RUB\n", year, alice.bank_account);
}


void alice_init()
{
    alice.bank_account = 5 * 1000 * 1000;
    alice.income = 140 * 1000;
    alice.expenses = 40 * 1000;
    alice.rent = 0;
}


////////////////////////////////////////////////////////////////////////////////


void bob_income(const int year, const int month)
{
    if(month == 10) {
        bob.income = bob.income * 1.02;  // Indexation
    }
        
    if(year == 2030 && month == 3) {
        bob.income *= 1.5;  // Promotion
    }
    
    bob.bank_account += bob.income ;
}


void bob_expenses(const int year, const int month)
{
    // covers common expenses, such as car, trip, food, etc
    bob.bank_account -= bob.expenses;
    bob.min_balance -= bob.expenses;

    if(month == 9) {
        bob.expenses = bob.expenses * 1.07;  // Inflation
    }
}


void bob_rent(const int year, const int month)
{
    if(month == 9) {
        bob.rent *= 1.05;  // Inflation
    }
    bob.bank_account -= bob.rent;
    bob.min_balance -= bob.rent;
}


void bob_try_buy_flat(const int year, const int month)
{
    if (bob.rent == 0) return; // already has flat
    if(month == 9) {
        inflated_flat_price *= 1.02;  // Inflation
    }
    if (bob.bank_account >= inflated_flat_price + 100*1000) {
        bob.bank_account -= inflated_flat_price;
        bob.min_balance -= inflated_flat_price;
        // printf("Bob bought flat in year %d\n", year); // dbg
        bob.rent = 0;
    }
}


void print_bob_info()
{   
    printf("Bob capital = %d RUB\n", bob.bank_account);
}


void print_bob_info_more(const int year)
{
    printf("%d: Bob capital = %d RUB\n", year, bob.bank_account);
}


void print_if_bob_no_flat()
{
    if (bob.rent != 0) printf("Bob could not buy a flat for %d RUB", inflated_flat_price);
}


void bob_init()
{
    bob.bank_account = 5 * 1000 * 1000;
    bob.income = 140 * 1000;
    bob.expenses = 40 * 1000;
    bob.rent = 70 * 1000;
}


////////////////////////////////////////////////////////////////////////////////


void simulation()
{
    int year = 2025;
    int month = 9;

    // print_alice_info_more(year);
    while( !(year == 2045 && month == 9) ) {
        alice.min_balance = alice.bank_account;
        alice_income(year, month);
        alice_expenses(year, month); // common expenses, such as car, trip, food, etc
        alice_cat(year, month);
        alice_mortgage(year, month);
        deposit(alice);

        // printf("y%d m%d bank %d debt %d\n", year, month, alice.bank_account, alice.debt); //dbg

        bob.min_balance = bob.bank_account;
        bob_income(year, month);
        bob_expenses(year, month);
        bob_rent(year, month);
        bob_try_buy_flat(year, month);
        deposit(bob);
        
        ++month;
        if(month == 13) {
            // print_alice_info_more(year);
            // print_bob_info_more(year);
            month = 1;
            ++year;
        }
    }
}


int main()
{
    alice_init();
    bob_init();
    
    simulation();

    // printf("====\n");

    print_alice_info();
    print_bob_info();
    print_if_bob_no_flat();
}
