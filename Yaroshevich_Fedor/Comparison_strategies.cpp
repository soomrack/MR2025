#include <stdio.h>

//Код в процессе создания

typedef int RUB;

struct Person {
	RUB bank_account;
	RUB income;
	RUB food;
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

void simulation()
{
	int year = 2025;
	int month = 12;

	while (!(year == 2045 && month == 9)) {
		walter_income(year, month);
		walter_food();
		// walter_trip();


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
	walter.income = 200 * 1000;
	walter.food = 30 * 1000;

}

int main()
{
	walter_int();
	simulation();
	print_walter_info();
}

// Запуск программы: CTRL+F5 или меню "Отладка" > "Запуск без отладки"
// Отладка программы: F5 или меню "Отладка" > "Запустить отладку"

// Советы по началу работы 
//   1. В окне обозревателя решений можно добавлять файлы и управлять ими.
//   2. В окне Team Explorer можно подключиться к системе управления версиями.
//   3. В окне "Выходные данные" можно просматривать выходные данные сборки и другие сообщения.
//   4. В окне "Список ошибок" можно просматривать ошибки.
//   5. Последовательно выберите пункты меню "Проект" > "Добавить новый элемент", чтобы создать файлы кода, или "Проект" > "Добавить существующий элемент", чтобы добавить в проект существующие файлы кода.
//   6. Чтобы снова открыть этот проект позже, выберите пункты меню "Файл" > "Открыть" > "Проект" и выберите SLN-файл.
