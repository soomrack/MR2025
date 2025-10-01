#include <stdio.h>
#include <math.h>
#include <stdlib.h>
#include <time.h>

typedef long long int RUB;

//============================================================================================
// Структуры

// Машина
struct Car {
    RUB price;            // покупка
    RUB fuel;             // бензин (ежемесячный)
    RUB amortization;     // ТО (раз в год)
    RUB tires;            // замена шин (сезонная)
    RUB insurance;        // страховка (раз в год)
    RUB fine_amount;      // штраф (по вероятности)
    bool chance_of_fine;  // вероятность штрафа
    bool winter_driver;   // меняет шины зимой?
};

// Депозит
struct Deposit {
    RUB amount;           // сумма на вкладе
    int years;            // срок депозита в годах
    int start_year;       // год, когда вклад был открыт
    bool active;          // активен ли вклад
};

// Квартира
struct Flat {
    RUB price;            // стоимость квартиры
    RUB repairs;          // ремонт (ежемесячные траты на период)
};

// Человек
struct Person {
    RUB bank_account;     // банковский счет
    RUB income;           // доходы

    RUB food;             // еда
    RUB clothes;          // траты на одежду
    RUB utilities;        // оплата ЖКХ
    RUB rent;             // аренда жилья (только у Bob)
    RUB medical_expense;  // медицина (раз в год)
    RUB vacation_expense; // отпуск (раз в год)

    Car car;              // расходы на машину
    Deposit deposit;      // сбережения
    Flat flat;            // недвижимость

    // Займ
    RUB loan;             
    int loan_years;       
    double loan_rate;     
    RUB monthly_payment; 
    int loan_months_left; 
};


//============================================================================================

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

//============================================================================================
// Инициализация персонажей

Person alice;


void alice_init() {
    alice.bank_account = 1000 * 1000;
    alice.income = 100000;

    alice.food = 40000;
    alice.clothes = 20000;
    alice.utilities = 10000;
    alice.rent = 0;
    alice.medical_expense  = 100000;
    alice.vacation_expense = 150000;

    alice.car.price = 1500 * 1000; 
    alice.car.fuel = 12000;     
    alice.car.amortization = 30000;     
    alice.car.tires = 15000;       // только летняя смена    
    alice.car.insurance = 25000;
    alice.car.fine_amount = 10000;
    alice.car.winter_driver = false;
    alice.car.chance_of_fine = 10;
    
    alice.deposit.amount = 0;
    alice.deposit.years = 15;  
    alice.deposit.start_year = 2025;
    alice.deposit.active = true;

    alice.flat.price = 6 * 1000 * 1000;
    alice.flat.repairs = 20000;

    alice.loan = alice.flat.price - 1 * 1000 * 1000; 
    alice.loan_years = 20;
    alice.loan_rate = 0.05;
    alice.monthly_payment = annuity_payment(alice.loan, alice.loan_rate, alice.loan_years);
    alice.loan_months_left = alice.loan_years * 12;

    alice.bank_account -= 1 * 1000 * 1000; // первый взнос
}

Person bob;

void bob_init() {
    bob.bank_account = 1000 * 1000;
    bob.income = 100000;

    bob.food = 20000;
    bob.clothes = 10000;
    bob.utilities = 15000;
    bob.rent = 20000;
    bob.medical_expense  = 120000;
    bob.vacation_expense = 100000;

    bob.car.price = 1500 * 1000; 
    bob.car.fuel = 18000;  // больший расход   
    bob.car.amortization = 40000; // ТО дороже
    bob.car.tires = 15000;     // два раза в год
    bob.car.insurance = 30000;   // страховка дороже
    bob.car.fine_amount = 15000;// больший штраф, неаккуратное вожение машины
    bob.car.winter_driver = true;
    bob.car.chance_of_fine = 8;

    bob.deposit.amount = 0;
    bob.deposit.years = 15;  
    bob.deposit.start_year = 2025;
    bob.deposit.active = true;

    bob.flat.price = 0;

    bob.loan = 1 * 1000 * 1000; 
    bob.loan_years = alice.loan_years;
    bob.loan_rate = 0.05;
    bob.monthly_payment = annuity_payment(bob.loan, bob.loan_rate, bob.loan_years);
    bob.loan_months_left = bob.loan_years * 12;
}

//============================================================================================
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


//============================================================================================
// Базовые расходы

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

void pay_medical(Person &p, int month) {
    if (month == 3) { 
        p.bank_account -= p.medical_expense;
        p.medical_expense *= (1 + INFLATION_RATE);
    }
}

void pay_vacation(Person &p, int month) {
    if (month == 7) { 
        p.bank_account -= p.vacation_expense;
        p.vacation_expense *= (1 + INFLATION_RATE);
    }
}

void pay_loan(Person &p) {
    if (p.loan_months_left > 0) {
        p.bank_account -= p.monthly_payment;   // аннуитетный платеж
        p.loan_months_left--;  
    }
}

void pay_repairs(Person &p, int year, int month) {
    const int START_YEAR = 2030;
    const int END_YEAR   = 2033; 
    if (year >= START_YEAR && year < END_YEAR) {
        p.bank_account -= p.flat.repairs;
        p.flat.repairs *= (1 + INFLATION_RATE/12.0);
    }
}



//============================================================================================
// Общая функция расходов по машине

void car_expenses(Person &p, int year, int month) {
    // покупка машины в начале симуляции
    if (month == 9 && year == 2025) {
        p.bank_account -= p.car.price;
    }

    // топливо (с топливным кризисом)
    if (year >= 2028 && year <= 2030 && month == 1) {
        p.car.fuel *= 1.2; 
    } else {
        p.car.fuel *= (1 + INFLATION_RATE/12.0); 
    }
    p.bank_account -= p.car.fuel;

    // страховка
    if (month == 4) {
        p.bank_account -= p.car.insurance;
        p.car.insurance *= (1 + INFLATION_RATE);
    }

    // ТО
    if (month == 6) {
        p.bank_account -= p.car.amortization;
        p.car.amortization *= (1 + INFLATION_RATE);
    }

    // шины
    if (month == 10 && p.car.winter_driver) {
        p.bank_account -= p.car.tires;
        p.car.tires *= (1 + INFLATION_RATE);
    }
    if (month == 4) {
        p.bank_account -= p.car.tires;
    }

    // штрафы
    if ((rand() % 100) < p.car.chance_of_fine) { // 10% и 8%  шанс
        p.bank_account -= p.car.fine_amount;
    }
}


//  случайное событие
void pay_fine(Person &p, int month) {
    if ((rand() % 100) < 5) { // 5% шанс
        p.bank_account -= p.fine_amount;
    }
}

//============================================================================================
// Управление вкладом

void manage_deposit(Person &p, int year, int month) {
    if (!p.deposit.active) return;

    if (p.bank_account > 0) { 
        p.deposit.amount += p.bank_account; 
        p.bank_account = 0;
    }

    // ежемесячная капитализация
    p.deposit.amount *= (1 + DEPOSIT_RATE / 12.0);

    // проверка срока вклада
    if (year >= p.deposit.start_year + p.deposit.years) {
        p.bank_account += p.deposit.amount; 
        p.deposit.amount = 0;
        p.deposit.active = false;
    }
}

//============================================================================================

// Итоги
void results() {
    RUB alice_total = alice.bank_account + alice.deposit.amount + alice.flat.price;
    RUB bob_total   = bob.bank_account + bob.deposit.amount + bob.flat.price;

    printf("\n=== Итог через 20 лет ===\n");
    printf("Alice bank = %lld руб. (+ квартира %lld)\n", 
           alice.bank_account, alice.flat.price);
    printf("Bob   bank = %lld руб. (+ собственность %lld)\n", 
           bob.bank_account, bob.flat.price);

    if (alice_total > bob_total) {
        printf("Жизнь лучше у Alice.\n");
    } else if (bob_total > alice_total) {
        printf("Жизнь лучше у Bob.\n");
    } else {
        printf("Жизнь одинаковая.\n");
    }
}

//=========================================================================================

// Симуляция
void simulation() {
    int year = 2025;
    int month = 9;

    while (!(year == 2045 && month == 9)) {
        
        alice_income(year, month);
        pay_food(alice);
        pay_clothes(alice);
        pay_utilities(alice);
        pay_rent(alice);
        car_expenses(alice, year, month);
        pay_repairs(alice, year, month);
        pay_medical(alice, month);
        pay_vacation(alice, month);
        pay_loan(alice);
        manage_deposit(alice, year, month);

        bob_income(year, month);
        pay_food(bob);
        pay_clothes(bob);
        pay_utilities(bob);
        pay_rent(bob);
        car_expenses(bob, year, month);
        pay_repairs(bob, year, month);
        pay_medical(bob, month);
        pay_vacation(bob, month);
        pay_loan(bob);
        manage_deposit(bob, year, month);

        month++;
        if (month == 13) { month = 1; year++; }
    }
}

//=============================================================================================

int main() {
    srand(time(0));// для случайных результатов
    //srand(42);// для отключения случайности
    alice_init();
    bob_init();

    simulation();
    results();

    return 0;
}
