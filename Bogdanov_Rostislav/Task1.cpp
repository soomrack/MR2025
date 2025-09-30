#include <iostream>
#include <vector>
#include <string>
#include <cmath>
#include <random>


typedef long long int RUB;
std::mt19937 mt{ std::random_device{}() };

// Тип покупки (накопления или ипотека)
enum typeOfMoneySaving 
{
    contribution = 1,
    mortgage = 2
};

// Структура - Человек; 
struct Person 
{
    const std::string name;                                                         // Имя
    const typeOfMoneySaving type;                                                   // Тип покупки
    const bool isHasCar;                                                            // Имеет ли машину
    RUB payment;                                                                    // Оплата квартиры (аренда  или ипотека)
    RUB income;                                                                     // Начисления
    RUB bank_account;                                                               // Текущий счёт
    RUB saves = 0;                                                                  // Вклад
};

 // Расчёт ежемесячной оплаты ипотеки
RUB month_payout(double percent, int sum, int months) 
{
    return (sum * (((percent / 12.0) * pow((1 + percent / 12.0), months)) / (pow((1 + percent / 12.0), months) - 1)));
}

RUB first_payment = 1500 * 1000 + (mt() % (500 * 1000));                            // Первый взнос
RUB appartament_price = 8 * 1000 * 1000 + (mt() % (2 * 1000 * 1000));               // Стоимость квартиры
RUB car_price = 1000 * 1000 + (mt() % (500 * 1000));                                // Стоимость машины
double banks_percent = 0.1;                                                         // Процент в банке (по вкладам и ипотеке)
double inflation = 1;                                                               // Коэффициент инфляции
double percent_inflation = 0.005;                                                   // Процент инфляции

 // Вектор людей
std::vector<Person> human
{
    {"Alice", mortgage, 1, month_payout(banks_percent, appartament_price - first_payment, 12 * 20)},
    {"Bob", contribution, 0, 60000}
};

 // Инициализация начислений и текущего счёта людей
void person_init() 
{

    for (auto& per : human)
    {
        per.income = 100000 + (mt() % 20000);
        per.bank_account = per.income;
    }
    
}

 // Итоговый вывод счёта людей
void person_print() 
{
    for (auto& per : human)
    {
        if ((per.type == contribution) && (per.bank_account + 2 * per.income >= appartament_price))
            per.bank_account -= (appartament_price);
        per.bank_account += appartament_price;
        if (per.isHasCar)
            per.bank_account += car_price;
        std::cout << per.name << " bank account = " << per.bank_account << " rub.\n";
    }

    // Наибольшее количество денег под конец срока и счётчик (для индекса вектора)
    int counter = 0;

    for (int i = 1; i < human.size(); i++)
    {
        if (human[i].bank_account > human[i - 1].bank_account)
            counter = i;
    }

    std::cout << "\nThe happiest person is " << human[counter].name << " with " << human[counter].bank_account << " rubles\n";
}

 // Начисления людей + повышение
void person_income(const int& year, const int& month) 
{
    for (auto& per : human)
    {
        if (year == 2030 && month == 10) {
            per.income *= 1.5; //Promotion
        }
        per.bank_account += per.income;
    }

}

 // Траты на еду
void person_food() 
{
    int food_average_price = 7000 + (mt() % 2000);
    for (auto& per : human)
    {
        per.bank_account -= (food_average_price * inflation);
    }
}

// Траты на одежду
void person_clothes() 
{
    int clothes_average_price = 3000 + (mt() % 2000);
    for (auto& per : human)
    {
        per.bank_account -= (clothes_average_price * inflation);
    }
}

// Траты на аренду жилья или ипотеки
void person_mortage() 
{
    for (auto& per : human)
    {
        if (per.type == mortgage)
            per.bank_account -= per.payment;
        else
            per.bank_account -= (per.payment * inflation);
    }
}

// Траты на автомобиль (при его наличии)
void person_car() 
{
    int car_service_average_price = 4000 + (mt() % 2000);
    for (auto& per : human)
    {
        if (per.isHasCar == 1)
            per.bank_account -= (car_service_average_price * inflation);
    }
}

 // Траты на коммунальные услуги
void person_public_utilities() 
{
    int public_utilities_average_price = 1500 + (mt() % 2000);
    for (auto &per: human)
    {
        per.bank_account -= public_utilities_average_price * inflation;
    }
}
// Вложение на вклад + начисления процентов на него
void money_saving() 
{
    for (auto& per : human) {
        if (per.type == contribution) {
            per.saves += per.bank_account;
            per.bank_account = 0;
            per.saves *= (1 + (banks_percent / 12.0));
        }
    }
}

// Ошибка, при недостаточном количестве средств
bool isEnoughMoney()
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
void inflation_raise() 
{
    appartament_price *= 1 + percent_inflation;
    car_price *= 1 + percent_inflation;
    inflation = (1 + percent_inflation);
}

// увеличение зарплаты относительно инфляции (годовой)
void salary_raise() 
{
    for (auto& per : human)
    {
            per.income *= inflation;
    }
}

// Снятие средств со вклада
void withdrawal_saves() 
{
    for (auto& per : human)
    {
        if (per.type == contribution) {
            per.bank_account += per.saves;
        }
    }
}

 // Симуляция логики течения времени: начислений и трат
void simulation() {
    int year = 2025;                        // Начальный год
    int month = 9;                          // Начальный месяц


    // Основной цикл течения времени
    while (!(year == 2045 && month == 9)) 
    {
        // Функции прибавок, трат и откладываний денег
        person_income(year, month);                                 
        person_food();                                              
        person_mortage();                                           
        person_clothes();                                           
        person_car();                                               
        person_public_utilities();                                  
        money_saving();                                             

        // + месяц, + инфляция
        month++;
        inflation_raise();

        // Смена года
        if (month == 13) {
            year++;
            month = 1;

            // Рост зарплаты с инфляцией
            salary_raise();
        }
        // Выход из цикла при ошибке
        if (isEnoughMoney())
            break;
    }
    withdrawal_saves();
}
int main() 
{
    person_init();

    simulation();

    person_print();
}
