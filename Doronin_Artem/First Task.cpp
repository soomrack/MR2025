#include <stdio.h>
#include <math.h>

typedef long long int RUB;

struct Home
{
    RUB apartments_price;
    RUB mortgage;
    RUB down_payment;
    RUB rent;
};


struct Car
{
    RUB price;
    RUB gasoline;
    RUB consumables;
    RUB insurance;

};


struct Person {
    RUB bank_account;
    RUB income;

    Home home;

    RUB food;
    RUB clothes;
    RUB trip;

    Car car;
};


struct Person alice;
struct Person bob;


void alice_init() {
    alice.bank_account = 200 * 1000;
    alice.income = 135 * 1000;

    alice.home.apartments_price = 7000 * 1000;
    alice.home.down_payment = 2000 * 1000;
    alice.home.mortgage = 5000 * 1000;
    
    alice.food = 30 * 1000;
    alice.clothes = 20 * 1000;
    alice.trip = 160 * 1000;

    alice.car.price = 1500 * 1000;
    alice.car.gasoline = 15 * 1000;
    alice.car.consumables = 7 * 1000;
    alice.car.insurance = 30 * 1000;
}


void bob_init() {
    bob.bank_account = 300 * 1000;
    bob.income = 165 * 1000;

    bob.home.rent = 45 * 1000;

    bob.food = 30 * 1000;
    bob.clothes = 10 * 1000;
    bob.trip = 180 * 1000;

    bob.car.price = 2500 * 1000;
    bob.car.gasoline = 25 * 1000;
    bob.car.consumables = 10 * 1000;
    bob.car.insurance = 7 * 1000;
}


void alice_print() {
    printf("Alice bank account = %lld руб.\n", alice.bank_account);
    printf("Alice aparments price = %lld руб.\n", alice.home.apartments_price); // Mortgage amount + down payment
    printf("Alice car price = %lld руб.\n", alice.car.price);
    printf("\n");
}


void bob_print() {
    printf("Bob bank account = %lld руб.\n", bob.bank_account);
    printf("Bob car price = %lld руб.\n", bob.car.price);
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
        bob.home.rent *= 1.07;
    }
    bob.bank_account -= bob.food;
    bob.bank_account -= bob.clothes;
}


void alice_trip(const int month) {
    if ((month == 8)  && (alice.bank_account > alice.trip + 100 * 1000)) {
        alice.bank_account -= alice.trip;
    }
    if (month == 1) {
        alice.trip *= 1.07;  // Inflation
    }
}


void bob_trip(const int month) {
    if ((month == 6 || month == 12) && (bob.bank_account > bob.trip + 100 * 1000)) {
        bob.bank_account -= bob.trip;
    }
    if (month == 1) {
        bob.trip *= 1.07;  // Inflation
    }
}
    

void alice_car_expences(const int year, const int month) {
    static bool car_crashed = false;
    static RUB car_crash = 150 * 1000;  // Driving skill issues
    if (year % 5 == 0 && month == 12) {
        car_crashed = true;
    }
    if (car_crashed == true && alice.bank_account >= car_crash + 20 * 1000) {
        alice.bank_account -= car_crash;
        car_crash *= 1.2;
        car_crashed = false;

    }

    if (month == 1) {
        alice.car.price *= 0.92 * 1.07;  // Deprecation + inflation
        alice.car.gasoline *= 1.07;  // Inflation
        alice.car.consumables *= 1.07;
        alice.bank_account -= alice.car.insurance;
    }

    static char alice_car_change = 0;
    if (year >= 2036 && alice_car_change == 0 && alice.bank_account >= 1600 * 1000) {
        alice.car.price += 1500 * 1000;
        alice.bank_account -= 1500 * 1000;
        alice_car_change += 1;
    }
    
    if (car_crashed == false) {
        alice.bank_account -= alice.car.gasoline;
        alice.bank_account -= alice.car.consumables;  
    } else {
        alice.bank_account -= alice.car.gasoline / 3;  // Public transport
    }
}


void bob_car_expences(const int year, const int month) {
    if (month == 1) {
        bob.car.price *= 0.95 * 1.07;  // Deprecation + inflation
        bob.car.gasoline *= 1.07;  // Inflation
        bob.car.consumables *= 1.07;
        bob.bank_account -= bob.car.insurance;
    }

    static char bob_car_change = 0;
    if ((year >= 2031 && bob_car_change == 0 && bob.bank_account > 1100 * 1000)
        || (year >= 2041 && bob_car_change == 1 && bob.bank_account > 1100 * 1000)) {
        bob.car.price += 1000 * 1000;  // Buying a new car
        bob.bank_account -= 1000 * 1000;
        bob_car_change += 1;
    }

    bob.bank_account -= bob.car.gasoline;
    bob.bank_account -= bob.car.consumables;
}


void alice_mortgage(const int month) {
    RUB payment = 55054;
    alice.bank_account -= (payment);
    if (month == 1) {
        alice.home.apartments_price *= 1.07;
    }
}


void bob_rent(const int year, const int month) {
    if ((year == 2030 && month == 11) || (year == 2040 && month == 11)) {
        bob.home.rent *= 1.25;
    }
     bob.bank_account -= bob.home.rent;
}


void alice_deposit() {
    alice.bank_account *= 1.005;  // Deposit at 6 percent per annum
}


void bob_deposit() {
    bob.bank_account *= 1.005;  // Deposit at 6 percent per annum
}


void bank_account_check(const int year, const int month) {
    if ((alice.bank_account < 0) || (bob.bank_account < 0)) {
            printf("%lld, %lld \n", year, month);
            printf ("Alice bank account = %lld \n", alice.bank_account);
            printf ("Bob bank account = %lld \n", bob.bank_account);
            printf("\n");
    }
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
        alice_deposit();

        bob_income(year, month);
        bob_rent(year, month);
        bob_spending(year, month);  // Food and clothes 
        bob_trip(month);
        bob_car_expences(year, month);
        bob_deposit();

        bank_account_check(year, month);  // Checking if bank account is below zero

        month++;
        if (month == 13) {
            year++;
            month = 1;
        }
    }
}


int main() {
    alice_init();
    bob_init();
    
    simulation();

    alice_print();
    bob_print();
}
