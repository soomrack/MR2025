#include <iostream>

using RUB = long long int;

struct Person {
    RUB bank_account;
    RUB income;
    RUB food;
    RUB trip;
    RUB vznos;
    RUB vklad;
    RUB deposit;
    RUB capital;
};

struct Car {
    RUB cartex;
    RUB car;
    RUB car_price;
    RUB util_vznos;
};

struct Apartamernt {
    RUB utility_bills;
    RUB rent;
    RUB mortgage;
    RUB mortgage_pay;
    RUB apartment;
};

Person Alice_Person;
Person Bob_Person;
Car Alice_Car;
Car Bob_Car;
Apartamernt Alice_Apartamernt;
Apartamernt Bob_Apartamernt;

void Alice_income(const int year, const int month)                     // зарплата и премия
{
    if (year == 2030 && month == 3) {
        Alice_Person.income = Alice_Person.income * 1.5;
    }

    Alice_Person.bank_account += Alice_Person.income;
    Alice_Person.income = Alice_Person.income * (1.0 + 8.0 / 1200);
}


void Bob_income(const int year, const int month)
{
    if (year == 2030 && month == 3) {
        Bob_Person.income = Bob_Person.income * 1.5;
    }

    Bob_Person.bank_account += Bob_Person.income;
    Bob_Person.income = Bob_Person.income * (1.0 + 8.0 / 1200);
}


void Alice_food()                                                     // траты на еду
{
    Alice_Person.bank_account -= Alice_Person.food;
    Alice_Person.food = Alice_Person.food * (1.0 + 8.0 / 1200);
}


void Bob_food()
{
    Bob_Person.bank_account -= Bob_Person.food;
    Bob_Person.food = Bob_Person.food * (1.0 + 8.0 / 1200);
}


void Alice_trip(const int month)                                     // траты на путешествие
{
    if (month == 8) {
        Alice_Person.bank_account -= Alice_Person.trip;
    }

    Alice_Person.trip = Alice_Person.trip * (1.0 + 8.0 / 1200);
}


void Bob_trip(const int month)
{
    if (month == 8) {
        Bob_Person.bank_account -= Bob_Person.trip;
    }

    Bob_Person.trip = Bob_Person.trip * (1.0 + 8.0 / 1200);
}


void Alice_car(const int month)                                     // траты на бензин и тех обслуживание, пробег
{
    Alice_Person.bank_account -= Alice_Car.car;
    Alice_Car.car_price -= Alice_Car.car_price * 0.001;

    if (month == 1) {
        Alice_Person.bank_account -= Alice_Car.cartex;
    }

    Alice_Car.cartex = Alice_Car.cartex * (1.0 + 8.0 / 1200);
    Alice_Car.car = Alice_Car.car * (1.0 + 8.0 / 1200);
    Alice_Car.car_price = Alice_Car.car_price * (1.0 + 8.0 / 1200);
    Alice_Car.util_vznos = Alice_Car.util_vznos * (1.0 + 8.0 / 1200);
}


void Bob_car(const int month)
{
    Bob_Person.bank_account -= Bob_Car.car;
    Bob_Car.car_price -= Bob_Car.car_price * 0.001;

    if (month == 1) {
        Bob_Person.bank_account -= Bob_Car.cartex;
    }

    Bob_Car.cartex = Bob_Car.cartex * (1.0 + 8.0 / 1200);
    Bob_Car.car = Bob_Car.car * (1.0 + 8.0 / 1200);
    Bob_Car.car_price = Bob_Car.car_price * (1.0 + 8.0 / 1200);
    Bob_Car.util_vznos = Bob_Car.util_vznos * (1.0 + 8.0 / 1200);
}


void Alice_mortgage()                                              // выплаты по ипотеке Элис
{
    if (Alice_Apartamernt.mortgage > 0) {
        Alice_Person.bank_account -= Alice_Apartamernt.mortgage_pay;
    }

    Alice_Apartamernt.mortgage_pay = Alice_Apartamernt.mortgage_pay * (1.0 + 8.0 / 1200);
    Alice_Apartamernt.apartment = Alice_Apartamernt.apartment * (1.0 + 8.0 / 1200);
}


void Bob_rent()                                                    // выплаты аренды Боба
{
    Bob_Person.bank_account -= Bob_Apartamernt.rent;
    Bob_Apartamernt.rent = Bob_Apartamernt.rent * (1.0 + 8.0 / 1200);
}


void Alice_utility_bills()                                        // коммунальные платежи
{
    Alice_Person.bank_account -= Alice_Apartamernt.utility_bills;
    Alice_Apartamernt.utility_bills = Alice_Apartamernt.utility_bills * (1.0 + 8.0 / 1200);
}


void Bob_utility_bills()
{
    Bob_Person.bank_account -= Bob_Apartamernt.utility_bills;
    Bob_Apartamernt.utility_bills = Bob_Apartamernt.utility_bills * (1.0 + 8.0 / 1200);
}


void Alice_deposit()                                            // процент, пополнение депозита
{
    Alice_Person.deposit = Alice_Person.deposit * 1.015;
    Alice_Person.deposit += Alice_Person.bank_account;
    Alice_Person.bank_account = 0;
}


void Bob_deposit()
{
    Bob_Person.deposit = Bob_Person.deposit * 1.015;
    Bob_Person.deposit += Bob_Person.bank_account;
    Bob_Person.bank_account = 0;
}


void Alice_vznos(const int year, const int month)         // первоначальный взнос Элис
{
    if (year == 2025 && month == 9) {
        Alice_Person.bank_account -= Alice_Person.vznos;
    }
}


void Bob_vklad(const int year, const int month)                 // вклад Боба
{
    Bob_Person.vklad = Bob_Person.vklad * 1.02;
}


void Alice_capital()                                               // итоговый капитал
{
    Alice_Person.capital = Alice_Person.deposit + Alice_Car.car_price
        + Alice_Apartamernt.apartment - Alice_Car.util_vznos;
}


void Bob_capital()
{
    Bob_Person.capital = Bob_Person.deposit + Bob_Person.vklad
        + Bob_Car.car_price - Bob_Car.util_vznos;
}


void print_Alice_info()                                        // вывод капиатала
{
    std::cout << "Alice capital = " << Alice_Person.capital << " RUB\n";
}


void print_Bob_info()
{
    std::cout << "Bob capital = " << Bob_Person.capital << " RUB\n";
}


void Alice_init()                                             // инфа про Элис
{
    Alice_Person.bank_account = 100 * 100;
    Alice_Person.income = 150 * 1000;
    Alice_Person.food = 30000;
    Alice_Person.trip = 80 * 1000;
    Alice_Person.deposit = 0;
    Alice_Person.vznos = 500 * 1000;
    Alice_Car.cartex = 50 * 100;
    Alice_Car.car = 2000;
    Alice_Car.car_price = 1000 * 1000;
    Alice_Car.util_vznos = 10 * 1000;
    Alice_Apartamernt.utility_bills = 8000;
    Alice_Apartamernt.mortgage_pay = 60 * 1000;
    Alice_Apartamernt.mortgage = 200 * 1000;
    Alice_Apartamernt.apartment = 2000 * 1000;
}


void Bob_init()                                               // инфа про Боба
{
    Bob_Person.bank_account = 100 * 100;
    Bob_Person.income = 150 * 1000;
    Bob_Person.food = 35000;
    Bob_Person.vklad = 5000;
    Bob_Person.deposit = 0;
    Bob_Person.trip = 90 * 1000;
    Bob_Car.cartex = 70 * 100;
    Bob_Car.car = 2000;
    Bob_Car.car_price = 1000 * 1000;
    Bob_Car.util_vznos = 15 * 1000;
    Bob_Apartamernt.utility_bills = 7000;
    Bob_Apartamernt.rent = 30 * 1000;
}


void comparison()                                             // сравнение банковских счетов
{
    if (Bob_Person.capital > Alice_Person.capital) {
        std::cout << "Bob is living better\n";
    }
    else if (Alice_Person.capital > Bob_Person.capital) {
        std::cout << "Alice is living better\n";
    }
    else {
        std::cout << "Alice and Bob live equally well\n";
    }
}


void death()                                                  // проверка банковских счетов на уход в минус
{
    if (Bob_Person.capital < 0) {
        std::cout << "Bob need more money\n";
    }
    else if (Alice_Person.capital < 0) {
        std::cout << "Alice need more money\n";
    }
    else {
        std::cout << "Alice and Bob have enough money to live on\n";
    }
}


void simulation()                                            // симуляция
{
    int year = 2025;
    int month = 9;

    while (!(year == 2045 && month == 9)) {
        Alice_income(year, month);
        Alice_food();
        Alice_mortgage();
        Alice_utility_bills();
        Alice_car(month);
        Alice_trip(month);
        Alice_vznos(year, month);
        Alice_deposit();

        Bob_income(year, month);
        Bob_food();
        Bob_rent();
        Bob_utility_bills();
        Bob_car(month);
        Bob_trip(month);
        Bob_vklad(year, month);
        Bob_deposit();

        ++month;
        if (month == 13) {
            month = 1;
            ++year;
        }
    }
}


int main()
{
    Alice_init();
    Bob_init();

    simulation();

    Alice_capital();
    Bob_capital();

    print_Alice_info();
    print_Bob_info();

    comparison();

    death();

    return 0;
}
