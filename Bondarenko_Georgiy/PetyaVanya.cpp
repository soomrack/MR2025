#include <iostream>

typedef int RUB;

int year = 2025;
int month = 9;
int endyear = 2045;

float salary_indexation = 1.09;
float salary_bonus = 1.5;
float monthly_inflation = 1.01; 
float deposit_rate = 1.17;

int year_of_purchase;
int month_of_purchase;
RUB cost_of_the_apartment_to_buy;
bool apartment_bought = false;

struct Person {
    RUB bank_account;
    RUB salary;
    RUB food;
    RUB clothes;
    RUB car;
    RUB other_expenses;
    RUB rent;
    RUB mortgage;
    RUB deposit;
    RUB cost_of_the_apartment;
    RUB deposit_account;
};


struct Person Ivan;


void Ivan_salary() {
    if (month == 10)
    {
        Ivan.salary *= salary_indexation; // индексация
    }

    if (year == 2030 && month == 3) {
        Ivan.salary *= salary_bonus;  // премия
    }

    Ivan.bank_account += Ivan.salary;
}


void Ivan_food() {
    Ivan.bank_account -= Ivan.food;
    Ivan.food *= monthly_inflation;
}


void Ivan_clothes() {
    Ivan.bank_account -= Ivan.clothes;
    Ivan.clothes *= monthly_inflation;
}


void Ivan_car() {
    Ivan.bank_account -= Ivan.car;
    Ivan.car *= monthly_inflation;
}


void Ivan_mortgage() {
    Ivan.bank_account -= Ivan.mortgage;
}


void Ivan_other_expenses() {
    Ivan.bank_account -= Ivan.other_expenses;
    Ivan.other_expenses *= monthly_inflation;
}


void Ivan_simulation() {

    while (!(year == endyear && month == 10))
    {
        Ivan_salary();
        Ivan_food();
        Ivan_clothes();
        Ivan_car();
        Ivan_mortgage();
        Ivan_other_expenses();

        month++;
        if (month == 13)
        {
            month = 1;
            year++;
        }

    }

}


void Ivan_start() {
    Ivan.bank_account = 100 * 1000;
    Ivan.salary = 100 * 1000;
    Ivan.food = 20000;
    Ivan.clothes = 5000;
    Ivan.car = 15000;
    Ivan.mortgage = 45000;
    Ivan.other_expenses = 5000;
}


void Ivan_print_results() {
    printf("Ivan financial status = %d RUB\n", Ivan.bank_account);
}


struct Person Petya;


void Petya_salary() {
    if (month == 10)
    {
        Petya.salary *= salary_indexation;
    }

    if (year == 2035 && month == 7)
    {
        Petya.salary *= salary_bonus;

    }

    Petya.bank_account += Petya.salary;
}


void Petya_food() {
    Petya.bank_account -= Petya.food;
    Petya.food *= monthly_inflation;
}


void Petya_clothes() {
    Petya.bank_account -= Petya.clothes;
    Petya.clothes *= monthly_inflation;
}


void Petya_car() {
    Petya.bank_account -= Petya.car;
    Petya.car *= monthly_inflation;
}

void Petya_rent() {
    if (month == 1) {
        Petya.rent *= salary_indexation; // годовая инфляция
    }

    Petya.bank_account -= Petya.rent;
}


void Petya_other_expenses() {
    Petya.bank_account -= Petya.other_expenses;
    Petya.other_expenses *= monthly_inflation;
}


void Petya_deposit() {
    if (month == 1) {
        Petya.deposit_account *= deposit_rate;
    }
}

void Petya_cost_of_the_apartment() {
    Petya.cost_of_the_apartment *= monthly_inflation;
}
void Petya_purchase() {
    if (!apartment_bought && ((Petya.cost_of_the_apartment + Petya.food + Petya.clothes + Petya.car + Petya.rent + Petya.other_expenses) < (Petya.deposit_account + Petya.bank_account))) {
        year_of_purchase = year;
        month_of_purchase = month;
        cost_of_the_apartment_to_buy = Petya.cost_of_the_apartment;
        apartment_bought = true;
    }
}

void Petya_start() {
    Petya.bank_account = 100 * 1000;
    Petya.salary = 100 * 1000;
    Petya.food = 20000;
    Petya.clothes = 5000;
    Petya.car = 15000;
    Petya.rent = 20000;
    Petya.other_expenses = 5000;
    Petya.deposit_account = 1000 * 1000;
    Petya.cost_of_the_apartment = 3.5 * 1000 * 1000;
    year = 2025;
    month = 9;

}


void Petya_simulation() {

    while (!(year == endyear && month == 10))
    {
        Petya_salary();
        Petya_food();
        Petya_clothes();
        Petya_car();
        Petya_rent();
        Petya_other_expenses();
        Petya_deposit();
        Petya_cost_of_the_apartment();
        Petya_purchase();

        month++;
        if (month == 13)
        {
            month = 1;
            year++;
        }

    }

}


void Petya_print_results() {
    printf("Petya financial status = %d RUB\n", Petya.deposit_account + Petya.bank_account);
    printf("Cost_of_the_apartment = %d RUB\n", cost_of_the_apartment_to_buy);
    printf("Petya can buy an apartment in the %d month of %d \n", month_of_purchase, year_of_purchase);
}

int main() {
    Ivan_start();
    Ivan_simulation();
    Ivan_print_results();
    Petya_start();
    Petya_simulation();
    Petya_print_results();
}