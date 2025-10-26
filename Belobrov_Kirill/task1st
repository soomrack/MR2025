#include <iostream>
#include <vector>
#include <string>
#include <cmath>


typedef long long int RUB;

// Вариант покупки (ипотека или накопления)
enum bankSaving {
    contribution = 1,
    mortgage = 2
};

RUB first_payment = 2 * 1000 * 1000;                    // Первый взнос
RUB appartament_price = 10 * 1000 * 1000;               // Стоимость квартиры
RUB car_price = 1000 * 1000;                            // Стоимость машины
const double banks_percent = 0.12;                      // Процент в банке (по ипотеке и вкладам)
const double percent_inflation = 0.003;                 // Процент инфляции (сделать нормальны)
double inflation = 1.000;                               // Коэффициент инфляции 
RUB clothes_average_price = 4000;                       // Стоимость одежды
RUB public_utilities_average_price = 2500;              // Плата коммунальных услуг
RUB food_average_price = 8000;                          // Цены на пропитание
RUB car_service_average_price = 5000;                   // Затраты на автомобиль
int mortgage_month = 20 * 12;                           // Срок по ипотеке

// Структура - Человек;
struct Person {
    const std::string name;                             // Имя
    const bankSaving type;                              // Вид покупки
    const bool isHasCar;                                // Наличие машины
    RUB payment;                                        // Оплата квартиры (ипотека или аренда)
    RUB bank_account;                                   // Текущий счёт
    RUB income;                                         // Начисления
    RUB bank_saves;
    bool is_bankrot;

    Person(std::string n, bankSaving t, bool car, RUB p, RUB account, RUB inc)
        : name(n), type(t), isHasCar(car), payment(p), bank_account(account), income(inc)
    {
        bank_saves = (type == bankSaving::mortgage ? -appartament_price : 0);
        is_bankrot = false;
    }
};

// Расчёт ежемесячной оплаты ипотеки
RUB month_payout(double percent, int sum, int months) {
    return (sum * (((percent / 12.0) * pow((1 + percent / 12.0), months))
        / (pow((1 + percent / 12.0), months) - 1)));
}

// Итоговый вывод счёта у людей
void person_print(Person person) {
    std::cout << person.name << " bank account = " << person.bank_account
        + appartament_price + (person.isHasCar ? car_price : 0 ) << " rub.\n";
}

// Начисления людей + повышение ( отдельные функции на каждого)
void alice_income(Person& person, const int& year, const int& month) {
    if (year == 2030 && month == 10) {
        person.income *= 1.5; //Promotion
    }
    person.bank_account += person.income;
    if (month == 12) {
        person.income *= pow(1 + percent_inflation, 12);
    }
}

void bob_income(Person& person, const int year, const int month) {
    if (year == 2030 && month == 10) {
        person.income *= 1.5; //Promotion
    }
    person.bank_account += person.income;
    if (month == 12) {
        person.income *= pow(1 + percent_inflation, 12);
    }
}

// Затраты на еду
void alice_food(Person& person) {
    person.bank_account -= (food_average_price * inflation);
}

void bob_food(Person& person) {
    person.bank_account -= (food_average_price * inflation);
}

// Затраты на одежду
void alice_clothes(Person& person) {
    person.bank_account -= (clothes_average_price * inflation);
}

void bob_clothes(Person& person) {
    person.bank_account -= (clothes_average_price * inflation);
}

// Затраты на аренду жилья или ипотеки
void alice_housing(Person& person) {
    if (person.type == bankSaving::mortgage)
        person.bank_account -= person.payment;
    else {
        person.bank_account -= person.payment;
        person.payment *= inflation;
    }
}

void bob_housing(Person& person) {
    if (person.type == bankSaving::mortgage)
        person.bank_account -= person.payment;
    else {
        person.bank_account -= person.payment;
        person.payment *= inflation;
    }
}

// Затраты на автомобиль (при его наличии)
void alice_car(Person& person) {
    if (person.isHasCar)
        person.bank_account -= (car_service_average_price * inflation);
}

void bob_car(Person& person) {
    if (person.isHasCar)
        person.bank_account -= (car_service_average_price * inflation);
}

// Затраты на коммунальные услуги
void alice_public_utilities(Person& person) {
    person.bank_account -= (public_utilities_average_price * inflation);
}

void bob_public_utilities(Person& person) {
    person.bank_account -= (public_utilities_average_price * inflation);
}


void is_bankort(Person& person)
{
    person.is_bankrot = true;
    std::cout << person.name << " can\'t handle this life" << std::endl;
}

void alice_contribution(Person& person)
{
    if (person.type == bankSaving::contribution) {
        person.bank_saves += person.bank_account;
        person.bank_account = 0;
        person.bank_saves *= (1 + (banks_percent / 12.0));
    }
}

void bob_contribution(Person& person)
{
    if (person.type == bankSaving::contribution) {
        person.bank_saves += person.bank_account;
        person.bank_account = 0;
        person.bank_saves *= (1 + (banks_percent / 12.0));
    }
}

void price_raise() {
    appartament_price *= pow(1 + percent_inflation, 12);
    car_price *= pow(1 + percent_inflation, 12);
}

// Симуляция течения времени: начислений и трат
void simulation(std::vector<Person>& persons) {
    int year = 2025;                        // Первоначальный год
    int month = 9;                          // Первоначальный месяц

    // Цикл течения времени
    while (!(year == 2045 && month == 9)) {
        alice_income(persons[0], year, month);
        alice_food(persons[0]);
        alice_housing(persons[0]);
        alice_clothes(persons[0]);
        alice_car(persons[0]);
        alice_public_utilities(persons[0]);
        alice_contribution(persons[0]);

        bob_income(persons[1], year, month);
        bob_food(persons[1]);
        bob_housing(persons[1]);
        bob_clothes(persons[1]);
        bob_car(persons[1]);
        bob_public_utilities(persons[1]);
        bob_contribution(persons[1]);

        for (auto& person : persons) {
            // Вложение на вклад + начисления процентов на него ( у каждого человека есть сбережения и депозит)
            if (person.type == bankSaving::mortgage && person.bank_saves > 0)
            {
                person.type == bankSaving::contribution;
            }
        }
        // При недостаточном количестве средств
        if (persons[0].bank_account < 0) {
            is_bankort(persons[0]);
            break;
        }
        else if (persons[1].bank_account < 0) {
            is_bankort(persons[1]);
            break;
        }

        // + месяц
        month++;

        // + инфляция
        inflation *= (1 + percent_inflation);

        // Смена года 
        if (month == 13) {
            year++;
            month = 1;
            price_raise();
        }

        for (Person person : persons)
        {
            // Снятие средств со вклада
            if (person.type == bankSaving::contribution) {
               person.bank_account +=person.bank_saves;
            }
        }
    }

}

int main() {
    double month_payout_alice = month_payout(banks_percent, appartament_price, mortgage_month);
    Person alice = Person("Alice", bankSaving::mortgage, true, month_payout_alice, 0, 135000); //сделать раздельно, т.е слишком много
    Person bob = Person("Bob", bankSaving::mortgage, true, 75000, 0, 100000);

    std::vector<Person> persons;
    persons.push_back(alice);
    persons.push_back(bob);

    simulation(persons);

    for (Person person : persons)
    {
        person_print(person);
    }
    //добавить деталей, стоимось рстёт с годами, индексация зарплаты, стоимсоь машины растёт из-за инфляции и дешевеет из-за амортизации
}
