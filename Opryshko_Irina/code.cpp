#include <stdio.h>
#include <windows.h>
#include <iostream>
#include <cmath>
using namespace std;

typedef long long int RUB;

struct Person {
    RUB bank_account;
    RUB income;
    RUB mortage;
    RUB mortgage_balance;     // Остаток долга по ипотеке
    double mortgage_rate;     // Годовая процентная ставка
    RUB investment_acc;
    RUB monthly_payment;
    RUB costs; 
    RUB food;
    RUB cloth;
    RUB pet;
    RUB monthly_investment;
    RUB house;
};

struct car {
    RUB cost;
    RUB payment;
    RUB TO;
    RUB petrol;
    RUB credit;
    bool purchased; // пометка машина уже куплена
};

struct Person alice;
struct Person bob;
struct car alice_car;
struct car bob_car;

void alice_init() {
    alice.bank_account = 2000000;
    cout << "Начальный счет Алис: " << alice.bank_account << "\n";

    alice.income = 200000;
    cout << "Ежемесячный доход Алис: " << alice.income << "\n";

    alice.mortage = 10000000;
    cout << "Сумма ипотеки Алис: " << alice.mortage << "\n";

    alice.monthly_payment = 50000;
    alice.mortgage_balance = alice.mortage - alice.bank_account;
    alice.bank_account = 0;
    alice.investment_acc = 0;

    cout << "Ежемесячный платеж по ипотеке: " << alice.monthly_payment << " руб.\n";
}

void bob_init() {
    bob.bank_account = 2000000;
    cout << "Начальный счет Боба: " << bob.bank_account << "\n";

    bob.income = 200000;
    cout << "Ежемесячный доход Боба: " << bob.income << "\n";
    bob.investment_acc = 0;
    bob.house = 10000000;

    // Инициализация машины Боба
    bob_car.purchased = false;
    bob_car.credit = 0;
}

void alice_print() {
    cout << "Банковский счет Алис = " << alice.bank_account << " руб.\n";
    cout << "ЗП Алис к концу периода = " << alice.income << " руб.\n";
    cout << "Инвестиционный счет Алис = " << alice.investment_acc << " руб.\n";
    if (alice.mortgage_balance > 0) {
        cout << "Остаток по ипотеке: " << alice.mortgage_balance << " руб.\n";
    }
    else {
        cout << "Ипотека полностью погашена\n";
    }
}

void bob_print() {
    cout << "Банковский счет Боба = " << bob.bank_account << " руб.\n";
    cout << "ЗП Боба к концу периода = " << bob.income << " руб.\n";
    cout << "Инвестиционный счет Боба = " << bob.investment_acc << " руб.\n";
}

void alice_income(const int year, const int month) {
    if ((year == 2030 && month == 10)
        || (year == 2035 && month == 1)
        || (year == 2040 && month == 1)) {
        alice.income = alice.income * 1.5; // Повышение
    }
    alice.bank_account += alice.income;
}

void alice_mortage(const int year, const int month) {
    if (alice.mortgage_balance <= 0) {
        return;
    }

    RUB payment = alice.monthly_payment;

    // Если остаток меньше платежа, платим только остаток
    if (alice.mortgage_balance < payment) {
        payment = alice.mortgage_balance;
    }

    alice.bank_account -= payment;
    alice.mortgage_balance -= payment;

    if (alice.mortgage_balance <= 0) {
        cout << "Ипотека Алисы погашена в " << year << " году, месяц " << month << "\n";
    }
}

void alice_car_ownership(int month) {
    
    if (!alice_car.purchased && alice.bank_account >= 2000000) {
        alice_car.credit = 1000000;
        alice_car.purchased = true;
        alice.bank_account -= 200000; // Первоначальный взнос
    }

    
    if (alice_car.purchased) {
        alice_car.petrol = 10000;
        alice_car.payment = 15000;

       
        if (month == 5) {
            alice_car.TO = 30000;
        }
        else {
            alice_car.TO = 0;
        }

        alice_car.cost = alice_car.petrol + alice_car.TO;

        
        if (alice_car.credit > 0) {
            alice_car.cost += alice_car.payment;
            alice_car.credit -= alice_car.payment;
        }

        alice.bank_account -= alice_car.cost;
    }
}

void alice_costs(const int month) {
    
    if (month == 1) {
        alice.food *= 1.05;  
        alice.pet *= 1.05;
        alice.cloth *= 1.05;
    }

    alice.costs = alice.food + alice.pet + alice.cloth;
    alice.bank_account -= alice.costs;
}

void alice_investment(const int month) {
    alice.monthly_investment = alice.income * 0.1;

    // Инвестируем только если есть свободные средства
    if (alice.bank_account >= alice.monthly_investment) {
        alice.bank_account -= alice.monthly_investment;
        alice.investment_acc += alice.monthly_investment;
    }

    if (month == 1) {
        RUB investment_income = alice.investment_acc * 0.15;
        alice.investment_acc += investment_income;
    }
}

void bob_income(const int year, const int month) {
    if ((year == 2030 && month == 1)
        || (year == 2035 && month == 1)
        || (year == 2040 && month == 1)) {
        bob.income = bob.income * 1.5; // Повышение
    }
    bob.bank_account += bob.income;
}

void bob_rent(const int year, const int month) {
    static RUB rent_payment = 60000; // static для сохранения значения между вызовами

    
    if (month == 1 && year >= 2027 && (year - 2027) % 2 == 0) {
        rent_payment *= 1.1;
    }

    bob.bank_account -= rent_payment;
}

void bob_costs(const int month) {
    
    if (month == 1) {
        bob.food *= 1.05;  
        bob.pet *= 1.05;
        bob.cloth *= 1.05;
    }

    bob.costs = bob.food + bob.pet + bob.cloth;
    bob.bank_account -= bob.costs;
}

void bob_car_ownership(int month) {
    
    if (!bob_car.purchased && bob.bank_account >= 2000000) {
        bob_car.credit = 1000000;
        bob_car.purchased = true;
        bob.bank_account -= 200000; // Первоначальный взнос
    }

    
    if (bob_car.purchased) {
        bob_car.petrol = 10000;
        bob_car.payment = 15000;

        
        if (month == 5) {
            bob_car.TO = 30000;
        }
        else {
            bob_car.TO = 0;
        }

        bob_car.cost = bob_car.petrol + bob_car.TO;

       
        if (bob_car.credit > 0) {
            bob_car.cost += bob_car.payment;
            bob_car.credit -= bob_car.payment;
        }

        bob.bank_account -= bob_car.cost;
    }
}

void bob_investment(const int month) {
    bob.monthly_investment = bob.income * 0.1;

    // Инвестируем только если есть свободные средства
    if (bob.bank_account >= bob.monthly_investment) {
        bob.bank_account -= bob.monthly_investment;
        bob.investment_acc += bob.monthly_investment;
    }

    if (month == 1) {
        RUB investment_income = bob.investment_acc * 0.15;
        bob.investment_acc += investment_income;
    }
}

void bob_house(int year, int month) {
    static bool house_purchased = false;

    if (!house_purchased && bob.bank_account >= bob.house) { 
        bob.bank_account -= bob.house;
        house_purchased = true;
        cout << "Боб купил дом в " << year << " году, месяц " << month << "\n";
    }
}

void simulation() {
    int year = 2025;
    int month = 9;

    alice.food = 20000;
    alice.pet = 6000;
    alice.cloth = 20000;

    bob.food = 20000;
    bob.pet = 6000;
    bob.cloth = 20000;

    // Инициализация машин
    alice_car.purchased = false;
    bob_car.purchased = false;

    while (!(year == 2045 && month == 9)) {
        
        alice_income(year, month);
        bob_income(year, month);

       
        alice_mortage(year, month);
        alice_car_ownership(month);
        alice_costs(month);
        alice_investment(month);

     
        bob_rent(year, month);
        bob_car_ownership(month);
        bob_costs(month);
        bob_investment(month);
        bob_house(year, month);

        month++;
        if (month == 13) {
            year++;
            month = 1;
        }
    }
}

int main() {
    SetConsoleCP(1251);
    SetConsoleOutputCP(1251);

    alice_init();
    bob_init();

    simulation();

    alice_print();
    bob_print();

    return 0;
}
