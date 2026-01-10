#include <iostream>
#include <vector>
#include <stdexcept>
#include <cmath>

using RUB = int;

// Конфигурационные константы
constexpr double MORTGAGE_INTEREST_RATE = 0.15;
constexpr double INITIAL_RENT_RATE = 0.055;
constexpr double HOUSE_APPRECIATION_RATE = 0.04;
constexpr double RENT_INFLATION_RATE = 0.03;
constexpr double GENERAL_INFLATION_RATE = 0.06;
constexpr RUB UTILITY_BILL = 4000;
constexpr RUB FOOD_EXPENSES_ALICE = 30000;
constexpr RUB FOOD_EXPENSES_BOB = 35000;
constexpr RUB ENTERTAINMENT_EXPENSES_ALICE = 15000;
constexpr RUB ENTERTAINMENT_EXPENSES_BOB = 20000;
constexpr RUB TRANSPORT_EXPENSES = 10000;
constexpr RUB INSURANCE_EXPENSES = 5000;

struct Date {
    int year;
    int month;

    Date(int y, int m) : year(y), month(m) {
        if (y < 0) throw std::invalid_argument("Год должен быть >= 0");
        if (m < 1 || m > 12) throw std::invalid_argument("Месяц должен быть от 1 до 12");
    }

    Date next_month() const {
        return month == 12 ? Date(year + 1, 1) : Date(year, month + 1);
    }

    bool operator==(const Date& other) const { return year == other.year && month == other.month; }
    bool operator!=(const Date& other) const { return !(*this == other); }
    bool operator<(const Date& other) const {
        return year < other.year || (year == other.year && month < other.month);
    }
    bool operator>(const Date& other) const { return other < *this; }
    bool operator<=(const Date& other) const { return !(other < *this); }
    bool operator>=(const Date& other) const { return !(*this < other); }
};

class Person {
public:
    RUB salary;
    RUB total_capital;
    RUB money_left_in_month = 0;
    bool owns_house = false;

    Person(RUB s, RUB c, bool owns = false) : salary(s), total_capital(c), owns_house(owns) {}
    virtual ~Person() = default;
    virtual void spend_money(RUB rent_payment) = 0;
    virtual void add_money(RUB house_price, int mortgage_months_left) = 0;
};

class Alice : public Person {
public:
    Alice(RUB s, RUB c) : Person(s, c, true) {}

    void spend_money(RUB /*rent_payment*/) override {
        pay_utility_bill();
        pay_food();
        pay_entertainment();
        pay_transport();
        pay_insurance();
        if (mortgage_payment > 0) {
            money_left_in_month -= mortgage_payment;
        }
        invest();
    }

    void add_money(RUB house_price, int mortgage_months_left) override {
        // Рассчитываем ежемесячный платёж по ипотеке один раз в начале срока
        // Используем упрощённую формулу аннуитетного платежа:
        // payment = P * r * (1 + r)^n / ((1 + r)^n - 1),
        // где P — сумма кредита, r — месячная ставка, n — количество месяцев.
        // Чтобы избежать сложных вычислений каждый месяц, рассчитываем один раз при первом вызове.
        if (mortgage_payment == 0 && mortgage_months_left > 0) {
            double monthly_rate = MORTGAGE_INTEREST_RATE / 12.0;
            int n = mortgage_months_left;
            double numerator = monthly_rate * std::pow(1 + monthly_rate, n);
            double denominator = std::pow(1 + monthly_rate, n) - 1;
            mortgage_payment = static_cast<RUB>(house_price * (numerator / denominator));
        }
        money_left_in_month = salary;
    }

private:
    RUB mortgage_payment = 0;

    void pay_utility_bill() { money_left_in_month -= UTILITY_BILL; }
    void pay_food() { money_left_in_month -= FOOD_EXPENSES_ALICE; }
    void pay_entertainment() { money_left_in_month -= ENTERTAINMENT_EXPENSES_ALICE; }
    void pay_transport() { money_left_in_month -= TRANSPORT_EXPENSES; }
    void pay_insurance() { money_left_in_month -= INSURANCE_EXPENSES; }
    void invest() { 
        if (money_left_in_month > 0) {
            total_capital += money_left_in_month;
        }
    }
};

class Bob : public Person {
public:
    Bob(RUB s, RUB c) : Person(s, c, false) {}

    void spend_money(RUB rent_payment) override {
        money_left_in_month -= rent_payment;
        pay_utility_bill();
        pay_food();
        pay_entertainment();
        pay_transport();
        pay_insurance();
        invest();
    }

    void add_money(RUB /*house_price*/, int /*mortgage_months_left*/) override {
        money_left_in_month = salary;
    }

private:
    void pay_utility_bill() { money_left_in_month -= UTILITY_BILL; }
    void pay_food() { money_left_in_month -= FOOD_EXPENSES_BOB; }
    void pay_entertainment() { money_left_in_month -= ENTERTAINMENT_EXPENSES_BOB; }
    void pay_transport() { money_left_in_month -= TRANSPORT_EXPENSES; }
    void pay_insurance() { money_left_in_month -= INSURANCE_EXPENSES; }
    void invest() { 
        if (money_left_in_month > 0) {
            total_capital += money_left_in_month;
        }
    }
};

int main() {
    Date current_date(2025, 9);
    const Date end_date(2045, 12);

    Alice alice(200'000, 0);
    Bob bob(200'000, 0);
    RUB house_price = 9'000'000;
    RUB rent_payment = static_cast<RUB>(house_price * INITIAL_RENT_RATE / 12);

    const int total_mortgage_months = 12 * 20;
    int mortgage_months_left = total_mortgage_months;

    while (current_date <= end_date) {
        // Доходы и расходы
        alice.add_money(house_price, mortgage_months_left);
        bob.add_money(house_price, 0);

        alice.spend_money(0);
        bob.spend_money(rent_payment);

        // Обновление ипотеки
        if (mortgage_months_left > 0) --mortgage_months_left;

        // Ежегодная инфляция и рост цен
        if (current_date.month == 12) {
            house_price = static_cast<RUB>(house_price * (1.0 + HOUSE_APPRECIATION_RATE + GENERAL_INFLATION_RATE));
            rent_payment = static_cast<RUB>(rent_payment * (1.0 + RENT_INFLATION_RATE));
        }

        current_date = current_date.next_month();
    }

    std::cout << "Конечная цена дома: " << house_price << "\n";
    std::cout << "Капитал Алисы (деньги): " << alice.total_capital<< "\n";
    std::cout << "Капитал Алисы (деньги + дом): " << alice.total_capital + house_price<< "\n";
    std::cout << "Капитал Боба (только деньги): " << bob.total_capital << "\n";

    return 0;
}
