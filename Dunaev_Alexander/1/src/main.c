#include <stdio.h>
#include <stdbool.h>

typedef long long int RUB;

struct Car {
    RUB price;
    RUB spending;
};

struct House {
    RUB price;
    RUB first_payment;
    RUB mortgage;
};

struct Person{
    RUB bank_account;
    RUB pocket_cash;
    RUB income;
    RUB bank_procent;
    RUB trip;
    RUB food;
    struct House house;
    struct Car car;
};


struct Person Alice;
struct Person Bob;


void Persons_print(){
    RUB Alice_total_bank = Alice.bank_account + Alice.pocket_cash + Alice.car.price + Alice.house.price;
    RUB Bob_total_bank = Bob.bank_account + Bob.pocket_cash + Bob.car.price;
    printf("Alice_total_bank =\t%lld RUB\n", Alice_total_bank);
    printf("Bob_total_bank =\t%lld RUB\n", Bob_total_bank);
}


void Alice_init(){
    Alice.bank_account = 2 * 1000 * 1000;
    Alice.pocket_cash = 1000;
    Alice.income = 250 * 1000;
    Alice.bank_procent = 1.17;
    Alice.trip = 20 * 1000;
    Alice.food = 20 * 1000;
    Alice.house.price = 20 * 1000 * 1000;
    Alice.house.first_payment = 2 * 1000 * 1000;
    Alice.house.mortgage = 180 * 1000;
    Alice.car.price = 3 * 1000 * 1000;
    Alice.car.spending = 20 * 1000;
}


void Bob_init(){
    Bob.bank_account = 2 * 1000 * 1000;
    Bob.pocket_cash = 1000;
    Bob.income = 250 * 1000;
    Bob.bank_procent = 1.17;
    Bob.trip = 20 * 1000;
    Bob.food = 20 * 1000;
    Bob.house.mortgage = 180 * 1000;
    Bob.car.price = 3 * 1000 * 1000;
    Bob.car.spending = 20 * 1000;
}


void Alice_income(const int year, const int month){
    if ((year == 2030) && (month == 5)){
        Alice.income *= 1.5;
    }
    Alice.pocket_cash += Alice.income;
}


void Alice_mortgage(const int year, const int month){
    static bool is_first_payment = false;
    if (!is_first_payment){
        Alice.bank_account -= Alice.house.first_payment;
        is_first_payment = !is_first_payment;
    }
    Alice.pocket_cash -= Alice.house.mortgage;
}


void Alice_savings(){
    RUB month_cash = (Alice.bank_procent / 12) * Alice.bank_account;
    Alice.bank_account += month_cash;
}


void Alice_pocket2bank(){
    Alice.bank_account += Alice.pocket_cash;
    Alice.pocket_cash = 0;
}


void Alice_car(){
    static bool is_car = false;
    Alice.pocket_cash -= Alice.car.spending;
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
    Bob.pocket_cash -= Bob.house.mortgage;
}


void Bob_savings(){
    RUB month_cash = (Bob.bank_procent / 12) * Bob.bank_account;
    Bob.bank_account += month_cash;
}


void Bob_pocket2bank(){
    Bob.bank_account += Bob.pocket_cash;
    Bob.pocket_cash = 0;
}


void Bob_car(){
    static bool is_car = false;
    Bob.pocket_cash -= Bob.car.spending;
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

        if (++month == 13){
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
