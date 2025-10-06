#include <iostream>
#include <array>
#include <string>
#include <cmath>
#include <random>


typedef long long int RUB;
std::mt19937 random_num{ std::random_device{}() };

// Тип покупки (накопления или ипотека)
enum TypeOfMoneySaving 
{
    CONTRIBUTION = 1,
    MORTGAGE = 2
};

// Структура - Человек; 
struct Person 
{
    const std::string name;                                                         // Имя
    const TypeOfMoneySaving type;                                                   // Тип покупки
    const bool is_has_car;                                                            // Имеет ли машину
    RUB payment;                                                                    // Оплата квартиры (аренда  или ипотека)
    RUB income = 0;                                                                 // Начисления
    RUB bank_account = 0;                                                           // Текущий счёт
    RUB saves = 0;                                                                  // Вклад
};

 // Расчёт ежемесячной оплаты ипотеки
RUB month_payout(const double &percent, const int &sum, const int &months) //tut
{
    return (sum * (((percent / 12.0) * pow((1 + percent / 12.0), months))
        / (pow((1 + percent / 12.0), months) - 1)));
}

RUB first_payment = 1500 * 1000 + (random_num() % (500 * 1000));                    // Первый взнос
RUB appartament_price = 8 * 1000 * 1000 + (random_num() % (2 * 1000 * 1000));       // Стоимость квартиры
RUB car_price = 1000 * 1000 + (random_num() % (500 * 1000));                        // Стоимость машины
double banks_percent = 0.1;                                                         // Процент в банке (по вкладам и ипотеке)
double inflation = 1;                                                               // Коэффициент инфляции
double percent_inflation = 0.005;                                                   // Процент инфляции


Person alice = { "Alice", MORTGAGE, 1, month_payout(banks_percent, 
    appartament_price - first_payment, 12 * 20)};
Person bob = { "Bob", CONTRIBUTION, 0, 60000};
std::array<Person, 2> human = { alice, bob };

 // Инициализация начислений и текущего счёта людей
void person_init() 
{
    for (auto& per : human) {
        per.income = 100000 + (random_num() % 20000);
        per.bank_account = per.income;
    }
}

 // Итоговый вывод счёта людей
void person_print()
{
    for (auto& per : human) {
        if ((per.type == CONTRIBUTION) && (per.bank_account + 2 *
            per.income >= appartament_price)) {
            per.bank_account -= (appartament_price);
        }

        std::cout << per.name << " bank account = " << appartament_price
            + per.bank_account + per.saves + (per.is_has_car ? car_price : 0) << " rub.\n";
    }

    // Наибольшее количество денег под конец срока и счётчик (для индекса вектора)
    int counter = 0;

    for (size_t i = 1; i < human.size(); i++) {
        if ((human[i].bank_account + human[i].saves +
        (human[counter].is_has_car ? car_price : 0)) > (human[i - 1].bank_account
        + human[i - 1].saves + (human[counter].is_has_car ? car_price : 0))) {
            counter = i;
        }
    }

    std::cout << "\nThe happiest person is " << human[counter].name << " with "
        << appartament_price + human[counter].bank_account + human[counter].saves
        + (human[counter].is_has_car ? car_price : 0) << " rubles\n";
}

 // Начисления людей + повышение
void alice_income(const int& year, const int& month) 
{
        if (year == 2030 && month == 10) {
            human[0].income *= 1.5; //Promotion
        }
        if (month == 10) {
            human[0].income *= inflation;
        }
        human[0].bank_account += human[0].income;

}

void bob_income(const int& year, const int& month)
{

        if (year == 2030 && month == 10) {
            human[1].income *= 1.5; //Promotion
        }
        if (month == 10) {
            human[1].income *= inflation;
        }
        human[1].bank_account += human[1].income;

}

 // Траты на еду
void alice_food() 
{
    RUB food_average_price = 7000 + (random_num() % 2000);
    human[0].bank_account -= (food_average_price * inflation);
}

void bob_food()
{
    RUB food_average_price = 7000 + (random_num() % 2000);
    human[1].bank_account -= (food_average_price * inflation);
}

// Траты на одежду
void alice_clothes() 
{
    RUB clothes_average_price = 3000 + (random_num() % 2000);
    human[0].bank_account -= (clothes_average_price * inflation);
}

void bob_clothes()
{
    RUB clothes_average_price = 3000 + (random_num() % 2000);
    human[1].bank_account -= (clothes_average_price * inflation);
}

// Траты на аренду жилья или ипотеки
void alice_mortage() 
{
    if (human[0].type == MORTGAGE)
        human[0].bank_account -= human[0].payment;
    else
        human[0].bank_account -= (human[0].payment * inflation);
}

void bob_mortage()
{
    if (human[1].type == MORTGAGE)
        human[1].bank_account -= human[1].payment;
    else
        human[1].bank_account -= (human[1].payment * inflation);

}

// Траты на автомобиль (при его наличии)
void alice_car() 
{
    RUB car_service_average_price = 4000 + (random_num() % 2000);
    if (human[0].is_has_car)
        human[0].bank_account -= (car_service_average_price * inflation);
}

void bob_car()
{
    RUB car_service_average_price = 4000 + (random_num() % 2000);
    if (human[1].is_has_car)
        human[1].bank_account -= (car_service_average_price * inflation);
}

 // Траты на коммунальные услуги
void alice_public_utilities() 
{
    RUB public_utilities_average_price = 1500 + (random_num() % 2000);
    human[0].bank_account -= public_utilities_average_price * inflation;

}

void bob_public_utilities()
{
    RUB public_utilities_average_price = 1500 + (random_num() % 2000);
    human[1].bank_account -= public_utilities_average_price * inflation;

}

// Вложение на вклад + начисления процентов на него
void bank_money_saving() 
{
    for (auto& per : human) {
        if (per.type == CONTRIBUTION) {
            per.saves += per.bank_account;
            per.bank_account = 0;
            per.saves *= (1 + (banks_percent / 12.0));
        }
    }
}

// Ошибка, при недостаточном количестве средств
bool isnt_enough_money()
{
    for (auto& per : human) {
        if (per.bank_account < 0) {
            std::cout << per.name << " can\'t handle this life" << std::endl;
            return 1;
            break;
        }
    }
    return 0;
}

// увеличение инфляции, стоимости квартиры и машины
void global_inflation_raise()
{
    inflation = (1 + percent_inflation);
    car_price *= 1 + percent_inflation;
    appartament_price *= 1 + percent_inflation;
}


 // Симуляция логики течения времени: начислений и трат
void simulation() {
    int year = 2025;                        // Начальный год
    int month = 9;                          // Начальный месяц


    // Основной цикл течения времени
    while (!(year == 2045 && month == 9))
    {
        // Функции прибавок, трат и откладываний денег
        
        alice_income(year, month);
        alice_food();
        alice_mortage();
        alice_clothes();
        alice_car();
        alice_public_utilities();

        bob_income(year, month);
        bob_food();
        bob_mortage();
        bob_clothes();
        bob_car();
        bob_public_utilities();
        

        bank_money_saving();
        global_inflation_raise();

        // + месяц
        month++;
        if (month == 13) {
            year++;
            month = 1;
        }
        // Выход из цикла при ошибке
        if (isnt_enough_money())
            break;
    }
}
int main() 
{
    person_init();

    simulation();

    person_print();
}
