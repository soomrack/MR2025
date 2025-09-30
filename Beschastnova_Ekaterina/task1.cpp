#include <stdio.h>

typedef long long int Money;   

const double INFLATION_RATE = 1.11;
const double DEPOSIT_RATE = 0.18;
const double DEPOSIT_RATE_HIGH = 0.22;
const double TAX_RATE = 0.13;
const double TAX_FREE_DEPOSIT_INCOME = 210 * 1000; //не облагается налогом по УК РФ :)


typedef struct {
    Money sum;
    Money first_pay;
    Money monthly_payment;
    double mortgage_rate;
} Mortgage;


typedef struct {
    Money salary;
    Money account;
    Money deposit_account_high_rate;
    Mortgage mortgage;
    Money house_price;
    Money food;
    Money house_bills;
    Money personal_expens;
    Money deposit;
    Money rent;
    Money total_deposit_income_year;
    Money tax_to_pay;
} Person;

typedef struct {

}deposit;

Person alice;
Person bob;


void alice_init()
{
    alice.salary = 200 * 1000;
    alice.account = 1 * 1000 * 1000;
    alice.deposit_account_high_rate = 0;

    alice.food = 20 * 1000;
    alice.house_bills = 7 * 1000;
    alice.personal_expens = 15 * 1000;

    alice.mortgage.sum = 9 * 1000 * 1000;
    alice.mortgage.first_pay = 900 * 1000; 
    alice.mortgage.monthly_payment = 136 * 1000;  
    alice.mortgage.mortgage_rate = 0.18;
    alice.account -= alice.mortgage.first_pay;
    alice.house_price = alice.mortgage.sum;

    alice.total_deposit_income_year = 0;
    alice.tax_to_pay = 0;
}


void bob_init()
{
    bob.salary = 200 * 1000;
    bob.account = 1 * 1000 * 1000;
    bob.deposit_account_high_rate = 0;

    bob.food = 24 * 1000;
    bob.house_bills = 7 * 1000;
    bob.personal_expens = 12 * 1000;
    bob.rent = 90 * 1000;

    bob.total_deposit_income_year = 0;
    bob.tax_to_pay = 0;
}


void alice_salary(const int month)
{
    if (month == 12) {
        alice.salary *= INFLATION_RATE;
    }
    alice.account += alice.salary;
}


void bob_salary(const int month)
{
    if (month == 12) {
        bob.salary *= INFLATION_RATE;
    }
    bob.account += bob.salary;
}


void alice_mortgage()
{
    alice.account -= alice.mortgage.monthly_payment;
}


void bob_rent(const int month)
{
    if (month == 12) {
        bob.rent *= INFLATION_RATE;
    }
    bob.account -= bob.rent;
}


void alice_expenditure(const int month) 
{
    if (month == 12) {
        alice.food *= INFLATION_RATE;
        alice.house_bills *= INFLATION_RATE;
        alice.personal_expens *= INFLATION_RATE;
    }
    alice.account -= (alice.food + alice.house_bills + alice.personal_expens);
}


void bob_expenditure(const int month)
{
    if (month == 12) {
        bob.food *= INFLATION_RATE;
        bob.house_bills *= INFLATION_RATE;
        bob.personal_expens *= INFLATION_RATE;
    }
    bob.account -= (bob.food + bob.house_bills + bob.personal_expens);
}


void alice_house_price(const int month)
{
    if (month == 1) { alice.house_price *= INFLATION_RATE; 
    }
}


void alice_transfer_all_to_high_rate_deposit() {
    if (alice.account > 0) { 
        alice.deposit_account_high_rate += alice.account;
        alice.account = 0; 
    }
}


void bob_transfer_all_to_high_rate_deposit() {
    if (bob.account > 0) {
        bob.deposit_account_high_rate += bob.account;
        bob.account = 0; 
    }
}


void alice_deposit(const int month)
{
    // доход с обычного счета (если мы сначала деньги перевели, потом процент начислили, то тут 0)
    alice.account += alice.account * DEPOSIT_RATE / 12;
    alice.total_deposit_income_year += alice.account * DEPOSIT_RATE / 12;

    // доход с накопительного счета
    alice.deposit_account_high_rate += alice.deposit_account_high_rate * DEPOSIT_RATE_HIGH / 12;
    alice.total_deposit_income_year += alice.deposit_account_high_rate * DEPOSIT_RATE_HIGH / 12;
}


void bob_deposit(const int month)
{
    bob.account += bob.account * DEPOSIT_RATE / 12;
    bob.total_deposit_income_year += bob.account * DEPOSIT_RATE / 12;

    bob.deposit_account_high_rate += bob.deposit_account_high_rate * DEPOSIT_RATE_HIGH / 12;
    bob.total_deposit_income_year += bob.deposit_account_high_rate * DEPOSIT_RATE_HIGH / 12;
}


void alice_tax(const int month) {
    if (month == 12) { 
        Money tax_income = 0; //облагаемое налогом
        if (alice.total_deposit_income_year > TAX_FREE_DEPOSIT_INCOME) {
            tax_income = alice.total_deposit_income_year - TAX_FREE_DEPOSIT_INCOME;
        }
        alice.tax_to_pay = tax_income * TAX_RATE;
        alice.deposit_account_high_rate -= alice.tax_to_pay;
        if (alice.deposit_account_high_rate < 0) alice.deposit_account_high_rate = 0;
        alice.total_deposit_income_year = 0; 
    }
}


void bob_tax(const int month) {
    if (month == 12) { 
        Money tax_income = 0;
        if (bob.total_deposit_income_year > TAX_FREE_DEPOSIT_INCOME) {
            tax_income = bob.total_deposit_income_year - TAX_FREE_DEPOSIT_INCOME;
        }
        bob.tax_to_pay = tax_income * TAX_RATE;
        bob.deposit_account_high_rate -= bob.tax_to_pay;
        if (bob.deposit_account_high_rate < 0) bob.deposit_account_high_rate = 0; 
        bob.total_deposit_income_year = 0; 
    }
}


void simulation()
{
    int month = 9;
    int year = 2025;

    while (!((year == 2025 + 30) && (month == 9))) {
        alice_salary(month); 
        alice_mortgage(); 
        alice_expenditure(month); 
        alice_house_price(month); 
 
        alice_transfer_all_to_high_rate_deposit(); 
        alice_deposit(month); 
        alice_tax(month);
       
        bob_salary(month);
        bob_rent(month); 
        bob_expenditure(month);

        bob_transfer_all_to_high_rate_deposit(); 
        bob_deposit(month); 
        bob_tax(month);

        month++;
        if (month == 13) {
            month = 1;
            year++;
        }
    }
}


void print_alice()
{
    printf("Alice account (main) = %lld RUB\n", alice.account);
    printf("Alice deposit account (high rate) = %lld RUB\n", alice.deposit_account_high_rate);
    printf("Alice house price = %lld RUB\n", alice.house_price);
    printf("---------------------\n");
    printf("Alice capital = %lld RUB\n", (alice.account + alice.deposit_account_high_rate + alice.house_price));
    printf("Alice total tax paid (last year) = %lld RUB\n", alice.tax_to_pay);
}


void print_bob()
{
    printf("Bob account (main) = %lld RUB\n", bob.account);
    printf("Bob deposit account (high rate) = %lld RUB\n", bob.deposit_account_high_rate);
    printf("---------------------\n");
    printf("Bob capital = %lld RUB\n", (bob.account + bob.deposit_account_high_rate));
    printf("Bob total tax paid (last year) = %lld RUB\n", bob.tax_to_pay);
}


int main()
{
    alice_init();
    bob_init();

    simulation();

    print_alice();
    print_bob();
    return 0;
}