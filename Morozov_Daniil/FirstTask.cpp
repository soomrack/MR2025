#include <stdio.h>
#include <math.h>

typedef long long int RUB;


struct Person {
    RUB bank_account;
    RUB income;
    RUB spending;
    RUB food;
    RUB mortgage;
    RUB car_price;
    RUB trip_price;
    RUB rent;
    RUB savings_account;
    RUB gasoline;
};


struct Person alice;
struct Person bob;

double inflation_rate = 0.1; // 10%-ая годовая инфляция


RUB calc_mortgage_payment(RUB principal, double annual_rate, int years) {
    int months_in_year = 12;
    int n = years * months_in_year;                        // срок в месяцах
    double r = annual_rate / (double) months_in_year / 100.0;     // месячная ставка

    // формула аннуитетного платежа
    double A = principal * (r * pow(1 + r, n)) / (pow(1 + r, n) - 1);

    return (RUB)A;
}


void alice_init() {
    alice.bank_account = 1000 * 1000;
    alice.savings_account = 0;
    alice.income = 130 * 1000;
    alice.food = 20 * 1000;
    alice.spending = 50 * 1000;  // Косметика, уходовые средства, одежда и др..
    alice.mortgage = calc_mortgage_payment(5 * 1000 * 1000, 10, 20); //5M, 10%, 20 years
    alice.car_price = 1 * 1000 * 1000;
    alice.trip_price = 200 * 1000;
    alice.gasoline = 5 * 1000;
}


void bob_init() {
    bob.bank_account = 500 * 1000;
    bob.savings_account = 0;
    bob.income = 140 * 1000;
    bob.food = 20 * 1000;
    bob.spending = 20 * 1000; // Зал, стейки, протеин, тестостерон
    bob.rent = 25 * 1000;
    bob.car_price = 700 * 1000;
    bob.trip_price = 100 * 1000;
    bob.gasoline = 7 * 1000;
}

void alice_print() {
    printf("Alice bank account = %lld руб.\n", alice.savings_account);
}


void bob_print() {
    printf("Bob bank account = %lld руб.\n", bob.savings_account);
}


void alice_income(const int year, const int month) {
    if (month == 10) {
        alice.income *= 1.05;  // Индексация
    }
    if (year == 2035 || year == 2040) {
        alice.income += 15000; // Повышение
    }
    alice.bank_account += alice.income;
}


void bob_income(const int year, const int month)  {
    if (month == 10) {
        bob.income *= 1.05;  // Индексация
    }
    if (year == 2035 || year == 2040) {
        bob.income += 10000; // Повышение
    }
    bob.bank_account += bob.income;
}


void alice_mortage() {
    alice.bank_account -= alice.mortgage;
}


void alice_spending() {
    alice.bank_account -= alice.spending;
}


void bob_spending() {
    bob.bank_account -= bob.spending;
}

void alice_food(const int year, const int month) {
    if (year == 2035 && month == 1) {
        alice.food *= 1.2;  // Продуктовый кризис
    }
    alice.bank_account -= alice.food;
}


void bob_food(const int year, const int month) {
    if (year == 2035 && month == 1) {
        bob.food *= 1.2;  // Продуктовый кризис
    }
    bob.bank_account -= bob.food;
}

void alice_transfer_money_to_savings_account() {
    if (alice.bank_account > 0) {
        alice.savings_account += alice.bank_account;
        alice.bank_account = 0;
    }
}

void bob_transfer_money_to_savings_account() {
    if (bob.bank_account > 0) {
        bob.savings_account += bob.bank_account;
        bob.bank_account = 0;
    }
}

void alice_accrual_to_savings_account() {
    alice.savings_account *= 1.0065;
}

void bob_accrual_to_savings_account() {
    bob.savings_account *= 1.0065;
}

void alice_car(const int year, const int month) {
    static bool can_buy_car;
    static bool must_repair_car;

    if (year % 5 == 0) {
            must_repair_car = true;
        }
    if (year == 2035) {
            can_buy_car = true;
        }   
    if (must_repair_car && alice.savings_account > alice.car_price * 0.05) {
            alice.savings_account -= alice.car_price * 0.05; // Ремонт машины
            must_repair_car = false;
    } else if (can_buy_car && alice.savings_account > (alice.car_price - alice.car_price * 0.75)){
        if (year == 2035) {
            alice.savings_account -= (alice.car_price - alice.car_price * 0.75);  // Покупка новой машины c учетом продажи старой
            can_buy_car = false;
        }
    } 
    if (!must_repair_car) {
        alice.savings_account -= alice.gasoline;
    } else {
        alice.savings_account -= alice.gasoline / 3; // Поездки на общественном транспорте
    }

    if (month == 1) {
        alice.gasoline *= 1.01; // Рост цен на бензин
        alice.car_price *= 1.03; // Рост цен на машину
    }
}


void bob_car(const int year, const int month) {
    static bool can_buy_car;
    static bool must_repair_car;

    if (year % 5 == 0) {
            must_repair_car = true;
        }
    if (year == 2038) {
            can_buy_car = true;
        }   
    if (must_repair_car && bob.savings_account > bob.car_price * 0.05) {
            bob.savings_account -= bob.car_price * 0.04; // Ремонт машины
            must_repair_car = false;
    } else if (can_buy_car && bob.savings_account > (bob.car_price - bob.car_price * 0.75)) {
        if (year == 2035) {
            bob.savings_account -= (bob.car_price - bob.car_price * 0.8);  // Покупка новой машины c учетом продажи старой
            can_buy_car = false;
        }
    } 
    if (!must_repair_car) {
        bob.savings_account -= bob.gasoline;
    } else {
        bob.savings_account -= bob.gasoline / 3; // Поездки на общественном транспорте
    }

    if (month == 1) {
        bob.gasoline *= 1.01; // Рост цен на бензин
        bob.car_price *= 1.03; // Рост цен на машину
    }
}


void alice_trip(const int year, const int month) {
    if (month == 1 && alice.savings_account > alice.trip_price) {
        alice.savings_account -= alice.trip_price;
    }
    if (month == 1) {
        bob.trip_price *= 1.05; // Инфляция
    }
}


void bob_trip(const int year, const int month) {
    if (month % 6 == 0 && bob.savings_account > bob.trip_price) {
        bob.savings_account -= bob.trip_price;
    }
    if (month == 1) {
        bob.trip_price *= 1.05; // Инфляция
    }
}


void bob_rent(const int year) {
    bob.bank_account -= bob.rent;
    if (year == 2035) {
        bob.rent *= 1.2; // Рост цен на аренду жилья
    }
}


void comparision() {
    if (alice.bank_account > bob.bank_account) {
        printf("Alice has more money than Bob.\n");
    } else if (alice.bank_account < bob.bank_account) {
        printf("Bob has more money than Alice.\n");
    } else {
        printf("Alice and Bob have the same amount of money.\n");
    }
}


void simulation() {
    int year = 2025;
    int month = 9;

    while( !(year == 2030 && month == 9)){

        alice_income(year, month);
        alice_mortage();
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

        // printf ("Alice savings account = %lld \n", alice.savings_account);
        // printf ("Bob savings account = %lld \n", bob.savings_account);
   
    }

}


int main() {
    alice_init();
    bob_init();

    simulation();

    bob_print();
    alice_print();

    comparision();

}
