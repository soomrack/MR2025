#include <stdio.h>
#include <math.h>

typedef long int RUB;

struct Person {
    RUB balance;
    RUB income;
    RUB foodcost;
    RUB car;
    RUB mortgage_payment;
    RUB total_paid;
    RUB remaining_debt;
};

struct Person alice;


void alice_income(const int year, const int month)
{       if (month == 10) {
        alice.income = alice.income * 1.07;} // Indexation 

    if (year == 2030 && month == 3) {
        alice.income *= 1.5;  // Promotion
    }

    alice.balance += alice.income;
}

void alice_foodcost() //Затраты на продукты
{
    alice.balance -= alice.foodcost;
}

void alice_car() //Затраты на обслуживание машины
{
    alice.balance -= alice.car;
}

void alice_mortgage(const int year, const int month)
{
    // Выплата ипотеки в начале месяца
    if (alice.balance >= alice.mortgage_payment) {
        alice.balance -= alice.mortgage_payment;
        alice.total_paid += alice.mortgage_payment;
        alice.remaining_debt -= alice.mortgage_payment;

        // Ежегодная индексация платежа на 7% (в январе каждого года)
        if (month == 1) {
            alice.mortgage_payment = alice.mortgage_payment * 1.07;
        }
    }
}

void simulation()
{
    int year = 2025;
    int month = 9;

    while (!(year == 2045 && month == 9)) {
        alice_income(year, month);
        alice_mortgage(year, month);
        alice_foodcost();
        alice_car();

        ++month;
        if (month == 13) {
            month = 1;
            ++year;
        }
    }
}


void print_alice_info()
{
    printf("Alice capital = %d RUR\n", alice.balance);
    printf("Total mortgage paid = %ld RUR\n", alice.total_paid);
    printf("Remaining debt = %ld RUR\n", alice.remaining_debt);
}

void alice_init()
{
    RUB apartment_price = 9000000;
    RUB initial_payment = 100000;
    RUB mortgage_amount = apartment_price - initial_payment;

    // Расчет ежемесячного платежа (аннуитетный)
    // Ставка 8% годовых, срок 20 лет = 240 месяцев
    double annual_rate = 0.08;
    double monthly_rate = annual_rate / 12;
    int months = 240;

    double annuity_coef = (monthly_rate * pow(1 + monthly_rate, months)) /
        (pow(1 + monthly_rate, months) - 1);
    RUB monthly_payment = (RUB)round(mortgage_amount * annuity_coef); //Округлим

    alice.balance = 1000 * 1000 - initial_payment; // После внесения первоначального взноса
    alice.income = 200 * 1000;
    alice.foodcost = 30000;
    alice.car = 40000;
    alice.mortgage_payment = monthly_payment;
    alice.total_paid = 0;
    alice.remaining_debt = mortgage_amount;
}

int main()
{
    alice_init();
    simulation();
    print_alice_info();
}

