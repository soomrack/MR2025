#include<stdio.h>
#include<math.h>
#include<stdbool.h>
#include <iostream>

using namespace std;

typedef long long int RUB;

struct Cat {
    RUB food;
    bool buy_cat;
    bool cat_is_alive;
};


struct Car {
    RUB benzin;
    RUB remont;
};


struct Mortgage {
    double rate;
    RUB credit;
    RUB payment;
    RUB month_pay;
};


struct Arenda {
    RUB price;
    RUB month_pay;
};


struct Person {
    RUB salary;
    RUB account;
    RUB food;
    RUB expences;
    struct Cat cat;
    struct Mortgage mortgage;
    struct Arenda arenda;
    struct Car car;
};


struct Person alice;
struct Person bob;


void alice_mortgage()
{
    alice.account -= alice.mortgage.month_pay;
}


int calculate_credit()
{
    RUB year = 30;
    RUB month = year * 12;
    return 30 * (alice.mortgage.rate * pow(1 + alice.mortgage.rate, month))
        / (pow(1 + alice.mortgage.rate, month) - 1);
}


void alice_food(const int month, const int years)
{
    if (month == 1) {
        alice.food *= 1.1;
    }
    alice.account -= alice.food;
}


void bob_food(const int month)
{
    if (month == 1) {
        bob.food *= 1.1;
    }
    bob.account -= alice.food;
}


void alice_init()
{
    alice.account = 1000000;
    alice.salary = 200000;
    alice.food = 10000;
    alice.expences = 70000;

    alice.mortgage.payment = 1000000;
    alice.mortgage.credit = 14000000;
    alice.mortgage.rate = 0.17;
    alice.mortgage.month_pay = calculate_credit();
    alice.account -= alice.mortgage.payment;

    alice.cat.food = 5000;
    alice.cat.buy_cat = false;
    alice.cat.cat_is_alive = true;
}


void bob_init()
{
    bob.account = 1000000;
    bob.salary = 200000;
    bob.food = 15000;
    bob.expences = 50000;

    bob.arenda.price = 30000;
    bob.arenda.month_pay = 10000; //плата за воду и газ   

    bob.car.benzin = 100000;
}


void alice_buy_cat()
{
    if (alice.cat.buy_cat == false) {
        alice.account -= 30000;
        alice.cat.buy_cat = true;
    }
}



void alice_cat(const int month, const int year)
{
    if (alice.cat.cat_is_alive) {
        alice.account -= alice.cat.food;

        if ((month == 12) && (year == 2027)) {
            alice.account -= 30000;
        }
        if ((month == 3) && (year == 2035)) {
            alice.account -= 30000;
        }
        if ((month == 6) && (year == 2040)) {
            alice.cat.cat_is_alive = false;
        }
    }

}


void bob_car_obslug(const int month, const int year)
{
    if ((month == 11) && (year == 2027)) {
        bob.account -= bob.car.remont;
    }
    if ((month == 3) && (year == 2035)) {
        bob.account -= bob.car.remont;
    }
    if ((month == 6) && (year == 2043)) {
        bob.account -= bob.car.remont;
    }
}


void alice_salary(const int month, const int year)
{
    alice.account += alice.salary;
}


void bob_salary(const int month, const int year)
{
    bob.account += bob.salary;
}


void influense(int const month, int const year)
{
    if (month == 1) {
        alice.expences *= 1.07;
        bob.expences *= 1.07;
    }
}


void print_result(const int year)
{
    cout << "Alice account = " << alice.account << endl;
    cout << "Bob account =  " << bob.account << endl;
    cout << "Year " << year << endl;
    cout << "\n" << endl;
}
void alice_catp(const int month, const int year) {
    
    alice.cat.food *= 1, 03;
    
}

void simulation()
{
    int month = 1;
    int year = 2024;

    while (!((month == 1) && (year == 2025 + 30))) {

        alice_salary(month, year);
        alice_mortgage();
        alice_food(month, year);
        alice_buy_cat();
        alice_cat(month, year);
        alice_catp(month, year);

        bob_salary(month, year);
        bob_food(month);
        bob_car_obslug(month, year);

        influense(month, year);

        month++;
        if (month == 13) {
            month = 1;
            year++;
            print_result(year);
        }
    }
    
}


int main()
{
    alice_init();

    bob_init();

    simulation();

    return 1;
}
