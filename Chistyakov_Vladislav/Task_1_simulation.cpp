#include <iostream>
#include <cmath>
#include <random>

typedef long long int RUB;

std::mt19937 mt{ std::random_device{}() };

struct apartment {
	RUB cost;               // Цена имеющейся квартиры
	RUB desired_cost;            // Цена желаемой квартиры
	RUB rent;               // Аренда квартиры
	RUB utilities;          // Коммунальные услуги
};

struct bank{
	RUB account;            // Счёт в банке
	RUB mortgage_amount;    // Остаток суммы по ипотеке
	RUB mortgage_payment;   // Ежемесячный платёж по ипотеке
	RUB investment;         // Вклад в банке
};

struct person {
	RUB income;          // Зарплата
	RUB food;            // Траты на еду
	apartment apartment;
	bank bank;
};


struct person alice;   // Берёт в ипотеку
struct person bob;     // Копит на вкладах


int mortgage_payment(long sum, double percent, int months) {
	return (sum * (percent / 12) / (1 - pow(1 + percent / 12, -months - 1)));
}


void alice_init() {
	alice.income = 100 * 1000;
	alice.bank.account = 0;
	alice.bank.mortgage_amount = 8 * 1000 * 1000;
	alice.bank.mortgage_payment = mortgage_payment(alice.bank.mortgage_amount, 0.06, 12 * 20); // 6%, на 20 лет
	alice.apartment.cost = 10 * 1000 * 1000;
	alice.apartment.utilities = 4 * 1000;
	alice.food = 20 * 1000;
}


void bob_init() {
	bob.income = 100 * 1000;
	bob.bank.account = 0;
	bob.bank.investment = 2 * 1000 * 1000;
	bob.apartment.desired_cost = 10 * 1000 * 1000;
	bob.apartment.cost = 0;
	bob.apartment.rent = 30 * 1000;
	bob.apartment.utilities = 4 * 1000;
	bob.food = 20 * 1000;
}


void alice_income(const int& year, const int& month) {
	if (month == 10) alice.income *= 1.07;                 // Учёт инфляции на зарплату
	if (year == 2030 && month == 3) alice.income *= 1.5;   // Повышение в должности
	alice.bank.account += alice.income;
}


void alice_apartment(const int& month) {
	if (month == 10) alice.apartment.cost *= 1.07;
}


void alice_food(const int& month) {
	if (month == 10) alice.food *= 1.07;

	alice.bank.account -= alice.food;
}

void alice_mortgage(const int& year, const int& month) {
	alice.bank.account -= alice.bank.mortgage_payment;
}

void alice_utilities(const int& month) {
	if (month == 10) alice.apartment.utilities *= 1.07;
	alice.bank.account -= alice.apartment.utilities;
}


void alice_rand_expenses() {
	int rand_num = 10 + (mt() % 50);
	alice.bank.account -= rand_num * 1000;
}


void alice_investment(const int& year, const int& month) {
	alice.bank.investment *= 1.0 + 0.20 / 12;              // Начисление процетов на вклад в банке под 20% годовых (каждый месяц)

	alice.bank.investment += alice.bank.account - 10000;
	alice.bank.account == 10000;
}


void bob_income(const int& year, const int& month) {
	if (month == 10) bob.income *= 1.07;                   // Учёт инфляции на зарплату
	if (year == 2030 && month == 3) bob.income *= 1.5;     // Повышение в должности

	bob.bank.account += bob.income;
}


void bob_apartment(const int& month) {
	if (month == 10) {
		bob.apartment.cost *= 1.07;
		bob.apartment.desired_cost *= 1.07;
	}
}


void bob_food(const int& month) {
	if (month == 10) bob.food *= 1.07;
	bob.bank.account -= bob.food;
}


void bob_utilities(const int& month) {
	if (month == 10) {
		bob.apartment.rent *= 1.07;
		bob.apartment.utilities *= 1, 07;
	}

	if (bob.apartment.cost == 0) bob.bank.account -= bob.apartment.rent;    // Пока нет квартиры - арендует
	else bob.bank.account -= bob.apartment.utilities;                           // С квартирой платит комунальные услуги
}


void bob_rand_expenses() {
	int rand_num = 10 + (mt() % 50);
	bob.bank.account -= rand_num * 1000;
}


void bob_investment(const int& year, const int& month) {
	bob.bank.investment *= 1.0 + 0.20 / 12;                     // Начисление процетов на вклад в банке под 20% годовых (каждый месяц)

	bob.bank.investment += alice.bank.account;
	alice.bank.account == 0;

	if (bob.apartment.cost == 0 && bob.bank.investment >= bob.apartment.desired_cost) {       // Если хватает инвестиций снимает и покупает
		bob.bank.investment -= bob.apartment.desired_cost;
		bob.apartment.cost = bob.apartment.desired_cost;
		std::cout << "Боб покупает квартиру: " << month << " месяц " << year << " год\n";
	}
}


void simulation() {
	int year = 2025;
	int month = 9;

	while (!(year == 2045 && month == 9)) {

		alice_income(year, month);
		alice_apartment(month);
		alice_food(month);
		alice_mortgage(year, month);
		alice_utilities(month);
		alice_rand_expenses();
		alice_investment(year, month);

		bob_income(year, month);
		bob_apartment(month);
		bob_food(month);
		bob_utilities(month);
		bob_rand_expenses();
		bob_investment(year, month);

		++month;
		if (month == 13) {
			month = 1;
			++year;
		}
	}
}


void alice_print() {
	std::cout << "Счёт Алисы в банке: " << alice.bank.account + alice.apartment.cost + alice.bank.investment << "\n";
	if (alice.apartment.cost != 0) std::cout << "Есть квартира\n";
	else std::cout << "Нет квартиры\n";
}


void bob_print() {
	std::cout << "Счёт Боба в банке: " << bob.bank.account + bob.apartment.cost + bob.bank.investment << "\n";
	if (bob.apartment.cost != 0) std::cout << "Есть квартира\n";
	else std::cout << "Нет квартиры\n";
}


int main() {
	setlocale(LC_ALL, "RU");

	alice_init();
	bob_init();

	simulation();

	alice_print();
	bob_print();
}