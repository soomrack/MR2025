#include <stdio.h>
#include <cmath>

typedef double RUB;

struct Person {
	RUB bank_account; 
	RUB income;
	RUB food;
	RUB diff_services;  
	RUB clothes;
	RUB unforeseen_expenses; 
	RUB trip;
	RUB technique;   
	RUB rent_flat;
	RUB flatcost; 
	RUB deposit;  
	RUB mortage;
	
	RUB mortgage_balance;
	RUB mortgage_monthly_payment;
	RUB mortgage_total_paid;

	bool has_flat;
};

struct Person walter;
struct Person saul;


void walter_income(const int year, const int month)
{
	if (month == 10) {
		walter.income *= 1.05;
	}
	if (year == 2035 && month == 3) {
		walter.income *= 1.3;
	}
	walter.bank_account += walter.income;
}


void saul_income(const int year, const int month)
{
	if (month == 10) {
		saul.income *= 1.05;
	}
	if (year == 2030 && month == 3) {
		saul.income *= 1.2;
	}
	saul.bank_account += saul.income;
}


void deposit_simulation(struct Person& person, const int year, const int month)
{
	static double deposit_rate = 15;
	static double month_rate = (deposit_rate * 0.01) / 12;

	person.deposit = person.deposit * (1.0 + month_rate);

	if (person.bank_account > 0) {
		person.deposit += person.bank_account;
		person.bank_account = 0;
	}

	if (person.bank_account < 0 && person.deposit > 0) {
		double need = -person.bank_account;
		double take = (person.deposit >= need) ? need : person.deposit;
		person.deposit -= take;
		person.bank_account += take;
	}

	if (person.has_flat == false && person.deposit >= person.flatcost) {
		person.deposit -= person.flatcost;    
		person.has_flat = true;
	}
}


void mortage_simulation(Person& p, int year, int month)
{
	if (p.mortgage_balance <= 0)
		return;

	if (p.bank_account < p.mortgage_monthly_payment)
		return;

	double annual_rate = 15.0;
	double month_rate = annual_rate / 12.0 / 100.0;

	double interest = p.mortgage_balance * month_rate;
	double principal = p.mortgage_monthly_payment - interest;

	if (principal > p.mortgage_balance) {
		principal = p.mortgage_balance;
		p.mortgage_monthly_payment = principal + interest;
	}

	p.mortgage_balance -= principal;
	p.bank_account -= p.mortgage_monthly_payment;
	p.mortgage_total_paid += p.mortgage_monthly_payment;

	if (p.mortgage_balance <= 0) {
		p.mortgage_balance = 0;
		printf("Mortgage fully paid in %d.%02d\n", year, month);
	}

	if (month == 12) {
		printf(
			"Year %d | mortgage left: %.0f | total paid: %.0f\n\n",
			year,
			p.mortgage_balance,
			p.mortgage_total_paid
		);
	}
}


void person_food(struct Person& person)
{
	person.food *= 1.008;
	person.bank_account -= person.food;
}


void person_diff_services(struct Person& person)
{
	person.diff_services *= 1.008;
	person.bank_account -= person.diff_services;
}


void person_clothes(struct Person& person, const int month)
{
	if (month % 3 == 0) {
		person.clothes *= 1.025;
		person.bank_account -= person.clothes;
	}
}


void person_unforeseen_expenses(struct Person& person, const int month)
{
	if (month % 3 == 0) {
		person.unforeseen_expenses *= 1.025;
		person.bank_account -= person.unforeseen_expenses;
	}
}


void person_trip(struct Person& person, const int month)
{
	if (month == 7) {
		person.trip *= 1.08;
		person.bank_account -= person.trip;
	}
}


void person_technique(struct Person& person, const int year, const int month)
{
	if (year % 5 == 0 && month == 1) {
		person.technique *= 1.3;
		person.bank_account -= person.technique;
	}
}


void person_rent_flat(struct Person& person)
{
	if (person.has_flat)
		return;
	person.rent_flat *= 1.005;
	person.bank_account -= person.rent_flat;
}


void person_flatcost(struct Person& person, const int month)
{
	if (month == 7) {
		person.flatcost = (person.flatcost * 1.1);
	}
}


void simulation()
{
	int year = 2025;
	int month = 12;

	while (!(year == 2045 && month == 9)) {
		walter_income(year, month);
		person_food(walter);
		person_diff_services(walter);
		person_clothes(walter, month);
		person_unforeseen_expenses(walter, month);
		person_trip(walter, month);
		person_flatcost(walter, month);
		person_technique(walter, year, month);
		person_rent_flat(walter);
		deposit_simulation(walter, year, month);

		saul_income(year, month);
		person_food(saul);
		person_diff_services(saul);
		person_clothes(saul, month);
		person_unforeseen_expenses(saul, month);
		person_trip(saul, month);
		person_flatcost(saul, month);
		person_technique(saul, year, month);
		mortage_simulation(saul, year, month);

		printf("saul capital = %.0f\n", saul.bank_account);
		printf("walter capital = %.0f\n", walter.bank_account);

		if (month == 12) {
			printf(
				"Year %d | Walter flat: %s | Saul flat: %s\n\n",
				year,
				walter.has_flat ? "YES" : "NO",
				saul.has_flat ? "YES" : "NO"
			);
		}

		++month;
		if (month == 13) {
			month = 1;
			++year;
		}
	}
}


void print_person_info()
{
	printf("walter capital = %.0f\n", walter.bank_account);
	printf("saul capital = %.0f\n\n", saul.bank_account);
	printf("deposit = %.0f\n", walter.deposit);
	printf("flatcost = %.0f\n", walter.flatcost);
	printf("  Walter bank account: %.0f RUB\n", walter.bank_account);
	printf("  Saul bank account: %.0f RUB\n", saul.bank_account);
}


void walter_init()
{
	walter.has_flat = false;
	walter.bank_account = 1100 * 1000;
	walter.income = 200 * 1000;
	walter.food = 30 * 1000;
	walter.diff_services = 25 * 1000;
	walter.clothes = 15 * 1000;
	walter.unforeseen_expenses = 50 * 1000;
	walter.trip = 120 * 1000;
	walter.technique = 200 * 1000;
	walter.rent_flat = 40 * 1000;
	walter.flatcost = 10000 * 1000;
	walter.deposit = 10 * 1000;
}


void saul_init()
{
	saul.has_flat = true;
	saul.bank_account = 1100 * 1000;
	saul.income = 230 * 1000;
	saul.food = 30 * 1000;
	saul.diff_services = 25 * 1000;
	saul.clothes = 15 * 1000;
	saul.unforeseen_expenses = 50 * 1000;
	saul.trip = 120 * 1000;
	saul.technique = 200 * 1000;
	saul.flatcost = 10000 * 1000;

	double annual_rate = 15.0;
	int years = 20;
	int total_months = years * 12;

	double initial_payment = 1'000'000;
	double loan_amount = saul.flatcost - initial_payment;

	saul.bank_account -= initial_payment;
	saul.mortgage_balance = loan_amount;
	saul.mortgage_total_paid = 0;

	double month_rate = annual_rate / 12.0 / 100.0;

	double annuity =
		(month_rate * pow(1 + month_rate, total_months)) /
		(pow(1 + month_rate, total_months) - 1);

	saul.mortgage_monthly_payment = loan_amount * annuity;

	printf(
		"Saul mortgage: %.0f, payment: %.0f\n",
		loan_amount,
		saul.mortgage_monthly_payment
	);
}


int main()
{	
	walter_init();
	saul_init();

	simulation();

	print_person_info();

}
