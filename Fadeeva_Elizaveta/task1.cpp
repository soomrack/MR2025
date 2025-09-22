#include <iostream>

using RUB = long int;

struct Person {
    RUB bank_account;
    RUB income;
    RUB food;
    RUB trip;
    RUB cartex;
    RUB car;
    RUB utility_bills;
    RUB rent;
    RUB mortgage;
    RUB vznos;
    RUB vklad;
};

Person alice;

void alice_income(const int year, const int month)                     // зарплата Ёлис и преми€
{
    if (year == 2030 && month == 3) {
        alice.income = static_cast<int>(alice.income * 1.5);  
    }

    alice.bank_account += alice.income;
}

void alice_vznos(const int year, const int month)                     // первоначальный взнос
{
    if (year == 2025 && month == 9) {
        alice.bank_account += alice.vznos;
    }
}

void alice_food()                                               // траты на еду
{
    alice.bank_account -= alice.food;
}

void alice_trip(const int month)                                // траты на путешествие
{
    if (month == 8) {
        alice.bank_account -= alice.trip;
    }
}

void alice_car()                                                // траты на бензин
{
    alice.bank_account -= alice.car;
}

void alice_cartex(const int month)                              // траты на тех обслуживание машины
{
    if (month == 1) {
        alice.bank_account -= alice.cartex;
    }
}

void alice_mortgage()                                           // выплаты по ипотеке
{
    alice.bank_account -= alice.mortgage;
}

void print_alice_info()                                         // вывод капиатала Ёлис
{
    std::cout << "Alice capital = " << alice.bank_account << " RUR\n";
}

void alice_utility_bills()                                      // коммунальные платежи
{
    alice.bank_account -= alice.utility_bills;
}

Person Bob;

void Bob_income(const int year, const int month)                // зарплата Ѕоба и преми€
{
    if (year == 2030 && month == 3) {
        Bob.income = static_cast<int>(Bob.income * 1.5);
    }

    Bob.bank_account += Bob.income;
}

void Bob_food()                                                 // траты на еду
{
    Bob.bank_account -= Bob.food;
}

void Bob_trip(const int month)                                // траты на путешествие
{
    if (month == 8) {
        Bob.bank_account -= Bob.trip;
    }
}

void Bob_car()                                                // траты на бензин
{
    Bob.bank_account -= Bob.car;
}

void Bob_cartex(const int month)                              // траты на тех обслуживание машины
{
    if (month == 1) {
    Bob.bank_account -= Bob.cartex;
    }
}

void Bob_utility_bills()                                      // коммунальные платежи
{
    Bob.bank_account -= Bob.utility_bills;
}

void Bob_rent()                                              // выплаты аренды
{
    Bob.bank_account -= Bob.rent;
}

void Bob_vklad(const int year, const int month)              // вклад Ѕоба
{
    Bob.vklad = static_cast<int>(Bob.vklad * 1.02);

    if (year == 2045 && month == 8)
    {
        Bob.vklad = static_cast<int>(Bob.vklad * 1.02);
        Bob.bank_account += Bob.vklad;
    }
}

void print_Bob_info()                                        // вывод капиатала Ѕоба
{
    std::cout << "Bob capital = " << Bob.bank_account << " RUR\n";
}

void inflation()                                             // расчет инфл€ции ежемес€чной
{
    alice.income = static_cast<RUB>(alice.income * 1.0067);
    alice.food = static_cast<RUB>(alice.food * 1.0067);
    alice.trip = static_cast<RUB>(alice.trip * 1.0067);
    alice.cartex = static_cast<RUB>(alice.cartex * 1.0067);
    alice.car = static_cast<RUB>(alice.car * 1.0067);
    alice.utility_bills = static_cast<RUB>(alice.utility_bills * 1.0067);
    alice.mortgage = static_cast<RUB>(alice.mortgage * 1.0067);

    Bob.income = static_cast<RUB>(Bob.income * 1.0067);
    Bob.food = static_cast<RUB>(Bob.food * 1.0067);
    Bob.trip = static_cast<RUB>(Bob.trip * 1.0067);
    Bob.cartex = static_cast<RUB>(Bob.cartex * 1.0067);
    Bob.car = static_cast<RUB>(Bob.car * 1.0067);
    Bob.utility_bills = static_cast<RUB>(Bob.utility_bills * 1.0067);
    Bob.rent = static_cast<RUB>(Bob.rent * 1.0067);
}

void alice_init()                                            // инфа про Ёлис
{
    alice.bank_account = 1000 * 100;
    alice.income = 150 * 1000;
    alice.food = 30000;
    alice.trip = 80 * 1000;
    alice.cartex = 50 * 100;
    alice.car = 2000;
    alice.utility_bills = 8000;
    alice.mortgage = 60 * 1000;
    alice.vznos = 70 * 1000;
}

void Bob_init()                                              // инфа про Ѕоба
{
    Bob.bank_account = 1000 * 100;
    Bob.income = 150 * 1000;
    Bob.food = 35000;
    Bob.trip = 90 * 1000;
    Bob.cartex = 70 * 100;
    Bob.car = 2000;
    Bob.utility_bills = 7000;
    Bob.rent = 30 * 1000;
    Bob.vklad = 50 * 000;
}

void comparison()                                           // сравнение банковских счетов
{
    if (Bob.bank_account > alice.bank_account) {
        std::cout << "Bob is living better\n";
    }
    else if (alice.bank_account > Bob.bank_account) {
        std::cout << "Alice is living better\n";
    }
    else {
        std::cout << "Alice and Bob live equally well\n";
    }
}

void death()                                                 // проверка банковских счетов на уход в минус
{
    if (Bob.bank_account < 0) {
        std::cout << "Bob need more money\n";
    }
    else if (alice.bank_account < 0) {
        std::cout << "Alice need more money\n";
    }
    else {
        std::cout << "Alice and Bob have enough money to live on\n";
    }
}

void simulation()                                           // симул€ци€
{
    int year = 2025;
    int month = 9;

    while (!(year == 2045 && month == 9)) {
        alice_income(year, month);
        alice_food();
        alice_mortgage();
        alice_utility_bills();
        alice_car();
        alice_cartex(month);
        alice_trip(month);
        alice_mortgage();
        alice_vznos(year, month);

        Bob_income(year, month);
        Bob_food();
        Bob_rent();
        Bob_utility_bills();
        Bob_car();
        Bob_cartex(month);
        Bob_trip(month);
        Bob_rent();
        Bob_vklad(year, month);

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
    alice_init();

    Bob_init();

    simulation();

    print_alice_info();

    print_Bob_info();

    comparison();

    death();

    return 0;
}