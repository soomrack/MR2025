#include <stdio.h>
#include <math.h>


typedef int RUB;
typedef int USDT;


// Структура для персонажа
struct Person {
    RUB bank_account;
    RUB income;
    RUB food;
    RUB rent;           // Аренда жилья
    RUB car;            // Расходы на машину
    RUB trips;          // Расходы на путешествия
    RUB other;          // Прочие расходы
    RUB mortgage_payment; // Ежемесячный платеж по ипотеке
    RUB down_payment;   // Первоначальный взнос
    RUB apartment_price; // Стоимость квартиры
    int has_apartment;  // Флаг владения квартирой
};


struct Person alice;
struct Person bob;


// Функции для Alice
void alice_income(const int year, const int month)
{
    if(month == 10) {
        alice.income = alice.income * 1.07;  // Индексация
    }
        
    if(year == 2030 && month == 3) {
        alice.income *= 1.5;  // Повышение
    }
    
    alice.bank_account += alice.income;
}


void alice_food()
{
    alice.bank_account -= alice.food;
}


void alice_mortgage()
{
    alice.bank_account -= alice.mortgage_payment;
}


void alice_car()
{
    alice.bank_account -= alice.car;
}


void alice_trip()
{
    alice.bank_account -= alice.trips;
}


void alice_other()
{
    alice.bank_account -= alice.other;
}


// Функции для Bob
void bob_income(const int year, const int month)
{
    if(month == 10) {
        bob.income = bob.income * 1.07;  // Индексация
    }
        
    if(year == 2030 && month == 3) {
        bob.income *= 1.5;  // Повышение
    }
    
    bob.bank_account += bob.income;
}


void bob_food()
{
    bob.bank_account -= bob.food;
}


void bob_rent()
{
    bob.bank_account -= bob.rent;
}


void bob_car()
{
    bob.bank_account -= bob.car;
}


void bob_trip()
{
    bob.bank_account -= bob.trips;
}


void bob_other()
{
    bob.bank_account -= bob.other;
}


void bob_buy_apartment(const int year, const int month)
{
    // Bob покупает квартиру, когда накопил достаточно
    if (!bob.has_apartment && bob.bank_account >= bob.apartment_price) {
        bob.bank_account -= bob.apartment_price;
        bob.has_apartment = 1;
        printf("Bob купил квартиру в %d году, месяце %d за %d рублей\n", 
               year, month, bob.apartment_price);
    }
}


// Расчет ежемесячного платежа по ипотеке
RUB calculate_mortgage_payment(RUB principal, double annual_rate, int term_years)
{
    double monthly_rate = annual_rate / 12;
    int total_payments = term_years * 12;
    
    double payment = principal * monthly_rate * pow(1. + monthly_rate, total_payments) / 
                    (pow(1. + monthly_rate, total_payments) - 1.);
    
    return (RUB)payment;
}


// Обновление цен с учетом инфляции
void update_prices(const int year, const int month)
{
    if (month == 1) { // Ежегодная индексация в январе
        double inflation_rate = 0.07; // 7% годовая инфляция
        
        // Обновляем стоимость квартиры
        bob.apartment_price = (RUB)(bob.apartment_price * (1. + inflation_rate));
        
        // Обновляем арендную плату (растет с инфляцией)
        bob.rent = (RUB)(bob.rent * (1. + inflation_rate));
        
        // Обновляем другие расходы
        alice.food = (RUB)(alice.food * (1. + inflation_rate));
        bob.food = (RUB)(bob.food * (1. + inflation_rate));
        
        alice.car = (RUB)(alice.car * (1. + inflation_rate));
        bob.car = (RUB)(bob.car * (1. + inflation_rate));
        
        alice.trips = (RUB)(alice.trips * (1. + inflation_rate));
        bob.trips = (RUB)(bob.trips * (1. + inflation_rate));
        
        alice.other = (RUB)(alice.other * (1. + inflation_rate));
        bob.other = (RUB)(bob.other * (1. + inflation_rate));
    }
}


void simulation()
{
    int year = 2025;
    int month = 9;

    while(!(year == 2045 && month == 9)) {
        // Обновление цен с учетом инфляции
        update_prices(year, month);
        
        // Месяц Alice
        alice_income(year, month);
        alice_food();
        alice_mortgage();
        alice_car();
        alice_trip();
        alice_other();
        
        // Месяц Bob
        bob_income(year, month);
        bob_food();
        bob_rent();
        bob_car();
        bob_trip();
        bob_other();
        bob_buy_apartment(year, month);
        
        ++month;
        if(month == 13) {
            month = 1;
            ++year;
        }
    }
}


void print_alice_info()
{
    printf("Alice:\n");
    printf("  Капитал = %d RUR\n", alice.bank_account);
    printf("  Владеет квартирой: %s\n", alice.has_apartment ? "Да" : "Нет");
    if (alice.has_apartment) {
        printf("  Стоимость квартиры: %d RUR\n", alice.apartment_price);
    }
    printf("  Общая стоимость активов: %d RUR\n", 
           alice.bank_account + (alice.has_apartment ? alice.apartment_price : 0));
}


void print_bob_info()
{
    printf("Bob:\n");
    printf("  Капитал = %d RUR\n", bob.bank_account);
    printf("  Владеет квартирой: %s\n", bob.has_apartment ? "Да" : "Нет");
    if (bob.has_apartment) {
        printf("  Стоимость квартиры: %d RUR\n", bob.apartment_price);
    }
    printf("  Общая стоимость активов: %d RUR\n", 
           bob.bank_account + (bob.has_apartment ? bob.apartment_price : 0));
}


void compare_strategies()
{
    RUB alice_total = alice.bank_account + (alice.has_apartment ? alice.apartment_price : 0);
    RUB bob_total = bob.bank_account + (bob.has_apartment ? bob.apartment_price : 0);
    
    printf("\n=== СРАВНЕНИЕ СТРАТЕГИЙ ===\n");
    printf("Alice общая стоимость: %d RUR\n", alice_total);
    printf("Bob общая стоимость: %d RUR\n", bob_total);
    
    if (alice_total > bob_total) {
        printf("Стратегия Alice выгоднее на %d RUR\n", alice_total - bob_total);
    } else if (bob_total > alice_total) {
        printf("Стратегия Bob выгоднее на %d RUR\n", bob_total - alice_total);
    } else {
        printf("Стратегии равны по результату\n");
        
    }
}


void alice_init()
{
    const int initial_apartment_price = 10 * 1000 * 1000;  // 10 млн рублей
    const double mortgage_rate = 0.08;            // 8% годовая ставка по ипотеке
    const int mortgage_term = 20;                 // Срок ипотеки 20 лет
    const double down_payment_percent = 0.15;     // 15% первоначальный взнос

    alice.bank_account = 1000 * 1000;
    alice.income = 200 * 1000;
    alice.food = 30000;
    alice.rent = 0;
    alice.car = 15000;
    alice.trips = 20000;
    alice.other = 10000;
    alice.apartment_price = initial_apartment_price;
    alice.down_payment = alice.apartment_price * down_payment_percent;
    alice.has_apartment = 1; // Alice сразу покупает квартиру
    
    // Расчет ипотечного платежа
    RUB mortgage_amount = alice.apartment_price - alice.down_payment;
    alice.mortgage_payment = calculate_mortgage_payment(mortgage_amount, mortgage_rate, mortgage_term);
    
    // Первоначальный взнос
    alice.bank_account -= alice.down_payment;
}


void bob_init()
{
    const int initial_apartment_price = 10 * 1000 * 1000;  // 10 млн рублей

    bob.bank_account = 1000 * 1000;
    bob.income = 200 * 1000;
    bob.food = 30000;
    bob.rent = 40000; // Аренда квартиры
    bob.car = 15000;
    bob.trips = 20000;
    bob.other = 10000;
    bob.apartment_price = initial_apartment_price;
    bob.has_apartment = 0;
    bob.mortgage_payment = 0;
}


int main()
{
    alice_init();
    bob_init();
    simulation();
    print_alice_info();
    printf("\n");
    print_bob_info();
    compare_strategies();
    return 0;
}