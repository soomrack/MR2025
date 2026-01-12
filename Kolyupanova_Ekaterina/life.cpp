#include <stdio.h>
#include <stdexcept>
#include <iostream>

typedef int RUB;

// === Константы ===
constexpr double INFLATION_RATE = 0.05;                // 5% инфляции в год
constexpr double SALARY_INCREASE_RATE = 0.10;          // +10% к зарплате
constexpr int SALARY_INCREASE_PERIOD_YEARS = 3;        // раз в 3 года
constexpr double DEPOSIT_ANNUAL_RATE = 0.03;           // 3% годовых по депозиту


class Date {
public:
    int year;
    int month;

public:
    Date(int year, int month);
};

Date::Date(int year, int month): year(year), month(month) {
    if (year < 0) {
        throw std::invalid_argument("Год не может быть отрицательным");
    }
    if (month > 12 || month < 1) {
        throw std::invalid_argument("Месяц не может быть больше 12 или отрицательным");
    }
}

class Person
{
public:
    RUB bank_account;

    Person(RUB c, RUB r)
        : bank_account(c) {}
};

class Alice: public Person
{
public:
    RUB salary;
    RUB food_expense;
    RUB utility_bill;
    RUB internet_bill;
    RUB mortage_payment;
    int salary_years;

    Alice(RUB c, RUB r)
        : Person(c, r), salary(200 * 1000), mortage_payment(130362), food_expense(40 * 1000), utility_bill(4 * 1000), internet_bill(1000), salary_years(0) {};

    void sell_picture(const int year, const int month);
    void recieve_salary();
    void pay_mortage();
    void pay_food();
    void pay_utility_bill();
    void pay_internet_bill();

    void apply_inflation();
    void check_salary_increase();

    void income(Date current_date);
    void spend(Date current_date);
};

void Alice::sell_picture(const int year, const int month) {
        if (year == 2030 && month == 3)
            {
                bank_account += 50 * 1000;
            }
}
void Alice::recieve_salary(){
        bank_account += salary; // зарплата
}
void Alice::pay_mortage() {
        bank_account -= mortage_payment; // платеж за ипотеку
}
void Alice::pay_food(){
    bank_account -= food_expense; 
}
void Alice::pay_utility_bill(){
    bank_account -= utility_bill; // комунальные
}
void Alice::pay_internet_bill() {
    bank_account -= internet_bill;
}

void Alice::apply_inflation() {
    food_expense = static_cast<RUB>(food_expense * (1.0 + INFLATION_RATE));
    utility_bill = static_cast<RUB>(utility_bill * (1.0 + INFLATION_RATE));
    internet_bill = static_cast<RUB>(internet_bill * (1.0 + INFLATION_RATE));
}

void Alice::check_salary_increase() {
    salary_years++;
    if (salary_years >= SALARY_INCREASE_PERIOD_YEARS) {
        salary = static_cast<RUB>(salary * (1.0 + SALARY_INCREASE_RATE));
        salary_years = 0;
    }
}    

void Alice::income(Date current_date) {
    Alice::recieve_salary();
    Alice::sell_picture(current_date.year, current_date.month);
}

void Alice::spend(Date current_date) {
    Alice::pay_food();
    Alice::pay_mortage();
    Alice::pay_utility_bill();
    Alice::pay_internet_bill();
}

class Bob: public Person
{
public:
    RUB salary;
    RUB food_expense;
    RUB rent_expense;
    int salary_years;

    Bob(RUB c, RUB r)
        : Person(c, r), salary(200 * 1000), rent_expense(50 * 1000), food_expense(40 * 1000), salary_years(0) {};

    void pay_shtraf(const int year, const int month);
    void recieve_salary();
    void pay_rent();
    void pay_food();

    void apply_inflation();
    void check_salary_increase();

    void income(Date current_date);
    void spend(Date current_date);
};

void Bob::pay_shtraf(const int year, const int month) {
        if (year == 2030 && month == 3)
        {
            bank_account -= 10*1000;
        }
}
void Bob::recieve_salary(){
    bank_account += salary;
}
void Bob::pay_rent(){
    bank_account -= rent_expense;
}
void Bob::pay_food(){
    bank_account -= food_expense;
}

    void Bob::apply_inflation() {
    food_expense = static_cast<RUB>(food_expense * (1.0 + INFLATION_RATE));
    rent_expense = static_cast<RUB>(rent_expense * (1.0 + INFLATION_RATE));
}

    void Bob::check_salary_increase() {
    salary_years++;
    if (salary_years >= SALARY_INCREASE_PERIOD_YEARS) {
        salary = static_cast<RUB>(salary * (1.0 + SALARY_INCREASE_RATE));
        salary_years = 0;
    }
}

void Bob::income(Date current_date){
    recieve_salary();
}
void Bob::spend(Date current_date){
    pay_shtraf(current_date.year, current_date.month);
    pay_food();
    pay_rent();
}

class Simulation
{
public:
    Date current_date;
    Date start_date;
    Date end_date;
    Alice alice;
    Bob bob;
    int current_year;

    Simulation(Date start, Date end, Alice a, Bob b)
        : start_date(start), end_date(end), alice(a), bob(b), current_date(start), current_year(start.year) {}

    void apply_deposit_interest() {
        alice.bank_account += static_cast<RUB>(alice.bank_account * DEPOSIT_ANNUAL_RATE / 12);
        bob.bank_account += static_cast<RUB>(bob.bank_account * DEPOSIT_ANNUAL_RATE / 12);
    }

    void handle_yearly_events() {
        if (current_date.year > current_year) {
            alice.apply_inflation();
            alice.check_salary_increase();
            bob.apply_inflation();
            bob.check_salary_increase();
            current_year = current_date.year;
        }
    }

    void run() {
        current_date = start_date;
        while( !(current_date.year == end_date.year 
                && current_date.month == end_date.month)){
            // 1. Обрабатываем текущий месяц
            alice.spend(current_date);
            alice.income(current_date);
            bob.income(current_date);
            bob.spend(current_date);
            
             // 2. Начисляем проценты на остаток месяца
            apply_deposit_interest();
            // 3. Проверяем, сменился ли год → обновляем инфляцию и зарплату (для следующих месяцев)
            handle_yearly_events();

            // 4. Переходим к следующему месяцу
            current_date.month++;
            if (current_date.month == 13){
                current_date.year ++;
                current_date.month = 1;
            }
        }
    }

    void print_resulte() {
        printf("У Алисы в 2045 счет в банке: %d руб.\n", alice.bank_account);
        printf("У Боба в 2045 счет в банке: %d руб.\n", bob.bank_account);

    }
};

int main() {
    try{
        Date start_date = {2025, 9};
        Date end_date = {2045, 9};
        Alice alice(0, 0);
        Bob bob(0, 0);

        Simulation simulation(
            start_date,
            end_date,
            alice,
            bob); 

        simulation.run();
        simulation.print_resulte();
    }
    
    catch (const std::invalid_argument& e) {
        printf("Ошибка аргумента %s \n", e.what());
    }
    catch (const std::exception& e) {
        printf("Общая ошибка %s \n", e.what());
    }
}
