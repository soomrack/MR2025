#include <stdio.h>
#include <math.h>

typedef long long int RUB;

struct Person {
    RUB bank_account;
    RUB income;

    RUB mortgage;
    RUB down_payment;
    RUB rent;

    RUB food;
    RUB clothes;
    RUB trip;

    RUB car_price;
    RUB gasoline;
    RUB car_consumables;
    RUB car_insurance;
};


struct Person alice;
struct Person bob;


void alice_init() {
    alice.bank_account = 150 * 1000;
    alice.income = 135 * 1000;

    alice.mortgage = 5000 * 1000;
    alice.down_payment = 2000 * 1000;

    alice.food = 30 * 1000;
    alice.clothes = 20 * 1000;
    alice.trip = 200 * 1000;

    alice.car_price = 1500 * 1000;
    alice.gasoline = 15 * 1000;
    alice.car_consumables = 7 * 1000;
    alice.car_insurance = 30 * 1000;
}


void bob_init() {
    bob.bank_account = 300 * 1000;
    bob.income = 165 * 1000;

    bob.rent = 50 * 1000;

    bob.food = 30 * 1000;
    bob.clothes = 10 * 1000;
    bob.trip = 200 * 1000;

    bob.car_price = 2500 * 1000;
    bob.gasoline = 25 * 1000;
    bob.car_consumables = 10 * 1000;
    bob.car_insurance = 7 * 1000;
}


void alice_print() {
    printf("Alice bank account = %lld руб.\n", alice.bank_account);
    printf("Alice aparments price = %lld руб.\n", alice.mortgage + alice.down_payment); // Mortgage amount + down payment
    printf("Alice car price = %lld руб.\n", alice.car_price);
}


void bob_print() {
    printf("Bob bank account = %lld руб.\n", bob.bank_account);
    printf("Bob car price = %lld руб.\n", bob.car_price);
}


void alice_income(const int year, const int month) {
    if (month == 10) {
        alice.income *= 1.05;  // Indexation
    }
    if (year == 2035 && month == 10) {
        alice.income *= 1.35;  // Promotion
    }
    alice.bank_account += alice.income;

}


void bob_income(const int year, const int month) {
    if (month == 10) {
        bob.income *= 1.05;  // Indexation
    }
    if ((year == 2030 && month == 10) || (year == 2040 && month == 10)) {
        bob.income *= 1.25;  // Promotion
    }
    bob.bank_account += bob.income;
}


void alice_spending(const int year, const int month) {
    if (month == 1) {
        alice.food *= 1.07;  // Inflation
        alice.clothes *= 1.07;
    }
    alice.bank_account -= alice.food;
    alice.bank_account -= alice.clothes;
}


void bob_spending(const int year, const int month) {
    if (month == 1) {
        bob.food *= 1.07;   // Inflation
        bob.clothes *= 1.07;
        bob.rent *= 1.07;
    }
    bob.bank_account -= bob.food;
    bob.bank_account -= bob.clothes;
}


void alice_trip(const int month) {
    if ((month == 8)  && (alice.bank_account > 200 * 1000)) {
        alice.bank_account -= alice.trip;
    }
}


void bob_trip(const int month) {
    if ((month == 6 || month == 12) && (bob.bank_account > 200 * 1000)) {
        bob.bank_account -= bob.trip;
    }
}
    

void alice_car_expences(const int year, const int month) {
    RUB car_crash = 0 * 1000;  // Driving skill issues
    if (year % 5 == 0 && month == 12) {
        car_crash = 200 * 1000;
    }
    if (month == 1) {
        alice.car_price *= 0.98;  // Deprecation
        alice.gasoline *= 1.07;  // Inflation
        alice.car_consumables *= 1.07;
        alice.bank_account -= alice.car_insurance;
    }
    if (year == 2037 && month == 6) {
        alice.car_price += 1500 * 1000;
        alice.bank_account -= 1500 * 1000;
    }
    alice.bank_account -= alice.gasoline;
    alice.bank_account -= alice.car_consumables;
}


void bob_car_expences(const int year, const int month) {
    if (month == 1) {
        bob.car_price *= 0.97;  // Deprecation
        bob.gasoline *= 1.07;  // Inflation
        bob.car_consumables *= 1.07;
        bob.bank_account -= bob.car_insurance;
    }
    if ((year == 2031 && month == 8) || (year == 2041 && month == 8)) {
        bob.car_price += 1000 * 1000;
        bob.bank_account -= 1000 * 1000;
    }
    bob.bank_account -= bob.gasoline;
    bob.bank_account -= bob.car_consumables;
}


void alice_mortgage(const int month) {
    RUB payment = 55054;
    alice.bank_account -= (payment);
    if (month == 1) {
        alice.mortgage *= 1.07;
        alice.down_payment *= 1.07;
    }
}


void bob_rent(const int year, const int month) {
    if ((year == 2030 && month == 11) || (year == 2040 && month == 11)) {
        bob.rent *= 1.25;
    }
     bob.bank_account -= bob.rent;
}


void simulation() {
    int year = 2025;
    int month = 9;

    while( !(year == 2045 && month == 9)){
        alice_income(year, month);
        alice_mortgage(month);
        alice_spending(year, month);  // Food and clothes 
        alice_trip(month);
        alice_car_expences(year, month);

        bob_income(year, month);
        bob_rent(year, month);
        bob_spending(year, month);  // Food and clothes 
        bob_trip(month);
        bob_car_expences(year, month);

        month++;
        if (month == 13) {
            year++;
            month = 1;
        }

        if ((alice.bank_account < 0) || (bob.bank_account < 0)) {  // Checking if bank account is below zero
            printf("%lld, %lld \n", year, month);
            printf ("Alice bank account = %lld \n", alice.bank_account);
            printf ("Bob bank account = %lld \n", bob.bank_account);
            printf("\n");
        }   
    }
}


int main() {
    alice_init();
    bob_init();

    simulation();

    alice_print();
    printf("\n");
    bob_print();
}
