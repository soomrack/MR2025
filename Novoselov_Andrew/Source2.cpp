#include <iostream>
#include <iomanip>

using namespace std;

typedef int RUB;

struct Person {
    RUB bank_account;
    RUB income;
    RUB food;
    RUB rent;
    RUB savings;
    RUB mortgage_payment;
    RUB apartment_value;
};

struct Person alice;
struct Person bob;

void alice_init()
{
    alice.bank_account = 1000 * 1000;
    alice.income = 150 * 1000;
    alice.food = 30000;
    alice.rent = 0;
    alice.savings = 0;
    alice.mortgage_payment = 80000;
    alice.apartment_value = 10000 * 1000;
}

void bob_init()
{
    bob.bank_account = 1000 * 1000;
    bob.income = 150 * 1000;
    bob.food = 30000;
    bob.rent = 40000;
    bob.savings = 0;
    bob.mortgage_payment = 0;
    bob.apartment_value = 0;
}

void alice_income(const int year, const int month)
{
    if (month == 1) {
        alice.income = alice.income * 1.07;  // Annual salary indexation
    }

    if (year == 2028 && month == 3) {
        alice.income *= 1.5;  // Promotion
    }

    alice.bank_account += alice.income;
}

void bob_income(const int year, const int month)
{
    if (month == 1) {
        bob.income = bob.income * 1.07;  // Annual salary indexation
    }

    if (year == 2028 && month == 3) {
        bob.income *= 1.3;  // Promotion
    }

    bob.bank_account += bob.income;
}

void alice_food()
{
    alice.bank_account -= alice.food;
}

void bob_food()
{
    bob.bank_account -= bob.food;
}

void alice_mortgage(const int year, const int month)
{
    if (year == 2026 && month == 1) {
        alice.bank_account -= 2000 * 1000;  // Down payment
    }

    if (year >= 2026 && year <= 2035) {
        alice.bank_account -= alice.mortgage_payment;
    }
}

void bob_rent()
{
    bob.bank_account -= bob.rent;
}

void alice_invest(const int year, const int month)
{
    if (month == 12) {
        RUB investment = alice.bank_account * 0.1;  // 10% of savings
        alice.savings += investment;
        alice.bank_account -= investment;

        // Investment return
        alice.savings *= 1.12;

        // Apartment value appreciation
        alice.apartment_value *= 1.10;
    }
}

void bob_invest(const int year, const int month)
{
    if (month == 12) {
        RUB investment = (bob.income - bob.rent - bob.food) * 0.3;  // 30% of disposable income
        bob.savings += investment;
        bob.bank_account -= investment;

        // Investment return
        bob.savings *= 1.12;
    }
}

void simulation()
{
    int year = 2026;
    int month = 1;

    while (!(year == 2036 && month == 1)) {
        alice_income(year, month);
        bob_income(year, month);

        alice_food();
        bob_food();

        alice_mortgage(year, month);
        bob_rent();

        alice_invest(year, month);
        bob_invest(year, month);

        ++month;
        if (month == 13) {
            month = 1;
            ++year;
        }
    }
}

void print_results()
{
    RUB alice_total = alice.bank_account + alice.savings + alice.apartment_value;
    RUB bob_total = bob.bank_account + bob.savings;

    cout << "10-YEAR FINANCIAL RESULTS (2026-2035):" << endl;
    cout << "Alice (Mortgage Strategy):" << endl;
    cout << "  Bank Account: " << alice.bank_account << " RUB" << endl;
    cout << "  Investments: " << alice.savings << " RUB" << endl;
    cout << "  Apartment Value: " << alice.apartment_value << " RUB" << endl;
    cout << "  TOTAL: " << alice_total << " RUB" << endl;

    cout << "Bob (Rent Strategy):" << endl;
    cout << "  Bank Account: " << bob.bank_account << " RUB" << endl;
    cout << "  Investments: " << bob.savings << " RUB" << endl;
    cout << "  TOTAL: " << bob_total << " RUB" << endl;

    cout << "DIFFERENCE: ";
    if (alice_total > bob_total) {
        cout << "Alice is richer by " << (alice_total - bob_total) << " RUB" << endl;
    }
    else {
        cout << "Bob is richer by " << (bob_total - alice_total) << " RUB" << endl;
    }
}

int main()
{
    alice_init();
    bob_init();

    simulation();

    print_results();

    return 0;
}