#include <stdio.h>

/*АААААА оно не отправялется из vs code сюда, ничего не работает я не опнимаю в чем ошибка, по кнопке отправить
обновляется только репозиторий из профиля в гитхабе, не понимаю как это все работает*/

typedef int RUB;

struct Person {
	RUB bank_account;
	RUB income;
	RUB food;
	RUB diff_services;
};
struct Person walter;

void walter_income(const int year, const int month)
{
	if (month == 10) {
		walter.income = walter.income * 1.07;
	}
	if (year == 2030 && month == 3) {
		walter.income += 1.5;
	}
	walter.bank_account += walter.income;
}

void walter_food() {
	walter.bank_account -= walter.food;

}

void walter_diff_services() {
	walter.bank_account -= walter.diff_services;

}

void simulation()
{
	int year = 2025;
	int month = 12;

	while (!(year == 2045 && month == 9)) {
		walter_income(year, month);
		walter_food();
		walter_diff_services();
		// walter_trip();
		// walter_unforeseen_expenses();
		// walter_technique();
		// walter_clothes();

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
}

int main()
{
	walter_int();
	simulation();
	print_walter_info();
}
