#include <stdio.h>
#include <corecrt_math.h>

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
	int has_flat;
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


void deposit_simulation(struct Person* p, const int year, const int month)
{
	static double deposit_rate = 15;
	static double month_rate = (deposit_rate * 0.01) / 12;

	if (p->has_flat) {
		return;
	}

	p->deposit = p->deposit * (1.0 + month_rate);

	if (p->bank_account > 0) {
		p->deposit += p->bank_account;
		p->bank_account = 0;
	}

	if (p->bank_account < 0 && p->deposit > 0) {
		static double need = -p->bank_account;
		static double take = (p->deposit >= need) ? need : p->deposit;
		p->deposit -= take;
		p->bank_account += take;
	}

	if (p->deposit >= p->flatcost) {
		p->deposit -= p->flatcost;  
		p->has_flat = 1;            
	}
}


void mortage_simulation(struct Person* p, const int year, const int month)
{
	static double remaining_loan = 0.0; 
	static double monthly_payment = 0.0;
	static int initialized = 0;

	if (!initialized) {
		double loan_amount = p->flatcost; 
		double initial_payment = 1000000;
		double annual_rate = 12;       
		double month_rate = (annual_rate / 12) / 100;
		int years = 20;
		int total_months = years * 12;

		loan_amount -= initial_payment;
		p->bank_account -= initial_payment;

		double annuity_coeff =
			(month_rate * pow(1.0 + month_rate, total_months)) /
			(pow(1.0 + month_rate, total_months) - 1.0);

		monthly_payment = loan_amount * annuity_coeff;
		remaining_loan = loan_amount;
		initialized = 1;

		printf("Saul took mortage: %.0f RUB, monthly payment: %.0f RUB\n",
			loan_amount, monthly_payment);
	}

	if (remaining_loan > 0.0) {
		double annual_rate = 0.12;
		double month_rate = annual_rate / 12.0;

		double interest = remaining_loan * month_rate;

		double principal_payment = monthly_payment - interest;

		remaining_loan -= principal_payment;
		if (remaining_loan < 0) remaining_loan = 0;

		p->bank_account -= monthly_payment;
		
		if (month == 12) { 
			printf("Year %d: mortgage balance %.0f RUB\n", year, remaining_loan);
		}
	}
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

		deposit_simulation(&walter, year, month);
		mortage_simulation(&saul, year, month);

		printf("saul capital = %d\n\n", saul.bank_account);
		printf("walter capital = %d\n", walter.bank_account);

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
	walter.income = 160 * 1000;
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
	saul.bank_account = 1100 * 1000;
	saul.income = 190 * 1000;
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
