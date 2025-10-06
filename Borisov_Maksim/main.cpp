#include <stdio.h>
#include <locale.h>
#include <math.h>

// === TODO ===
// все виды расходов и доходов вынести в отдельные функции
// отдельный счет для накоплений в банке
// еще структуру для дома (с арендой и прочими затратами по дому)
// 
// функцию проверки покупки квартиры боба в отдельную функцию
// лишние переменные months и years убрать
// === TODO ===

typedef long long int RUB; //--- В КОПЕЙКАХ РАСЧЕТЫ INT


struct Home {
	RUB rent;            // аренда (для Боба), для Алисы = 0
	RUB apartment_price; // цена квартиры (для покупки/ипотеки)
	RUB apartment_price_buy; // цена квартиры на момент покупки
	RUB mortgage;        // ежемесячный аннуитетный платёж (для Алисы), для Боба = 0
	RUB mortage_total;
};


struct Work {
	RUB income;          // зарплата
	RUB new_year_award;	 // новогодняя премия
};


struct Person {
	RUB initial_payment; // первоначальный взнос (только для Алисы)
	RUB money;			 // наличка
	RUB bank_account;	 // банковсвкий счет для накоплений
	RUB expenses;        // расходы (одежда, еда, развлечения и т.д.)
	bool has_apartment;  // 1 если есть квартира, 0 если нет

	Home home;
	Work work;
};


struct Person alice;
struct Person bob;

const double inflation = 0.07;    // 7% инфляция
const double bank_rate = 0.155;	// 15,5% годовых по вкладам 
const double mortgage_interest = 0.205; //20,5% годовых ставка по ипотеке
const double mipm = mortgage_interest / 12; //mortgage_interest_per_month

const int start_year = 2025;
const int start_month = 9;
const int end_year = 2045;
const int end_month = 9;

const int years = end_year - start_year;	//Количество лет ипотеки
const int months_total = years * 12 + end_month - start_month; // Количчество месяцев ипотеки


// === ИНИЦИАЛИЗАЦИЯ АЛИСЫ ===
void alice_init() {
	alice.initial_payment = 800 * 1000 * 100;  // Первоначальный взнос Алисы
	alice.money = 1000 * 1000 * 100;		   // Наличка у Алисы
	alice.expenses = 50 * 1000 * 100;          // Траты в месяц у Алисы
	alice.has_apartment = true;				   // Наличие квартиры у Алисы
	alice.home.rent = 0;                       // Аренда жилья у Алисы
	alice.home.apartment_price = 7000 * 1000 * 100; // Цена квартиры Алисы
	alice.work.new_year_award = 30 * 1000 * 100; //новогодняя премия у Алисы
	alice.work.income = 200 * 1000 * 100;           // Зарплата у Алисы


	// === РАСЧЕТ АННУИТЕТНОГО ПЛАТЕЖА ===
	double f1 = mipm * pow(1 + mipm, months_total);
	double f2 = pow(1 + mipm, months_total) - 1;
	alice.home.mortgage = (alice.home.apartment_price - alice.initial_payment) * (f1 / f2);
	alice.bank_account = alice.money - alice.initial_payment;
	alice.money = 0;
	alice.home.mortage_total = 0;
}


void alice_income(const int year, const int month) {
	if (year % 3 == 0 && month == 1) {
		alice.work.income *= 1.1; //Повышение зарплаты Алисы раз в 3 года
		printf("Повышение зарплаты Алисы %d %02d до %lld руб.\n", year, month, alice.work.income/100);
	}
	if (month == 12) {
		alice.money += alice.work.new_year_award;
		printf("*** Выплата новогодней премии Алисы %d %02d %lld руб.\n", year, month, alice.work.new_year_award / 100);
		alice.work.new_year_award += 5 * 1000 * 100; // раз в год повышение премии
	}
	alice.money += alice.work.income;
}


void alice_expenses(const int month) {
	if (month % 4 == 0) {
		alice.money -= 40 * 1000 * 100;	// Крупная трата раз в 3 месяца
	}
	alice.money -= alice.expenses;
}


void alice_rent() {
	alice.money -= alice.home.rent;	// Аренда квартиры
}


void alice_mortage(int year, int month) {
	alice.money -= alice.home.mortgage;	// Платеж по ипотеке
	alice.home.mortage_total += alice.home.mortgage;
	printf("Алиса заплатила ипотеку %d %02d %lld руб.\n", year, month, alice.home.mortgage / 100);
}


void alice_money_transfer(int year, int month) {
	alice.bank_account += alice.money;
	printf("Алиса положила в банк %d %02d %lld руб.\n", year, month, alice.money / 100);
	alice.money = 0;
}


// === ИНИЦИАЛИЗАЦИЯ БОБА ===
void bob_init() {
	bob.initial_payment = 0;                 // У Боба нет первоначального взноса
	bob.money = 1000 * 1000 * 100;			 // Наличка у Боба
	bob.expenses = 50 * 1000 * 100;          // Траты в месяц у Боба
	bob.has_apartment = false;			     // Наличие квартиры у Боба
	bob.home.rent = 35 * 1000 * 100;         // Аренда жилья у Боба
	bob.home.mortgage = 0;                   // У Боба нет ипотеки
	bob.home.apartment_price = 7000 * 1000 * 100; // Цена квартиры Боба
	bob.work.new_year_award = 40 * 1000 * 100; //новогодняя премия у Боба
	bob.work.income = 200 * 1000 * 100;           // Зарплата у Боба

	bob.bank_account = bob.money;
	bob.money = 0;
}


void bob_income(const int year, const int month) {
	if (year % 3 == 0 && month == 1) {
		bob.work.income *= 1.1; //Повышение зарплаты Боба раз в 3 года
		printf("Повышение зарплаты Боба %d %02d до %lld руб.\n", year, month, bob.work.income / 100);
	}
	if (month == 12) {
		bob.money += bob.work.new_year_award;
		printf("*** Выплата новогодней премии Боба %d %02d %lld руб.\n", year, month, bob.work.new_year_award / 100);
		bob.work.new_year_award += 5 * 1000 * 100; // раз в год повышение премии
	}
	bob.money += bob.work.income;
}


void bob_expenses(const int month) {
	if (month % 4 == 0) {
		bob.money -= 40 * 1000 * 100;	// Крупная трата раз в 3 месяца
	}
	bob.money -= bob.expenses;
}


void bob_rent() {
	bob.money -= bob.home.rent;	// Аренда квартиры
}


void bob_money_transfer(int year, int month) {
	bob.bank_account += bob.money;
	printf("Боб положил в банк %d %02d %lld руб.\n\n", year, month, bob.money / 100);
	bob.money = 0;
}


void bob_can_buy_apartment(int year, int month) {
	if (bob.bank_account > bob.home.apartment_price && !bob.has_apartment) {
		bob.bank_account -= bob.home.apartment_price;
		bob.has_apartment = true;
		printf("!!! Боб купил квартиру %d %02d\n", year, month);
		printf("!!! На момент покупки квартира Боба стоит: %lld руб.\n", bob.home.apartment_price / 100);
		bob.home.apartment_price_buy = bob.home.apartment_price;
	}
}


// === ИНФЛЯЦИЯ (РАЗ В ГОД) ===
void apply_inflation() {
	alice.expenses *= (1.0 + inflation);
	bob.expenses *= (1.0 + inflation);
	bob.home.rent *= (1.0 + inflation);
	bob.home.apartment_price *= (1.0 + inflation);
}


// === ПРОЦЕНТЫ ПО ВКЛАДУ (РАЗ В МЕСЯЦ) ===
void apply_bank_interest() {
	alice.bank_account *= (1.0 + bank_rate / 12);	// Ежемесячное начисление процентов по вкладу у Алисы
	bob.bank_account *= (1.0 + bank_rate / 12);		// Ежемесячное начисление процентов по вкладу у Боба
}


// === СИМУЛЯЦИЯ ===
void simulation() {
	int year = start_year;
	int month = start_month;
	alice_init();
	bob_init();

	while (!(year == end_year && month == end_month)) {
		apply_bank_interest();

		alice_income(year, month);
		alice_expenses(month);
		alice_rent();
		alice_mortage(year, month);
		alice_money_transfer(year, month);

		bob_income(year, month);
		bob_expenses(month);
		bob_rent();
		bob_money_transfer(year, month);

		bob_can_buy_apartment(year, month);

		if (month == 12) apply_inflation();

		month++;
		if (month == 13) {
			year++;
			month = 1;
		}
	}
}


// === ВЫВОД РЕЗУЛЬТАТОВ ===
void results() {
	printf("\n === РЕЗУЛЬТАТЫ ЗА %d ЛЕТ ===\n", end_year - start_year);
	printf("Алиса: счет = %.2lld руб., квартира %s\n", alice.bank_account / 100, alice.has_apartment ? "есть" : "нету");
	printf("Боб:   счет = %.2lld руб., квартира %s\n", bob.bank_account / 100, bob.has_apartment ? "есть" : "нету");

	RUB alice_total = (alice.bank_account + (alice.has_apartment ? alice.home.apartment_price : 0)) / 100;
	RUB bob_total = (bob.bank_account + (bob.has_apartment ? bob.home.apartment_price_buy : 0)) / 100;

	printf("\nСравнение:\n");

	if (alice_total > bob_total) printf("Лучше вариант Алисы (итого: %.2lld руб.)\n", alice_total);
	else if (bob_total > alice_total) printf("Лучше вариант Боба (итого: %.2lld руб.)\n", bob_total);
	else printf("Итог одинаковый (%.2lld руб.)\n", alice_total);

	printf("Цена квартиры Боба через 20 лет: %lld руб.\n", bob.home.apartment_price / 100);
	printf("Сумма выплат по ипотеке Алисы через 20 лет: %lld руб.\n", alice.home.mortage_total / 100);
}


int main() {
	setlocale(LC_ALL, "RU");
	simulation();
	results();
}
