#include <stdio.h>
#include <math.h>

typedef long long int RUB;

struct apartment {
    RUB cost;           // Цена квартиры
    RUB desired_cost;   // Цена купли квартиры
    RUB rent;           // Аренда квартиры
    RUB utilities;      // Коммуналка + страховка
};

struct bank {
    RUB account;            // Счет в банке
    RUB mortgage_amount;    // Остаток ипотеки
    RUB mortgage_payment;   // Ежимесячная плата ипотеки
    RUB investments;        // Инвестиции
};

struct expenses {       // Личные траты за месяц
    RUB food;           // Траты на еду
    RUB clothes;        // Траты на одежду
    RUB transport;      // Траты на транспорт
    RUB entertainment;  // Траты на развлечения
    RUB medical;        // Траты на медицину
};

struct person {
    RUB income;             // Зарплата
    apartment apartment;
    bank bank;
    expenses expenses;
};

struct person alice;    // Алиса берет ипотеку
struct person bob;      // Боб копит на квартиру

// Ежимесячеая плата ипотеки
int mortgage_payment(long sum, double percent, int months){
    return (sum * (percent / 12) / (1 - pow(1 + percent / 12, -months - 1)));
}

// Начальные данные
void alice_init() {
    alice.income = 100 * 1000;

    alice.bank.account = 0;
    alice.bank.mortgage_amount = 8 * 1000 * 1000;
    alice.bank.mortgage_payment = mortgage_payment(alice.bank.mortgage_amount, 0.06, 20 * 12); // кредит ипотеки 6% на 20 лет
    alice.bank.investments = 0;

    alice.apartment.cost = 10 * 1000 * 1000;
    alice.apartment.utilities = 5 * 1000;
    alice.apartment.rent = 0;

    alice.expenses.food = 20 * 1000 + 250 * 5 * 4;
    alice.expenses.clothes = 5 * 1000;
    alice.expenses.transport = 5 * 1000;
    alice.expenses.entertainment = 5 * 1000;
    alice.expenses.medical = 5 * 1000;
};

void bob_init() {
    bob.income = 100 * 1000;

    bob.bank.account = 0;
    bob.bank.investments = 2 * 1000 * 1000;

    bob.apartment.desired_cost = 10 * 1000 * 1000;
    bob.apartment.cost = 0;     // Нету квартиры
    bob.apartment.rent = 35 * 1000;
    bob.apartment.utilities = 5 * 1000;

    bob.expenses.food = 3 * 300 * 7 * 4;
    bob.expenses.clothes = 5 * 1000;
    bob.expenses.transport = 5 * 1000;
    bob.expenses.entertainment = 5 * 1000;
    bob.expenses.medical = 5 * 1000;
}

// Инфляция
const double inflation_rate = 1.07;

// Зарплата
void alice_income(const int& year, const int& month) {
    if (month == 10) alice.income *= inflation_rate;
    if (year == 2030 && month == 3) {
        alice.income *= 1.5;            //Продвижение в работе
    }
    alice.bank.account += alice.income;
}

void bob_income(const int& year, const int& month) {
    if (month == 10) bob.income *= inflation_rate;
    if (year == 2030 && month == 3) {
        bob.income *= 1.5;              //Продвижение в работе
    }
    bob.bank.account += bob.income;
}

// Квартиры + Коммуналка
void alice_apartment(const int& month) {
    if (month == 10) {
        alice.apartment.cost *= inflation_rate;

        alice.apartment.utilities *= inflation_rate;
    }
    alice.bank.account -= alice.apartment.utilities;
}

void alice_mortgage(const int& month) {
    alice.bank.account -= alice.bank.mortgage_payment;
}

void bob_apartment(const int& month) {
    if (month == 10) {
        bob.apartment.cost *= inflation_rate;
        bob.apartment.desired_cost *= inflation_rate;

        bob.apartment.rent *= inflation_rate;
        bob.apartment.utilities *= inflation_rate;
    }
    if (bob.apartment.cost == 0) bob.bank.account -= bob.apartment.rent;        // Пока не купил квартиру - арендует
    else bob.bank.account -= bob.apartment.utilities;       // Только плата за коммуналку
}

// Коммуналка
/*
void alice_utilities(const int& month) {
    if (month == 10) alice.apartment.utilities *= inflation_rate;
    alice.bank.account -= alice.apartment.utilities;
}

void bob_utilities(const int& month) {
    if (month == 10) {
        bob.apartment.rent *= inflation_rate;
        bob.apartment.utilities *= inflation_rate;
    }
    if (bob.apartment.cost == 0) bob.bank.account -= bob.apartment.rent;        // Пока не купил квартиру - арендует
    else bob.bank.account -= bob.apartment.utilities;       // Только плата за коммуналку
}
*/

// Траты в месяц
void alice_expenses(const int& month) {
    if (month == 10) {
        alice.expenses.clothes *= inflation_rate;
        alice.expenses.entertainment *= inflation_rate;
        alice.expenses.food *= inflation_rate;
        alice.expenses.medical *= inflation_rate;
        alice.expenses.transport *= inflation_rate;
    }
    alice.bank.account -= alice.expenses.clothes;
    alice.bank.account -= alice.expenses.entertainment;
    alice.bank.account -= alice.expenses.food;
    alice.bank.account -= alice.expenses.medical;
    alice.bank.account -= alice.expenses.transport;
    
}

void bob_expenses(const int& month) {
    if (month == 10) {
        bob.expenses.clothes *= inflation_rate;
        bob.expenses.entertainment *= inflation_rate;
        bob.expenses.food *= inflation_rate;
        bob.expenses.medical *= inflation_rate;
        bob.expenses.transport *= inflation_rate;
    }
    
    bob.bank.account -= bob.expenses.clothes;
    bob.bank.account -= bob.expenses.entertainment;
    bob.bank.account -= bob.expenses.food;
    bob.bank.account -= bob.expenses.medical;
    bob.bank.account -= bob.expenses.transport;
}

// Инвестиции
const double percent_yield = 0.2;       // Годовая процентая ставка
void alice_investment(const int& year, const int& month) {
    alice.bank.investments *= 1 + percent_yield / 12;       // Начисление процентов на вклад в банке

    alice.bank.investments += alice.bank.account - 50 * 1000;
    alice.bank.account = 50 * 1000;     // Оставляет 50 тыс.
}

void bob_investment(const int& year, const int& month) {
    bob.bank.investments *= 1 + percent_yield / 12;      // Начисление процентов на вклад в банке

    bob.bank.investments += bob.bank.account;
    bob.bank.account = 0;       // Вкладывает полнуюю ЗП (после трат)

    if (bob.apartment.cost == 0 && bob.bank.investments >=bob.apartment.desired_cost) {
        bob.bank.investments -= bob.apartment.desired_cost;
        bob.apartment.cost = bob.apartment.desired_cost;
        //printf("Боб покупает квартиру", month, "месяц", year, "год \n");
    }
}


void simulation()
{
    int year = 2025;
    int month = 9;

    while( !(year == 2045 && month == 9) ) {

        alice_income(year, month);
        alice_apartment(month);
        alice_mortgage(month);
        alice_expenses(month);      // В состав входит трата за еду, одежду, транспорт, развленчения, медицину
        alice_investment(year, month);
        //alice_utilities(month);
        
        bob_income(year, month);
        bob_apartment(month);
        bob_expenses(month);        // В состав входит трата за еду, одежду, транспорт, развленчения, медицину
        bob_investment(year, month);
        //bob_utilities(month);

        month++;
        if (month == 13) {
            year++;
            month = 1;
        }
    }
}

void alice_print()
{
    printf("Alice net worth = %lld руб.\n", alice.bank.account + alice.apartment.cost + alice.bank.investments);
    if (alice.apartment.cost != 0) printf("Есть квартира\n");
    else printf("Нет квартиры\n");
}

void bob_print() {
    printf("Bob net worth = %lld руб.\n", bob.bank.account + bob.apartment.cost + bob.bank.investments);
    if (bob.apartment.cost != 0) printf("Есть квартира\n");
    else printf("Нет квартиры\n");
}

int main()
{
    alice_init();
    bob_init();

    simulation();
    
    alice_print();
    bob_print();
}