#include <iostream>

typedef int RUB;

int year = 2025;
int month = 9;
float inflation = 1.01; // за месяц
float deposit_rate = 1.17; // за год
int year_of_purchase;
int month_of_purchase;
int apartment_bought;

struct Person {
    RUB bank_account;
    RUB salary;
    RUB food;
    RUB clothers;
    RUB car;
    RUB other_expenses;
    RUB rent;
    RUB mortgage;
    RUB deposit;
    RUB cost_of_the_apartment;
    RUB deposit_account;
};


struct Person Vlad;


void Vlad_salary(){
    if (month == 10)
    {
        Vlad.salary *= 1.09; // индексация
    }

    if (year == 2030 && month == 3) {
        Vlad.salary *= 1.5;  // премия
    }

    Vlad.bank_account += Vlad.salary;
}


void Vlad_food(){
    Vlad.bank_account -= Vlad.food;
    Vlad.food *= inflation;
}


void Vlad_clothers(){
    Vlad.bank_account -= Vlad.clothers;
    Vlad.clothers *= inflation;
}


void Vlad_car(){
    Vlad.bank_account -= Vlad.car;
    Vlad.car *= inflation;
}


void Vlad_mortgage(){
    Vlad.bank_account -= Vlad.mortgage;
}


void Vlad_other_expenses(){
    Vlad.bank_account -= Vlad.other_expenses;
    Vlad.other_expenses *= inflation;
}


void Vlad_simulation(){

    while (!(year == 2045 && month == 10))
    {
        Vlad_salary();
        Vlad_food();
        Vlad_clothers();
        Vlad_car();
        Vlad_mortgage();
        Vlad_other_expenses();

        month++;
        if (month == 13)
        {
            month = 1;
            year++;
        }

    }

}


void Vlad_start(){
    Vlad.bank_account = 100 * 1000;
    Vlad.salary = 100 * 1000;
    Vlad.food = 20000;
    Vlad.clothers = 5000;
    Vlad.car = 15000;
    Vlad.mortgage = 45727;
    Vlad.other_expenses = 5000;
}


void Vlad_print_results(){
    printf("Vlad financial status = %d RUB\n", Vlad.bank_account);
}


struct Person Slava;


void Slava_salary(){
    if (month == 10)
    {
        Slava.salary *= 1.09;
    }

    if (year == 2035 && month == 7)
    {
        Slava.salary *= 1.5;

    }

    Slava.bank_account += Slava.salary;
}


void Slava_food(){
    Slava.bank_account -= Slava.food;
    Slava.food *= inflation;
}


void Slava_clothers(){
    Slava.bank_account -= Slava.clothers;
    Slava.clothers *= inflation;
}


void Slava_car(){
    Slava.bank_account -= Slava.car;
    Slava.car *= inflation;
}

void Slava_rent(){
    if (month == 1) {
        Slava.rent *= 1.09; // годовая инфляция
    }

    Slava.bank_account -= Slava.rent;
}


void Slava_other_expenses(){
    Slava.bank_account -= Slava.other_expenses;
    Slava.other_expenses *= inflation;
}


void Slava_deposit(){
    if (month == 1) {
        Slava.deposit_account *= deposit_rate;
    }
}

void SLava_cost_of_the_apartment(){
    Slava.cost_of_the_apartment *= inflation;
}
void SLava_purchase() {
    if (!apartment_bought && (Slava.cost_of_the_apartment < (Slava.deposit_account + Slava.bank_account))) {
        year_of_purchase = year;
        month_of_purchase = month;
        apartment_bought = 1;
    }
}

void Slava_start(){
    Slava.bank_account = 100 * 1000;
    Slava.salary = 100 * 1000;
    Slava.food = 20000;
    Slava.clothers = 5000;
    Slava.car = 15000;
    Slava.rent = 20000;
    Slava.other_expenses = 5000;
    Slava.deposit_account = 1000 * 1000;
    Slava.cost_of_the_apartment = 3.5 * 1000 * 1000;
    year = 2025;
    month = 9;

}


void Slava_simulation(){

    while (!(year == 2045 && month == 10))
    {
        Slava_salary();
        Slava_food();
        Slava_clothers();
        Slava_car();
        Slava_rent();
        Slava_other_expenses();
        Slava_deposit();
        SLava_cost_of_the_apartment();
        SLava_purchase();

        month++;
        if (month == 13)
        {
            month = 1;
            year++;
        }

    }

}


void Slava_print_results(){
    printf("Slava financial status = %d RUB\n", Slava.deposit_account + Slava.bank_account);
    printf("cost_of_the_apartment = %d RUB\n", Slava.cost_of_the_apartment);
    printf("Slava can buy an apartment in the %d month of %d \n", month_of_purchase, year_of_purchase);
}
 
int main(){
    Vlad_start();
    Vlad_simulation();
    Vlad_print_results();
    Slava_start();
    Slava_simulation();
    Slava_print_results();
}
