#include <stdio.h>
#include <math.h>

typedef long long int RUB;

struct Person {
   RUB bank_account;
    RUB income;

    RUB food;
    RUB clothes;  // траты на одежду
    RUB utilities;  // оплата ЖКХ
    RUB rent;  // аренда жилья (только у Bob)

    // Машина
    RUB car_price;         // покупка
    RUB fuel;              // бензин (ежемесячный)
    RUB car_amortization;  // ТО (раз в год)
    RUB tire_replacement;  // замена шин (сезонная)
    RUB insurance;         // страховка (раз в год)

    // Сбережения
    RUB deposit;

    RUB repairs_expenses;  // расходы на ремонт квартиры
    RUB flat_price;

    // Займ
    RUB loan;             
    int loan_years;       
    double loan_rate;     
    RUB monthly_payment; 
    int loan_months_left; 
};

Person alice;
Person bob;


//  Константы экономики

const double INFLATION_RATE   = 0.05; 
const double DEPOSIT_RATE     = 0.08; // 8% годовых на вклад


//  Функции расчётов 
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
    double r = annual_rate / 12.0; // месячная ставка
    double powf = pow(1.0 + r, total_months);  // (1 + r)^n
    double payment = amount * (r * powf) / (powf - 1.0);//P = S * (r * (1 + r)^n) / ((1 + r)^n - 1)
    return (RUB) payment;  // округление вниз до целого RUB
}

// Инициализация персонажей

void alice_init() {
    alice.bank_account = 1000 * 1000;
    alice.income = 90000;

    alice.food = 40000;
    alice.clothes = 20000;
    alice.utilities = 10000;
    alice.rent = 0;

    alice.car_price = 1500 * 1000; 
    alice.fuel = 12000;     
    alice.car_amortization = 30000;     
    alice.tire_replacement = 15000;       // только летняя смена
    alice.insurance = 25000;     

    alice.deposit = 0;

    alice.repairs_expenses = 20000;
    alice.flat_price = 6 * 1000 * 1000;

    alice.loan = alice.flat_price - 1 * 1000 * 1000; 
    alice.loan_years = 20;
    alice.loan_rate = 0.05;
    alice.monthly_payment = annuity_payment(alice.loan, alice.loan_rate, alice.loan_years);
    alice.loan_months_left = alice.loan_years * 12;

    alice.bank_account -= 1 * 1000 * 1000; 
}

void bob_init() {
    bob.bank_account = 1000 * 1000;
    bob.income = 80000;

    bob.food = 20000;
    bob.clothes = 10000;
    bob.utilities = 15000;
    bob.rent = 20000;

    bob.car_price = 1500 * 1000; 
    bob.fuel = 18000;      // больший расход
    bob.car_amortization = 40000;      // ТО дороже
    bob.tire_replacement = 15000;      // два раза в год
    bob.insurance = 30000;      // страховка дороже

    bob.deposit = 0;

    bob.flat_price = 0;

    bob.loan = 1 * 1000 * 1000; 
    bob.loan_years = alice.loan_years;
    bob.loan_rate = 0.05;
    bob.monthly_payment = annuity_payment(bob.loan, bob.loan_rate, bob.loan_years);
    bob.loan_months_left = bob.loan_years * 12;
}


// Доходы
void alice_income(const int year, const int month) {
    if (month == 9) {
        alice.income *= 1.07; // индексация раз в год
    }
    if (year == 2030 && month == 10) {
        alice.income *= 1.5; // повышение
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
    if (p.rent > 0) {
        p.bank_account -= p.rent;  
        p.rent *= (1 + INFLATION_RATE/12.0);
    }
}

void pay_loan(Person &p) {
    if (p.loan_months_left > 0) {
        p.bank_account -= p.monthly_payment;   // аннуитетный платеж по кредиту/ипотеке
        p.loan_months_left--;  // уменьшение срока
    }
}

void car_expenses(Person &p, int year, int month, bool winter_driver) {
    if (month == 9 && year == 2025) { // покупка машины в начале симуляции
        p.bank_account -= p.car_price;
    }

    // Топливный кризис: 2028–2030 (ускоренный рост)
    if (year >= 2028 && year <= 2030 && month == 1) {
        p.fuel *= 1.05; // рост только в январе этих лет
    } else {
        p.fuel *= (1 + INFLATION_RATE/12.0); // обычная инфляция 
    }
    p.bank_account -= p.fuel;
    

    // страховка раз в год в апреле
    if (month == 4) {
        p.bank_account -= p.insurance;
        p.insurance *= (1 + INFLATION_RATE);
    }

    // ТО раз в год в июне
    if (month == 6) {
        p.bank_account -= p.car_amortization;
        p.car_amortization *= (1 + INFLATION_RATE);
    }

    // сезонные шины
    if (month == 10) { // осень
        if (winter_driver) {
            p.bank_account -= p.tire_replacement;
            p.tire_replacement *= (1 + INFLATION_RATE);
        }
    }
    if (month == 4) { // весна
        p.bank_account -= p.tire_replacement;
    }
}


// Вклады
void manage_deposit(Person &p) {
    if (p.bank_account > 0) {
        p.deposit += p.bank_account; // всё свободное переводится на депозит
        p.bank_account = 0;
    }
    p.deposit *= (1 + DEPOSIT_RATE/12.0);
}

void pay_repairs(Person &p, int year, int month) {
    const int START_YEAR = 2030;
    const int END_YEAR   = 2033; 
    if (year >= START_YEAR && year < END_YEAR) {
        p.bank_account -= p.repairs_expenses;
        p.repairs_expenses *= (1 + INFLATION_RATE/12.0);
    }
}


// Итоги
void results() {
    RUB alice_total = alice.bank_account + alice.deposit + alice.flat_price;
    RUB bob_total   = bob.bank_account + bob.deposit + bob.flat_price;

    printf("\n=== Итог через 20 лет ===\n");
    printf("Alice bank+deposit = %lld руб. (+ квартира %lld)\n", 
           alice.bank_account + alice.deposit, alice.flat_price);
    printf("Bob   bank+deposit = %lld руб. (+ собственность %lld)\n", 
           bob.bank_account + bob.deposit, bob.flat_price);

    if (alice_total > bob_total) {
        printf("Жизнь лучше у Alice.\n");
    } else if (bob_total > alice_total) {
        printf("Жизнь лучше у Bob.\n");
    } else {
        printf("Жизнь одинаковая.\n");
    }
}
// Блок 8 Симуляция 

// Симуляция
void simulation() {
    int year = 2025;
    int month = 9;

    while (!(year == 2045 && month == 9)) {
        
        alice_income(year, month);
        pay_food(alice);
        pay_clothes(alice);
        pay_utilities(alice);
        car_expenses(alice, year, month, false); // Алиса зимой не ездит
        pay_repairs(alice, year, month);
        pay_loan(alice);
        manage_deposit(alice);

        bob_income(year, month);
        pay_food(bob);
        pay_clothes(bob);
        pay_utilities(bob);
        pay_rent(bob);
        car_expenses(bob, year, month, true); // Боб ездит круглый год
        pay_loan(bob);
        manage_deposit(bob);

        month++;
        if (month == 13) { month = 1; year++; }
    }
}


int main() {
    alice_init();
    bob_init();

    simulation();

    results();

    return 0;
}
