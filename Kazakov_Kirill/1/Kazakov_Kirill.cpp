#include <iostream>


/* рассмотрим случай, когда иван и степан живут в городе смоленск.
иван берет ипотеку по условию: покупка вторичного жилья, стоимость 3,3млн
срок выплаты 10 лет, базовая ставка 23,3% первоначальный взнос 1млн,
ежемесячный платеж 49752руб.
степан же живет в съемном жилье и снимает квартиру за 30000, а свой 1млн
он кладет на накопительный счет с 10 простыми процентами годовыми .
остальные раходы у них одинаковые, посмотрим, на их состояние через 10 лет */


typedef int Ruble;

int apartment_cost = 3300000;
int year = 2025;
int month = 6;
float deposit_percent = 0.1; // процент по вкладу Степана в месяц
float inflation = 0.06;
float inflation_multiplier = 1.0; // коэффициент роста инфляции 

struct Name {
    Ruble bank_account_transaction;
    Ruble bank_account_deposit;
    Ruble bank_account;
    Ruble food;
    Ruble transport;
    Ruble other_expenses;
    Ruble unexpected_expenses;
    Ruble rent;
    Ruble mortgage;
    Ruble salary;
    Ruble all_expenses;
    Ruble last_salary;  // зарплата степы перед увольнением
};


Name Ivan, Stepan;

//  сперва рассмотрим функции, описывающие жизнь Ивана, потом Степана, а после сравним их 

void Ivan_apartment_cost()
{
    apartment_cost *= inflation_multiplier;
}


void Ivan_salary()
{
    if (month == 10)
    {
        Ivan.salary *= 1.05;
    }

    Ivan.bank_account += Ivan.salary;
}


void Ivan_start()
{
    Ivan.bank_account = 0;
    Ivan.food = 25000;
    Ivan.transport = 1000;
    Ivan.other_expenses = 5000;
    Ivan.unexpected_expenses = 7000;
    Ivan.mortgage = 49752;
    Ivan.salary = 100000;
    Ivan.bank_account_deposit = 0;
    Ivan.bank_account_transaction = 0;
}


void Ivan_deposit()
{
    Ivan.bank_account_deposit += Ivan.bank_account_deposit * deposit_percent / 12;

    if (Ivan.bank_account > 50000)
    {
        Ivan.bank_account_transaction = Ivan.bank_account - 50000;
        Ivan.bank_account_deposit += Ivan.bank_account_transaction;
        Ivan.bank_account -= Ivan.bank_account_transaction;
    }
    
    deposit_percent = inflation + 0.05;
}


void Ivan_mortgage()
{
    Ivan.bank_account -= Ivan.mortgage;
}


void Ivan_food()
{
    Ivan.bank_account -= Ivan.food * inflation_multiplier;
}


void Ivan_transport()
{
    Ivan.bank_account -= Ivan.transport * inflation_multiplier;
}


void Ivan_unexpected_expenses()
{
    Ivan.bank_account -= Ivan.unexpected_expenses * inflation_multiplier;
}


void Ivan_other_expenses()
{
    Ivan.bank_account -= Ivan.other_expenses * inflation_multiplier;
}


void inflation_growth()
{

    if (month == 4)
    {
        inflation += 0.002;
    }

    inflation_multiplier = 1 + inflation / 12;
}


void Ivan_action()
{

    while (!(year == 2035 && month == 10))
    {

        Ivan_salary();
        Ivan_mortgage();
        Ivan_food();
        Ivan_transport();
        Ivan_unexpected_expenses();
        Ivan_other_expenses();
        Ivan_deposit();

        Ivan_apartment_cost();

        inflation_growth();

        month++;
        if (month == 13)
        {
            month = 1;
            year++;
        }

    }
}


void Ivan_print_results()
{
    printf("Ivan capital = %d\n\n", Ivan.bank_account_deposit);
}


// История Степана


void Stepan_salary()
{
    if (year == 2030 && month == 3)
    {
        Stepan.last_salary = Stepan.salary;
        Stepan.salary = 0;
    }

    if (year == 2030 && month == 9)
    {
        Stepan.salary = Stepan.last_salary;
        Stepan.salary *= 1.5;
    }

    if (month == 10)
    {
        Stepan.salary *= 1.05;
    }

    Stepan.bank_account += Stepan.salary;

}


void Stepan_rent()
{

    if (month == 1)
    {
        Stepan.rent += inflation_multiplier;
    }

    Stepan.bank_account -= Stepan.rent;
}


void Stepan_food()
{
    Stepan.bank_account -= Stepan.food * inflation_multiplier;
}


void Stepan_transport()
{
    Stepan.bank_account -= Stepan.transport * inflation_multiplier;
}


void Stepan_unexpected_expenses()
{
    Stepan.bank_account -= Stepan.unexpected_expenses * inflation_multiplier;
}


void Stepan_other_expenses()
{
    Stepan.bank_account -= Stepan.other_expenses * inflation_multiplier;
}


void Stepan_start()
{
    Stepan.bank_account = 0;
    Stepan.food = 25000;
    Stepan.transport = 1000;
    Stepan.other_expenses = 5000;
    Stepan.unexpected_expenses = 7000;
    Stepan.rent = 30000;
    Stepan.salary = 100000;
    Stepan.bank_account_deposit = 1000000;
    Stepan.bank_account_transaction = 0;
    Stepan.last_salary = 0;
}


void Stepan_deposit_profit()
{

    if (Stepan.bank_account > 50000)
    {
        Stepan.bank_account_transaction = Stepan.bank_account - 50000;
        Stepan.bank_account_deposit += Stepan.bank_account_transaction;
        Stepan.bank_account -= Stepan.bank_account_transaction;
    }

    Stepan.bank_account_deposit += Stepan.bank_account_deposit * deposit_percent / 12;
    deposit_percent = inflation + 0.05;
}


void Stepan_action()
{

    while (!(year == 2035 && month == 10))
    {
        inflation_growth();

        Stepan_salary();
        Stepan_deposit_profit();
        Stepan_rent();
        Stepan_food();
        Stepan_transport();
        Stepan_unexpected_expenses();
        Stepan_other_expenses();

        month++;
        if (month == 13)
        {
            month = 1;
            year++;
        }

    }
}


void Stepan_print_results()
{
    printf("Stepan capital = %d\n\n", Stepan.bank_account_deposit);
}


void Date_reset()
{
    year = 2025;  // обнуляем год, инфляцию и месяц после расчета Ивана
    month = 6;
    inflation = 0.06;
    deposit_percent = 0.1;
}


void print_conclusion()
{
    printf("P.s. As a result, Stepan earned %d more than Ivan and the house of Ivan"
        " costs %d\n\n", 
        Stepan.bank_account_deposit - Ivan.bank_account_deposit,apartment_cost);
    
    if (Stepan.bank_account_deposit - Ivan.bank_account_deposit - apartment_cost > 0)
    {
        printf("Stepan has a great profit!!!!\n");
    }

    else
    {
        printf("Stepan made a mistake ((((((");
    }

}


int main()
{
    Ivan_start();
    Ivan_action();
    Ivan_print_results();

    Date_reset();

    Stepan_start();
    Stepan_action();
    Stepan_print_results();

    print_conclusion();
}
