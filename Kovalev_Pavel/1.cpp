// зачтено 20251006
#include <stdio.h>
#include <cmath>
#include <algorithm>


typedef long int RUB;
typedef int USDT;


struct Car {
    RUB cost;
    RUB fuel;
    RUB tax; // yearly
};

struct Home {
    RUB cost;
    RUB rent;
    RUB annuity_payment;
    RUB home_expenses;
};

struct Person {
    RUB bank_account;
    RUB income;
    RUB misc_expenses; // common expenses, such as food
    RUB cat_expenses;
    RUB trip; // yearly
    RUB min_balance; // minimal balance during month for depositing

    Car car;
    Home home;
};

struct Person alice;
struct Person bob;


RUB inflated_flat_price = 20 * 1000 * 1000; // this price will later be inflated
RUB inflated_car_price = 1500 * 1000;


// common functions ////////////////////////////////////////////////////////////


void deposit_interest(Person &person /*, const int year, const int month */) {
    // increase money on deposit by interest_rate
    // assume person keeps all of money on deposit
    float interest_rate = 0.05;
    if (person.bank_account > 3 * 1000 * 1000) { // increase interest rate if person has a lot of money
        interest_rate += 0.01;
    }
    float monthly_interest_rate = pow(1+interest_rate, 1.0/12) - 1; // e.g. 5% yearly will result in 0.00407 monthly

    if (person.min_balance > 0) {
        person.bank_account += person.min_balance * monthly_interest_rate;
        // person.bank_account *= 1.0+monthly_interest_rate;
    }
}


void deposit_or_withdraw(Person &person, RUB amount) {
    // positive amount deposits money, negative — withdraws
    person.bank_account += amount;
    person.min_balance = std::min(person.min_balance, person.bank_account);
}


////////////////////////////////////////////////////////////////////////////////


void alice_income(const int year, const int month)
{
    if(month == 10) {
        alice.income = alice.income * 1.02;  // Indexation
    }
        
    if(year == 2030 && month == 3) {
        alice.income *= 1.5;  // Promotion
    }
    
    deposit_or_withdraw(alice, alice.income);
}


void alice_misc_expenses(const int year, const int month)
{
    deposit_or_withdraw(alice, -alice.misc_expenses);
    if(month == 9) alice.misc_expenses = alice.misc_expenses * 1.07; // Inflation
}


void alice_trip(const int year, const int month)
{
    if (month == 7 && alice.bank_account >= 100 * 1000) { // TODO: проверить пробелы вокруг знаков
        // only travels in summer if has enough spare money
        deposit_or_withdraw(alice, -alice.trip);
    }
    if (month == 9) alice.trip *= 1.05; // Inflation
}


void alice_cat(const int year, const int month)
{
    if (year == 2026 && month == 4) alice.cat_expenses = 5000;
    if ((year == 2026 && month >= 4) || (year == 2035 && month <= 2) || (2026 < year && year < 2035)) {
        if (month == 12) alice.cat_expenses *= 1.05; // Inflation
        deposit_or_withdraw(alice, -alice.cat_expenses);
    }
}


void alice_mortgage(const int year, const int month)
{
    const float interest_rate = 0.06;
    if ( year == 2025 && month == 9) {
        const RUB flat_price = 20 * 1000 * 1000;
        const RUB first_payment = 4000 * 1000;
        // (RUB) flat_price * 0.2;
        // first payment
        alice.home.cost = flat_price;
        deposit_or_withdraw(alice, -first_payment);

        const float monthly_interest_rate = interest_rate / 12;
        alice.home.annuity_payment = (RUB) round ( 
            // alice.debt * (monthly_interest_rate * pow(1+monthly_interest_rate, 20*12) ) / ( pow(1+monthly_interest_rate, 20*12) - 1 ) 
            (flat_price - first_payment) * monthly_interest_rate / ( 1 - pow(1+monthly_interest_rate, -20*12) )
        );
        // printf("Annuity payment %d RUB\n", alice.annuity_payment); //dbg
    }
    
    // monthly annuity mortgage payment
    deposit_or_withdraw(alice, -alice.home.annuity_payment);
}


void alice_home(const int year, const int month)
{
    alice_mortgage(year, month);

    if (month == 9) { // Inflation and cost drop
        alice.home.cost *= 0.98;
        alice.home.home_expenses *= 1.05;
    }
}


void alice_car(const int year, const int month)
{
    if (alice.car.cost == 0) { // does not have a car
        if (alice.bank_account >= inflated_car_price + 100*1000) {
            deposit_or_withdraw(alice, -inflated_car_price);
            // printf("Alice bought car in year %d\n", year); // dbg
            alice.car.cost = inflated_car_price;
            alice.car.tax = 3000;
            alice.car.fuel = 2000;
        }
    } else { // has a car
        deposit_or_withdraw(alice, -alice.car.fuel);
        if (month == 9) {
            alice.car.fuel *= 1.05;
            alice.car.tax *= 1.05; // Inflation
            alice.car.cost *= 0.97;

            deposit_or_withdraw(alice, -alice.car.tax);
        }
    }
}


void print_alice_info()
{   
    printf("Alice money = %d RUB, total capital %d RUB\n", alice.bank_account, alice.bank_account + alice.car.cost + alice.home.cost);
}


void print_alice_info_more(const int year)
{
    printf("%d: Alice capital = %d RUB\n", year, alice.bank_account);
}


void alice_init()
{
    alice.bank_account = 5 * 1000 * 1000;
    alice.income = 140 * 1000;
    alice.misc_expenses = 10 * 1000;
    alice.trip = 30 * 000;

    alice.car.cost = 0; // no car yet
    alice.car.fuel = 0;
    alice.car.tax = 0;

    alice.home.cost = 0; // no own home yet
    alice.home.home_expenses = 3000;
    alice.home.rent = 0;
}


////////////////////////////////////////////////////////////////////////////////


void bob_income(const int year, const int month)
{
    if(month == 10) {
        bob.income = bob.income * 1.02;  // Indexation
    }
        
    if(year == 2030 && month == 3) {
        bob.income *= 1.5;  // Promotion
    }
    
    deposit_or_withdraw(bob, bob.income);
}


void bob_misc_expenses(const int year, const int month)
{
    // covers common expenses, such as food
    deposit_or_withdraw(bob, -bob.misc_expenses);
    if(month == 9) bob.misc_expenses = bob.misc_expenses * 1.07; // Inflation
}


void bob_trip(const int year, const int month)
{
    if (month == 7 && bob.bank_account >= 100*1000) {
        // only travels in summer if has enough spare money
        deposit_or_withdraw(bob, -bob.trip);
    }
    if (month == 9) bob.trip *= 1.05; // Inflation
}


void bob_rent(const int year, const int month)
{
    if(month == 9) {
        bob.home.rent *= 1.05;  // Inflation
    }
    deposit_or_withdraw(bob, -bob.home.rent);
}


void bob_try_buy_flat(const int year, const int month)
{
    if (bob.home.rent == 0) return; // already has flat
    if (month == 9) inflated_flat_price *= 1.02; // Inflation
    if (bob.bank_account >= inflated_flat_price + 100*1000) {
        deposit_or_withdraw(bob, -inflated_flat_price);
        // printf("Bob bought flat in year %d\n", year); // dbg
        bob.home.rent = 0;
        bob.home.cost = inflated_flat_price;
    }
}


void bob_home(const int year, const int month)
{
    bob_rent(year, month);
    bob_try_buy_flat(year, month);

    if (month == 9) { // Inflation and cost drop
        bob.home.cost *= 0.98;
        bob.home.home_expenses *= 1.05;
    }
}


void bob_car(const int year, const int month)
{
    if (bob.car.cost == 0) { // does not have a car
        if (bob.bank_account >= inflated_car_price + 100*1000) {
            deposit_or_withdraw(bob, -inflated_car_price);
            // printf("Bob bought car in year %d\n", year); // dbg
            bob.car.cost = inflated_car_price;
            bob.car.tax = 3000;
            bob.car.fuel = 2000;
        }
    } else { // has a car
        deposit_or_withdraw(bob, -bob.car.fuel);
        if (month == 9) {
            bob.car.fuel *= 1.05;
            bob.car.tax *= 1.05; // Inflation
            bob.car.cost *= 0.97;

            deposit_or_withdraw(bob, -bob.car.tax);
        }
    }
}


void print_bob_info()
{   
    printf("Bob money = %d RUB, total capital %d RUB\n", bob.bank_account, bob.bank_account + bob.car.cost + bob.home.cost);
}

void print_bob_info_more(const int year)
{
    printf("%d: Bob money = %d RUB\n", year, bob.bank_account);
}


void print_if_bob_no_flat()
{
    if (bob.home.rent != 0) printf("Bob could not buy a flat for %d RUB", inflated_flat_price);
}


void bob_init()
{
    bob.bank_account = 5 * 1000 * 1000;
    bob.income = 140 * 1000;
    bob.misc_expenses = 7000;
    bob.trip = 30 * 000;

    bob.car.cost = 0; // no car yet
    bob.car.fuel = 0; 
    bob.car.tax = 0;

    bob.home.cost = 0; // no own home yet
    bob.home.home_expenses = 3000;
    bob.home.rent = 70 * 1000;
}


////////////////////////////////////////////////////////////////////////////////


void simulation()
{
    int year = 2025;
    int month = 9;

    // print_alice_info_more(year);
    while( !(year == 2045 && month == 9) ) {
        alice.min_balance = alice.bank_account;
        alice_income(year, month);
        alice_misc_expenses(year, month); // common expenses, such as food
        alice_trip(year, month);
        alice_car(year, month);
        alice_cat(year, month);
        alice_home(year, month);
        deposit_interest(alice); // TODO: * сначала interest, потом min, да и min можно внутри deposit (но можно и так)

        // printf("y%d m%d bank %d debt %d\n", year, month, alice.bank_account, alice.debt); //dbg

        bob.min_balance = bob.bank_account;
        bob_income(year, month);
        bob_misc_expenses(year, month);
        bob_trip(year, month);
        bob_car(year, month);
        bob_home(year, month);
        deposit_interest(bob);
        
        ++month;
        if(month == 13) {
            // print_alice_info_more(year);
            // print_bob_info_more(year);
            month = 1;
            ++year;
        }
    }
}


int main()
{
    alice_init();
    bob_init();
    
    simulation();

    // printf("====\n");

    print_alice_info();
    print_bob_info();
    print_if_bob_no_flat();
}
