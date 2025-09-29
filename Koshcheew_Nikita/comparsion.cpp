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


RUB get_mortgage_pay(RUB cost_of_flat, int inflation, int years) {
    double credit_rate = (static_cast<double>(inflation) * 2 + 3) / (12 * 100);

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
    bob.set_food_spending(30000);

    bob.set_rent(45000);

    while (!(date.year == 2045 && date.month == 8)) {
        alice.income();
        bob.income();

        alice.buy_food();
        bob.buy_food();
        alice.pay_mortgage();
        bob.pay_rent();

        alice.get_dep_percent(inflation);
        bob.get_dep_percent(inflation);

        date.month += 1;

        if (date.month == 13) {
            date.month = 1;
            date.year += 1;
            
            alice.increase_prices(inflation);
            bob.increase_prices(inflation);
            cost_of_flat *= ((100 + static_cast<double>(inflation)) / 100);

            alice.increase_salary(inflation);
            bob.increase_salary(inflation);

            set_random_inflation(inflation);
        }
    }

    bob.buy_flat(cost_of_flat);
}

int main() {
    Person Alice(200000);
    Person Bob(200000);

    int startingInflation;
    std::cin >> startingInflation;

    simulation(Alice, Bob, startingInflation);

    Alice.print_info();
    Bob.print_info();

    return 0;
}



