#include <iostream>
#include <vector>
#include <string>
#include <cmath>
#include <iomanip>

typedef long long int RUB;

// Тип покупки (накопления или ипотека)
enum typeOfMoneySaving {
    contribution = 1,
    mortgage = 2
};

// Структура - Человек; 
struct Person {
    const std::string name;                             // Имя
    const typeOfMoneySaving type;                       // Тип покупки
    const bool isHasCar;                                // Имеет ли машину
    RUB payment;                                        // Оплата квартиры (аренда  или ипотека)
    RUB bank_account;                                   // Текущий счёт
    RUB income;                                         // Начисления
    RUB saves = 0;                                      // Вклад
};

 // Расчёт ежемесячной оплаты ипотеки
RUB month_payout(double percent, int sum, int months) {
    //std::cout << (sum * (((percent / 12) * pow((1 + percent / 12), months)) / (pow((1 + percent / 12), months) - 1))) << std::endl;
    return (sum * (((percent / 12.0) * pow((1 + percent / 12.0), months)) / (pow((1 + percent / 12.0), months) - 1)));
}

RUB first_payment = 2 * 1000 * 1000;                    // Первый взнос
RUB appartament_price = 10 * 1000 * 1000;               // Стоимость квартиры
double banks_percent = 0.12;                            // Процент в банке (по вкладам и ипотеке)
double inflation = 1;                                   // Коэффициент инфляции
double percent_inflation = 0.005;                       // Процент инфляции

 // Вектор людей
std::vector<Person> human{
    {"Alice", mortgage, 1, month_payout(0.12, appartament_price - first_payment, 12 * 20)},
    {"Bob", contribution, 0, 40000}
};

 // Вектор зарплат
std::vector<RUB> salaries{ 100000, 100000 };

 // Инициализация начислений и текущего счёта людей
void person_init(const std::vector<RUB>& income) {

    for (int i = 0; i < human.size(); i++)
    {
        human[i].bank_account = income[i];
        human[i].income = income[i];
        // Debug
        //std::cout << human[i].bank_account << " " << human[i].income << std::endl;
    }
    
}

 // Итоговый вывод счёта людей
void person_print() {
    for (int i = 0; i < human.size(); i++)
    {
        if ((human[i].type == contribution) && (human[i].bank_account + 2 * human[i].income >= appartament_price))
            human[i].bank_account -= (appartament_price);
        std::cout << human[i].name << " bank account = " << human[i].bank_account << " rub.\n";
    }
}

 // Начисления людей + повышение
void person_income(const int& year, const int& month) {
    for (int i = 0; i < human.size(); i++)
    {
        if (year == 2030 && month == 10) {
            human[i].income *= 1.5; //Promotion
        }
        human[i].bank_account += human[i].income;
    }

}

 // Траты на еду
void person_food() {
    int food_average_price = 8000;
    for (int i = 0; i < human.size(); i++)
    {
        human[i].bank_account -= (food_average_price * inflation);
    }
}

// Траты на одежду
void person_clothes() {
    int clothes_average_price = 4000;
    for (int i = 0; i < human.size(); i++)
    {
        human[i].bank_account -= (clothes_average_price * inflation);
    }
}

// Траты на аренду жилья или ипотеки
void person_mortage() {
    for (int i = 0; i < human.size(); i++)
    {
        if (human[i].type == mortgage)
            human[i].bank_account -= human[i].payment;
        else
            human[i].bank_account -= (human[i].payment * inflation);
    }
}

// Траты на автомобиль (при его наличии)
void person_car() {
    int car_service_average_price = 5000;
    for (int i = 0; i < human.size(); i++)
    {
        if (human[i].isHasCar == 1)
            human[i].bank_account -= (car_service_average_price * inflation);
    }
}

 // Траты на коммунальные услуги
void person_public_utilities() {
    int public_utilities_average_price = 2500;
    for (int i = 0; i < human.size(); i++)
    {
        human[i].bank_account -= public_utilities_average_price * inflation;
    }
}

 // Симуляция логики течения времени: начислений и трат
void simulation() {
    int year = 2025;                        // Начальный год
    int month = 9;                          // Начальный месяц
    bool error_handle = 0;                  // Флаг ошибки (при недостаточном количестве средств)


    // Основной цикл течения времени
    while (!(year == 2045 && month == 9)) {
        person_income(year, month);
        //std::cout << human[1].bank_account << "- 1" << std::endl;     // Debug
        person_food();
        //std::cout << human[1].bank_account << "- 2" << std::endl;     // Debug
        person_mortage();
        //std::cout << human[1].bank_account << "- 3" << std::endl;     // Debug
        person_clothes();
        person_car();
        person_public_utilities();
        //std::cout << human[1].bank_account << "- 4" << std::endl;     // Debug

        appartament_price *= 1 + percent_inflation;
        
        for (int i = 0; i < human.size(); i++)
        {
            // Вложение на вклад + начисления процентов на него
            if (human[i].type == contribution) {
                human[i].saves += 20000;
                human[i].bank_account -= 20000;
                human[i].saves *= (1 + (banks_percent / 12.0));
            }
            // Ошибка, при недостаточном количестве средств
            if (human[i].bank_account < 0) {
                std::cout << human[i].name << " can\'t handle this life" << std::endl;
                error_handle = 1;
                break;
            }
        }

        // + месяц, + инфляция
        month++;
        inflation *= (1 + percent_inflation);

        // Смена года
        if (month == 13) {
            year++;
            month = 1;
            for (int i = 0; i < human.size(); i++)
            {
                //std::cout << "NEW YEARRRRRRRRR" << std::endl;                     // Debug
                
                // увеличение зарплаты относительно инфляции (годовой)
                if (year == 2025)
                    human[i].income *= inflation;
                else
                    human[i].income *= pow(1 + percent_inflation, 12);
                //std::cout << human[i].bank_account << " - " << i << std::endl;    // Debug
            }
        }
        // Выход из цикла при ошибке
        if (error_handle)
            break;
    }

    // Снятие средств со вклада
    for (int i = 1; i < human.size(); i++)
    {
        if (human[i].type == contribution) {
            human[i].bank_account += human[i].saves;
        }
    }
}
int main() {
    person_init(salaries);

    simulation();

    person_print();

    // Наибольшее количество денег под конец срока и счётчик (для индекса вектора)
    RUB most_money = human[0].bank_account;
    int counter = 0;

    for (int i = 1; i < human.size(); i++)
    {
        if (human[i].bank_account > human[i - 1].bank_account)
            counter = i;
    }
    
    std::cout << "\nThe happiest person is " << human[counter].name << " with " << human[counter].bank_account << " rubles\n";
    //std::cout << humans[0].income << std::endl;               // Debug
}
