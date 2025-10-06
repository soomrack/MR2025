#include <iostream>
#include "Person.h"
#include <random>
#include <format>


struct Date {
    int year;
    int month;
};


void set_random_inflation(int& inflation) {
    static std::random_device rd;
    static std::mt19937 gen(rd());
    static std::uniform_int_distribution<> distrib(1, 9);
    inflation = distrib(gen);
    std::cout << inflation << '\n';
}


void increase_price(RUB& price, int inflation) {
    price *= ((100.0 + inflation) / 100.0);
}


RUB get_mortgage_pay(RUB cost_of_flat, int inflation, int years) {
    double credit_rate = (inflation + 3.0) / (12.0 * 100.0);

    double coefficient = credit_rate * pow(1 + credit_rate, years * 12) / (pow(1 + credit_rate, years * 12) - 1);
    RUB mortgage_pay = cost_of_flat * coefficient;
    return mortgage_pay;
}


void simulation(Person& alice, Person& bob, int startingInflation) {
    int inflation{ startingInflation };
    RUB cost_of_flat{ 12500000 };
    RUB mortgage_pay = get_mortgage_pay(cost_of_flat, inflation, 20);
    alice.set_mortgage_pay(mortgage_pay);
    
    Date date{ 2025, 9 };

    alice.set_food_spending(30000);
    alice.set_communal(10000);

    bob.set_food_spending(30000);
    bob.set_communal(10000);
    bob.set_rent(45000);

    while (!(date.year == 2045 && date.month == 8)) {
        alice.get_income(date.month, inflation);
        alice.spend_money(date.month, inflation);
        alice.put_money_on_deposit();
        alice.get_dep_percent(inflation);

        bob.get_income(date.month, inflation);
        bob.spend_money(date.month, inflation);
        bob.put_money_on_deposit();
        bob.get_dep_percent(inflation);

        date.month += 1;

        if (date.month == 13) {
            date.month = 1;
            date.year += 1;

            increase_price(cost_of_flat, inflation);
            set_random_inflation(inflation);
        }
    }

    bob.buy_flat(cost_of_flat);
}


int main() {
    setlocale(LC_ALL, "RUS");
    Person Alice(300000);
    Person Bob(300000);

    int startingInflation;
    std::cin >> startingInflation;

    simulation(Alice, Bob, startingInflation);

    Alice.print_info();
    Bob.print_info();

    return 0;
}

