#include <stdio.h>
#include <cmath>



typedef int RUB;
typedef int USDT;



struct Person {
    RUB bank_account;
    RUB income;
    RUB food;
    RUB spendings;
};

struct Person alice;
struct Person bob;


void alice_income(const int year, const int month)
{
    if(month == 12) {
        alice.income = alice.income * 1.05;  // Indexation
    }
        
    if(year == 2030 && month == 3) {
        alice.income *= 1.5;  // Promotion
    }
    
    alice.bank_account += alice.income ;
}


void alice_mortgage(const RUB ap_cost)
{
    int mortgage_rate = 10;  // 10% per year
    int mortgage_term = 20;  // 20 years
    RUB mortgage = ap_cost * pow(1 + mortgage_rate / 100.0, mortgage_term) / (mortgage_term * 12); // Monthly payment
    alice.bank_account -= mortgage;
}


void alice_food(const int month)
{ if (month == 1) {
        alice.food *= 1.1;  // Annual food cost increase
    }
    alice.bank_account -= alice.food;
}


void alice_spendings(const int year, const int month)
{
  /*  if (year == 2035 && month == 6) {
        alice.bank_account -= 1500000;  // One_time car purchase
    }*/
    if (month == 1) {
        alice.spendings *= 1.07;  // Annual spendings increase
    }
    alice.bank_account -= alice.spendings;
}


void print_alice_info()
{
    printf("Alice's capital = %d RUB\n", alice.bank_account);
}


void alice_init()
{
    alice.bank_account = 1000 * 1000;
    alice.income = 200 * 1000;
    alice.food = 30000;
    alice.spendings = 20000;
}


void bob_income(const int year, const int month)
{
    if(month == 12) {
        bob.income = bob.income * 1.05;  // Indexation
    }
        
    if(year == 2030 && month == 3) {
        bob.income *= 1.5;  // Promotion
    }
    
    bob.bank_account += bob.income;
}


void bob_savings(const int month, RUB &c_ap_cost, bool &has_apartment)
{   if (month == 12) {
        bob.bank_account *= 1.05;  // Annual savings increase 
        c_ap_cost *= 1.05;  // Annual apartment cost increase
    }
    if (has_apartment == false) {
        if (bob.bank_account >= c_ap_cost + 500000) {  // Ensure some buffer after purchase
        bob.bank_account -= c_ap_cost;  // Apartment purchase
        printf("Bob bought an apartment for %d RUB\n", c_ap_cost);
        has_apartment = true;  // Status update
        }
    }
    
}


void bob_food(const int month)
{ 
    if (month == 1) {
        bob.food *= 1.1;  // Annual food cost increase
    }
    bob.bank_account -= bob.food;
}


void bob_spendings(const int year, const int month)
{
  /*  if (year == 2035 && month == 6) {
        bob.bank_account -= 1500000;  // One_time car purchase
    }*/
    if (month == 1) {
        bob.spendings *= 1.07;  // Annual spendings increase
    }
    bob.bank_account -= bob.spendings;
}



void simulation()
{
    int year = 2025;
    int month = 9;
    RUB ap_cost = 1 * pow(10, 7);  // Apartment cost
    RUB c_ap_cost = ap_cost;  // Current apartment cost
    bool has_apartment = false;  // Apartment ownership status

    while( !(year == 2045 && month == 9) ) {
        alice_income(year, month);
        alice_food(month);
        alice_spendings(year, month);
        alice_mortgage(ap_cost);
        bob_income(year, month);
        bob_food(month);
        bob_spendings(year, month);
        bob_savings(month, c_ap_cost, has_apartment);
        // alice_car();
        // alice_trip();
        
        ++month;
        if(month == 13) {
            month = 1;
            ++year;
        }
    }
}


void print_bob_info()
{
    printf("Bob's capital = %d RUB\n", bob.bank_account);
}


void bob_init()
{
    bob.bank_account = 1000 * 1000;
    bob.income = 200 * 1000;
    bob.food = 30000;
    bob.spendings = 60000;
}


int main()
{
    alice_init();
    
    bob_init();

    simulation();

    print_alice_info();

    print_bob_info();
}
