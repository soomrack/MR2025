#include <stdio.h>
#include <math.h>

typedef long long int RUB;

struct Person {
    RUB bank_account;
    RUB income;

    RUB food;
    RUB clothes;
    RUB utilities;
    RUB rent;

    RUB car_expenses;     // машина (Bob)
    RUB repairs_expenses;
    // Собственность
    RUB flat_price;

    // Займ (ипотека или кредит)
    RUB loan;             
    int loan_years;       
    double loan_rate;     
    RUB monthly_payment; 
    int loan_months_left; 
};

Person alice;
Person bob;


// Блок 1 Константы экономики

const double INFLATION_RATE   = 0.05; 


// Блок 2 Функции расчётов 
// amount      — сумма займа (основной долг)
// annual_rate — годовая процентная ставка  
// years       — срок кредита в годах
// Возвращает фиксированный ежемесячный платёж 

// Расчёт аннуитетного платежа
RUB annuity_payment(RUB amount, double annual_rate, int years) { 
    int total_months = years * 12;  // всего месяцев
    if (annual_rate <= 0.0) { 
        return amount / total_months; // если ставка 0% - равномерное деление суммы
    }
    double r       = annual_rate / 12.0; // месячная ставка
    double powf    = pow(1.0 + r, total_months);  // (1 + r)^n
    double payment = amount * (r * powf) / (powf - 1.0);//P = S * (r * (1 + r)^n) / ((1 + r)^n - 1)
    return (RUB) payment;  // округление вниз до целого RUB
}

//Блок 3 Инициализация персонажей

void alice_init() {
    alice.bank_account      = 1000 * 1000;
    alice.income            = 80 * 1000;

    alice.food              = 40 * 1000;
    alice.clothes           = 20 * 1000;
    alice.utilities         = 1 * 1000;
    alice.rent              = 0;

    alice.repairs_expenses  = 20 * 1000;

    alice.flat_price        = 6 * 1000 * 1000;

    alice.loan              = alice.flat_price - 1 * 1000 * 1000; 
    alice.loan_years        = 20;
    alice.loan_rate         = 0.05;
    alice.monthly_payment   = annuity_payment(alice.loan, alice.loan_rate, alice.loan_years);
    alice.loan_months_left  = alice.loan_years * 12;

    alice.bank_account     -= 1 * 1000 * 1000; // первый взнос
}

void bob_init() {
    bob.bank_account      = 1000 * 1000;
    bob.income            = 80 * 1000;

    bob.food              = 20 * 1000;
    bob.clothes           = 10 * 1000;
    bob.utilities         = 15 * 1000;
    bob.rent              = 20 * 1000;

    bob.car_expenses      = 15 * 1000; // расходы на машину (ежемесячно)

    bob.flat_price        = 0;

    bob.loan              = 1 * 1000 * 1000; 
    bob.loan_years        = 20;
    bob.loan_rate         = 0.05;
    bob.monthly_payment   = annuity_payment(bob.loan, bob.loan_rate, bob.loan_years);
    bob.loan_months_left  = bob.loan_years * 12;
}


// Блок 4 Доходы


void alice_income(const int year, const int month) {
    if (month == 9) {
        alice.income *= 1.07; // индексация раз в год
    }
    if (year == 2030 && month == 10) {
        alice.income *= 1.5; 
    }
    alice.bank_account += alice.income;
}

void bob_income(const int year, const int month) {
    if (month == 9) {
        bob.income *= 1.07;
    }
    if (year == 2032 && month == 5) {
        bob.income *= 1.5;
    }
    bob.bank_account += bob.income;
}


// Блок 5 Расходы


void pay_food(Person &p) {
    p.bank_account -= p.food;
    p.food *= (1 + INFLATION_RATE/12.0);
}

void pay_clothes(Person &p) {
    p.bank_account -= p.clothes;
    p.clothes *= (1 + INFLATION_RATE/12.0);
}

void pay_utilities(Person &p) {
    p.bank_account -= p.utilities;
    p.utilities *= (1 + INFLATION_RATE/12.0);
}

void pay_rent(Person &p) {
    p.bank_account -= p.rent;
    p.rent *= (1 + INFLATION_RATE/12.0);
}

void pay_car(Person &p) {
    if (p.car_expenses > 0) {
        p.bank_account -= p.car_expenses;
        p.car_expenses *= (1 + INFLATION_RATE/12.0);
    }
}

void pay_loan(Person &p) {
    if (p.loan_months_left > 0) {
        p.bank_account -= p.monthly_payment;
        p.loan_months_left--;
    }
}


void pay_repairs(Person &p, int year, int month) {
    const int START_YEAR = 2030;
    const int END_YEAR   = 2033; 
    if (year >= START_YEAR && year < END_YEAR) {
        p.bank_account -= p.repairs_expenses;
        p.repairs_expenses *= (1 + INFLATION_RATE/12.0);
    }
}


// Блок 7 Функции вывода

void results() {
    RUB alice_total = alice.bank_account + alice.flat_price;
    RUB bob_total   = bob.bank_account + bob.flat_price; // если появятся активы у Bob( и чтобы показать что их нет)

    printf("\n=== Итог через 20 лет ===\n");
    printf("Alice bank account = %lld руб. (+ квартира %lld)\n", 
           alice.bank_account, alice.flat_price);
    printf("Bob   bank account = %lld руб. (+ собственность %lld)\n", 
           bob.bank_account, bob.flat_price);

    if (alice_total > bob_total) {
        printf("Жизнь лучше у Alice.\n");
    } else if (bob_total > alice_total) {
        printf("Жизнь лучше у Bob.\n");
    } else {
        printf("Жизнь одинаковая.\n");
    }
}

// Блок 8 Симуляция 

void simulation() {
    int year = 2025;
    int month = 9;

    while (!(year == 2045 && month == 9)) {
        
        alice_income(year, month);
        
        pay_food(alice);
        pay_clothes(alice);
        pay_utilities(alice);
        pay_car(alice);
        pay_repairs(alice, year, month);
        pay_loan(alice);


        bob_income(year, month);

        pay_food(bob);
        pay_clothes(bob);
        pay_utilities(bob);
        pay_rent(bob);
        pay_car(bob);
        pay_loan(bob);

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

    results();

    return 0;
}
