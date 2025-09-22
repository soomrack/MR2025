#include <stdio.h>

typedef long long int RUB;

struct Person{
    RUB bank_account;
    RUB income;
    RUB food;
    RUB rent;
    RUB car;
    RUB trip;
    RUB credit;
    RUB house;
    RUB first_payment;
};


struct Person Alice;
struct Person Bob;



void Bob_init(void){
    Bob.bank_account = 1000 * 1000;
    Bob.income = 200 * 1000;
    Bob.food = 40 * 1000;
    Bob.car = 20 * 1000;
    Bob.rent = 80 * 1000;
}


void Alice_init(void){
    Alice.bank_account = 1000 * 1000;
    Alice.income = 200 * 1000;
    Alice.food = 40 * 1000;
    Alice.car = 20 * 1000;
    Alice.credit = 90 * 1000;
    Alice.house= 10 * 1000 * 1000;
    Alice.first_payment = 1000 * 1000;
}


void Bob_print(void){
    printf("Bob.bank_account = %lld\n", Bob.bank_account);
}


void Alice_print(void){
    printf("Alice.bank_account = %lld\n", Alice.bank_account+Alice.house);
}


void Bob_income(const int year, const int month){
    if(year == 2030 && month == 10) Bob.income *= 1.5;
    Bob.bank_account += Bob.income;
}


void Alice_income(const int year, const int month){
    if(year == 2030 && month == 10) Alice.income *= 1.5;
    Alice.bank_account += Alice.income;
}


void Bob_food(void){
    Bob.bank_account -= Bob.food;
}


void Alice_food(void){
    Alice.bank_account -= Alice.food;
}


void Bob_car(void){
    Bob.bank_account -= Bob.car;
}


void Alice_car(void){
    Alice.bank_account -= Alice.car;
}


void Bob_trip(void){
    Bob.bank_account -= Bob.trip;
}


void Alice_trip(void){
    Alice.bank_account -= Alice.trip;
}

void Bob_rent(void){
    Bob.bank_account -= Bob.rent;
}


void Alice_credit(void){
    Alice.bank_account -= Alice.credit;
}


void inflation(const float percent){
    Bob.food *= percent;
    Bob.trip *= percent;
    Bob.car *= percent;
    Bob.rent *= percent;
    Bob.income *= percent;
    
    Alice.food *= percent;
    Alice.trip *= percent;
    Alice.car *= percent;
    Alice.income *= percent;
}


void simulation(void){
    int year={2025};
    int month={9};
    
    Alice.bank_account -= Alice.first_payment;
    
    while (!(year==2045 && month ==9)){
        Bob_income(year, month);
        Bob_food();
        Bob_car();
        Bob_trip();
        Bob_rent();
        
        
        Alice_income(year, month);
        Alice_food();
        Alice_car();
        Alice_trip();
        Alice_credit();
        
        
        if (++month == 13){
            inflation(1.05);
            Bob_print();
            Alice_print();
            year++;
            month = 1;
        }
    }
}

int main(void){
    Bob_init();
    Alice_init();
    
    simulation();
    
    Bob_print();
    Alice_print();
}
