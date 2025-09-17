#include <stdio.h>


typedef int RUB;

struct Person {
	RUB bank_account;
	RUB income;
	RUB food;
};
struct Person alice;

void alice_income(const int year, const int month)
{
	if (month == 10) {
		alice.income = alice.income * 1.07;
	}
	if (year == 2030 && month == 3) {
		alice.income += 1.5;
	}
	alice.bank_account += alice.income;
}

void alice_food() {
	alice.bank_account -= alice.food;

}

void simulation()
{
	int year = 2025;
	int month = 12;

	while (!(year == 2045 && month == 9)) {
		alice_income(year, month);
		alice_food();
		// alice_trip();


		++month;
		if (month == 13) {
			month = 1;
			++year;
		}
	}
}

void print_alice_info()
{
	printf("alice capital = %d\n", alice.bank_account);
}
void alice_int()
{
	alice.bank_account = 1000 * 1000;
	alice.income = 200 * 1000;
	alice.food = 30 * 1000;

}

int main()
{
	alice_int();
	simulation();
	print_alice_info();
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
