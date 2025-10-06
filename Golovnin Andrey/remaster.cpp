#include <iostream>
#include <math.h>

typedef long int RUB;

struct Simulation_propertis
{
    int actual_month = 0;  // текущий месяц
    float key_bid = 0.18;  // ключивая ставка
    float inflation_coefficient = 1.01; // коэффиценнт инфляции
    int credit_mought = 180; // кредит на пятнадцать лет
    float salary_indexation = 1.008; // коэффицент индексации
}sim;


struct Person
{
    RUB salary;           // Зарплата   
    RUB deposit_interest; // Процент по вкладу
    RUB mortgage_debit;   // Ипотека
    
    RUB bank_account;    // Счёт в банке
    RUB contribution;    // Сберегательный счёт

    int car_quantity;
    RUB car_cost;        // Стоимость автомобиля
    RUB car_insuranceb;  // Страховка автомобиля
    RUB car_tax;         // Налог на автомобиль
    
    int home_quantity;   // Количество домов
    RUB home_cost;       // Стоимость дома
    RUB rent;            // Аренда дома в месяц
    RUB utility_bills;   // Комунальные платежи
    RUB home_insuranceb; // Страховка дома
    RUB home_tax;        // Налог на дом

    RUB food;            // Траты на еду в месяц
    RUB clothes;         // Среднеднии траты на одежду в месяц
    RUB different;       // Иные траты

};

struct Person bob;
struct Person alice;

void bob_init()
{
    bob.bank_account = 0;
    bob.contribution = 1000 * 1000;
    
    bob.salary = 200 * 1000;
    bob.deposit_interest = bob.contribution * sim.key_bid / 12;
    bob.deposit_interest = 0;

    bob.car_quantity = 1;
    bob.car_cost = 3 * 1000 * 1000;
    bob.car_insuranceb = 5000;
    bob.car_tax = 2000;

    bob.home_quantity = 0;
    bob.home_cost = 10 * 1000 * 1000;
    bob.rent= 45 * 1000;
    bob.utility_bills = 8000;
    bob.home_insuranceb = 1500;
    bob.home_tax = 8500;

    bob.food = 20 * 1000;
    bob.clothes = 3000;
    bob.different = 6000;
}

void alice_init()
{
    alice.bank_account = 0;
    alice.contribution = 0;
    
    alice.salary = 200 * 1000;
    alice.deposit_interest = alice.contribution * sim.key_bid / 12;\
    bob.deposit_interest = 9 * 1000 * 1000 * (((sim.key_bid / 12)) / ( 1 - pow((1 + (sim.key_bid / 12)), (-1 * (sim.credit_mought - 1)) )));;

    alice.car_quantity = 1;
    alice.car_cost = 3 * 1000 * 1000;
    alice.car_insuranceb = 5000;
    alice.car_tax = 2000;

    alice.home_quantity = 1;
    alice.home_cost = 10 * 1000 * 1000;
    alice.rent = 0;
    alice.utility_bills = 8000;
    alice.home_insuranceb = 1500;
    alice.home_tax = 8500;

    alice.food = 20 * 1000;
    alice.clothes = 4000;
    alice.different = 6000;
}

void bob_receiving_money()
{
    bob.bank_account += bob.salary;                            // начисление зарплаты
    bob.bank_account += bob.contribution * (sim.key_bid / 12); // дипозит на остаток по счёту
}

void alice_receiving_money()
{
    alice.bank_account += alice.salary;                             // начисление зарплаты
    alice.bank_account += alice.contribution * (sim.key_bid / 12); // дипозит на остаток по счёту
}


void bob_home_cost()
{   
    if (bob.contribution == bob.home_cost) // условие покубки дома
    {
        bob.contribution -= bob.home_cost;
        bob.home_quantity ++ ;
        bob.rent = 0;
    }
    
   bob.bank_account -= bob.car_quantity * (bob.utility_bills + bob.home_insuranceb);
}

void alice_home_cost()
{
    if (alice.contribution == alice.home_cost) // условие покубки дома
    {
        alice.contribution -= alice.home_cost;
        alice.home_quantity ++ ;
    }

   alice.bank_account -= alice.car_quantity * (alice.utility_bills + alice.home_insuranceb);
}


void bob_car_cost()
{    
   bob.bank_account -= bob.car_quantity * bob.car_insuranceb;
}

void alice_car_cost()
{
   alice.bank_account -= alice.car_quantity * alice.car_insuranceb;
}


void bob_live_cost()
{    
   bob.bank_account -= bob.food + bob.clothes + bob.different;
}

void alice_live_cost()
{
   alice.bank_account -= alice.food + alice.clothes + alice.different;
}

void bob_tax()
{    
   bob.bank_account -= bob.home_tax + bob.car_tax;
}

void alice_tax()
{
   alice.bank_account -= alice.home_tax + alice.car_tax;
}

void bob_inflation()
{
    bob.home_cost *= sim.inflation_coefficient; // влияние инфляции на стоимость дома
    bob.car_cost *= sim.inflation_coefficient;  // влияние инфляции на стоимость автомобиля
    bob.salary *= sim.salary_indexation;        // индексация зарплаты
}

void alice_inflation()
{
    alice.home_cost *= sim.inflation_coefficient; // влияние инфляции на стоимость дома
    alice.car_cost *= sim.inflation_coefficient;  // влияние инфляции на стоимость автомобиля
    alice.salary *= sim.salary_indexation;        // индексация зарплаты
}


void simulation ()
{
    while (sim.actual_month < sim.credit_mought)
    {
        bob_receiving_money();
        bob_home_cost();
        bob_car_cost();
        bob_live_cost();
        bob_inflation();

        alice_receiving_money();
        alice_home_cost();
        alice_car_cost();
        alice_live_cost();
        alice_inflation();
        
        if(sim.actual_month % 12 == 0)
        {
            bob_tax();
            alice_tax();
        }

        sim.actual_month ++;
    }
}

void Output()
{
    RUB bob_sum = bob.bank_account + bob.contribution + bob.home_quantity * bob.home_cost; 
    RUB alice_sum =alice.bank_account + alice.contribution + alice.home_quantity * alice.home_cost;

    std::cout << "Alice:  " << alice_sum << "       " << "Bob:  " << bob_sum << std::endl;
}

int main()
{
    setlocale(LC_ALL, "Rus");

    bob_init();
    alice_init();

    simulation();

    Output();

}

