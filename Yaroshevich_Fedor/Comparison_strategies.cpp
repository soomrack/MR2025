#include <stdio.h>

//получил исправления на паре, исправляю код

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
};
struct Person walter;

void walter_income(const int year, const int month)
{
	if (month == 10) {
		walter.income = walter.income * 1.05;
	}
	if (year == 2030 && month == 3) {
		walter.income += 1.4;
	}
	walter.bank_account += walter.income;
}

void walter_bank_account(const int year, const int month)
{
	float deposit_rate = 15;
	float month_rate = (deposit_rate * 0.01)/12;
	walter.bank_account = static_cast<RUB>(walter.bank_account * (1.0 + month_rate));
} 

void walter_food() {
	walter.food = static_cast<RUB>(walter.food * 1.008);
	walter.bank_account -= walter.food;

}

void walter_diff_services() {
	walter.diff_services = static_cast<RUB>(walter.diff_services * 1.008);
	walter.bank_account -= walter.diff_services;

}

void walter_clothes() {
	walter.clothes = static_cast<RUB>(walter.clothes * 1.02);
	walter.bank_account -= walter.clothes;

}

void walter_unforeseen_expenses() {
	walter.unforeseen_expenses = static_cast<RUB>(walter.unforeseen_expenses * 1.02);
	walter.bank_account -= walter.unforeseen_expenses;

}

void walter_trip() {
	walter.trip = static_cast<RUB>(walter.trip * 1.08);
	walter.bank_account -= walter.trip;

}

void walter_technique() {
	walter.technique = static_cast<RUB>(walter.technique * 1.3);
	walter.bank_account -= walter.technique;

}

void simulation()
{
	int year = 2025;
	int month = 12;

	while (!(year == 2045 && month == 9)) {
		walter_income(year, month);
		walter_bank_account(year, month);
		walter_food();
		walter_diff_services();

		if (month % 3 == 0) {
			walter_clothes();
			walter_unforeseen_expenses();
		}
	
		if (month == 7) {
			walter_trip();
		}

		if (year % 5 == 0 && month == 1) {
			walter_technique();
		}

		/*предусмотреть изменение ставки вклада, */

		++month;
		if (month == 13) {
			month = 1;
			++year;
		}
	}
}

void print_walter_info()
{
	printf("walter capital = %d\n", walter.bank_account);
}
void walter_int()
{
	walter.bank_account = 10 * 1000;
	walter.income = 150 * 1000;
	walter.food = 30 * 1000;
	walter.diff_services = 25 * 1000;
	walter.clothes = 15 * 1000;
	walter.unforeseen_expenses = 50 * 1000;
	walter.trip = 120000;
	walter.technique = 200000;
}

int main()
{
	walter_int();
	simulation();
	print_walter_info();
}



