#include <iostream>

typedef long long int YUAN;

struct Person {
	YUAN bank_account;
	YUAN income;
	YUAN beverage_per_month, saving_money;
    int retire_year, retire_month;
};

Person lixia, bob;

int current_year = 2025, current_month = 10;

void lixia_init()
{
	lixia.bank_account = 1000000;
	lixia.income = 3000;
	lixia.beverage_per_month = 2000;
	lixia.saving_money = 1000;
	lixia.retire_year = 2060;
	lixia.retire_month = 10;
}

void bob_init()
{
	bob.bank_account = 300000;
	bob.income = 20000;
	bob.beverage_per_month = 3000;
	bob.saving_money = 2000;
	bob.retire_year = 2060;
	bob.retire_month = 10;
}

void lixia_output()
{
	if (lixia.bank_account < 0) {
		std::cout << "Damn! Lixia is a bankrupt. Money has gone down the drain on beverages. "
			<< "Lixia's bank account is " << lixia.bank_account;
	}
	else {
		std::cout << "Lixia's bank account in " << lixia.retire_year << " is equal to "
			<< lixia.bank_account << " yuan" << std::endl;
	}
}

void bob_output()
{
	if (bob.bank_account < 0) {
		std::cout << "Damn! Bob is a bankrupt. Money has gone down the drain on beverages. "
			<< "Bob's bank account is " << bob.bank_account;
	}
	else {
		std::cout << "Bob's bank account in " << bob.retire_year << " is equal to "
			<< bob.bank_account << " yuan" << std::endl;
	}
}

void lixia_income()
{
	if (current_month == 1) {
		lixia.income *= 1.1; // Yearly salary's increasing
	}                        
	if (current_year == 2030 && current_month == 9) {
		lixia.income *= 1.5; // Promotion for her company's loyalty
	}
	lixia.bank_account += lixia.income;
}

void bob_income()
{
	if (current_month == 1) {
		bob.income *= 1.2; // Yearly salary's increasing
	}
	if (current_year == 2030 && current_month == 9) {
		bob.income *= 1.5; // Promotion for her company's loyalty
	}
	bob.bank_account += bob.income;
}

void lixia_beverage_and_food()
{
	if (current_month == 1) {
		lixia.beverage_per_month *= 1.1; // yearly inflation on beverages
	}
	lixia.bank_account -= lixia.beverage_per_month;
}

void bob_beverage_and_food()
{
	if (current_month == 1) {
		bob.beverage_per_month *= 1.1; // yearly inflation on beverages
	}
	bob.bank_account -= bob.beverage_per_month;
}

// Lixia will not not buy a car, since she will be gifted by her husband.

void bob_car()
{
	if (current_year == 2035){
		bob.bank_account -= 200000;  // Bob has bought a car
	}
}

void lixia_saving_money()
{
	lixia.bank_account -= lixia.saving_money;
}

void bob_saving_money()
{
	bob.bank_account -= bob.saving_money;
}

void lixia_mortgage()
{
	lixia.bank_account -= 3000;
}

void bob_mortgage()
{
	lixia.bank_account -= 3000;
}

void simulation()
{
	while (!(current_year == lixia.retire_year && current_month == lixia.retire_month)) {
		lixia_income();
		lixia_beverage_and_food();
		lixia_mortgage();

		bob_income();
		bob_beverage_and_food();
		bob_car();
		bob_mortgage();

		current_month++;
		if (current_month == 13) {
			current_year++;
			current_month = 1;
		}
	}
}

int main()
{
	lixia_init();
	bob_init();

	simulation();

	lixia_output();
	bob_output();
	return 0;
}
