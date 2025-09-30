include <iostream>
#include <cstdlib> //завершение кода
#include <clocale>

//sing namespace std;

typedef long long int RUB;

struct Person {
	RUB bank_account; // баланс
	RUB income; // ежемесячный доход
};

Person Alice_rub, Bob_rub;

int current_year;
int current_month;
int retire_year;
int retire_year_Bob; 
int retire_month;
int retire_month_Bob;
int bank_rate;

void initialize() 
{
	std::cout << "Введите текущий баланс Алисы, затем Боба в рублях: ";
	
	std::cin >> Alice_rub.bank_account;
	std::cin >> Bob_rub.bank_account;
	
	std::cout << "Введите текущий ежемесячный доход Алисы, затем Боба в рублях: ";
	
	std::cin >> Alice_rub.income;
	std::cin >> Bob_rub.income;
	
	std::cout << "Текущий год: ";
	
	std::cin >> current_year;
	
	std::cout << "Номер текущего месяца: ";
	
	std::cin >> current_month;
	
	std::cout << "Год выхода Алисы на пенсию: ";
	
	std::cin >> retire_year;
	std::cout << "Год выхода Боба на пенсию:";
	std::cin >> retire_year_Bob;
	
	if (current_month > 13 && current_month == 0) {
		std::cout << "Ошибка (месяц не может быть равено 0 и больше 13";
		exit(1);
	}
	if (retire_year_Bob < current_year) {
		std::cout << "Ошибка (год выхода на пенсию не может быть меньше года";
		exit(1);
	}
	std::cout << "Номер месяца выхода Боба на пенсию: ";
	std::cin >> retire_month_Bob;
	
	if (retire_year < current_year) {
		std::cout << "Ошибка (год выхода на пенсию не может быть меньше года";
		exit(1);
	}
	std::cout << "Номер месяца выхода Алисы на пенсию: ";

	std::cin >> retire_month;
	if (retire_year == current_year && retire_month <= current_month) 
	{
		std::cout << "Некорректный ввод. Месяц выхода на пенсию должен быть позже текущего месяца в том же году.";
		exit(1); //немедленно завершили код
	}
	std::cout << std :: endl;
}
void monthly_expenses_Bob() {
	const RUB delicious_food = 5000;
	const RUB child_hang_out = 2000;
	const RUB car_petrol = 10000;
	const RUB additional_utilities = 4000;
	Bob_rub.bank_account -= additional_utilities + car_petrol + child_hang_out + delicious_food;

}
void monthly_expenses_Alice() /*/ежемесячные расходы /*/ 
{
	const RUB food = 2000;
	const RUB every_month_pay = 5000;
	const RUB mortgage = 3000;
	const RUB additional_utilities = 2000;

	Alice_rub.bank_account -= (food + mortgage + every_month_pay + additional_utilities);
}


void monthly_income_Bob(int year, int month) /*/Ежемесячные доходы/*/
{
	if (current_year == current_year + 15) {
		int bought_car = 250000; 
		Bob_rub.income -= bought_car;
	}
	Bob_rub.bank_account -= Bob_rub.income;


	if (month == 1) {
		Bob_rub.income *= 1.1;
	}
	Bob_rub.bank_account += Bob_rub.income;
}
void monthly_income_Alice(int year, int month) {
	// Пример: повышение дохода на 50% в январе 2035
	if (year == 2035 && month == 1) {
		Alice_rub.income = static_cast<RUB>(Alice_rub.income * 1.5);
	}
	Alice_rub.bank_account += Alice_rub.income;
}



void simulate() {
	int year = current_year;
	int month = current_month;

	while (!(year == retire_year && month == retire_month)) /*/ цикл продолжается, пока текущий месяц и год не совпадут с датой выхода на пенсию, то есть, он завершается в последний месяц перед пенсией или в месяц выхода, в зависимости от логики./*/{
		monthly_income_Alice(year, month);
		monthly_income_Bob(year, month);
		monthly_expenses_Alice();
		monthly_expenses_Bob();

		month++;
		if (month == 13) {
			month = 1;
			year++;
		}
	}
}

void output_result_Alice() {
	if (Alice_rub.bank_account < 0) {
		std::cout << "Банкрот" << "Баланс Алисы: " << Alice_rub.bank_account << " рублей.";
	}
	else {
		std::cout << "Баланс Алисы в " << retire_year << " году составляет "
			<< Alice_rub.bank_account << " рублей." << std :: endl;
	}
}


void output_result_Bob() 
{
	
	if (Bob_rub.bank_account < 0) {
		std::cout << "Банкрот" << "Баланс Боба: " << Bob_rub.bank_account << " рублей.";
	}
	else {
		std::cout << "Баланс Боба в " << retire_year << " году составляет "
			<< Bob_rub.bank_account << " рублей.";
	}


}
int main() {
	setlocale(LC_ALL, "Russian");

	initialize();

	simulate();

	output_result_Alice();

	output_result_Bob();

	return 0;
}
