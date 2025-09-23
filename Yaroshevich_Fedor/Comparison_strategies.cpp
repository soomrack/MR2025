#include <stdio.h>

/*АААААА оно не отправялется из vs code сюда, ничего не работает я не опнимаю в чем ошибка, по кнопке отправить
обновляется только репозиторий из профиля в гитхабе, не понимаю как это все работает*/

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

void walter_food() {
	walter.bank_account -= walter.food;

}

void walter_diff_services() {
	walter.bank_account -= walter.diff_services;

}

void walter_clothes() {
	walter.bank_account -= walter.clothes;

}

void walter_unforeseen_expenses() {
	walter.bank_account -= walter.unforeseen_expenses;

}

void walter_trip() {
	walter.bank_account -= walter.trip;

}

void walter_technique() {
	walter.bank_account -= walter.technique;

}

void simulation()
{
	int year = 2025;
	int month = 12;

	while (!(year == 2045 && month == 9)) {
		walter_income(year, month);
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

		/*нужен цикл для расходов, чаще всего происходящих раз в 3 месяца(ситуативные / сезонные),
		цикл вклада, предусмотреть изменение ставки вклада, предусмотреть редкие дорогие покупки (technique), 
		включить также путешествия, предусмотреть инфляцию на основные расходы, */

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
	walter.bank_account = 1000 * 1000;
	walter.income = 150 * 1000;
	walter.food = 30 * 1000;
	walter.diff_services = 15 * 1000;
	walter.clothes = 15 * 1000;
	walter.unforeseen_expenses = 50 * 1000;
	walter.trip = 100000;
	walter.technique = 170000;
}

int main()
{
	walter_int();
	simulation();
	print_walter_info();
}

