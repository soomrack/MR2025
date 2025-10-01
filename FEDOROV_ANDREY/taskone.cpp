#include <iostream>
#include <math.h>


typedef int RUB;

struct Person {
    RUB bank_account; // счёт в банке
    RUB income; // поступление средств
    RUB food; // расходы на еду
    RUB car_expences; // расходы на машину
    RUB cloth; // расходы на одежду
    RUB vacation; // расходы на отпуск
    RUB gifts; // расходы на подарки
    RUB medicine; // расходы на медицину
    RUB pet; // расходы на питомца
    RUB wildberries; // расходы на вайлдберриз
    RUB mortgage_payment; // платеж по ипотеке
    RUB first_payment;   // первый платеж
    RUB flat_price; // цена квартиры
    int has_flat; // есть квартира или нет
    int rent; // рента
};



struct Person anna;
struct Person bob;

void anna_income( const int year, const int month) {
    if (month == 11) {
        anna.income = anna.income * 1.1;
    }
    if (year == 2030 && month == 1 ) {
        anna.income = anna.income * 1.2;
    }
    anna.bank_account += anna.income;
    
}

void anna_food(const int month)
{
    if(month == 1) {
        anna.food = anna.food * 1.05;
    }
    anna.bank_account -= anna.food;
}

void anna_car_expences(const int month)
{
    if(month == 1) {
        anna.car_expences = anna.car_expences * 1.05;
    }
    anna.bank_account -= anna.car_expences;
}

void anna_cloth(const int month)
{
    if(month == 1) {
        anna.cloth = anna.cloth * 1.05;
    }
    anna.bank_account -= anna.cloth;
}

void anna_vacations(const int month)
{
    if(month == 1) {
        anna.vacation = anna.vacation * 1.05;
    }
    if (month == 7){
    anna.bank_account -= anna.vacation; //ездит в отпуск в июле
    }
}

void anna_gifts(const int month)
{
    if(month == 1) {
        anna.gifts = anna.gifts * 1.05;
    }
    anna.bank_account -= anna.gifts;
}

void anna_medicine(const int month)
{
    if(month == 1) {
        anna.medicine = anna.medicine * 1.05;
    }
    anna.bank_account -= anna.medicine;
}

void anna_pet(const int month)
{
    if(month == 1) {
        anna.pet = anna.pet * 1.05;
    }
    anna.bank_account -= anna.pet;
}

void anna_wildberries(const int month)
{
    if(month == 1) {
        anna.wildberries = anna.wildberries * 1.05;
    }
    anna.bank_account -= anna.wildberries;
}

void anna_mortgage()
{
    anna.bank_account -= anna.mortgage_payment;
}
// перейдем к рассчету Боба

void bob_income(const int year, const int month)
{
    if(month == 11) {
        bob.income = bob.income * 1.1;  // Индексация
    }
        
    if(year == 2030 && month == 1) {
        bob.income *= 1.2;  // Повышение
    }
    
    bob.bank_account += bob.income;
}

void bob_food(const int month)
{
    if(month == 1) {
        bob.food = bob.food * 1.05;
    }
    bob.bank_account -= bob.food;
}


void bob_car_expences(const int month)
{
    if(month == 1) {
        bob.car_expences = bob.car_expences * 1.05;
    }
    bob.bank_account -= bob.car_expences;
}


void bob_cloth(const int month)
{
    if (month == 1) {
        bob.cloth = bob.cloth * 1.05;
    }
    bob.bank_account -= bob.cloth;
}


void bob_vacation(const int month)
{
    if(month == 1) {
        bob.vacation = bob.vacation * 1.05;
    }
    if (month == 8) {
    bob.bank_account -= bob.vacation; // ездит в отпуск в августе
    };
}


void bob_gifts(const int month)
{
    if(month == 1) {
        bob.gifts = bob.gifts * 1.05;
    }
    bob.bank_account -= bob.gifts;
}

void bob_medicine(const int month)
{
    if(month == 1) {
        bob.medicine = bob.medicine * 1.05;
    }
    bob.bank_account -= bob.medicine;
}

void bob_rent(const int month)
{
    if(month == 1) {
        bob.rent = bob.rent * 1.05;
    }
    bob.bank_account -= bob.rent;
}

void bob_pet(const int month)
{
    if(month == 1) {
        bob.pet = bob.pet * 1.05;
    }
    bob.bank_account -= bob.pet;
}

void bob_buy_flat(const int year, const int month)
{
    // Bob покупает квартиру, когда накопил достаточно
    if (!bob.has_flat && bob.bank_account >= bob.flat_price * 0.85) {
        bob.bank_account -= bob.flat_price;
        bob.has_flat = 1;
        printf("Bob купил квартиру в %d году, месяце %d за %d рублей\n", 
               year, month, bob.flat_price);
    }
     
}

RUB calculate_monthly_payment(RUB indicator, double annual, int years)
{
    double monthly_percent = annual / 12;
     int total_payments = years * 12;
     double payment = indicator * monthly_percent * pow(1. + monthly_percent, total_payments) / 
                    (pow(1. + monthly_percent, total_payments) - 1.);
                    
    return (RUB)payment;
}


void action()
{
    int year = 2025;
    int month = 9;
    while(!(year == 2045 && month == 9)) { 
        anna_income(year, month);
        anna_car_expences(month);
        anna_cloth(month);
        anna_food(month);
        anna_gifts(month);
        anna_medicine(month);
        anna_pet(month);
        anna_vacations(month);
        anna_wildberries(month);
        anna_mortgage();

        bob_income(year, month);
        bob_car_expences(month);
        bob_cloth(month);
        bob_food(month);
        bob_gifts(month);
        bob_medicine(month);
        bob_pet(month);
        bob_vacation(month);
        bob_buy_flat(year, month);
        
        ++month;
        if(month == 13) {
            month = 1;
            ++year;
        }
    }
}

void show_anna_info()
{
    printf("Аnna\n");
    printf("Капитал Анны на 2045 год составляет %d рублей\n", anna.bank_account);
    printf("Есть своя квартра: %s\n", anna.has_flat ? "ДА" : "НЕТ");
        if (anna.has_flat) {
            printf("Стоимость квартиры %d рублей\n", anna.flat_price);
        }
        printf("Общая стоимость всего имущества %d рублей\n", anna.bank_account + (anna.has_flat ? anna.flat_price : 0));
}

void show_bob_info()
{
    printf("Bob:\n");
    printf("Капитал Боба на 2045 год составляет %d RUR\n", bob.bank_account);
    printf("Есть своя квартира: %s\n", bob.has_flat ? "ДА" : "НЕТ");
    if (bob.has_flat) {
        printf("  Стоимость квартиры: %d рублей\n", bob.flat_price);
    }
    printf("  Общая стоимость всего имущества: %d рублей\n", bob.bank_account + (bob.has_flat ? bob.flat_price : 0));
}
void anna_data()
{
    const int initial_flat_price = 12 * 1000 * 1000;  // 12 млн рублей
    const double mortgage_percent = 0.1;            // 10% годовая ставка по ипотеке
    const int mortgage_period = 20;                 // Срок ипотеки 20 лет
    const double first_payment_percent = 0.2;     // 20% первоначальный взнос
    
    // данные Анны

    anna.bank_account = 1000*100;
    anna.income = 2000*100;
    anna.food = 35000;
    anna.car_expences = 10000;
    anna.cloth = 10000;
    anna.gifts = 3000;
    anna.medicine = 3000;
    anna.vacation = 100*1000;
    anna.pet = 1500;
    anna.wildberries = 5000;
    anna.flat_price = initial_flat_price;
    anna.has_flat = 1;
    anna.first_payment = anna.flat_price * first_payment_percent;

    RUB mortgage_sum = anna.flat_price - anna.first_payment;
    anna.mortgage_payment = calculate_monthly_payment(mortgage_sum, mortgage_percent, mortgage_period);

}

void bob_data()
{
    const int initial_flat_price = 12*1000*1000;

    bob.bank_account = 100*1000;
    bob.income = 200*1000;
    bob.food = 35000;
    bob.car_expences = 15000;
    bob.cloth = 10000;
    bob.gifts = 3000;
    bob.medicine = 3000;
    bob.vacation = 100 * 1000;
    bob.pet = 1500;
    bob.flat_price = initial_flat_price;
    bob.has_flat = 0;
    bob.first_payment = 0;
}


void comparesment()
{
    printf("---------------------------------------------------------\n");
    printf("     СРАВНЕНИЕ СТРАТЕГИЙ\n");
    printf("Стратегия Боба выгоднее на %d рублей", (bob.bank_account + (bob.has_flat ? bob.flat_price : 0)) - (anna.bank_account + (anna.has_flat ? anna.flat_price : 0)));
}

int main()
{
    anna_data();
    bob_data();

    action();
    
    show_anna_info();
    show_bob_info();
    comparesment();
    return 0;
}