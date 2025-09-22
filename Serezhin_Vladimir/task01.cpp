#include <iostream>

typedef long long YUAN;

struct Person {
	YUAN bank_account;
	YUAN income;
};

Person lixia;

int year, month, retire_year, retire_month;

void lixia_init() {
	std::cout << "Initialize Lixia's current bank account in yuan: ";
	std::cin >> lixia.bank_account;
	std::cout << "Initialize Lixia's current monthly income in yuan: ";
	std::cin >> lixia.income;
	std::cout << "Current year: ";
	std::cin >> year;
	std::cout << "Number of the current month: ";
	std::cin >> month;
	std::cout << "The year when Lixia aims to retire: ";
	std::cin >> retire_year;
	if (retire_year < year) {
		std::cout << "Incorrect input. Compare your input years of retirement and current one.";
		exit(1);
	}
	std::cout << "The number of month when Lixia aims to retire: ";
	std::cin >> retire_month;
	if (retire_year == year && retire_month <= month) {
		std::cout << "Incorrect input. Compare your input months of retirement and current one.";
		exit(1);
	}
	std::cout << std::endl;
}

void lixia_output() {
	if (lixia.bank_account < 0) {
		std::cout << "Damn! Lixia is a bankrupt. Money has gone down the drain on beverages. "
			<< "Lixia's bank account is " << lixia.bank_account;
	}
	else {
		std::cout << "Lixia's bank account in " << retire_year << " is equal to "
			<< lixia.bank_account << " yuan" << std::endl;
	}
}

void lixia_income() {
	if (year == 2030 && month == 9) {
		lixia.income *= 1.5; // Promotion
	}
	lixia.bank_account += lixia.income;
}

void lixia_beverage_and_food() {
	lixia.bank_account -= 2000; //Every month Lixia spends 300 yuan on her passion
}

void lixia_mortgage() {
	lixia.bank_account -= 3000;
}

void simulation() {
	while (!(year == retire_year && month == retire_month)) {
		lixia_income();
		lixia_beverage_and_food();
		lixia_mortgage();
		month++;
		if (month == 13) {
			year++;
			month = 1;
		}
	}
}

int main() {
	lixia_init();

	simulation();

	lixia_output();

	return 0;
}
