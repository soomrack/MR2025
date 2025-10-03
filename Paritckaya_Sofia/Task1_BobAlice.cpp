#include <stdio.h>
#include <stdlib.h>
#include <math.h>

typedef long long int Money; // RUB

typedef struct {
    Money savings;
    Money salary;
    Money monthly_fee;
    Money expenses;
    Money property_value; //стоимость недвижимости
} Person;

bool bob_buy = false;
bool count = false;

Person alice;
Person bob;

static void alice_init()
{
    alice.savings = 0;
    alice.salary = 220 * 1000;
    alice.monthly_fee = 130 * 1000;  // monthly mortgage payment
    alice.property_value = 9610579;
}


static void bob_init()
{
    bob.savings = 2000 * 1000;
    bob.salary = 220 * 1000;
    bob.monthly_fee = 42 * 1000;  // rent an apartment
    bob.property_value = 0;
}


static void alice_salary(const int month)
{
    alice.savings += alice.salary;

    if (month == 12) {
        alice.salary *= 1.02;
    }
}


static void alice_expenses(const int month)
{
    alice.expenses = 0;
    Money utility_expenses = 5000; //коммунальные расходы
    Money food_expenses = alice.salary * 0.1;
    Money other_expenses = alice.salary * 0.05;

    alice.expenses += utility_expenses + food_expenses + other_expenses + alice.monthly_fee;

    if (month == 12) {
        utility_expenses *= 1.09;
        food_expenses *= 1.09;
        other_expenses *= 1.09;
        alice.expenses += utility_expenses + food_expenses + other_expenses + alice.monthly_fee;
    }

}


static void alice_savings(const int month)
{
    alice.savings -= alice.expenses;

    if (month == 12) {
        alice.savings *= 1.06; //Алиса кладёт остатки на накопительный счёт
    }

    if (month == 12) {
        alice.property_value *= 1.02;
    }
}


static void bob_salary(const int month)
{
    bob.savings += bob.salary;

    if (month == 12) {
        bob.salary *= 1.02;
    }
}


static void bob_expenses(const int month)
{
    bob.expenses = 0;
    Money utility_expenses = 5000;
    Money food_expenses = bob.salary * 0.1;
    Money other_expenses = bob.salary * 0.05;
    bob.expenses += utility_expenses + food_expenses + other_expenses + bob.monthly_fee;

    if (month == 12) {
        utility_expenses *= 1.09;
        food_expenses *= 1.09;
        other_expenses *= 1.09;
        bob.monthly_fee *= 1.09;
        if (bob_buy == false) {
            bob.expenses += utility_expenses + food_expenses + other_expenses + bob.monthly_fee;
        }
        else {
            bob.expenses += utility_expenses + food_expenses + other_expenses;
        }
    }

}


static void bob_savings(const int month)
{
    bob.savings -= bob.expenses;

    if (month == 12) {
        bob.savings *= 1.06;
    }

    if (month == 12) {
        bob.property_value *= 1.02;
    }
}

static void bob_buy_home(const int year, const int month)
{
    if (bob.savings >= alice.property_value && count == false) {
        printf("Bob buy home date = %lld \n", year, month);
        bob.savings -= alice.property_value;
        bob.property_value = alice.property_value;
        bob_buy = true;
        count = true;
    }
}


static void print_info_alice() {
    printf("Alice savings = %lld RUB\n", alice.savings + alice.property_value);
}


static void print_info_bob() {
    printf("Bob savings = %lld RUB\n", bob.savings + bob.property_value);
}


static void simulation()
{
    int year = 2025;
    int month = 9;

    while (!((year == 2025 + 20) && (month == 10))) {

        alice_salary(month);
        alice_expenses(month);  // utilities, food, monthly fee
        alice_savings(month);

        bob_salary(month);
        bob_expenses(month);
        bob_savings(month);
        bob_buy_home(year, month);

        ++month;
        if (month == 13)
        {
            month = 1;
            ++year;
        }
    }
}


int main() {

    alice_init();
    bob_init();

    simulation();

    print_info_alice();
    print_info_bob();

    return 0;
}