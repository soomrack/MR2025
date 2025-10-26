#include <iostream>
#include <string>


using RUB = long long;


enum class HousingType {
    Mortgage,
    Rent
};


enum class CarType {
    Economy,
    Comfort,
    Premium
};

// Структура для инфляции
struct Inflation {
    double products;
    double utilities;
    double housing;
    double salary;
    double savings;


    Inflation(double p, double u, double h, double s, double sav) {
        products = p;
        utilities = u;
        housing = h;
        salary = s;
        savings = sav;
    }


    Inflation() {
        products = 0.0;
        utilities = 0.0;
        housing = 0.0;
        salary = 0.0;
        savings = 0.0;
    }
};

// Класс автомобиля
class Car {
private:
    std::string brand;
    CarType type;
    RUB fuel_cost;
    RUB maintenance_cost;
    RUB insurance_cost;
    bool has_insurance;

public:
    Car(const std::string& b, CarType t, RUB fuel, RUB maintenance, RUB insurance, bool insurance_flag) {
        brand = b;
        type = t;
        fuel_cost = fuel;
        maintenance_cost = maintenance;
        insurance_cost = insurance;
        has_insurance = insurance_flag;
    }


    RUB getMonthlyFuelCost() const {
        return fuel_cost;
    }


    RUB getMaintenanceCost(int month) const {
        // Обслуживание раз в 6 месяцев
        if (month % 6 == 0) {
            return maintenance_cost;
        }
        else {
            return 0;
        }
    }


    RUB getInsuranceCost(int month) const {
        // Страховка оплачивается в июне
        if (has_insurance && month == 6) {
            return insurance_cost;
        }
        else {
            return 0;
        }
    }


    void applyInflation(const Inflation& inflation) {
        fuel_cost = fuel_cost + (RUB)(fuel_cost * inflation.products);
        maintenance_cost = maintenance_cost + (RUB)(maintenance_cost * inflation.products);
        insurance_cost = insurance_cost + (RUB)(insurance_cost * inflation.products);
    }


    std::string getBrand() const {
        return brand;
    }


    CarType getType() const {
        return type;
    }
};

// Класс жилья
class Home {
private:
    HousingType type;
    RUB mortgage_payment;
    RUB rent_payment;
    RUB utilities_base;
    int mortgage_start_year;
    int mortgage_start_month;
    int mortgage_duration_months;

public:
    Home(HousingType t, RUB mortgage, RUB rent, RUB utilities,
        int start_year, int start_month, int duration) {
        type = t;
        mortgage_payment = mortgage;
        rent_payment = rent;
        utilities_base = utilities;
        mortgage_start_year = start_year;
        mortgage_start_month = start_month;
        mortgage_duration_months = duration;
    }


    RUB getHousingPayment(int year, int month) const {
        if (type == HousingType::Mortgage) {
            if (isMortgageActive(year, month)) {
                return mortgage_payment;
            }
            else {
                return 0;
            }
        }
        else {
            return rent_payment;
        }
    }


    RUB getUtilitiesCost() const {
        return utilities_base;
    }


    bool isMortgageActive(int year, int month) const {
        int total_months = (year - mortgage_start_year) * 12 + (month - mortgage_start_month);
        if (total_months >= 0 && total_months < mortgage_duration_months) {
            return true;
        }
        else {
            return false;
        }
    }


    void applyInflation(const Inflation& inflation) {
        if (type == HousingType::Rent) {
            rent_payment = rent_payment + (RUB)(rent_payment * inflation.housing);
        }
        utilities_base = utilities_base + (RUB)(utilities_base * inflation.utilities);
    }


    HousingType getType() const {
        return type;
    }
};

// Банковский счет
class BankAccount {
private:
    RUB balance;
    double interest_rate;

public:
    BankAccount(RUB initial_balance, double rate) {
        balance = initial_balance;
        interest_rate = rate;
    }


    BankAccount(RUB initial_balance) {
        balance = initial_balance;
        interest_rate = 0.0;
    }


    void deposit(RUB amount) {
        balance = balance + amount;
    }


    bool withdraw(RUB amount) {
        if (balance >= amount) {
            balance = balance - amount;
            return true;
        }
        return false;
    }


    void applyInterest(int year, int month) {
        // Начисление процентов в декабре
        if (month == 12 && interest_rate > 0) {
            RUB interest = (RUB)(balance * interest_rate);
            balance = balance + interest;
        }
    }


    void applyInflation(const Inflation& inflation) {
        // Инфляция уменьшает реальную стоимость сбережений
        RUB inflation_loss = (RUB)(balance * inflation.savings);
        balance = balance - inflation_loss;
    }


    RUB getBalance() const {
        return balance;
    }


    void setInterestRate(double rate) {
        interest_rate = rate;
    }
};

// Основной класс человека
class Person {
private:
    std::string name;
    BankAccount savings_account;
    BankAccount checking_account;
    Home home;
    Car car;
    RUB salary;
    RUB food_expenses;
    RUB entertainment_expenses;
    int salary_increase_year;
    int salary_increase_month;
    double salary_increase_factor;

public:
    Person(const std::string& n, RUB initial_savings, RUB initial_salary,
        Home h, Car c)
        : savings_account(initial_savings, 0.05), checking_account(0), home(h), car(c) {

        name = n;
        salary = initial_salary;
        food_expenses = 40000;
        entertainment_expenses = 15000;
        salary_increase_year = 0;
        salary_increase_month = 0;
        salary_increase_factor = 1.0;

        // Установка повышений зарплаты
        if (name == "Alice") {
            salary_increase_year = 2030;
            salary_increase_month = 10;
            salary_increase_factor = 1.5;
        }
        else if (name == "Bob") {
            salary_increase_year = 2035;
            salary_increase_month = 5;
            salary_increase_factor = 1.75;
        }
    }


    void monthlyUpdate(int year, int month, const Inflation& inflation) {
        applyIncome(year, month, inflation);
        applyExpenses(year, month, inflation);

        // Применяем инфляцию к различным компонентам
        home.applyInflation(inflation);
        car.applyInflation(inflation);
        savings_account.applyInflation(inflation);

        // Начисление процентов по вкладу
        savings_account.applyInterest(year, month);
    }


    void print() const {
        std::cout << name << "'s financial status:\n";
        std::cout << "  Savings account: " << savings_account.getBalance() << " руб.\n";
        std::cout << "  Checking account: " << checking_account.getBalance() << " руб.\n";
        RUB total = savings_account.getBalance() + checking_account.getBalance();
        std::cout << "  Total: " << total << " руб.\n\n";
    }

private:
    void applyIncome(int year, int month, const Inflation& inflation) {
        checkSalaryIncrease(year, month);

        // Зарплата с учетом инфляции
        RUB salary_increase = (RUB)(salary * inflation.salary);
        RUB current_salary = salary + salary_increase;
        checking_account.deposit(current_salary);
    }


    void applyExpenses(int year, int month, const Inflation& inflation) {
        // Продукты с учетом инфляции
        RUB food_increase = (RUB)(food_expenses * inflation.products);
        RUB current_food = food_expenses + food_increase;


        // Основные расходы
        RUB housing_cost = home.getHousingPayment(year, month);
        RUB utilities_cost = home.getUtilitiesCost();


        // Автомобильные расходы
        RUB car_fuel = car.getMonthlyFuelCost();
        RUB car_maintenance = car.getMaintenanceCost(month);
        RUB car_insurance = car.getInsuranceCost(month);


        // Развлечения
        RUB current_entertainment = entertainment_expenses;


        // Отпуск в августе
        RUB vacation = 0;
        if (month == 8) {
            vacation = 120000;
        }


        // Общая сумма расходов
        RUB total_expenses = current_food + housing_cost + utilities_cost +
            car_fuel + car_maintenance + car_insurance +
            current_entertainment + vacation;


        // Оплата с текущего счета
        if (checking_account.withdraw(total_expenses)) {
            // Все оплатили с текущего счета
        }
        else {
            // Если на текущем счете недостаточно средств, используем сбережения
            RUB checking_balance = checking_account.getBalance();
            RUB remaining = total_expenses - checking_balance;
            checking_account.withdraw(checking_balance);
            savings_account.withdraw(remaining);
        }
    }


    void checkSalaryIncrease(int year, int month) {
        if (year == salary_increase_year && month == salary_increase_month) {
            RUB old_salary = salary;
            salary = (RUB)(salary * salary_increase_factor);
            std::cout << name << " получил повышение зарплаты! ";
            std::cout << "Старая зарплата: " << old_salary << " руб. ";
            std::cout << "Новая зарплата: " << salary << " руб.\n";
        }
    }
};

class Simulation {
private:
    int year;
    int month;
    int end_year;
    int end_month;
    Person& alice;
    Person& bob;
    Inflation annual_inflation;

public:
    Simulation(int start_year, int start_month,
        int finish_year, int finish_month,
        Person& a, Person& b,
        const Inflation& inflation)
        : year(start_year), month(start_month),
        end_year(finish_year), end_month(finish_month),
        alice(a), bob(b), annual_inflation(inflation) {
    }

    Simulation(int start_year, int start_month,
        int finish_year, int finish_month,
        Person& a, Person& b)
        : year(start_year), month(start_month),
        end_year(finish_year), end_month(finish_month),
        alice(a), bob(b) {
        annual_inflation = Inflation(0.03, 0.04, 0.05, 0.02, 0.07);
    }


    void run() {
        std::cout << "Запуск экономической симуляции...\n";
        std::cout << "Период: " << year << "-" << month << " до "
            << end_year << "-" << end_month << "\n\n";

        while (true) {
            if (year == end_year && month == end_month) {
                break;
            }

            Inflation monthly_inflation = getMonthlyInflation();

            alice.monthlyUpdate(year, month, monthly_inflation);
            bob.monthlyUpdate(year, month, monthly_inflation);

            nextMonth();
        }
    }


    void printResults() const {
        std::cout << "\n=== РЕЗУЛЬТАТЫ СИМУЛЯЦИИ ===\n";
        alice.print();
        bob.print();
    }

private:
    void nextMonth() {
        month++;
        if (month == 13) {
            month = 1;
            year++;
        }
    }

    Inflation getMonthlyInflation() const {
        // Ежемесячная инфляция как 1/12 годовой
        Inflation monthly;
        monthly.products = annual_inflation.products / 12.0;
        monthly.utilities = annual_inflation.utilities / 12.0;
        monthly.housing = annual_inflation.housing / 12.0;
        monthly.salary = annual_inflation.salary / 12.0;
        monthly.savings = annual_inflation.savings / 12.0;
        return monthly;
    }
};

int main() {
    setlocale(LC_ALL, "rus");

    // Создание жилья для Алисы (ипотека) и Боба (аренда)
    Home alice_home(HousingType::Mortgage, 60000, 0, 15000, 2025, 9, 180);
    Home bob_home(HousingType::Rent, 0, 55000, 17000, 0, 0, 0);

    // Создание автомобилей
    Car alice_car("Toyota", CarType::Economy, 8000, 20000, 35000, true);
    Car bob_car("Honda", CarType::Comfort, 10000, 25000, 40000, true);

    // Создание людей
    Person alice("Alice", 100000, 150000, alice_home, alice_car);
    Person bob("Bob", 100000, 150000, bob_home, bob_car);

    // Настройка инфляции (годовые проценты)
    Inflation inflation(0.03, 0.04, 0.05, 0.02, 0.07);

    // Запуск симуляции
    Simulation sim(2025, 9, 2040, 9, alice, bob, inflation);
    sim.run();
    sim.printResults();

    return 0;
}
