#include <stdio.h>
#include <windows.h>

typedef long long int RUB;

struct Person {
    RUB bank_account;
    RUB income;
};

struct Person alice;

void alice_init() {
    alice.bank_account = 1000 * 1000;
    alice.income = 200 * 1000;
}

void alice_print() {
    printf("Alice bank account = %lld руб.\n", alice.bank_account);
}

void alice_income(const int year, const int month) {
    if (year == 2030 && month == 10) {
        alice.income *= 1.5; //Promotion
    }
    alice.bank_account += alice.income;

}
void simulation() {
    int year = 2025;
    int month = 9;

    while (!(year == 2045 && month == 9)) {
        alice_income(year, month);
        // alice_mortage();
        // alice_spending();
        // alice_food(year, month);
        // alice_car();
        // alice_trip();

        month++;
        if (month == 13) {
            year++;
            month = 1;
        }
    }
}
int main() {

    SetConsoleCP(1251);

    SetConsoleOutputCP(1251);

    alice_init();

    simulation();

    alice_print();


}

