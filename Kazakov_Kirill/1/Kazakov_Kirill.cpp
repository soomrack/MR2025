#include <iostream>


/* рассмотрим случай, когда иван и степан живут в городе смоленск.
иван берет ипотеку по условию: покупка вторичного жилья, стоимость 3,3млн
срок выплаты 10 лет, базовая ставка 23,3% первоначальный взнос 1млн,
ежемесячный платеж 49752руб.
степан же живет в съемном жилье и снимает квартиру за 30000, а свой 1млн
он кладет на накопительный счет с 10 простыми процентами годовыми .
остальные раходы у них одинаковые, посмотрим, на их состояние через 10 лет */


typedef int Ruble;

int year = 2025;
int month = 6;
const float deposit_percent = 0.1; // процент по вкладу Степана
float inflation = 0.06;
float inflation_multiplier = 1.0; // коэффициент роста инфляции 

struct Name {
    Ruble bank_account;
    Ruble capital;
    Ruble food;
    Ruble transport;
    Ruble other_expenses;
    Ruble unexpected_expenses;
    Ruble rent;
    Ruble mortgage;
    Ruble salary;
    Ruble all_expenses;
};


Name Ivan, Stepan;

//  сперва рассмотрим функции, описывающие жизнь Ивана, потом Степана, а после сравним их 


void Ivan_salary()
{
    if (month == 10)
    {
        Ivan.salary *= 1.05;
    }

    Ivan.capital += Ivan.salary;
}


void Ivan_start()
{
    Ivan.capital = 0;
    Ivan.food = 25000;
    Ivan.transport = 1000;
    Ivan.other_expenses = 5000;
    Ivan.unexpected_expenses = 7000;
    Ivan.mortgage = 49752;
    Ivan.salary = 100000;
}


void Ivan_mortgage()
{
    Ivan.capital -= Ivan.mortgage;
}


void Ivan_food()
{
    Ivan.capital -= Ivan.food * inflation_multiplier;
}


void Ivan_transport()
{
    Ivan.capital -= Ivan.transport * inflation_multiplier;
}


void Ivan_unexpected_expenses()
{
    Ivan.capital -= Ivan.unexpected_expenses * inflation_multiplier;
}


void Ivan_other_expenses()
{
    Ivan.capital -= Ivan.other_expenses * inflation_multiplier;
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

        inflation_growth();

        month++;

        if (month == 13)
        {
            month = 1;
            year++;
        }

    }
}


void Ivan_show_results()
{
    printf("Ivan capital = %d\n\n", Ivan.capital);
}


// История Степана


void Stepan_salary()
{
    if (month == 10)
    {
        Stepan.salary *= 1.05;
    }

    Stepan.capital += Stepan.salary;
}


void Stepan_rent()
{
    Stepan.capital -= Stepan.rent * inflation_multiplier;
}


void Stepan_food()
{
    Stepan.capital -= Stepan.food * inflation_multiplier;
}


void Stepan_transport()
{
    Stepan.capital -= Stepan.transport * inflation_multiplier;
}


void Stepan_unexpected_expenses()
{
    Stepan.capital -= Stepan.unexpected_expenses * inflation_multiplier;
}


void Stepan_other_expenses()
{
    Stepan.capital -= Stepan.other_expenses * inflation_multiplier;
}


void Stepan_start()
{
    Stepan.capital = 0;
    Stepan.food = 25000;
    Stepan.transport = 1000;
    Stepan.other_expenses = 5000;
    Stepan.unexpected_expenses = 7000;
    Stepan.rent = 30000;
    Stepan.salary = 100000;
    Stepan.bank_account = 1000000;
}


void Stepan_deposit_profit()
{
    Stepan.capital += Stepan.bank_account * deposit_percent / 12;
}


void Stepan_action()
{
    while (!(year == 2035 && month == 10))
    {
        Stepan_salary();

        Stepan_deposit_profit();

        Stepan_rent();

        Stepan_food();

        Stepan_transport();

        Stepan_unexpected_expenses();

        Stepan_other_expenses();

        inflation_growth();

        month++;

        if (month == 13)
        {
            month = 1;
            year++;
        }

    }
}


void Stepan_show_results()
{
    printf("Stepan capital = %d\n\n", Stepan.capital);
}


void Data_reset()
{
    year = 2025;  // обнуляем год, инфляцию и месяц после расчета Ивана
    month = 6;
    inflation = 0.06;
}


void conclusion()
{
    printf("P.s. As a result, Stepan earned %d more than Ivan, but at the\n\n" 
        "same time, Ivan will own an apartment that may cost more\n\n"
        "than 4 million by 2035 (it is difficult to estimate the growth\n\n"
        "of real estate in Russia over such a long period of time)\n\n",
        Stepan.capital - Ivan.capital);
}


int main()
{
    Ivan_start();

    Ivan_action();

    Ivan_show_results();

    Data_reset();

    Stepan_start();

    Stepan_action();

    Stepan_show_results();

    conclusion();
}

