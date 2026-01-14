#include <stdio.h>
#include <math.h>
#include <stdbool.h>

typedef long long int RUB;

struct Person {
    RUB bank_account; // банковский счет
    RUB income; // доход
    RUB spending; // расхлды
    RUB food; // еда
    RUB mortgage; // ипотека
    RUB car_price; // стоимость машины
    RUB trip_price; // стоимоть путишествия
    RUB rent; // аренда
    RUB savings_account; // сберегательный счет
    RUB gasoline; // бензин
};

struct Person alice;
struct Person bob;

RUB calc_mortgage_payment(RUB principal, double annual_rate, int years) {
    int months_in_year = 12;
    int n = years * months_in_year; //срок в месяцах
    double r = annual_rate / (double)months_in_year / 100.0; // месячная ставка

    double A = principal * (r * pow(1 + r, n)) / (pow(1 + r, n) - 1); //формула аннуитетного платежа
    return (RUB)A;
}

void alice_init() {
    alice.bank_account = 1000 * 1000;
    alice.savings_account = 0;
    alice.income = 130 * 1000;
    alice.food = 20 * 1000;
    alice.spending = 50 * 1000;
    alice.mortgage = calc_mortgage_payment(5 * 1000 * 1000, 10, 20);
    alice.car_price = 1 * 1000 * 1000;
    alice.trip_price = 200 * 1000;
    alice.gasoline = 5 * 1000;
}

void bob_init() {
    bob.bank_account = 500 * 1000;
    bob.savings_account = 0;
    bob.income = 140 * 1000;
    bob.food = 20 * 1000;
    bob.spending = 20 * 1000;
    bob.rent = 25 * 1000;
    bob.car_price = 700 * 1000;
    bob.trip_price = 100 * 1000;
    bob.gasoline = 7 * 1000;
}

void alice_print() {
    printf("Alice savings account = %lld rub.\n", alice.savings_account);
}

void bob_print() {
    printf("Bob savings account = %lld rub.\n", bob.savings_account);
}

void alice_income(const int year, const int month) {
    if (month == 10) {
        alice.income = (RUB)(alice.income * 1.05); // ежегодная индексация
    }
    if (year == 2035 || year == 2040) {
        alice.income += 15000; // прибавка к зарплате 
    }
    alice.bank_account += alice.income; // ежемесячная зарплата 
}

void bob_income(const int year, const int month) {
    if (month == 10) {
        bob.income = (RUB)(bob.income * 1.05); // ежегодная индексация
    }
    if (year == 2035 || year == 2040) {
        bob.income += 10000; // прибавка к зарплате 
    }
    bob.bank_account += bob.income; // ежемесячная зарплата
}

void alice_mortgage() {
    alice.bank_account -= alice.mortgage; // платеж по ипотеке 
}

void alice_spending() {
    alice.bank_account -= alice.spending; // прочие расходы 
}

void bob_spending() {
    bob.bank_account -= bob.spending; // прочие расходы
}

void alice_food(const int year, const int month) {
    if (year == 2035 && month == 1) {
        alice.food = (RUB)(alice.food * 1.2); // раст цен на продукты
    }
    alice.bank_account -= alice.food; // расходы на еду 
}

void bob_food(const int year, const int month) {
    if (year == 2035 && month == 1) {
        bob.food = (RUB)(bob.food * 1.2); // рост цен на продукты 
    }
    bob.bank_account -= bob.food; //расходы на еду 
}

void alice_transfer_money_to_savings_account() {
    if (alice.bank_account > 0) { // обноружение остатка на банковском счету
        alice.savings_account += alice.bank_account; // перевод денег на сберегательный счет
        alice.bank_account = 0;  
    }
}

void bob_transfer_money_to_savings_account() {
    if (bob.bank_account > 0) { // обноружение остатка на банковском счету
        bob.savings_account += bob.bank_account; // перевод денег на сберегательный счет
        bob.bank_account = 0;
    }
}

void alice_accrual_to_savings_account() {
    alice.savings_account = (RUB)(alice.savings_account * 1.0065); // начисление процентов
}

void bob_accrual_to_savings_account() {
    bob.savings_account = (RUB)(bob.savings_account * 1.0065); // начисление процентов
}

void alice_car(const int year, const int month) {
    static bool can_buy_car = false;
    static bool must_repair_car = false;

    if (year % 5 == 0 && month == 1) {
        must_repair_car = true;
    }
    if (year == 2035 && month == 1) {
        can_buy_car = true;
    }

    if (must_repair_car && alice.savings_account > alice.car_price * 0.05) {
        alice.savings_account -= (RUB)(alice.car_price * 0.05);
        must_repair_car = false;
    }
    else if (can_buy_car && alice.savings_account > (alice.car_price - alice.car_price * 0.75)) {
        alice.savings_account -= (RUB)(alice.car_price * 0.25);
        can_buy_car = false;
    }

    if (!must_repair_car) {
        alice.savings_account -= alice.gasoline;
    }
    else {
        alice.savings_account -= alice.gasoline / 3;
    }

    if (month == 1) {
        alice.gasoline = (RUB)(alice.gasoline * 1.01);
        alice.car_price = (RUB)(alice.car_price * 1.03);
    }
}

void bob_car(const int year, const int month) {
    static bool can_buy_car = false;
    static bool must_repair_car = false;

    if (year % 5 == 0 && month == 1) {
        must_repair_car = true;
    }
    if (year == 2038 && month == 1) {
        can_buy_car = true;
    }

    if (must_repair_car && bob.savings_account > bob.car_price * 0.05) {
        bob.savings_account -= (RUB)(bob.car_price * 0.04);
        must_repair_car = false;
    }
    else if (can_buy_car && bob.savings_account > (bob.car_price - bob.car_price * 0.75)) {
        if (year == 2038) {
            bob.savings_account -= (RUB)(bob.car_price * 0.2);
            can_buy_car = false;
        }
    }

    if (!must_repair_car) {
        bob.savings_account -= bob.gasoline;
    }
    else {
        bob.savings_account -= bob.gasoline / 3;
    }

    if (month == 1) {
        bob.gasoline = (RUB)(bob.gasoline * 1.01);
        bob.car_price = (RUB)(bob.car_price * 1.03);
    }
}

void alice_trip(const int year, const int month) { // отдых 
    if (month == 1 && alice.savings_account > alice.trip_price) {
        alice.savings_account -= alice.trip_price;
    }
    if (month == 1) {
        alice.trip_price = (RUB)(alice.trip_price * 1.05);
    }
}

void bob_trip(const int year, const int month) { //отдых
    if (month % 6 == 0 && bob.savings_account > bob.trip_price) {
        bob.savings_account -= bob.trip_price;
    }
    if (month == 1) {
        bob.trip_price = (RUB)(bob.trip_price * 1.05);
    }
}

void bob_rent(const int year) { // аренда жилья 
    bob.bank_account -= bob.rent; //
    if (year == 2035) {
        bob.rent = (RUB)(bob.rent * 1.2);
    }
}

void comparison() {
    RUB alice_total = alice.savings_account + alice.car_price + (RUB)(5000000 * 0.87); // 87% от 5M ипотеки как актива // благосостояние
    RUB bob_total = bob.savings_account + bob.car_price; // благосостояние

    printf("\nComparison:\n");
    printf("Alice total wealth: %lld rub.\n", alice_total);
    printf("Bob total wealth: %lld rub.\n", bob_total);

    if (alice_total > bob_total) {
        printf("Alice has more money than Bob.\n");
    }
    else if (alice_total < bob_total) {
        printf("Bob has more money than Alice.\n");
    }
    else {
        printf("Alice and Bob have equal money.\n");
    }
}

void simulation() {
    int year = 2025;
    int month = 9;

    while (!(year == 2045 && month == 9)) {
        alice_income(year, month);
        alice_mortgage();
        alice_spending();
        alice_food(year, month);
        alice_transfer_money_to_savings_account();
        alice_car(year, month);
        alice_trip(year, month);
        alice_accrual_to_savings_account();

        bob_income(year, month);
        bob_spending();
        bob_food(year, month);
        bob_rent(year);
        bob_transfer_money_to_savings_account();
        bob_car(year, month);
        bob_trip(year, month);
        bob_accrual_to_savings_account();

        month++;
        if (month == 13) {
            year++;
            month = 1;
        }
    }
}

int main() {
    alice_init();
    bob_init();

    simulation();

    bob_print();
    alice_print();

    comparison();

    return 0;
}