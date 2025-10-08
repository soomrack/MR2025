#include <stdio.h>

typedef long long int RUB;

struct Mortgage {
    RUB sum;
    RUB first_pay;
    RUB monthly_payment;
};

struct Person {
    RUB salary;
    RUB account;
    RUB wastes;
    RUB renta;
    RUB deposit;
};


struct Person bob;
struct Person alice;
struct Mortgage alice_mortgage;

void alice_inite() 
{
    alice.account = 1000 * 1000;
    alice.salary = 300 * 1000;
    alice.wastes = 30 * 1000;
    alice.deposit = 0;


    alice_mortgage.sum = 14 * 1000 * 1000;
    alice_mortgage.first_pay = 1000 * 1000;
    alice_mortgage.monthly_payment = 217232;
    alice.account -= alice_mortgage.first_pay;
}


void alice_salary(int month) 
{
    if (month == 1) {
        alice.salary *= 1.07;
    }
    alice.account += alice.salary;
}


void alice_print() 
{
    printf("Alice = %lld\n", alice.deposit);
}


void alice__mortgage() 
{
    alice.account -= alice_mortgage.monthly_payment;
}


void alice_wastes() 
{
    alice.account -= alice.wastes;
}


void alice_deposit()
{
    alice.deposit *= (20.0 / 12.) / 100.;
    alice.deposit += alice.account;
}

//Bob

void bob_inite() 
{
    bob.deposit = 1000 * 1000;
    bob.salary = 300 * 1000;
    bob.wastes = 50 * 1000;
    bob.renta = 60 * 1000;
} 


void bob_print() 
{
    printf("Bob= %lld\n", bob.deposit);
}


void bob_salary(int month) 
{
    if (month == 1) {
        bob.salary *= 1.07;
    }
    bob.account += bob.salary;
}


void bob_rent()
{
    bob.account -= bob.renta;
}


void bob_wastes() 
{
    bob.account -= bob.wastes;
}


void bob_deposit()
{
    bob.deposit *= (20.0 / 12.) / 100.;
    bob.deposit += bob.account;
}


void simulation()
{
    int month = 9;
    int year = 2025;

    while (!((year == 2025 + 30) && (month == 9))) {
        alice_salary(month);
        alice__mortgage();
        alice_wastes();
        alice_deposit();
       

        bob_salary(month);
        bob_rent();
        bob_wastes();
        bob_deposit();
        

        month++;
        if (month == 13) {
            month = 1;
            year++;
  
        }
    }
}


int main()
{
    alice_inite();
    bob_inite();

    simulation();

    bob_print();
    alice_print();

    return 0;
}