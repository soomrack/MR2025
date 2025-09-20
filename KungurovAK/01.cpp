#include <Math.h>
#include <iostream>
using namespace std; 


int mortgage_strategy() {
	int c = 0;
	float ap_cost = pow(10, 7); // стоимость квартиры
	float salary = 1.8 * pow(10, 5); // зарплата
	float mortgage_rate = 0.00833; // месячный процент по ипотеке
	float spendings = 4 * pow(10, 4); // ежемесячные расходы
	int months = 0; // количество месяцев на выплату ипотеки
	float mortgage_monthly_payment = pow(10, 20);

	while (mortgage_monthly_payment > salary - spendings) {
		months++;
		mortgage_monthly_payment = ap_cost * (mortgage_rate * pow((1 + mortgage_rate), months))/(pow((1 + mortgage_rate), months) - 1) ; // ежемесячный платёж по ипотеке	
		if (months % 12 == 0 and months != 0) { // ежегодные изменения
			spendings += spendings * 0.1; // рост расходов на 10% каждый год
			ap_cost += ap_cost * 0.05; // ежегодный рост стоимости квартиры на 5%
			if (months % 48 == 0 and months != 0) {
				salary += salary * 0.07; // раз в 4 года рост зарплаты на 10%
			}
		}
	}

	return months;
}

int saving_strategy() {
	float rent = 30000; // стоимость аренды
	float ap_cost = pow(10, 7); // стоимость квартиры
	double salary = 1.8 * pow(10, 5); // зарплата
	float deposit_rate = 7; // годовой процент по вкладу
	float spendings = 4 * pow(10, 4); // ежемесячные расходы
	float deposit = 0; // текущий размер вклада
	int months = 0; // количество месяцев на накопление
	while (deposit < ap_cost ) {
		deposit += salary - spendings - rent;
		deposit += deposit * deposit_rate/100/12;
		if (salary < rent + spendings) { // если расходы превышают доходы
			cout << "Расходы превышают доходы — проверьте параметры!\n";
			return -1;
		}
		if (months % 12 == 0 and months != 0) { // ежегодные изменения
			rent += rent * 0.1; // ежегодный рост аренды на 10%
			spendings += spendings * 0.1; // рост расходов на 10% каждый год
			ap_cost += ap_cost * 0.05; // ежегодный рост стоимости квартиры на 5%
			if (months % 48 == 0 and months != 0) {
				salary += salary * 0.07; // раз в 4 года рост зарплаты на 10%
			}
		}
		months++;
		if (months > 1200) { // защита от бесконечного цикла (максимум 50 лет)
			cout << "Слишком долго копить — проверьте параметры!\n";
			return -1;
		}
	}

	return months;
}


int main() 
{
	int mortgage_months = mortgage_strategy();
	int saving_months = saving_strategy();

	cout << "Ипотека: " << mortgage_months / 12 << " лет и " << mortgage_months % 12 << " мес.\n";
	cout << "Вклад: " << saving_months / 12 << " лет и " << saving_months % 12 << " мес.\n";
	return 0;
}
