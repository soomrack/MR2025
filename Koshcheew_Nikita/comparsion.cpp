#include <iostream>
#include "Person.h"
#include <random>


struct CostOfLife {
    RUB CostOfFood;
    RUB CostOfRent;
    RUB CostOfCommunal;
    RUB CostOfFlat;
};


struct Rates {
    int CreditRate;
    int DepositPercentage;
};


struct Date {
    int year;
    int month;
};


void setRandomInflation(int& inflation) {
    static std::random_device rd;
    static std::mt19937 gen(rd());
    static std::uniform_int_distribution<> distrib(1, 9);
    inflation = distrib(gen);
}


void increasePrices(CostOfLife& prices, int inflation) {
    prices.CostOfCommunal = prices.CostOfCommunal * (100 + inflation) / 100;
    prices.CostOfFood = prices.CostOfFood * (100 + inflation) / 100;
    prices.CostOfRent = prices.CostOfRent * (100 + inflation) / 100;
    prices.CostOfFlat = prices.CostOfFlat * (100 + inflation) / 100;
}


void printPrices(CostOfLife& prices) {
    std::cout << prices.CostOfCommunal << " ";
    std::cout << prices.CostOfFood << " ";
    std::cout << prices.CostOfRent << " ";
    std::cout << prices.CostOfFlat << "\n";
}

void simulation(Person& personWithCredit, Person& personWithoutCredit, int startingInflation) {
    int inflation{ startingInflation };
    CostOfLife prices{30000, 55000, 10000, 12500000};
    Rates rates{ (inflation * 2) + 3, (inflation * 2) - 2 };
    Date date{ 2025, 9 };

    personWithCredit.setCreditRate(rates.CreditRate);

    while (!(date.year == 2045 && date.month == 9)) {
        personWithCredit.getAndSpendMoney(prices.CostOfCommunal + prices.CostOfFood);
        personWithoutCredit.getAndSpendMoney(prices.CostOfCommunal + prices.CostOfFood, prices.CostOfRent);

        if (date.month == 12) {
            personWithCredit.increaseCredit();
            personWithCredit.increaseDeposit(rates.DepositPercentage);
            personWithoutCredit.increaseDeposit(rates.DepositPercentage);
            personWithCredit.increaseSalary(inflation);
            personWithoutCredit.increaseSalary(inflation);
            increasePrices(prices, inflation);
        }

        date.month++;
        if (date.month == 13) {
            date.year++;
            date.month = 1;
            setRandomInflation(inflation);
            std::cout << date.year << " " << inflation << "\n";
        }
    }

    printPrices(prices);
}

int main() {
    Person Alice(200000, 0, 12500000);
    Person Bob(200000, 0);

    int startingInflation;
    std::cin >> startingInflation;

    simulation(Alice, Bob, startingInflation);

    Alice.printInfo();
    Bob.printInfo();

    return 0;
}

