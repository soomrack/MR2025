#include <stdio.h>
#include <stdbool.h>

typedef long long int RUB;

struct Person{
    RUB bank_account;
    RUB pocket_cash;
    RUB income;
    RUB first_payment;
    RUB mortgage;
    RUB house;
    RUB bank_procent;
    RUB car;
    RUB car_spending;
    RUB trip;
    RUB food;
};


struct Person Alice;
struct Person Bob;


void Persons_print(){
    printf("Alice.bank_account =\t%lld RUB\n",Alice.bank_account);
    printf("Bob.bank_account =\t%lld RUB\n",Bob.bank_account);
}


void Alice_init(){
    Alice.bank_account = 2 * 1000 * 1000;
    Alice.pocket_cash = 1000;
    Alice.income = 250 * 1000;
    Alice.house = 20 * 1000 * 1000;
    Alice.mortgage = 180 * 1000;
    Alice.first_payment = 2 * 1000 * 1000;
    Alice.bank_procent = 1.1;
    Alice.car = 2 * 1000 * 1000;
    Alice.car_spending = 20 * 1000;
    Alice.trip = 20 * 1000;
    Alice.food = 20 * 1000;
}


void Bob_init(){
    Bob.bank_account = 2 * 1000 * 1000;
    Bob.pocket_cash = 1000;
    Bob.income = 250 * 1000;
    Bob.mortgage = 50 * 1000;
    Bob.bank_procent = 1.1;
    Bob.car = 2 * 1000 * 1000;
    Bob.car_spending = 20 * 1000;
    Bob.trip = 20 * 1000;
    Bob.food = 20 * 1000;
}


void Alice_income(const int year, const int month){
    if ((year == 2030) && (month == 5)){
        Alice.income *= 1.5;
    }
    Alice.pocket_cash += Alice.income;
}


void Alice_mortgage(const int year, const int month){
    static bool is_first_payment = false;
    if(!is_first_payment){
        Alice.bank_account -= Alice.first_payment;
        is_first_payment = !is_first_payment;
    }
    Alice.pocket_cash -= Alice.mortgage;
    if ((year == 2045) && (month == 8)){
        Alice.bank_account += Alice.house;
    }
}


void Alice_savings(){
    double month_cash = (Alice.bank_account * Alice.bank_procent - Alice.bank_account) / 12;
    Alice.bank_account += month_cash;
}


void Alice_pocket2bank(){
    Alice.bank_account += Alice.pocket_cash;
    Alice.pocket_cash = 0;
}


void Alice_car(){
    static bool is_car = false;
    if(!is_car){
        Alice.bank_account += Alice.car;
        is_car = !is_car;
    }
    Alice.pocket_cash -= Alice.car_spending;
}


void Alice_food(){
    Alice.pocket_cash -= Alice.food;
}


void Alice_trip(){
    Alice.pocket_cash -= Alice.trip;
}


void Bob_income(const int year, const int month){
    if ((year == 2030) && (month == 5)){
        Bob.income *= 1.5;
    }
    Bob.pocket_cash += Bob.income;
}


void Bob_mortgage(){
    Bob.pocket_cash -= Bob.mortgage;
}


void Bob_savings(){
    double month_cash = (Bob.bank_account * Bob.bank_procent - Bob.bank_account) / 12;
    Bob.bank_account += month_cash;
}


void Bob_pocket2bank(){
    Bob.bank_account += Bob.pocket_cash;
    Bob.pocket_cash = 0;
}


void Bob_car(){
    static bool is_car = false;
    if(!is_car){
        Bob.bank_account += Bob.car;
        is_car = !is_car;
    }
    Bob.pocket_cash -= Bob.car_spending;
}


void Bob_food(){
    Bob.pocket_cash -= Bob.food;
}


void Bob_trip(){
    Bob.pocket_cash -= Bob.trip;
}


void simulation(){
    int year = 2025;
    int month = 9;
    while (!((year == 2045) && (month == 9))){
        Alice_income(year, month);
        Alice_mortgage(year, month);
        Alice_car();
        Alice_trip();
        Alice_food();
        Alice_savings();
        Alice_pocket2bank();
        
        Bob_income(year, month);
        Bob_mortgage();
        Bob_car();
        Bob_trip();
        Bob_food();
        Bob_savings();
        Bob_pocket2bank();

        if(++month == 13){
            year++;
            month = 1;
        }
    }
}

int main(){
    Alice_init();
    Bob_init();

    simulation();

    Persons_print();
}
