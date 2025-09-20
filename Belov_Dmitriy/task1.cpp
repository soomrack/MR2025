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

    // Собственность
    RUB flat_price; // у Alice — квартира, у Bob — 0 (пока)

    // Займ (ипотека или кредит)
    RUB loan;             
    int loan_years;       
    double loan_rate;     
    RUB monthly_payment; 
    int loan_months_left; // срок в месяцах 
};

Person alice;
Person bob;


// Блок 1 Константы экономики

const double INFLATION_RATE   = 0.05; // 5%
const int    INFLATION_PERIOD = 5;    // каждые 5 лет


// Блок 2 Функции расчётов 

// Расчёт аннуитетного платежа
RUB annuity_payment(RUB amount, double annual_rate, int years) {
    int total_months = years * 12;
    if (annual_rate <= 0.0) {
        return amount / total_months; // без процентов
    }
    double r = annual_rate / 12.0;
    double powf = pow(1.0 + r, total_months);
    double payment = amount * (r * powf) / (powf - 1.0);
    return (RUB) payment;
}

//Блок 3 Инициализация персонажей

void alice_init() {
    alice.bank_account = 1000 * 1000;
    alice.income       = 200 * 1000;

    alice.food      = 40 * 1000;
    alice.clothes   = 20 * 1000;
    alice.utilities = 1 * 1000;
    alice.rent      = 0; // живёт в своей квартире
    
    alice.flat_price = 6 * 1000 * 1000;

    alice.loan       = alice.flat_price - 1 * 1000 * 1000; 
    alice.loan_years = 20;
    alice.loan_rate  = 0.05;
    alice.monthly_payment = annuity_payment(alice.loan, alice.loan_rate, alice.loan_years);
    alice.loan_months_left = alice.loan_years * 12;

    alice.bank_account -= 1 * 1000 * 1000; // первый взнос
}

void bob_init() {
    bob.bank_account = 800 * 1000;
    bob.income       = 200 * 1000;

    bob.food      = 20 * 1000;
    bob.clothes   = 10 * 1000;
    bob.utilities = 15 * 1000;
    bob.rent      = 20 * 1000; // аренда

    bob.flat_price = 0; // квартиры нет

    bob.loan       =    1 * 1000 * 1000; // кредит (например, на машину)
    bob.loan_years = 20;
    bob.loan_rate  = 0.05;
    bob.monthly_payment = annuity_payment(bob.loan, bob.loan_rate, bob.loan_years);
    bob.loan_months_left = bob.loan_years * 12;
}



// Блок 4 Доходы

void alice_income(const int year, const int month) {
    if (year == 2030 && month == 10) {
        alice.income *= 1.5; 
    }
    alice.bank_account += alice.income;
}

void bob_income(const int year, const int month) {
    if (year == 2032 && month == 5) {
        bob.income *= 1.3;
    }
    bob.bank_account += bob.income;
}


// Блок 5 Расходы


void monthly_expenses(Person *p) {
    p->bank_account -= (p->food + p->clothes + p->utilities);
}

void pay_rent(Person *p) {
    p->bank_account -= p->rent;
}
void pay_loan(Person *p) {
    if (p->loan_months_left > 0) {
        p->bank_account -= p->monthly_payment;
        p->loan_months_left--;
        if (p->loan_months_left == 0) {
            p->loan = 0;
            p->monthly_payment = 0;
        }
    }
}


// Блок 6 Инфляция 

void apply_inflation(Person *p) {
    p->food      = (RUB)(p->food * (1 + INFLATION_RATE));
    p->clothes   = (RUB)(p->clothes * (1 + INFLATION_RATE));
    p->utilities = (RUB)(p->utilities * (1 + INFLATION_RATE));
    p->rent      = (RUB)(p->rent * (1 + INFLATION_RATE));
}


// Блок 7 Функции вывода

void results() {
    RUB alice_total = alice.bank_account + alice.flat_price;
    RUB bob_total   = bob.bank_account + bob.flat_price; // если появятся активы у Bob

    printf("\n=== Итог через 20 лет ===\n");
    printf("Alice bank account = %lld руб. (+ квартира %lld)\n", alice.bank_account, alice.flat_price);
    printf("Bob   bank account = %lld руб. (+ собственность %lld)\n", bob.bank_account, bob.flat_price);

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

        // доходы
        alice_income(year, month);
        bob_income(year, month);

        // расходы
        monthly_expenses(&alice);
        monthly_expenses(&bob);

        // аренда (только у Bob ненулевая)
        pay_rent(&bob);

        // ипотека/кредит
        pay_loan(&alice);
        pay_loan(&bob);
        
        // инфляция каждые 5 лет
        if (month == 12 && year % INFLATION_PERIOD == 0) {
            apply_inflation(&alice);
            apply_inflation(&bob);
        }
        // шаг времени
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
