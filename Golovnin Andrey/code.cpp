#include <iostream>
using namespace std;

typedef long int RUB;


struct Simulation
{
	RUB home_cost = 0;
	RUB initial_deposit = 900 * 1000;
	RUB payments = 50 * 1000; // Долговая часть в месяц от тела долга 
	RUB debit_part = 0; // Долговая часть для обслуживания долга
	int month = 0;
	int key_bid = 18;

}properties;

void sinulation_properties_init()
{
	//cout << "Введите стоимость дома: ";
	//cin >> properties.home_cost;

	properties.home_cost = 10 * 1000 * 1000;
}

struct Person
{
	RUB bank_account;
	RUB salary;
	RUB debt;
};

struct Person bob;
struct Person alice;


void bob_start()
{
	bob.bank_account = 1000 * 1000;
	bob.salary = 150 * 1000;
}

void alice_start()
{
	alice.bank_account = 1000 * 1000;
	alice.salary = 150 * 1000;
	alice.debt = properties.home_cost;
}


void Alice_bank()
{

	if (properties.month == 0)
	{
		alice.bank_account = alice.bank_account - properties.initial_deposit;
		properties.debit_part = ((alice.debt * properties.key_bid) / 100) / 12;
	}
	else if (properties.month % 12 == 0)
	{
		properties.debit_part = ((alice.debt * properties.key_bid) / 100) / 12;
	}

	alice.bank_account = alice.bank_account - (properties.payments + properties.debit_part);
	alice.debt = alice.debt - properties.payments;


	cout << properties.debit_part << endl;

}


void timeline()
{

	while (properties.debit_part >= 0)
	{

		//bob.bank_account += bob.salary;
		alice.bank_account += alice.salary;


		Alice_bank();

		properties.month++;
	}

}

void Output()
{
	cout << properties.month;
}


int main() {

	setlocale(LC_ALL, "Rus");

	sinulation_properties_init();

	alice_start();
	bob_start();

	timeline();

	Output();

	return 0;
}