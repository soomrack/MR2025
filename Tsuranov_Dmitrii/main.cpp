#include <stdio.h>
#include <string>
#include <iostream>
#include <cmath>

/*
* В начальный момент времени Алиса и Боб имеют одинаковое количество денег на счету в банке.
* Зарплаты у них одинаковые, повышение происходит одновременно, в октябре 2030 года
*
* Алиса берёт в ипотеку под 20% годовых квартиру стоимостью 8 500 000 руб. Первоначальный взнос 1 710 000 руб
*/
typedef long long int RUB;


struct {
    RUB sum;
    const RUB first_payment;
    const RUB regular_payment;
    const float year_multiplier;
} mortage_features = { 8500000, 1710000, 132667, 1.2 };


struct {
    const RUB regular_payment;
    const RUB pledge;
} rent_features = { 60000, 50000 };


struct {
    RUB sum;
    const RUB minimumBalance;
    const double year_percent;
} deposit_features = { 0, 40000, 9.0 };


std::string formatRub(RUB sum) {
    std::string s = std::to_string(sum);
    for (int i = s.length() - 3; i > 0; i -= 3)
        s.insert(i, " ");
    return s;
}


class Person {
    RUB bank_account;
    RUB income_amount;

public:
    std::string name;

    void init(std::string gotName) {
        name = gotName;
        bank_account = 3 * 1000 * 1000;
        income_amount = 200 * 1000;
    }


    void income(const int year, const int month) {
        if (year == 2030 && month == 10) {
            income_amount *= 1.5; //Promotion
        }
        bank_account += income_amount;
    }


    void print() {
        printf("%s final bank account = %s руб.\n", name.c_str(), formatRub(bank_account).c_str());
    }


    void mortage(const int year, const int month) {
        if (month == 1) {
            mortage_features.sum *= mortage_features.year_multiplier;
        }
        
        if (mortage_features.sum < mortage_features.regular_payment) {
            RUB last_payment = mortage_features.sum;
            bank_account -= last_payment;
            mortage_features.sum -= last_payment;
        }
        else if (year == 2025 && month == 9) {
            bank_account -= mortage_features.first_payment;
            mortage_features.sum -= mortage_features.first_payment;
        }
        else {
            bank_account -= mortage_features.regular_payment;
            mortage_features.sum -= mortage_features.regular_payment;
        }
        //printf("%s final bank account = %lld руб.\n", name.c_str(), bank_account);

    }


    void rent(const int year, const int month) {
        if (year == 2025 && month == 9) {
            bank_account -= rent_features.pledge;
        }
        bank_account -= rent_features.regular_payment;
    }


    void food() {
        bank_account -= 15000;
    }


    void deposit() {
        double month_multiplier = 1.0 + pow(deposit_features.year_percent, 1.0/12)/100;      
        
        deposit_features.sum *= month_multiplier;
        if (bank_account > deposit_features.minimumBalance) {
            RUB month_deposit = bank_account-deposit_features.minimumBalance;
            bank_account -= month_deposit;
            deposit_features.sum += month_deposit;
        }
        //printf("%s\n", formatRub(deposit_features.sum).c_str());
    }

    
    void popDeposit() {
        bank_account += deposit_features.sum;
        deposit_features.sum = 0;
    }
};


void simulation(Person& alice, Person& bob) {
    int year = 2025;
    int month = 9;

    while (!(year == 2045 && month == 9)) {
        alice.income(year, month);
        bob.income(year, month);

        alice.mortage(year, month);
        bob.rent(year, month);

        alice.food();
        bob.food();

        bob.deposit();

        month++;
        if (month == 13) {
            year++;
            month = 1;
        }
        bob.popDeposit();
    }
}


int main() {
    setlocale(LC_ALL, "Russian");

    Person alice;
    Person bob;

    alice.init("Alice");
    bob.init("Bob");

    simulation(alice, bob);

    alice.print();
    bob.print();
}