#include <stdio.h>
#include <stdlib.h>

typedef long long int RUB;

struct Person
{
    RUB money; // просто деньги
    RUB income;
    RUB rent;
    RUB food;
    RUB trip;
    RUB car_cost;
    RUB car_expense;
    RUB mortgage_payment;
    RUB flat_price_mortgage;

    RUB pocket_money;
    RUB medical_expense;
    RUB sber_balance;
    RUB tinkoff_balance;
};

struct Person alice;
struct Person bob;

/*
------------------------------------------------------------------------------------------------------
*/

void bob_init()
{
    bob.money = 1000 * 1000; // со стпиендии отложил
    bob.income = 199 * 1000;
    bob.flat_price_mortgage = 25000 * 1000;
    bob.food = 25000;
    bob.trip = 150000;
    bob.mortgage_payment = 115000;

    bob.pocket_money = 50 * 1000;
    bob.sber_balance = 0;
    bob.tinkoff_balance = 0;
}

void alice_init()
{
    alice.money = 1000 * 1000;
    alice.income = 200 * 1000;
    alice.rent = 75 * 1000;
    alice.food = 30000;
    alice.trip = 150 * 1000;
    alice.car_cost = 5000 * 1000;
    alice.car_expense = 20 * 1000;

    alice.pocket_money = 50 * 1000;
    alice.sber_balance = 0;
    alice.tinkoff_balance = 0;
}

/*
------------------------------------------------------------------------------------------------------
*/

RUB rand_range(RUB min, RUB max)
{
    if (max <= min)
        return min;
    return min + rand() % (max - min + 1);
}

void random_pocket_spend(struct Person *p)
{
    RUB spend = rand_range(1000, 10000);
    if (spend > p->pocket_money)
        spend = p->pocket_money;
    p->pocket_money -= spend;
    p->money -= spend;
}

void random_medical_expense(struct Person *p)
{
    int chance = rand() % 100;
    if (chance < 15)
    {
        RUB med = rand_range(5000, 50000);
        if (med > p->money)
            med = p->money;
        p->money -= med;
        p->medical_expense = med;
    }
    else
    {
        p->medical_expense = 0;
    }
}

/*
------------------------------------------------------------------------------------------------------
*/

void bob_print()
{
    printf("============\n");
    printf("Bob money = %lld руб.\n", bob.money);
    printf("Bob Tinkoff deposit = %lld руб.\n", bob.tinkoff_balance);
    printf("Bob Sberbank deposit = %lld руб.\n", bob.sber_balance);
    if (bob.flat_price_mortgage == 0)
    {
        printf("Bob paid off the mortgage. Success!\n");
    }
    else
    {
        printf("Bob hasn't paid off the mortgage yet.\n");
    }
    printf("============\n");
}

void alice_print()
{
    printf("============\n");
    printf("Alice money = %lld руб.\n", alice.money);
    printf("Alice Tinkoff deposit = %lld руб.\n", alice.tinkoff_balance);
    printf("Alice Sberbank deposit = %lld руб.\n", alice.sber_balance);
    printf("============\n");
}

/*
------------------------------------------------------------------------------------------------------
*/

void alice_income(const int year, const int month)
{
    if (year == 2030 && month == 10)
        alice.income = (RUB)(alice.income * 1.5);
    alice.money += alice.income;
}

void alice_rent(const int year, const int month)
{
    if (year > 2025 && month == 1)
        alice.rent = (RUB)(alice.rent * 1.05);
    alice.money -= alice.rent;
}

void alice_food(const int year, const int month)
{
    if (year > 2025 && month == 1)
        alice.food = (RUB)(alice.food * 1.03);
    alice.money -= alice.food;
}

void alice_trip(const int year, const int month)
{
    if (month == 8)
        alice.money -= alice.trip;
}

void alice_car(const int year, const int month)
{
    if (month == 5 && year == 2034)
        alice.money -= alice.car_cost;
    if (year > 2034 || (year == 2034 && month > 5))
        alice.money -= alice.car_expense;
}

/*
------------------------------------------------------------------------------------------------------
*/

void bob_income(const int year, const int month)
{
    if (year == 2030 && month == 10)
        bob.income = (RUB)(bob.income * 1.5);
    bob.money += bob.income;
}

void bob_mortgage(const int year, const int month)
{
    if (year >= 2025 && year <= 2045 && bob.flat_price_mortgage > 0)
    {
        RUB payment = bob.mortgage_payment;
        if (payment > bob.flat_price_mortgage)
        {
            payment = bob.flat_price_mortgage;
        }
        bob.flat_price_mortgage -= payment;
        bob.money -= payment;
    }
}

void bob_food(const int year, const int month)
{
    if (year > 2025 && month == 1)
        bob.food = (RUB)(bob.food * 1.03);
    bob.money -= bob.food;
}

void bob_trip(const int year, const int month)
{
    if (month == 8)
        bob.money -= bob.trip;
}

/*
------------------------------------------------------------------------------------------------------
*/

void deposit_to_bank(struct Person *p, RUB amount, int bank)
{
    if (p->money < amount)
        amount = p->money;
    p->money -= amount;
    if (bank == 0)
        p->tinkoff_balance += amount;
    else
        p->sber_balance += amount;
}

void apply_interest(RUB *balance, double rate)
{
    double monthly = rate / 12.0;
    *balance += (RUB)(*balance * monthly);
}

/*
------------------------------------------------------------------------------------------------------
*/

void simulation()
{
    int year = 2025;
    int month = 9;

    while (!(year == 2045 && month == 9))
    {

        // Алиса
        alice_income(year, month);
        alice_rent(year, month);
        alice_food(year, month);
        alice_car(year, month);
        alice_trip(year, month);
        random_pocket_spend(&alice);
        random_medical_expense(&alice);
        deposit_to_bank(&alice, alice.income * 0.1, 0);
        deposit_to_bank(&alice, alice.income * 0.1, 1);
        apply_interest(&alice.tinkoff_balance, 0.03);
        apply_interest(&alice.sber_balance, 0.02);

        // Боб
        bob_income(year, month);
        bob_mortgage(year, month);
        bob_food(year, month);
        bob_trip(year, month);
        random_pocket_spend(&bob);
        random_medical_expense(&bob);
        deposit_to_bank(&bob, bob.income * 0.2, 0);
        deposit_to_bank(&bob, bob.income * 0.1, 1);
        apply_interest(&bob.tinkoff_balance, 0.02);
        apply_interest(&bob.sber_balance, 0.02);

        /*
        if (month == 12) {
            printf("=== Year %d ===\n", year);
            printf("Alice money: %lld руб., Sber: %lld, Tinkoff: %lld\n",
                   alice.money, alice.sber_balance, alice.tinkoff_balance);
            printf("Bob money:   %lld руб., Sber: %lld, Tinkoff: %lld, Mortgage remaining: %lld руб.\n",
                   bob.money, bob.sber_balance, bob.tinkoff_balance, bob.flat_price_mortgage);
        } */
        // DEBUG

        month++;
        if (month == 13)
        {
            year++;
            month = 1;
        }
    }
}

/*
------------------------------------------------------------------------------------------------------
*/

int main()
{
    srand(1239);

    alice_init();
    bob_init();
    simulation();
    alice_print();
    bob_print();
    return 0;
}
