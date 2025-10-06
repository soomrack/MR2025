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
    const RUB sum;
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


const double inflationMonthCoefficient = 1.0 + 0.6 / 100;
const double indexationQuarterCoefficient = 1.0 + 1.9 / 100;
const RUB flood_repair_cost = 100000;


std::string format_rub(RUB sum) 
{
    std::string s = std::to_string(sum);
    for (int i = s.length() - 3; i > 0; i -= 3)
        s.insert(i, " ");
    return s;
}


class Person {
public:
    RUB bank_account;
    RUB income_amount;
    RUB food_spendings = 15000;
    RUB car_sharing_month_spending = 15000;

    std::string name;

    void init(std::string got_name);    
    void print();
    void food();    
    void car_sharing();
};


void Person::init(std::string got_name) {
    name = got_name;
    bank_account = 3 * 1000 * 1000;
    income_amount = 200 * 1000;
}


void Person::print() {
    printf("%s final bank account = %s руб.\n", name.c_str(), format_rub(bank_account).c_str());
}


void Person::food() {
    bank_account -= food_spendings;
    food_spendings *= inflationMonthCoefficient;
}


void Person::car_sharing() {
    bank_account -= car_sharing_month_spending;
    car_sharing_month_spending *= inflationMonthCoefficient;
}


class Mortage_person : public Person {
public:
    void mortage(const int year, const int month);
    void income(const int year, const int month);
    void apartment_flood(const int year, const int month);
};


void Mortage_person::mortage(const int year, const int month) {
    //Начальный взнос
    if (year == 2025 && month == 9) {
        bank_account -= mortage_features.first_payment;
    }
    //Регулярный платёж
    bank_account -= mortage_features.regular_payment;

}


void Mortage_person::income(const int year, const int month) {
    if (year == 2030 && month == 10) {
        income_amount *= 1.5; //Promotion
    }
    if (month == 1 or month == 4 or month == 7 or month == 10) {
        income_amount *= indexationQuarterCoefficient; //indexation
    }
    bank_account += income_amount;
}


void Mortage_person::apartment_flood(const int year, const int month) {
    if (year == 2031 && month == 9) {
        bank_account -= flood_repair_cost;
    }
}


class Rent_person : public Person {
public:
    void rent(const int year, const int month);
    void income(const int year, const int month);
    void deposit();
};


void Rent_person::rent(const int year, const int month) {
    if (year == 2025 && month == 9) {
        bank_account -= rent_features.pledge;
    }
    bank_account -= rent_features.regular_payment;
}


void Rent_person::income(const int year, const int month) {
    if (month == 1 or month == 4 or month == 7 or month == 10) {
        income_amount *= indexationQuarterCoefficient; //indexation
    }
    bank_account += income_amount;
}


void Rent_person::deposit() {
    double month_multiplier = 1.0 + (deposit_features.year_percent / 12.0)/100;

    deposit_features.sum *= month_multiplier;
    if (bank_account > deposit_features.minimumBalance) {
        RUB month_deposit = bank_account - deposit_features.minimumBalance;
        bank_account -= month_deposit;
        deposit_features.sum += month_deposit;
    }
}


void simulation(Mortage_person& alice, Rent_person& bob) {
    int year = 2025;
    int month = 9;

    while (!(year == 2045 && month == 9)) {
        alice.income(year, month);
        alice.mortage(year, month);
        alice.food();
        alice.car_sharing();
        alice.apartment_flood(year, month);

        bob.income(year, month);
        bob.rent(year, month);
        bob.food();
        bob.car_sharing();
        bob.deposit();

        month++;
        if (month == 13) {
            year++;
            month = 1;
        }
    }
}


void compare(Mortage_person alice, Rent_person bob) {
    printf("\nAlice final fund = %s руб.\n", format_rub(alice.bank_account + mortage_features.sum).c_str());
    printf("Bob final fund = %s руб.\n", format_rub(bob.bank_account + deposit_features.sum).c_str());
}


int main() {
    setlocale(LC_ALL, "Russian");

    Mortage_person alice;
    Rent_person bob;

    alice.init("Alice");
    bob.init("Bob");

    simulation(alice, bob);

    alice.print();
    bob.print();

    compare(alice, bob);
}