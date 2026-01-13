#include <iostream>
#include <cmath>

typedef int RUB;

struct Person {
    RUB bank_account;
    RUB income;
    RUB car;
    RUB rent;
    RUB flat_amount;
    bool has_flat;
    
    RUB mortgage_payment;
    int mortgage_months_left;
    
    bool pays_rent;
};

struct Person alice;
struct Person bob;

RUB calculate_mortgage_payment(RUB loan_amount, float annual_rate, int years) {
    float monthly_rate = annual_rate / 12 / 100;
    int months = years * 12;
    return loan_amount * monthly_rate * pow(1 + monthly_rate, months) / (pow(1 + monthly_rate, months) - 1);
}

void alice_income(const int year, const int month)
{
    if(month == 1) {
        alice.income = alice.income * 1.01;
    }
    
    alice.bank_account += alice.income;
}

void bob_income(const int year, const int month)
{
    if(month == 1) {
        bob.income = bob.income * 1.01;
    }
    
    bob.bank_account += bob.income;
}

void alice_car()
{
    alice.bank_account -= alice.car;
}

void bob_car()
{
    bob.bank_account -= bob.car;
}

void bob_rent()
{
    if (bob.pays_rent) {
        bob.bank_account -= bob.rent;
    }
}

void alice_mortgage()
{
    if (alice.mortgage_months_left > 0) {
        alice.bank_account -= alice.mortgage_payment;
        alice.mortgage_months_left--;
        
        if (alice.mortgage_months_left == 0) {
            std::cout << "Alice выплатила ипотеку!" << std::endl;
        }
    }
}


void alice_check_buy_flat(const int year, const int month)
{
    if (!alice.has_flat) {
        RUB down_payment = alice.flat_amount * 0.2;
        if (alice.bank_account >= down_payment) {
            alice.bank_account -= down_payment;
            alice.has_flat = true;
            
            RUB mortgage_amount = alice.flat_amount - down_payment;
            alice.mortgage_payment = calculate_mortgage_payment(mortgage_amount,
                 10.0, 20);
            alice.mortgage_months_left = 20 * 12;
            
        }
    }
}

void bob_check_buy_flat(const int year, const int month)
{
    if (!bob.has_flat) {
        if (bob.bank_account >= bob.flat_amount) {
            bob.bank_account -= bob.flat_amount;
            bob.has_flat = true;
            bob.pays_rent = false;
        }
    }
}

void simulation()
{
    int year = 2025;
    int month = 1;
    
    while( !(year == 2070 && month == 1) ) {
        alice_income(year, month);
        bob_income(year, month);
        
        alice_car();
        alice_mortgage();

        bob_car();
        bob_rent();
        
        alice_check_buy_flat(year, month);
        bob_check_buy_flat(year, month);
        
        ++month;
        if(month == 13) {
            month = 1;
            ++year;
        }
    }
}

void alice_init()
{
    alice.bank_account = 0;
    alice.income = 100000;
    alice.car = 15000;
    alice.rent = 0;
    alice.has_flat = false;
    alice.mortgage_payment = 0;
    alice.mortgage_months_left = 0;
    alice.pays_rent = false;
}

void bob_init()
{
    bob.bank_account = 0;
    bob.income = 100000;
    bob.car = 15000;
    bob.rent = 30000;
    bob.has_flat = false;
    bob.mortgage_payment = 0;
    bob.mortgage_months_left = 0;
    bob.pays_rent = true;
}

int main()
{
    alice_init();
    bob_init();
    
    simulation();
    return 0;
}