#include <stdio.h>

typedef int RUB;

struct Person {
	RUB bank_account; 
	RUB income;
	RUB food;
	RUB diff_services;  
	RUB clothes;
	RUB unforeseen_expenses; 
	RUB trip;
	RUB technique;   
	RUB flatcost; 
	RUB deposit;  
	RUB mortage;
};

struct Person walter;
struct Person saul;


void person_income(struct Person* p, const int year, const int month)
{
	if (month == 10) {
		p->income = p->income * 1.05;
	}
	if (year == 2035 && month == 3) {
		p->income *= 1.3;
	}
	p->bank_account += p->income;
}


void deposit_simulation(const int year, const int month)
{
	float deposit_rate = 15; 
	float month_rate = (deposit_rate * 0.01) / 12;

	walter.deposit = walter.deposit * (1.0 + month_rate);

	if (walter.bank_account > 0) {
		walter.deposit += walter.bank_account;
		walter.bank_account = 0;
	}

	if (walter.bank_account < 0 && walter.deposit > 0) {
		float need = -walter.bank_account;
		float take = (walter.deposit >= need) ? need : walter.deposit;
		walter.deposit -= take;
		walter.bank_account += take;                 
	}
}


void mortage_simulation(const int year, const int month)
{
	float mortage_rate = 15;
	float month_rate = (mortage_rate * 0.01) / 12;



}


void person_food(struct Person* p)
{
	p->food *= 1.008;
	p->bank_account -= p->food;
}


void person_diff_services(struct Person* p)
{
	p->diff_services *= 1.008;
	p->bank_account -= p->diff_services;
}


void person_clothes(struct Person* p, const int month)
{
	if (month % 3 == 0) {
		p->clothes *= 1.025;
		p->bank_account -= p->clothes;
	}
}


void person_unforeseen_expenses(struct Person* p, const int month)
{
	if (month % 3 == 0) {
		p->unforeseen_expenses *= 1.025;
		p->bank_account -= p->unforeseen_expenses;
	}
}


void person_trip(struct Person* p, const int month)
{
	if (month == 7) {
		p->trip *= 1.08;
		p->bank_account -= p->trip;
	}
}


void person_technique(struct Person* p, const int year, const int month)
{
	if (year % 5 == 0 && month == 1) {
		p->technique *= 1.3;
		p->bank_account -= p->technique;
	}
}


void person_flatcost(struct Person* p, const int month)
{
	if (month == 7) {
		p->flatcost = (p->flatcost * 1.1);
	}
}


void simulation()
{
	int year = 2025;
	int month = 12;

	while (!(year == 2045 && month == 9)) {
		person_income(&walter, year, month);
		person_income(&saul, year, month);

		person_food(&walter);
		person_food(&saul);

		person_diff_services(&walter);
		person_diff_services(&saul);

		person_clothes(&walter, month);
		person_clothes(&saul, month);

		person_unforeseen_expenses(&walter, month);
		person_unforeseen_expenses(&saul, month);

		person_trip(&walter, month);
		person_trip(&saul, month);

		person_flatcost(&walter, month);
		person_flatcost(&saul, month);

		person_technique(&walter, year, month);
		person_technique(&saul, year, month);

		deposit_simulation(year, month);
		mortage_simulation(year, month);

		++month;
		if (month == 13) {
			month = 1;
			++year;
		}
	}
}


void print_person_info()
{
	printf("walter capital = %d\n", walter.bank_account);
	printf("saul capital = %d\n", saul.bank_account);
	printf("flatcost = %d\n", walter.flatcost);
	printf("deposit = %d\n", walter.deposit);
}


void walter_int()
{
	walter.bank_account = 100 * 1000;
	walter.income = 155 * 1000;
	walter.food = 30 * 1000;
	walter.diff_services = 25 * 1000;
	walter.clothes = 15 * 1000;
	walter.unforeseen_expenses = 50 * 1000;
	walter.trip = 120 * 1000;
	walter.technique = 200 * 1000;
	walter.flatcost = 10000 * 1000;
	walter.deposit = 10 * 1000;
}


void saul_int()
{
	saul.bank_account = 1000 * 1000;
	saul.income = 155 * 1000;
	saul.food = 30 * 1000;
	saul.diff_services = 25 * 1000;
	saul.clothes = 15 * 1000;
	saul.unforeseen_expenses = 50 * 1000;
	saul.trip = 120 * 1000;
	saul.technique = 200 * 1000;
	saul.flatcost = 10000 * 1000;
}


int main()
{	
	walter_int();
	saul_int();
	simulation();
	print_person_info();

}
