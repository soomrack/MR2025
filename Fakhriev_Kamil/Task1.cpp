#include <iostream>
#include <string>

using RUB = long long;

enum class HousingType {
    Mortgage,
    Rent
};

class Person {
private:
    std::string name;
    RUB bank_account;
    RUB income;
    HousingType housing;

public:
    Person(std::string n, RUB start_balance, RUB start_income, HousingType h)
        : name(n), bank_account(start_balance), income(start_income), housing(h) {}

    void monthlyUpdate(int year, int month) {
        applyIncome(year, month);
        applyExpenses(year, month);
    }

    void print() {
        std::cout << name << "'s bank account = " << bank_account << " руб.\n";
    }

private:
    void applyIncome(int year, int month) {
        if (name == "Alice" && year == 2030 && month == 10) {
            income = static_cast<RUB>(income * 1.5); // Повышение
        }
        if (name == "Bob" && year == 2035 && month == 5) {
            income = static_cast<RUB>(income * 1.75);
        }
        bank_account += income;
    }

    void applyExpenses(int year, int month) {
        RUB food = 40000;
        RUB utilities = 15000;
        if (name == "Alice") {
            RUB food = 40000;
            RUB utilities = 15000;
            RUB entertainment = 15000;
            bank_account -= (food + utilities + entertainment);
        }
        else if (name == "Bob") {
            RUB food = 45000;
            RUB utilities = 17000;
            RUB entertainment = 10000;
            bank_account -= (food + utilities + entertainment);
        }

        // Жильё: ипотека до 2040 включительно, или аренда
        if (housing == HousingType::Mortgage) {
            if (!(year > 2040 || (year == 2040 && month > 9))) {
                bank_account -= 60000; // ипотека до сентября 2040
            }
        }
        else if (housing == HousingType::Rent && year <= 2030) {
            bank_account -= 55000; // аренда 
        }
        else if (housing == HousingType::Rent && year > 2030) {
            bank_account -= 75000;
        }

        // Машина (страховка в июне)
        if (month == 6) {
            bank_account -= 35000;
        }

        // Отпуск (в августе)
        if (month == 8) {
            bank_account -= 120000;
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

public:
    Simulation(int start_year, int start_month,
        int finish_year, int finish_month,
        Person& a, Person& b)
        : year(start_year), month(start_month),
        end_year(finish_year), end_month(finish_month),
        alice(a), bob(b) {}

    void run() {
        while (!(year == end_year && month == end_month)) {
            alice.monthlyUpdate(year, month);
            bob.monthlyUpdate(year, month);
            nextMonth();
        }
    }

    void printResults() {
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
};

int main() {
    setlocale(LC_ALL, "rus");
    Person alice("Alice", 100 * 1000, 150 * 1000, HousingType::Mortgage);
    Person bob("Bob", 100 * 1000, 150 * 1000, HousingType::Rent);

    Simulation sim(2025, 9, 2040, 9, alice, bob);
    sim.run();
    sim.printResults();

    return 0;
}
