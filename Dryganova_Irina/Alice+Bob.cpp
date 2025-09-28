#include <stdio.h>
#include <math.h>

typedef long int RUB;

struct Car {
    RUB cost; 
    RUB expense;
};


struct Deposit {
    RUB deposit;
    RUB deposit_min; 
};


struct Person {
    Car car;
    Deposit deposit;
    RUB account;
    RUB income;
    RUB foodcost;
    RUB trip;
    RUB apartment;
    RUB mortgage_payment; 
    RUB rent;
    RUB tax_deduction; // Накопительная сумма налогового вычета
    RUB tax_deduction_used; // Использованная сумма вычета
};


struct Person alice;
struct Person bob;


void alice_income(const int year, const int month)
{     
    if (month == 10) {
        alice.income = alice.income * 1.07; } // Indexation 

    if (year == 2030 && month == 3) {
        alice.income *= 1.5;  // Promotion
    }

    alice.account += alice.income;
}


void alice_foodcost(const int year, const int month) //Products spending
{
    if (year > 2025 && month == 1) {
        alice.foodcost *= 1.03;
    }
    alice.account -= alice.foodcost;
}


void alice_car(const int year, const int month) 
{
    if (month == 5 && year == 2034) { //Car cost
        alice.account -= alice.car.cost;
    }

    if (year > 2034 || (year == 2034 && month > 5)) { //Car service
        alice.account -= alice.car.expense;
    }
}


void alice_trip(const int year, const int month) { //Trips every year
    if (month == 8) {
        alice.account -= alice.trip;
    }
}


void alice_apartment(const int year, const int month)
{
    if (month == 1) {
        alice.apartment *= 1.1;
    }
}


void alice_mortgage(const int year, const int month)
{
    if (year >= 2025 && year <= 2045)
    //Mortgage payment 
     {
        alice.account -= alice.mortgage_payment;
    }
}


void alice_tax_deduction(const int year, const int month)
{
    // Налоговый вычет начисляется ежегодно в апреле (месяц подачи налоговой декларации)
    if (month == 4 && year >= 2026) {
        RUB max_deduction = 2600000; // Максимальный вычет 2 млн руб. (13% от 2 млн)
        RUB annual_interest = alice.mortgage_payment * 12 * 13 / 100; // 13% от уплаченных процентов

        // Накопление налогового вычета
        alice.tax_deduction += annual_interest;

        // Если накопленная сумма превышает максимальную, ограничиваем её
        if (alice.tax_deduction > max_deduction) {
            alice.tax_deduction = max_deduction;
        }

        // Возврат налога на счет (если есть неиспользованный вычет)
        RUB available_deduction = alice.tax_deduction - alice.tax_deduction_used;
        if (available_deduction > 0) {
            RUB annual_income_tax = alice.income * 12 * 13 / 100; // 13% от годового дохода

            RUB refund = (available_deduction < annual_income_tax) ?
                available_deduction : annual_income_tax;

            alice.account += refund;
            alice.tax_deduction_used += refund;
        }
    }
}


void alice_deposit(const int month)
{
    {
        if (alice.account >= 70 * 1000)
        alice.deposit.deposit += (alice.account - 70 * 1000);
    }

    if (alice.deposit.deposit >= 3 * 1000 * 1000) {
        alice.deposit.deposit *= 1. + 0.11 / 12;
    }
    else {
        alice.deposit.deposit *= 1. + 0.1 / 12;
    }

    if (month == 1) {
        alice.deposit.deposit_min *= 1.07;
    }
}


void bob_income(const int year, const int month)
{
    if (month == 10) {
        bob.income = bob.income * 1.07;
    } // Indexation 

    if (year == 2030 && month == 3) {
        bob.income *= 1.5;  // Promotion
    }
    bob.account += bob.income;
}


void bob_foodcost(const int year, const int month)  // Products spending
{
    if (year > 2025 && month == 1) {
        bob.foodcost *= 1.03;
    }
    bob.account -= bob.foodcost;
}


void bob_car(const int year, const int month)
{
    if (month == 5 && year == 2034) { //Car cost
        bob.account -= bob.car.cost;
    }

    if (year > 2034 || (year == 2034 && month > 5)) { //Car service
        bob.account -= bob.car.expense;
    }
}


void bob_trip(const int year, const int month) { //Trips every year
    if (month == 8) {
        bob.account -= bob.trip;
    }
}


void bob_apartment(const int year, const int month)
{
    if (month == 1) {
       bob.apartment *= 1.1;
    }
}


void bob_rent(const int month)
{
    if (month == 1) {
        bob.rent *= 1.06;
    }

    bob.account -= bob.rent;
}


void bob_deposit(const int month)
{
        if (bob.account >= 30 * 1000) {
        bob.deposit.deposit += (bob.account - 30 * 1000);
        bob.account = 30 * 1000;
    }

    if (bob.deposit.deposit >= 3 * 1000 * 1000) {
        bob.deposit.deposit *= 1. + 0.11 / 12;
    }
    else {
        bob.deposit.deposit *= 1. + 0.1 / 12;
    }

    if (month == 1) {
        bob.deposit.deposit_min *= 1.07;
    }
}


void simulation()
{
    int year = 2025;
    int month = 9;

    while (!(year == 2045 && month == 9)) {
        alice_income(year, month);
        alice_mortgage(year, month);
        alice_foodcost(year, month);
        alice_car(year, month);
        alice_trip(year,month);
        alice_apartment(year,month);
        alice_mortgage(year, month);
        alice_tax_deduction(year, month);
        alice_deposit(month);


        bob_income(year, month);
        bob_foodcost(year, month);
        bob_car(year, month);
        bob_trip(year, month);
        bob_apartment(year, month);
        bob_rent(month);
        bob_deposit(month);

        ++month;
        if (month == 13) {
            month = 1;
            ++year;
        }
    }
}


void print_alice_info()
{
    printf("Alice bank account = %d RUR\n", alice.account);
    printf("Alice capital = %d RUB\n", alice.account + alice.apartment + alice.deposit.deposit);
    printf("Alice deposit = %d RUB\n", alice.deposit.deposit);
    printf("Alice apartment = %d RUB\n\n", alice.apartment);
    printf("Alice tax deduction accumulated = %ld RUB\n", alice.tax_deduction);
    printf("Alice tax deduction used = %ld RUB\n\n", alice.tax_deduction_used);
   
    printf("Bob capital = %d RUB\n", bob.account + bob.deposit.deposit);
    printf("Bob deposit = %d RUB\n", bob.deposit.deposit);
    printf("Bob bank account = %d RUB\n\n", bob.account);
}


void alice_init()
{
    RUB initial_payment = 100000;
    RUB mortgage_amount = alice.apartment - initial_payment;

    // Calculation of monthly payment (annuity)
    // Rate 8% per year, 10-year term = 120 months
    double annual_rate = 0.08;
    double monthly_rate = annual_rate / 12;
    int months = 120;

    double annuity_coef = (monthly_rate * pow(1 + monthly_rate, months)) /
        (pow(1 + monthly_rate, months) - 1);
    RUB monthly_payment = (RUB)round(mortgage_amount * annuity_coef); //Round off numbers

    alice.account = 1000 * 1000 - initial_payment; // After making the first payment
    alice.income = 200 * 1000;
    alice.apartment = 5000 * 9000;
    alice.foodcost = 30000;
    alice.car.cost = 4000*5000;
    alice.car.expense = 40 * 1000;
    alice.trip = 160 * 1000;
    alice.deposit.deposit = 0;
    alice.deposit.deposit_min = 3 * 1000 * 1000;
    alice.mortgage_payment = monthly_payment;
    alice.tax_deduction = 0;
    alice.tax_deduction_used = 0;
}


void bob_init()
{
    bob.account = 100 * 1000;
    bob.income = 130 * 1000;
    bob.apartment = 7000 * 5000;
    bob.foodcost = 25000;
    bob.car.cost = 6000 * 2000;
    bob.car.expense = 50000;
    bob.trip = 120 * 1000;
    bob.rent = 50000;
    bob.deposit.deposit = 0;
    bob.deposit.deposit_min = 3 * 1000 * 1000;
}


int main()
{
    alice_init();
    bob_init();

    simulation();

    print_alice_info();
}
// добавлен налог вычет на квартиру для алисы
