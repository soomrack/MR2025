#include <stdio.h>
#include <math.h>

typedef int RUB;


struct Person {
    RUB bank_account;
    RUB income;
    RUB food;
    RUB cure;
    RUB transport;
    RUB trip;
    RUB cloth;
    RUB moneybox;
    RUB mortgage_payment;
    RUB rent;
    RUB apartment_price;
    RUB down_payment;
    RUB bank_deposit;
    int has_apartment;
};

struct Person alice;
struct Person bob;

//=======================функции Алисы===================                                                        
void alice_income(const int year, const int month)
{
    if(month == 10) {
        alice.income = alice.income * 1.07;  // Indexation
    }
        
    if(year == 2030 && month == 3) {
        alice.income *= 1.5;  // Promotion
    }
    
    alice.bank_account += alice.income ;
}


void alice_food()
{
    alice.bank_account -= alice.food;
}


void alice_cure(const int month)
{
    if (month == 6) {
        alice.bank_account -= alice.cure;
    }
}


void alice_transport()
{
    alice.bank_account -= alice.transport;
}


void alice_trip(const int month)
{
    if (month == 9)
    {
        alice.bank_account -= alice.trip;
    }
}


void alice_cloth()
{
    alice.bank_account -= alice.cloth;
}



void alice_mortgage()
{
    alice.bank_account -= alice.mortgage_payment;
}


RUB calculate_mortgage_payment(RUB principal, double annual_rate, int term_years)
{
    double monthly_rate = annual_rate / 12;
    int total_payments = term_years * 12;
    
    double payment = principal * monthly_rate * pow(1. + monthly_rate, total_payments) / 
                    (pow(1. + monthly_rate, total_payments) - 1.);
    
    return (RUB)payment;
}


//=======================функции Боба===================
void bob_income(const int year, const int month)
{
    if(month == 10) {
        bob.income = bob.income * 1.07;  // Indexation
    }
        
    if(year == 2030 && month == 3) {
        bob.income *= 1.5;  // Promotion
    }
    
    bob.bank_account += bob.income ;
}


void bob_food()
{
    bob.bank_account -= bob.food;
}


void bob_cure(const int month)
{
    if (month == 6) {
        bob.bank_account -= bob.cure;
    }
}


void bob_transport()
{
    bob.bank_account -= bob.transport;
}


void bob_trip(const int month)
{
    if (month == 9)
    {
        bob.bank_account -= bob.trip;
    }
}


void bob_cloth()
{
    bob.bank_account -= bob.cloth;
}


void bob_mortgage()
{
    bob.bank_account -= bob.mortgage_payment;
}



void bob_buy_apartment(const int year, const int month)
{
    // Bob покупает квартиру, когда накопил достаточно
    if (!bob.has_apartment && bob.bank_account >= bob.apartment_price) {
        bob.bank_account -= bob.apartment_price;
        bob.has_apartment = 1;
        printf("Bob pay apartmen in %d year, month %d for %d rubls\n", 
               year, month, bob.apartment_price);
    }
}


void bob_bank_deposit(const int year, const int month)
{
    double deposit_procent;
    const int R = 100 * 1000; // вложения во вклад

    if(year == 2030 && month == 3)
    {
        bob.bank_account = bob.bank_account * (1 + deposit_procent) + 1.5*R;
    }

    if(month == 10) 
    {
        bob.bank_account = bob.bank_account * (1 + deposit_procent) + R;  
    }
}


//===================================================

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
        
        alice.transport = (RUB)(alice.transport * (1. + inflation_rate));
        bob.transport = (RUB)(bob.transport * (1. + inflation_rate));
        
        alice.trip = (RUB)(alice.trip * (1. + inflation_rate));
        bob.trip = (RUB)(bob.trip * (1. + inflation_rate));
        
        alice.cure = (RUB)(alice.cure * (1. + inflation_rate));
        bob.cure = (RUB)(bob.cure * (1. + inflation_rate));

        alice.cloth = (RUB)(alice.cloth * (1. + inflation_rate));
        bob.cloth = (RUB)(bob.cloth * (1. + inflation_rate));
    }
}


void simulation() 
{
    int year = 2025;
    int month = 9;

    while( !(year == 2045 && month == 9) ) {
        // симуляция Алисы
        alice_income(year, month);
        alice_food();
        alice_trip(month);
        alice_transport();
        alice_cure(month);
        alice_cloth();
        alice_mortgage();
        

        // симуляция Боба
        bob_cloth();
        bob_cure(month);
        bob_food();
        bob_income(year, month);
        bob_transport();
        bob_bank_deposit(year, month);

        
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
    printf("  CAPITAL = %d RUB\n", alice.bank_account);
    if (alice.has_apartment) {
        printf("  TOTAL COAST: %d RUB\n", alice.apartment_price);
    }
    printf("  TOTAL ASSETS VALUE: %d RUB\n", 
           alice.bank_account + (alice.has_apartment ? alice.apartment_price : 0));
}


void alice_init()
{
    const int initial_apartment_price = 10 * 1000 * 1000;  // 10 млн рублей
    const double mortgage_rate = 0.08;   // 8% годовая ставка по ипотеке
    const int mortgage_term = 20;                 // Срок ипотеки 20 лет
    const double down_payment_percent = 0.15;     // 15% первоначальный взнос

    alice.bank_account = 1000 * 1000;
    alice.income = 200 * 1000;
    alice.food = 30 * 000;
    alice.cure = 20 * 000;
    alice.transport = 5000; 
    alice.trip = 50 * 1000;
    alice.cloth = 20 * 1000;
    alice.moneybox = 10 * 1000;
    alice.apartment_price = initial_apartment_price;
    alice.down_payment = alice.apartment_price * down_payment_percent;
    alice.has_apartment = 1; // Alice сразу покупает квартиру

    RUB mortgage_amount = alice.apartment_price - alice.down_payment;
    alice.mortgage_payment = calculate_mortgage_payment(mortgage_amount, mortgage_rate, mortgage_term);

    alice.bank_account -= alice.down_payment;    
}


void bob_init()
{
   const int initial_apartment_price = 10 * 1000 * 1000;  // 10 млн рублей
   const double deposit_procent = 0.01; // процент за 1 месяц 
   

    bob.bank_account = 1000 * 1000;
    bob.income = 200 * 1000;
    bob.food = 30 * 000;
    bob.rent = 30 * 000; // Аренда квартиры
    bob.transport = 1500;
    bob.trip = 20 * 000;
    bob.cure = 10 * 000;
    bob.cloth = 8.5 * 1000;
    bob.apartment_price = initial_apartment_price;
    bob.has_apartment = 0;
    bob.mortgage_payment = 0; 
}


void print_bob_info()
{
    printf("Bob:\n");
    printf("  CAPITAL = %d RUB\n", bob.bank_account);
    if (bob.has_apartment) {
        printf("  TOTAL COAST: %d RUB\n", bob.apartment_price);
    }
    printf("  TOTAL ASSETS VALUE: %d RUB\n", 
           bob.bank_account + (bob.has_apartment ? bob.apartment_price : 0));
}


void compare_strategies()
{
    RUB alice_total = alice.bank_account + (alice.has_apartment ? alice.apartment_price : 0);
    RUB bob_total = bob.bank_account + (bob.has_apartment ? bob.apartment_price : 0);
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
