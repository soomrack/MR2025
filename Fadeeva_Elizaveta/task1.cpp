#include <iostream>

using RUB = long long int;

struct Person {
    RUB bank_account;
    RUB income;
    RUB food;
    RUB trip;
    RUB cartex;
    RUB car;
    RUB util_vznos;
    RUB car_price;
    RUB utility_bills;
    RUB rent;
    RUB mortgage;
    RUB mortgage_pay;
    RUB vznos;
    RUB vklad;
    RUB deposit;
    RUB apartment;
    RUB itog;
};

Person Alice;
Person Bob;

void Alice_income(const int year, const int month)                     // зарплата и премия
{
    if (year == 2030 && month == 3) {
        Alice.income = static_cast<int>(Alice.income * 1.5);
    }

    Alice.bank_account += Alice.income;
}


void Bob_income(const int year, const int month)                    
{
    if (year == 2030 && month == 3) {
        Bob.income = static_cast<int>(Bob.income * 1.5);
    }

    Bob.bank_account += Bob.income;
}


void Alice_food()                                                     // траты на еду
{
    Alice.bank_account -= Alice.food;
}


void Bob_food()                                                 
{
    Bob.bank_account -= Bob.food;
}


void Alice_trip(const int month)                                     // траты на путешествие
{
    if (month == 8) {
        Alice.bank_account -= Alice.trip;
    }
}


void Bob_trip(const int month)                                
{
    if (month == 8) {
        Bob.bank_account -= Bob.trip;
    }
}


void Alice_car(const int month)                                     // траты на бензин и тех обслуживание
{
    Alice.bank_account -= Alice.car;

    if (month == 1) {
        Alice.bank_account -= Alice.cartex;
    }
}


void Bob_car(const int month)                                                
{
    Bob.bank_account -= Bob.car;

    if (month == 1) {
        Bob.bank_account -= Bob.cartex;
    }
}


void Alice_car_ptice()                                         // снижение стоимости машины из-за пробега
{
    Alice.car_price -= static_cast<int>(Alice.car_price * 0.001);
}


void Bob_car_ptice()
{
    Bob.car_price -= static_cast<int>(Bob.car_price * 0.001);
}


void Alice_mortgage()                                              // выплаты по ипотеке Элис
{
    if (Alice.mortgage > 0) {
        Alice.bank_account -= Alice.mortgage_pay;
    }
}


void Bob_rent()                                                    // выплаты аренды Боба
{
    Bob.bank_account -= Bob.rent;
}


void Alice_utility_bills()                                        // коммунальные платежи
{
    Alice.bank_account -= Alice.utility_bills;
}


void Bob_utility_bills()                                         
{
    Bob.bank_account -= Bob.utility_bills;
}


void Alice_deposit()                                            // пополнение депозита
{
    Alice.deposit += Alice.bank_account;
    Alice.bank_account = 0;
}


void Bob_deposit()
{
    Bob.deposit += Bob.bank_account;
    Bob.bank_account = 0;
}


void Alice_deposit_protsent()                                  // начисление процентов по депозиту
{
    Alice.deposit = static_cast<int>(Alice.deposit * 1.015);
}


void Bob_deposit_protsent()
{
    Bob.deposit = static_cast<int>(Bob.deposit * 1.015);
}


void Alice_vznos(const int year, const int month)         // первоначальный взнос Элис
{
    if (year == 2025 && month == 9) {
        Alice.bank_account -= Alice.vznos;
    }
}


void Bob_vklad(const int year, const int month)                 // вклад Боба
{
    Bob.vklad = static_cast<int>(Bob.vklad * 1.02);
}


void Alice_itog()                                               // итоговый капитал
{
    Alice.itog = Alice.deposit + Alice.car_price + Alice.apartment - Alice.util_vznos;
}


void Bob_itog()                                              
{
    Bob.itog = Bob.deposit + Bob.vklad + Bob.car_price - Bob.util_vznos;
}


void print_Alice_info()                                        // вывод капиатала
{
    std::cout << "Alice capital = " << Alice.itog << " RUB\n";
}


void print_Bob_info()                                         
{
    std::cout << "Bob capital = " << Bob.itog << " RUB\n";
}


void inflation()                                              // расчет инфляции ежемесячной
{
    Alice.income        = Alice.income * (1 + 8.0 / 1200);
    Alice.food          = Alice.food * (1 + 8.0 / 1200);
    Alice.trip          = Alice.trip * (1 + 8.0 / 1200);
    Alice.cartex        = Alice.cartex * (1 + 8.0 / 1200);
    Alice.car           = Alice.car * (1 + 8.0 / 1200);
    Alice.car_price     = Alice.car_price * (1 + 8.0 / 1200);
    Alice.util_vznos    = Alice.util_vznos * (1 + 8.0 / 1200);
    Alice.utility_bills = Alice.utility_bills * (1 + 8.0 / 1200);
    Alice.mortgage_pay  = Alice.mortgage_pay * (1 + 8.0 / 1200);
    Alice.apartment     = Alice.apartment * (1 + 8.0 / 1200);

    Bob.income          = Bob.income * (1 + 8.0 / 1200);
    Bob.food            = Bob.food * (1 + 8.0 / 1200);
    Bob.trip            = Bob.trip * (1 + 8.0 / 1200);
    Bob.cartex          = Bob.cartex * (1 + 8.0 / 1200);
    Bob.car_price       = Bob.car_price * (1 + 8.0 / 1200);
    Bob.util_vznos      = Bob.util_vznos * (1 + 8.0 / 1200);
    Bob.car             = Bob.car * (1 + 8.0 / 1200);
    Bob.utility_bills   = Bob.utility_bills * (1 + 8.0 / 1200);
    Bob.rent            = Bob.rent * (1 + 8.0 / 1200);
}


void Alice_init()                                             // инфа про Элис
{
    Alice.bank_account  = 100 * 100;
    Alice.income        = 150 * 1000;
    Alice.food          = 30000;
    Alice.trip          = 80 * 1000;
    Alice.cartex        = 50 * 100;
    Alice.car           = 2000;
    Alice.utility_bills = 8000;
    Alice.mortgage_pay  = 60 * 1000;
    Alice.mortgage      = 200 * 1000;
    Alice.vznos         = 500 * 1000;
    Alice.apartment     = 2000 * 1000;
    Alice.car_price     = 1000 * 1000;
    Alice.deposit       = 0;
    Alice.util_vznos    = 10 * 1000;
}


void Bob_init()                                               // инфа про Боба
{
    Bob.bank_account  = 100 * 100;
    Bob.income        = 150 * 1000;
    Bob.food          = 35000;
    Bob.trip          = 90 * 1000;
    Bob.cartex        = 70 * 100;
    Bob.car           = 2000;
    Bob.utility_bills = 7000;
    Bob.rent          = 30 * 1000;
    Bob.vklad         = 50 * 100;
    Bob.car_price     = 1000 * 1000;
    Bob.deposit       = 0;
    Bob.util_vznos    = 15 * 1000;
}


void comparison()                                             // сравнение банковских счетов
{
    if (Bob.itog > Alice.itog) {
        std::cout << "Bob is living better\n";
    }
    else if (Alice.itog > Bob.itog) {
        std::cout << "Alice is living better\n";
    }
    else {
        std::cout << "Alice and Bob live equally well\n";
    }
}


void death()                                                  // проверка банковских счетов на уход в минус
{
    if (Bob.itog < 0) {
        std::cout << "Bob need more money\n";
    }
    else if (Alice.itog < 0) {
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
        Alice_mortgage();
        Alice_vznos(year, month);
        Alice_car_ptice();
        Alice_deposit_protsent();
        Alice_deposit();

        Bob_income(year, month);
        Bob_food();
        Bob_rent();
        Bob_utility_bills();
        Bob_car(month);
        Bob_trip(month);
        Bob_rent();
        Bob_vklad(year, month);
        Bob_car_ptice();
        Bob_deposit_protsent();
        Bob_deposit();

        inflation();

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

    Alice_itog();
    Bob_itog();

    print_Alice_info();
    print_Bob_info();

    comparison();

    death();

    return 0;
}
