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
};

Person alice;
Person bob;


// Блок 1 Функции расчётов 


// Расчёт аннуитетного платежа
RUB annuity_payment(RUB amount, double annual_rate, int years) {
    double monthly_rate = annual_rate / 12.0;
    int total_months = years * 12;
    double numerator   = monthly_rate * pow(1 + monthly_rate, total_months);
    double denominator = pow(1 + monthly_rate, total_months) - 1;
    return (RUB)(amount * (numerator / denominator));
}

//Блок 2 Инициализация персонажей

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
    alice.loan_rate  = 0.18;
    alice.monthly_payment = annuity_payment(alice.loan, alice.loan_rate, alice.loan_years);

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

    bob.loan       = 1 * 1000 * 1000; // кредит (возможно на машину TODO)
    bob.loan_years = 10;
    bob.loan_rate  = 0.5;
    bob.monthly_payment = annuity_payment(bob.loan, bob.loan_rate, bob.loan_years);
}



// Блок 3 Доходы

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


// Блок 4 Расходы


void monthly_expenses(Person *p) {
    p->bank_account -= (p->food + p->clothes + p->utilities);
}

void pay_rent(Person *p) {
    p->bank_account -= p->rent;
}
void pay_loan(Person *p) {
    if (p->loan_years > 0) {
        p->bank_account -= p->monthly_payment;
    }
}


// Блок 5 Инфляция 

void apply_inflation(Person *p) {
    const double INFLATION_RATE = 0.1; // 5% за период
    //const int INFLATION_PERIOD = 5;// 
    // todo придумать что то ещё

    p->food      = (RUB)(p->food * (1 + INFLATION_RATE));
    p->clothes   = (RUB)(p->clothes * (1 + INFLATION_RATE));
    p->utilities = (RUB)(p->utilities * (1 + INFLATION_RATE));
    p->rent      = (RUB)(p->rent * (1 + INFLATION_RATE));
}


// Блок 6 Функции вывода

void alice_print() {
    printf("Alice bank account = %lld руб.\n", alice.bank_account);
}

void bob_print() {
    printf("Bob   bank account = %lld руб.\n", bob.bank_account);
}
// TODO: при сравнении итогов учитывать стоимость квартиры у Alice

// Блок 7 Симуляция 

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
        if (month == 12 && year % 5 == 0) {
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

    alice_print();
    bob_print();

    return 0;
}
