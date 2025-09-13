#include <iostream>
/* рассмотрим случай, когда иван и степан живут в городе смоленск.
иван берет ипотеку по условию: покупка вторичного жилья, стоимость 3,3млн
срок выплаты 10 лет, базовая ставка 23,3% первоначальный взнос 1млн,
ежемесячный платеж 49752руб.
степан же живет в съемном жилье и снимает квартиру за 30000, а свой 1млн
он кладет на накопительный счет с 10 простыми процентами годовыми .
остальные раходы у них одинаковые, посмотрим, на их состояние через 10 лет */
typedef long int Ruble;

int year = 2025;
int month = 6;
const float deposit_percent = 0.1; // процент по вкладу Степана
float inflation = 0.06;

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

// сперва рассмотрим функции, описывающие жизнь Ивана, потом Степана, а после сравним их 

void Ivan_salary()
{
    Ivan.capital += Ivan.salary;
}


void Ivan_start()
{
    Ivan.all_expenses = 0;
    Ivan.capital = 0;  
    Ivan.food = 25000;  
    Ivan.transport = 1000;
    Ivan.other_expenses = 5000;  
    Ivan.unexpected_expenses = 7000;  
    Ivan.mortgage = 49752;  
    Ivan.salary = 100000;  
}
Ruble Ivan_all_expenses()
{
    float inflation_multiplier = (1.0 + inflation/12);
    Ivan.all_expenses = Ivan.food * inflation_multiplier +
        Ivan.mortgage +
        Ivan.transport * inflation_multiplier +
        Ivan.unexpected_expenses * inflation_multiplier +
        Ivan.other_expenses * inflation_multiplier;
    return Ivan.all_expenses;
}

void Ivan_loss()
{

    Ivan.capital -= Ivan_all_expenses();
}
void Ivan_positive_changes()
{
    if (month == 10)
    {
        Ivan.salary *= 1.05;
    }
    if (year == 2033 && month == 2)
    {
        Ivan.capital += 1000000;// нашел пиратский клад
    }
}

void Ivan_negative_changes()
{
    if (month == 4)
    {
        inflation += 0.002;
    }
    if (year == 2030 && month == 3)
    {
        Ivan.capital -= 200000; // купил жене новый айфон    
    }
}
void Ivan_action()
{
    while (!(year == 2035 && month == 10))
    {
        Ivan_salary();
        Ivan_loss();
        Ivan_negative_changes();
        Ivan_positive_changes();
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
    printf("Ivan capital = %d\n", Ivan.capital);
}

// История Степана


void Stepan_salary()
{
    Stepan.capital += Stepan.salary;
}


void Stepan_start()
{
    Stepan.all_expenses = 0;
    Stepan.capital = 0;
    Stepan.food = 25000;
    Stepan.transport = 1000;
    Stepan.other_expenses = 5000;
    Stepan.unexpected_expenses = 7000;
    Stepan.rent = 30000;
    Stepan.salary = 100000;
    Stepan.bank_account = 1000000;
}
Ruble Stepan_all_expenses()
{
    float inflation_multiplier_2 = (1.0 + inflation/12);
    Stepan.all_expenses = Stepan.food * inflation_multiplier_2 +
        Stepan.rent * inflation_multiplier_2 +
        Stepan.transport * inflation_multiplier_2 +
        Stepan.unexpected_expenses * inflation_multiplier_2 +
        Stepan.other_expenses * inflation_multiplier_2;
    return Stepan.all_expenses;
}

void Stepan_loss()
{

    Stepan.capital -= Stepan_all_expenses();
}

void Stepan_deposit_profit()
{
    Stepan.capital += Stepan.bank_account * deposit_percent / 12;
}

void Stepan_positive_changes()
{
    if (month == 10)
    {
        Stepan.salary *= 1.05;
    }
    if (year == 2027 && month == 4)
    {
        Stepan.capital += 500000;// выиграл в туринер по поеданию бургеров
    }
}

void Stepan_negative_changes()
{
    if (month == 4)
    {
        inflation += 0.002;
    }
    if (year == 2030 && month == 3)
    {
        Stepan.capital -= 200000; // купил бездомному новый айфон    
    }
}
void Stepan_action()
{
    while (!(year == 2035 && month == 10))
    {
        Stepan_salary();
        Stepan_loss();
        Stepan_deposit_profit();
        Stepan_negative_changes();
        Stepan_positive_changes();
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
    printf("Stepan capital = %d\n", Stepan.capital);
}

void Data_reset()
{
    year = 2025; // обнуляем год, инфляцию и месяц после расчета Ивана
    month = 6;
    inflation = 0.06;
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

}
