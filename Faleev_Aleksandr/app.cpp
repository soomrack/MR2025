#include <stdio.h>
#include <stdexcept>
#include <cmath>
#include <vector>
#include <memory>
#include <iostream>

// RUB - тип для рублей
typedef int RUB;

// --- Структура для даты с проверкой правильности ввода ---
struct Date
{
    int year;
    int month;

    // Конструктор с проверкой коректности даты
    Date(int y, int m)
    {
        // Проверка года
        if (y < 0)
        {
            throw std::invalid_argument("Год должен быть больше или равен 0");
        }

        // Проверка месяца
        if (m < 0 || m > 12)
        {
            throw std::invalid_argument("Месяц должен быть от 0 до 12");
        }

        // Если проверки пройдены, присваиваем значения
        year = y;
        month = m;
    }

    // Метод для получения следующей даты
    Date next_month() const
    {
        int next_year = year;
        int next_month = month;

        if (month == 12)
        {
            next_year++;
            next_month = 1;
        }
        else
        {
            next_month++;
        }

        return Date(next_year, next_month);
    }

    // Операторы для сравнения дат
    bool operator==(const Date &other) const
    {
        return year == other.year && month == other.month;
    }

    bool operator!=(const Date &other) const
    {
        return !(*this == other);
    }

    bool operator<(const Date &other) const
    {
        if (year < other.year)
            return true;
        if (year > other.year)
            return false;
        return month < other.month;
    }

    bool operator>(const Date &other) const
    {
        return other < *this;
    }

    bool operator<=(const Date &other) const
    {
        return !(*this > other);
    }

    bool operator>=(const Date &other) const
    {
        return !(*this < other);
    }
};

// --- Структура дома ---
class House {
public:
    RUB price;
    RUB rent_payment = price * 0.055 / 12;
    RUB mortage_payment;

    House(RUB p) : price(p) {
        mortage_payment = price / (12 * 20) + price / 12 * 0.15;
    };

    void raise_price() {
        price *= 1.04;
    }
};

// --- Класс человека ---
class Person {
// TODO: Остановка симуляции при отрицательном бюджете
public:
    RUB salary;
    RUB total_capital;
    House* house;
    RUB money_left_in_month = 0;

    // - Конструктор для передачи начальныхх значений -
    Person(RUB s, RUB c)
        : salary(s), total_capital(c) {}

    // - Деструктор -
    virtual ~Person() = default;

    // - Функция для списывания денег раз в месяц -
    virtual void spend_money() = 0;

    // - Функция для пополнения счета раз в месяц -
    virtual void add_money() = 0;
};
class Alice : public Person {
public:
    Alice(RUB s, RUB c)
        : Person(s, c) {}

    void spend_money() override {
        pay_mortage();
        pay_utility_bill();
        pay_monthly_expenses();
        invest();
    }
    void add_money() override {
        get_house_part();
        recieve_salary();
    }
private:
    void get_house_part() {
        total_capital += house->price / (12 * 20);
    }
    void recieve_salary() {
        money_left_in_month = salary;
    }
    void pay_mortage() {
        money_left_in_month -= house->mortage_payment;
    }
    void pay_utility_bill() {
        money_left_in_month -= 4000;
    }
    void pay_monthly_expenses() {
        money_left_in_month -= 65*1000;
    }
    void invest() {
        total_capital += money_left_in_month;
    }
};

class Bob : public Person {
public:
    Bob(RUB s, RUB c)
        : Person(s, c) {
    }

    void spend_money() override {
        pay_rent();
        pay_utility_bill();
        pay_monthly_expenses();
        invest();
    }
    void add_money() override {
        recieve_salary();
    }

private:
    void recieve_salary() {
        money_left_in_month = salary;
    }
    void pay_rent() {
        money_left_in_month -= house->rent_payment;
    }
    void pay_utility_bill() {
        money_left_in_month -= 4000;
    }
    void pay_monthly_expenses() {
        money_left_in_month -= 100*1000;
    }
    void invest() {
        total_capital += money_left_in_month;
    }
};


// --- Класс для проведения симуляции ---
class Simulation {
private:
    Date current_date;

public:
    Date start_date;
    Date end_date;

    std::vector<std::reference_wrapper<Person>> persons;
    House house;
    
    // Конструктор
    Simulation(
        Date& start,
        Date& end,
        std::vector<std::reference_wrapper<Person>> persons,
        House& h
    )
        : start_date(start), end_date(end), current_date(start), house(h), persons(persons) {
            for (auto& p : persons) {
                p.get().house = &house  ;
            }
        }
    
    // - Функция запускающая симуляцию с шагом в 1 месяц -
    void run() {
        printf("цена дома: %d\n", house.price); // DEBUG
        while(current_date <= end_date) {
            // TODO: Редкие события, например "повышение зп" или "непредвиденные расходы"

            for (auto& p : persons) {
                // Расходы и доходы людей
                p.get().add_money();
                p.get().spend_money();
            }

            // Рост дома в цене
                if (current_date.month == 12) {
                    house.raise_price();
                }

            
            

            // Переходим к следующему месяцу
            current_date = current_date.next_month();
        }
        printf("Конечная цена дома %d \n", house.price); // DEBUG

    }

    // - Функция для вывода результатов -
    void print_result() {
        for (auto& p : persons) {
            printf("Капитал равняется %d\n", p.get().total_capital);
        }
    }
};


int main() {
    // --- Создаем начальные условия ---
    Date start_date(2025, 9);
    Date end_date(2045, 12);

    Alice alice(200*1000, 0);
    Bob bob(200*1000, 0);
    House house(9*1000*1000);

    // --- Создаем симуляцию ---
    Simulation sim(
        start_date,
        end_date,
        std::vector<std::reference_wrapper<Person>>{alice, bob},
        house
    );

    // --- Запускаем симуляцию ---
    sim.run();
    sim.print_result();

    return 0;
}


// TODO:
// - Реализовать трату и получение денег у людей
