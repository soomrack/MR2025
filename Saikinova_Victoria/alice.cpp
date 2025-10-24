#include <cmath>
#include <cstdio>

typedef long long int RUB;

struct person {
    RUB salary;
    RUB balance;
    RUB payment;
    RUB savings;
};

person alice;
person bob;

RUB remittance;
RUB second_flat_cost;

RUB food = 10 * 1000;
RUB cosmetic = 3 * 1000;
RUB cat_food = 2 * 1000;
RUB communal_flat = 5 * 1000;
RUB clothes = 5 * 1000;
RUB relaxation = 15 * 1000;
RUB self_care = 4 * 1000;
RUB dog_food = 3 * 1000;
RUB dog_hospital = 5 * 1000;
RUB computer_game = 1000;
RUB car = 20 * 1000;
RUB hospital = 5 * 1000;

void init(RUB salary, RUB savings) {
    alice.salary = salary;
    alice.savings = savings;
    bob.salary = salary;
    bob.savings = savings;
}


void print1() {
    printf("ALice took out a mortgage\n");
    printf("ALice has salary = %lld RUB\n", alice.salary);
    printf("ALice has savings = %lld RUB\n\n", alice.savings);

    printf("Bob rented an apartment\n");
    printf("Bob has salary = %lld RUB\n", bob.salary);
    printf("Bob has savings = %lld RUB\n\n", bob.savings);
}


void print2() {
    printf("\nALice has savings = %lld RUB\n", alice.savings);
    if (alice.balance < 0) {
        printf("ALice starves(\n");
    }
    else {
        printf("ALice`s happy)\n");
    }

    printf("\nBob has savings = %lld RUB\n", bob.savings);
    if (bob.savings < 50 * 1000 * 1000) {
        printf("Bob starves( \n");
    }
    else printf("Bob will buy an apartment)\n");
}


void inflation_do(const int year, const int month, double inflation) {
    if (month == 12) {
        alice.salary = RUB(double(alice.salary) * (inflation - 0.1));
        bob.salary = RUB(double(bob.salary) * (inflation - 0.1));

        food *= inflation;
        cosmetic *= inflation;
        cat_food *= inflation;
        communal_flat *= inflation;
        clothes *= inflation;
        relaxation *= inflation;
        self_care *= inflation;
        dog_food *= inflation;
        dog_hospital *= inflation;
        computer_game *= inflation;
        car *= inflation;
        hospital *= inflation;
    
    }
}


//                  ALICE                  //


void payment_mortgage(RUB amount_mortgage, double mortgage_percentage) {
    double monthly_rate = mortgage_percentage / 12;
    RUB remaining_mortgage = amount_mortgage - 100 * 1000;
    alice.payment = RUB(double(remaining_mortgage) * (monthly_rate * pow((1 + monthly_rate), 20)));
    /*printf("payment %lld \n",payment);*/
}


void alice_payment() {
    alice.balance -= alice.payment;
}


void alice_salary(const int month, const int year) {
    alice.balance += alice.salary;
    if ((year - 2025) % 5 == 0 && month == 9 && year != 2025) {
        alice.salary += 30 * 1000;

    }
}


void alice_savings(const int month, const int year) {
    if (alice.balance < 0) {
        remittance = -alice.balance;
    //    printf(" %lld \n", remittance);
        if (alice.savings >= remittance) {
            alice.savings -= remittance;
            alice.balance = 0;
        }
        else {
            alice.balance -= alice.savings;
            alice.savings = 0;
       //     printf(" %lld \n", alice.savings);
        }
    }
    else {
        alice.savings += alice.balance;
        alice.balance = 0;
    }
}


void alice_deposit(double deposit) {
    alice.savings += alice.savings * deposit / 12;
}


void alice_food() {
    alice.balance -= food;
}


void alice_cosmetic(const int month) {
    if (month % 5 == 0) alice.balance -= cosmetic;
}


void alice_cat(const int month) {
    if (month % 3 == 0) alice.balance -= cat_food;
}


void alice_communal_flat() {
    alice.balance -= communal_flat;
}


void alice_clothes(const int month) {
    if (month % 2 == 0) alice.balance -= clothes;
}


void alice_relaxation(const int month) {
    if (month % 6 == 0) alice.balance -= relaxation;
}


void alice_hospital(const int month) {
    if (month == 6 || month == 12) {
        alice.balance -= hospital;
    }
}


//                  BOB                  //


void payment_rent_flat(RUB first_flat, RUB second_flat) {
    second_flat_cost = second_flat;
    bob.payment = first_flat;
}
