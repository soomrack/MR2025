#include <iostream>
#include <math.h>
using namespace std;

typedef long int RUB;


struct Simulation
{
	RUB home_cost = 10 * 1000 * 1000;
	RUB initial_deposit = 1000 * 1000;
	RUB debit_part = 0; 
	int actual_month = 0;
    int credit_mought = 180; // кредит на пятнадцать лет
	float key_bid = 0.18;
    
}properties;


struct Person
{
    RUB bank_account;
	RUB salary;
	RUB debt;
    RUB live_cost;
};

struct Person bob;
struct Person alice;


void bob_init()
{
    bob.bank_account = 1000 * 1000;
	bob.salary = 200 * 1000;
}

void alice_init()
{
    RUB food = 30000;
    RUB clothes = 10000; // среднеднии траты на одежду в месяц;
    RUB different = 10000; // иные траты



    alice.bank_account = 1000 * 1000;
	alice.salary = 200 * 1000;
	alice.debt = properties.home_cost - properties.initial_deposit;
    alice.live_cost = food + clothes + different;
}


void Alice_bank()
{
    double monght_bid = properties.key_bid / 12;
    
    
    alice.bank_account += alice.salary;
    properties.debit_part = alice.debt * (monght_bid / ( 1 - pow((1 + monght_bid), (-1 * (properties.credit_mought - 1)) )));
    alice.bank_account -= properties.debit_part;
    alice.bank_account -= alice.live_cost;

	cout << alice.bank_account<< "       " << properties.debit_part << endl;

}


void timeline()
{
    alice.bank_account -= properties.initial_deposit;

	while (properties.actual_month < properties.credit_mought)
	{

		//bob.bank_account += bob.salary;
		

        
		Alice_bank();

		properties.actual_month++;
	}

}

void Output()
{
	cout << properties.actual_month;
}


int main() {

	setlocale(LC_ALL, "Rus");



	alice_init();
	bob_init();

	timeline();

	Output();

	return 0;
}