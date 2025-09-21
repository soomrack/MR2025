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
    float inflation_coefficient = 1.01;
    float salary_indexation = 1.008;
    
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
    RUB different = 6000; // иные траты



    alice.bank_account = 1000 * 1000;
	alice.salary = 200 * 1000;
	alice.debt = properties.home_cost - properties.initial_deposit;
    alice.live_cost = food + clothes + different;
}


void alice_bank()
{
    double monght_bid = properties.key_bid / 12;
    
    
    alice.bank_account += alice.salary;
    properties.debit_part = alice.debt * (monght_bid / ( 1 - pow((1 + monght_bid), (-1 * (properties.credit_mought - 1)) )));
    alice.bank_account -= properties.debit_part;
    alice.bank_account -= alice.live_cost;

	cout << alice.bank_account<< "       " << properties.debit_part << endl;

}

void events ()
{

    switch (properties.actual_month)
    {
    case 15:

        alice.bank_account -= 50000; // пример любого случайного события, сломанный холодильник
        bob.bank_account -= 50000;

        break;
    case 30:

        alice.salary = 250 * 1000; // пример любого случайного события, повышения зарплаты
        bob.salary = 250 * 1000;

        break;
    default:
        break;
    }

}


void timeline()
{
    alice.bank_account -= properties.initial_deposit;

	while (properties.actual_month < properties.credit_mought)
	{

		//bob.bank_account += bob.salary;
		

        
		alice_bank();
        events();
        
        alice.live_cost *= properties.inflation_coefficient; //влияние инфляции/индексации
        bob.live_cost *= properties.inflation_coefficient;

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