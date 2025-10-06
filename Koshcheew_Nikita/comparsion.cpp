#include <iostream>
#include "Person.h"
#include <random>
#include <format>


struct Date {
    int year;
    int month;
};


void set_random_inflation(int month, int& inflation) {
    static std::random_device rd;
    static std::mt19937 gen(rd());
    static std::uniform_int_distribution<> distrib(1, 9);
    if (month == 1) {
        inflation = distrib(gen);
        std::cout << inflation << '\n';
    }
}


void increase_price(int month, RUB& price, int inflation) {
    if (month == 1) price *= ((100.0 + inflation) / 100.0);
}


RUB get_mortgage_pay(RUB cost_of_flat, int inflation, int years) {
    double credit_rate = (inflation + 3.0) / (12.0 * 100.0);

    double coefficient = credit_rate * pow(1 + credit_rate, years * 12) / (pow(1 + credit_rate, years * 12) - 1);
    RUB mortgage_pay = cost_of_flat * coefficient;
    return mortgage_pay;
}


void alice_init(Person& alice, RUB cost_of_flat, int inflation) {
    RUB mortgage_pay = get_mortgage_pay(cost_of_flat, inflation, 20);
    alice.set_mortgage_pay(mortgage_pay);

    alice.set_food_spending(30000);
    alice.set_communal(10000);
}


void bob_init(Person& bob) {
    bob.set_food_spending(30000);
    bob.set_communal(10000);
    bob.set_rent(45000);
}


void car_cost_amortisation(RUB& cost_of_car, int month) {
    if (month == 12) cost_of_car *= 0.97;
}


void bob_car_ownership(Person& bob, RUB& cost_of_car, Date date, int inflation) {
    static bool have_car{ false };
    static RUB spending_on_car{ 20000 };

    if (date.year == 2010 && date.month == 3) {
        bob.buy_car(cost_of_car);
        have_car = true;
    }

    increase_price(date.month, spending_on_car, inflation);

    if (have_car) {
        car_cost_amortisation(cost_of_car, date.month);
        bob.spend_money_on_car(spending_on_car);
    }
}


void simulation(Person& alice, Person& bob, int starting_inflation) {
    int inflation{ starting_inflation };
    RUB cost_of_flat{ 12500000 };
    RUB cost_of_car{ 1000000 };

    alice_init(alice, cost_of_flat, inflation);
    bob_init(bob);

    Date date{ 2025, 9 };

    while (!(date.year == 2045 && date.month == 8)) {
        alice.get_income(date.month, inflation);
        alice.spend_money(date.month, inflation);
        //регулярные траты на еду, коммуналку и ипотеку
        alice.put_money_on_deposit();
        alice.get_dep_percent(inflation);

        bob.get_income(date.month, inflation);
        bob.spend_money(date.month, inflation);
        //регулярные траты на еду, коммуналку и аренду
        bob_car_ownership(bob, cost_of_car, date, inflation);
        bob.put_money_on_deposit();
        bob.get_dep_percent(inflation);

        date.month += 1;
        if (date.month == 13) {
            date.month = 1;
            date.year += 1;   
        }

        increase_price(date.month, cost_of_flat, inflation);
        increase_price(date.month, cost_of_car, inflation);

        set_random_inflation(date.month, inflation);
    }

    alice.print_info_alice(cost_of_flat);
    bob.print_info_bob(cost_of_car);
}


int main() {
    setlocale(LC_ALL, "RUS");
    Person Alice(300000);
    Person Bob(300000);

    int starting_inflation;
    std::cin >> starting_inflation;

    simulation(Alice, Bob, starting_inflation);

    return 0;
}

