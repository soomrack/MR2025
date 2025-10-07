#include <iostream>

typedef int RUB;

int year = 2025;
int month = 9;
float inflation = 1.01; // за месяц


struct person
{
    RUB bank_account;
    RUB salary;
    RUB food;
    RUB clothers;
    RUB car;
    RUB other_expenses;
    RUB rent;
    RUB mortgage;
};


struct person Vlad;


void Vlad_salary()
{
    if (month == 10)
    {
        Vlad.salary *= 1.09; // indexation
    }

    if (year == 2030 && month == 3) {
        Vlad.salary *= 1.5;  // promotion
    }

    Vlad.bank_account += Vlad.salary;
}


void Vlad_food()
{
    Vlad.bank_account -= Vlad.food;
    Vlad.food *= inflation;
}


void Vlad_clothers()
{
    Vlad.bank_account -= Vlad.clothers;
    Vlad.clothers *= inflation;
}


void Vlad_car()
{
    Vlad.bank_account -= Vlad.car;
    Vlad.car *= inflation;
}


void Vlad_mortgage()
{
    Vlad.bank_account -= Vlad.mortgage;
}


void Vlad_other_expenses()
{
    Vlad.bank_account -= Vlad.other_expenses;
    Vlad.other_expenses *= inflation;
}


void Vlad_simulation()
{

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
    year = 2025;
    month = 9;

}


void Vlad_start()
{
    Vlad.bank_account = 100*1000;
    Vlad.salary = 100*1000;
    Vlad.food = 20000;
    Vlad.clothers = 5000;
    Vlad.car = 15000;
    Vlad.mortgage = 45727;
    Vlad.other_expenses = 5000;
}


void Vlad_print_results()
{
    printf("Vlad capital = %d\n\n", Vlad.bank_account);
}


struct person Slava;


void Slava_salary()
{
    if (month == 10)
    {
        Slava.salary *= 1.09; // indexation
    }

    if (year == 2030 && month == 3) {
        Slava.salary *= 1.5;  // promotion
    }

    Slava.bank_account += Slava.salary;
}


void Slava_food()
{
    Slava.bank_account -= Slava.food;
    Slava.food *= inflation;
}


void Slava_clothers()
{
    Slava.bank_account -= Slava.clothers;
    Slava.clothers *= inflation;
}


void Slava_car()
{
    Slava.bank_account -= Slava.car;
    Slava.car *= inflation;
}

void Slava_rent()
{
    if (month == 1)
    {
        Slava.rent *= 1.09; // годовая инфляция
    }

    Slava.bank_account -= Slava.rent;
}


void Slava_other_expenses()
{
    Slava.bank_account -= Slava.other_expenses;
    Slava.other_expenses *= inflation;
}


void Slava_start()
{
    Slava.bank_account = 100 * 1000;
    Slava.salary = 100 * 1000;
    Slava.food = 20000;
    Slava.clothers = 5000;
    Slava.car = 10000;
    Slava.rent = 20000;
    Slava.other_expenses = 5000;
}


void Slava_simulation()
{

    while (!(year == 2045 && month == 10))
    {
        Slava_salary();
        Slava_food();
        Slava_clothers();
        Slava_car();
        Slava_rent();
        Slava_other_expenses();

        month++;
        if (month == 13)
        {
            month = 1;
            year++;
        }

    }

}


void Slava_print_results()
{
    printf("Slava capital = %d\n\n", Slava.bank_account);
}
 
int main()
{
    Vlad_start();
    Vlad_simulation();
    Vlad_print_results();
    Slava_start();
    Slava_simulation();
    Slava_print_results();
}
