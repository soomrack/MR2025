#include <iostream>
#include <math.h>

typedef long int RUB;

struct Simulation_propertis
{
    int actual_month = 0;  // текущий месяц от начала симуляции
    float key_bid = 0.18;  // ключивая ставка
    float inflation_coefficient = 1.01; // коэффиценнт инфляции
    int credit_mought = 180; // кредит на пятнадцать лет
    float salary_indexation = 1.008; // коэффицент индексации
}sim;


struct Date
{
    int year = 2025;
    int mought = 3;
} date;


struct Car
{
    int quantity;
    RUB cost;        // Стоимость автомобиля
    RUB insuranceb;  // Страховка автомобиля
    RUB tax;         // Налог на автомобиль
};


struct Home
{
    int quantity;   // Количество домов
    RUB cost;       // Стоимость дома
    RUB rent;       // Аренда дома в месяц
    RUB utility_bills;      // Комунальные платежи
    RUB insuranceb; // Страховка дома
    RUB tax;        // Налог на дом

};

struct Person

{
    RUB salary;           // Зарплата   
    RUB deposit_interest; // Процент по вкладу
    RUB mortgage_debit;   // Ипотека
    
    RUB bank_account;    // Счёт в банке
    RUB contribution;    // Сберегательный счёт

    Car car;
    Home home;

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

    bob.car.quantity = 1;
    bob.car.cost = 3 * 1000 * 1000;
    bob.car.insuranceb = 5000;
    bob.car.tax = 2000;

    bob.home.quantity = 0;
    bob.home.cost = 10 * 1000 * 1000;
    bob.home.rent= 45 * 1000;
    bob.home.utility_bills = 8000;
    bob.home.insuranceb = 1500;
    bob.home.tax = 8500;

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

    alice.car.quantity = 1;
    alice.car.cost = 3 * 1000 * 1000;
    alice.car.insuranceb = 5000;
    alice.car.tax = 2000;

    alice.home.quantity = 1;
    alice.home.cost = 10 * 1000 * 1000;
    alice.home.rent = 0;
    alice.home.utility_bills = 8000;
    alice.home.insuranceb = 1500;
    alice.home.tax = 8500;

    alice.food = 20 * 1000;
    alice.clothes = 4000;
    alice.different = 6000;
}


void bob_receiving_money()
{
    bob.bank_account += bob.salary;                            // начисление зарплаты
    bob.bank_account += bob.contribution * (sim.key_bid / 12); // дипозит на остаток по счёту
    bob.salary *= sim.salary_indexation;                       // индексация зарплаты
}

void alice_receiving_money()
{
    alice.bank_account += alice.salary;                            // начисление зарплаты
    alice.bank_account += alice.contribution * (sim.key_bid / 12); // дипозит на остаток по счёту
    alice.salary *= sim.salary_indexation;                         // индексация зарплаты
}


void bob_home_cost()
{   
    if (bob.contribution == bob.home.cost) // условие покубки дома
    {
        bob.contribution -= bob.home.cost;
        bob.home.quantity ++ ;
        bob.home.rent = 0;
    }
    
    bob.bank_account -= bob.car.quantity * (bob.home.utility_bills + bob.home.insuranceb);
    bob.home.cost *= sim.inflation_coefficient / 12; // влияние инфляции на стоимость дома
}

void alice_home_cost()
{
    if (alice.contribution == alice.home.cost) // условие покубки дома
    {
        alice.contribution -= alice.home.cost;
        alice.home.quantity ++ ;
    }
    
    alice.bank_account -= alice.car.quantity * (alice.home.utility_bills + alice.home.insuranceb);
    alice.home.cost *= sim.inflation_coefficient / 12; // влияние инфляции на стоимость дома
}


void bob_car_cost()
{    
    bob.bank_account -= bob.car.quantity * bob.car.insuranceb;
    bob.car.cost *= sim.inflation_coefficient / 12;  // влияние инфляции на стоимость автомобиля
}

void alice_car_cost()
{
    alice.bank_account -= alice.car.quantity * alice.car.insuranceb;
    alice.car.cost *= sim.inflation_coefficient / 12;  // влияние инфляции на стоимость автомобиля
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
   bob.bank_account -= bob.home.tax + bob.car.tax;
}

void alice_tax()
{
   alice.bank_account -= alice.home.tax + alice.car.tax;
}


void simulation ()
{   
    while (sim.actual_month < sim.credit_mought)
    {
        bob_receiving_money();
        bob_home_cost();
        bob_car_cost();
        bob_live_cost(); //food, clothes, different

        alice_receiving_money();
        alice_home_cost();
        alice_car_cost();
        alice_live_cost(); //food, clothes, different
        
        if(sim.actual_month % 12 == 0)
        {
            bob_tax();
            alice_tax();
        }

        date.mought ++;

        if (date.mought > 12)
        {
            date.mought == 1;
            date.year++;
        }

        sim.actual_month ++;
    }
}


void Output()
{
    RUB bob_sum = bob.bank_account + bob.contribution + bob.home.quantity * bob.home.cost; 
    RUB alice_sum =alice.bank_account + alice.contribution + alice.home.quantity * alice.home.cost;

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

