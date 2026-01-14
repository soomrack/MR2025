#include <iostream>
#include <math.h>
using namespace std;

typedef long int RUB;


struct Simulation
{
    RUB home_cost = 10 * 1000 * 1000; // начальна€ стоимость дома
    RUB initial_deposit = 1000 * 1000; // стартовый дипосит
    RUB debit_part = 0;
    int actual_month = 0; // текущий мес€ц
    int credit_mought = 180; // кредит на п€тнадцать лет
    float key_bid = 0.18;  // ключива€ ставка
    float inflation_coefficient = 1.01; // коэффиценнт инфл€ции
    float salary_indexation = 1.008; // коэффицент индексации

}properties;


struct Person
{
    RUB bank_account;
    RUB salary;
    RUB debt;
    RUB live_cost;
    RUB contribution;
    int home_quantity;
};

struct Person bob;
struct Person alice;

void bob_init()
{
    RUB food = 30000;
    RUB rent = 40000; //арендна€ плата
    RUB clothes = 10000; // среднеднии траты на одежду в мес€ц
    RUB car = 8000; // средние траты на автомобиль в мес€ц
    RUB different = 6000; // иные траты
    RUB contribution = 0; //сберегательный счЄт

    bob.home_quantity = 0;
    bob.bank_account = 1000 * 1000;
    bob.salary = 200 * 1000;
    bob.debt = properties.home_cost - properties.initial_deposit;
    bob.live_cost = food + clothes + different + rent + car;

}

void alice_init()
{
    RUB food = 30000; // траты на еду
    RUB clothes = 10000; // среднеднии траты на одежду в мес€ц;
    RUB car = 8000; // средние траты на автомобиль в мес€ц;
    RUB different = 6000; // иные траты
    RUB contribution = 0; //сберегательный счЄт

    alice.home_quantity = 1;
    alice.bank_account = 1000 * 1000;
    alice.salary = 200 * 1000;
    alice.debt = properties.home_cost - properties.initial_deposit;
    properties.debit_part = alice.debt * (((properties.key_bid / 12)) / (1 - pow((1 + (properties.key_bid / 12)), (-1 * (properties.credit_mought - 1)))));
    alice.bank_account -= properties.initial_deposit;
    alice.live_cost = food + clothes + car + different;
}


void alice_bank()
{
    alice.bank_account += alice.contribution * (properties.key_bid / 12); // дипозит на остаток по счЄту
    alice.bank_account += alice.salary; // начисление зарплаты
    alice.bank_account -= properties.debit_part; //оплата ипотеки
    alice.bank_account -= alice.live_cost; // остальные траты
    alice.contribution += alice.bank_account;
    alice.bank_account = 0;

    alice.live_cost *= properties.inflation_coefficient; //вли€ние роста инфл€ции на траты
    alice.salary *= properties.salary_indexation; // индексаци€ зарплаты

    if (properties.actual_month % 48 == 0)  // повышение зарплаты раз в 4 года
    {
        alice.salary += 20 * 1000;
    }
}

void bob_bank()
{
    bob.bank_account += bob.salary;  // начисление зарплаты
    bob.bank_account += bob.contribution * (properties.key_bid / 12); // дипозит на остаток по счЄту
    bob.bank_account -= bob.live_cost; // остальные траты


    if (bob.contribution >= properties.home_cost) // условие покупки дома
    {
        bob.contribution -= properties.home_cost;
    }

    bob.contribution += bob.bank_account;
    bob.bank_account = 0;

    properties.home_cost *= properties.inflation_coefficient; // вли€ние инфл€ции на стоимость дома

    bob.live_cost *= properties.inflation_coefficient; // вни€ние инфл€ции на траты
    bob.salary *= properties.salary_indexation; // индексаци€ зарплаты

    if (properties.actual_month % 48 == 0) // повышение зарплаты раз в 4 года
    {
        bob.salary += 20 * 1000;
    }
}

void events()
{
    switch (properties.actual_month)
    {
    case 15:
        alice.bank_account -= 50000; // пример любого случайного событи€, сломанный холодильник
        bob.bank_account -= 50000;
        break;
    default:
        break;
    }

}

void simulation()
{
    while (properties.actual_month < properties.credit_mought)
    {
        events();
        bob_bank();
        alice_bank();

        properties.actual_month++;
    }
}


void Output()
{
    RUB bob_sum = bob.bank_account + bob.contribution + bob.home_quantity * properties.home_cost;
    RUB alice_sum = alice.bank_account + alice.contribution + alice.home_quantity * properties.home_cost;

    cout << "Alice:  " << alice_sum << "       " << "Bob:  " << bob_sum << endl;
}


int main() {

    setlocale(LC_ALL, "Rus");

    alice_init();
    bob_init();

    simulation();
    Output();

    return 0;
}