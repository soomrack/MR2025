#include <stdio.h>


typedef int RUB;
//typedef int USD; 


struct Person {
    RUB bank_account;
    RUB income;
    RUB food;
    RUB mortgage;
    RUB rent;
    RUB car;
    RUB utility_costs;
    RUB other; 
    RUB deposit;
    RUB apartment;
    RUB deposit_min; //сумма, при которой банки повышают процент по вкладу
};

struct Person alice;
struct Person bob;

//Возможно, имеет смысл отдельно создать функцию с индексацией всех переменных, 
//и создать функцию для прибавления/вычитания всех переменных.


void alice_income(const int year, const int month)
{
    if (month == 10) {
        alice.income = alice.income * 1.07;  // Indexation
    }

    if (year == 2027 && month == 3) {
        alice.income *= 1.2;  // Promotion
    }
    
    if (year == 2032 && month == 5) {
        alice.income *= 1.5;  // Promotion
    }

    alice.bank_account += alice.income;
}


void alice_food(const int month)
{
    if (month == 1) {
        alice.food *= 1.05;
    }
    
    if (month == 9) {
        alice.food *= 1.04;
    }

    alice.bank_account -= alice.food;
}


void alice_mortgage()
{
    alice.bank_account -= alice.mortgage;
}


void alice_car(const int year, const int month)
{
    if (year == 2030 and month == 2) {
        if ((alice.bank_account - (3000 * 1000)) >= 0) {
            alice.bank_account -= 3000 * 1000;
            printf("Alice bought a new car in 2030\n");
            //printf("Alice capital = %d RUB\n", alice.bank_account);
        }
        else {
            printf("Alice would not buy a new car in 2030\n");
        }
    }

    if (month == 1) {
        alice.car *= 1.05;
    }
    
    alice.bank_account -= alice.car;
}


void alice_utility_costs(const int month)
{
    if (month == 7) {
        alice.utility_costs *= 1.08;
    }

    alice.bank_account -= alice.utility_costs;
}


void alice_other(const int month)
{
    if (month == 9) {
        alice.other *= 1.11;
    }

    alice.bank_account -= alice.other;
}


void alice_apartment(const int month)
{ 
    if (month == 1) {
        alice.apartment *= 1.1;
    }
}


void alice_deposit(const int month)
{
    if (alice.bank_account >= 50 * 1000) {
        alice.deposit += (alice.bank_account - 50 * 1000);
        alice.bank_account = 50 * 1000;
    }

    if (alice.deposit >= 3 * 1000 * 1000) {
        alice.deposit *= 1. + 0.11 / 12; 
    }
    else {
        alice.deposit *= 1. + 0.1 / 12;
    }

    if (month == 1) {
        alice.deposit_min *= 1.07;
    }
}

////////////////////////////////////////////////////////////////////

void bob_income(const int year, const int month)
{
    if (month == 10) {
        bob.income = bob.income * 1.07;  // Indexation
    }

    if (year == 2027 && month == 3) {
        bob.income *= 1.2;  // Promotion
    }
    
    if (year == 2032 && month == 5) {
        bob.income *= 1.5;  // Promotion
    }

    bob.bank_account += bob.income;

}


void bob_food(const int month)
{
    if (month == 1) {
        bob.food *= 1.05;
    }
    
    if (month == 9) {
        bob.food *= 1.04;
    }

    bob.bank_account -= bob.food;
}


void bob_rent(const int month)
{
    if (month == 1) {
        bob.rent *= 1.06;
    }

    bob.bank_account -= bob.rent;
}


void bob_car(const int year, const int month)
{
    if (year == 2030 and month == 2) {
        if ((bob.bank_account - (3000 * 1000)) >= 0) {
            bob.bank_account -= 3000 * 1000;
            printf("Bob bought a new car in 2030\n\n");
            //printf("Bob capital = %d RUB\n", bob.bank_account);
        }
        else {
            printf("Bob would not buy a new car in 2030\n\n");
        }
    }

    if (month == 1) {
        bob.car *= 1.05;
    }

    bob.bank_account -= bob.car;
}


void bob_utility_costs(const int month)
{
    if (month == 7) {
        bob.utility_costs *= 1.08;
    }

    bob.bank_account -= bob.utility_costs;
}


void bob_other(const int month)
{
    if (month == 9) {
        bob.other *= 1.11;
    }

    bob.bank_account -= bob.other;
}


void bob_deposit(const int month)
{
    if (bob.bank_account >= 50 * 1000) {
        bob.deposit += (bob.bank_account - 50 * 1000);
        bob.bank_account = 50 * 1000;
    }

    if (bob.deposit >= 3 * 1000 * 1000) {
        bob.deposit *= 1. + 0.11 / 12; 
    }
    else {
        bob.deposit *= 1. + 0.1 / 12;
    }

    if (month == 1) {
        bob.deposit_min *= 1.07;
    }
}


void simulation()
{
    int year = 2025;
    int month = 9;

    while (!(year == 2045 && month == 9)) {
        alice_income(year, month);
        alice_food(month);
        alice_mortgage();
        alice_car(year, month);
        alice_utility_costs(month);
        alice_other(month);
        alice_apartment(month);
        alice_deposit(month);
        // alice_trip(); 
        
        bob_income(year, month);
        bob_food(month);
        bob_rent(month);
        bob_car(year, month);
        bob_utility_costs(month);
        bob_other(month);
        bob_deposit(month);
        // bob_trip();


        ++month;
        if (month == 13) {
            month = 1;
            ++year;
            //printf("%d\n", year);
            //printf("Alice capital = %d RUB\n", alice.bank_account);
            //printf("Bob capital = %d RUB\n", bob.bank_account);
        }
    }
}


//вывод и ввод данных

void print_info()
{
    printf("Alice capital = %d RUB\n\n", alice.bank_account + alice.apartment + alice.deposit);
    printf("Alice deposit = %d RUB\n", alice.deposit);
    printf("Alice apartment = %d RUB\n", alice.apartment);
    printf("Alice bank_account = %d RUB\n\n", alice.bank_account);

    printf("Bob capital = %d RUB\n\n", bob.bank_account + bob.deposit);
    printf("Bob deposit = %d RUB\n", bob.deposit);
    printf("Bob bank_account = %d RUB\n\n", bob.bank_account);
}


void alice_init()
{
    alice.bank_account = 1000 * 1000;
    alice.income = 200 * 1000;
    alice.food = 40000;
    alice.mortgage = 80000;
    alice.car = 15000;
    alice.utility_costs = 5000; // коммунальные расходы
    alice.other = 20000; 
    alice.apartment = 14 * 1000 * 1000;
    alice.deposit = 0;
    alice.deposit_min = 3 * 1000 * 1000;
}


void bob_init()
{
    bob.bank_account = 1000 * 1000;
    bob.income = 200 * 1000;
    bob.food = 40000;
    bob.rent = 40000;
    bob.car = 15000;
    bob.utility_costs = 5000;
    bob.other = 20000;
    bob.deposit = 0;
    alice.deposit_min = 3 * 1000 * 1000;
}


int main()
{
    alice_init();
    bob_init();;

    simulation();

    print_info();
}
