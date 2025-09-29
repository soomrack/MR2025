#include <stdio.h>
#include <locale.h>
#include <math.h>

void alice_init();
void bob_init();
void apply_inflation();
void apply_bank_interest();
void live(int year, int month);
void simulation();
void results();

int main() {
	setlocale(LC_ALL, "RU");
	simulation();
	results();
}



typedef double RUB;

struct Person {
	RUB initial_payment; // первоначальный взнос (только для Алисы)
	RUB bank_account;    // накопления
	RUB income;          // зарплата
	RUB expenses;        // расходы (одежда, еда, развлечения и т.д.)
	RUB rent;            // аренда (для Боба), для Алисы = 0
	RUB mortgage;        // ежемесячный аннуитетный платёж (для Алисы), для Боба = 0
	RUB apartment_price; // цена квартиры (для покупки/ипотеки)
	bool has_apartment;  // 1 если есть квартира, 0 если нет
};

struct Person alice;
struct Person bob;

double inflation = 0.07;    // 7% инфляция
double bank_rate = 0.155;	// 15,5% годовых по вкладам 
double mortgage_interest = 0.205; //20,5% годовых ставка по ипотеке
double mipm = mortgage_interest / 12; //mortgage_interest_per_month

int start_year = 2025;
int start_month = 9;
int end_year = 2045;
int end_month = 9;

int years = end_year - start_year;	//Количество лет ипотеки
int months_total = years * 12 + end_month - start_month; // Количчество месяцев ипотеки

// === ИНИЦИАЛИЗАЦИЯ АЛИСЫ ===
void alice_init() {
	alice.initial_payment = 800.0 * 1000.0;  // Первоначальный взнос Алисы
	alice.bank_account = 1000.0 * 1000.0;    // Стартовый капитал у Алисы
	alice.income = 200.0 * 1000.0;           // Зарплата у Алисы
	alice.expenses = 50.0 * 1000.0;          // Траты в месяц у Алисы
	alice.rent = 0.0;                        // Аренда жилья у Алисы
	alice.apartment_price = 7000.0 * 1000.0; // Цена квартиры Алисы
	alice.has_apartment = true;				 // Наличие квартиры у Алисы

	// === РАСЧЕТ АННУИТЕТНОГО ПЛАТЕЖА ===
	double f1 = mipm * pow(1 + mipm, months_total);
	double f2 = pow(1 + mipm, months_total) - 1;
	alice.mortgage = (alice.apartment_price - alice.initial_payment) * (f1 / f2);
	alice.bank_account -= alice.initial_payment;
}

// === ИНИЦИАЛИЗАЦИЯ БОБА ===
void bob_init() {
	bob.initial_payment = 0.0;             // У Боба нет первоначального взноса
	bob.bank_account = 1000.0 * 1000.0;    // Стартовый капитал у Боба
	bob.income = 200.0 * 1000.0;           // Зарплата у Боба
	bob.expenses = 50.0 * 1000.0;          // Траты в месяц у Боба
	bob.rent = 35.0 * 1000.0;              // Аренда жилья у Боба
	bob.mortgage = 0.0;                    // У Боба нет ипотеки
	bob.apartment_price = 7000.0 * 1000.0; // Цена квартиры Боба
	bob.has_apartment = false;			   // Наличие квартиры у Боба
}

// === ИНФЛЯЦИЯ (РАЗ В ГОД) ===
void apply_inflation() {
	alice.expenses *= (1.0 + inflation);
	bob.expenses *= (1.0 + inflation);
	bob.rent *= (1.0 + inflation);
	bob.apartment_price *= (1.0 + inflation);
}

// === ПРОЦЕНТЫ ПО ВКЛАДУ (РАЗ В МЕСЯЦ) ===
void apply_bank_interest() {
	alice.bank_account *= (1.0 + bank_rate / 12);	// Ежемесячное начисление процентов по вкладу у Алисы
	bob.bank_account *= (1.0 + bank_rate / 12);		// Ежемесячное начисление процентов по вкладу у Боба
}

// === ЖИЗНЬ (1 МЕСЯЦ) ===
void live(int year, int month) {
	apply_bank_interest();

	alice.bank_account += alice.income;
	alice.bank_account -= alice.expenses;
	alice.bank_account -= alice.rent;
	alice.bank_account -= alice.mortgage;

	bob.bank_account += bob.income;
	bob.bank_account -= bob.expenses;
	bob.bank_account -= bob.rent;

	if (bob.bank_account > bob.apartment_price && !bob.has_apartment) {
		bob.bank_account -= bob.apartment_price;
		bob.has_apartment = true;
		printf("Боб купил квартиру %d %02d\n", year, month);
	}
}

// === СИМУЛЯЦИЯ ===
void simulation() {
	int year = start_year;
	int month = start_month;
	alice_init();
	bob_init();

	while (!(year == end_year && month == end_month)) {
		live(year, month);
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
	printf("Алиса: счет = %.2f, квартира %s\n", alice.bank_account, alice.has_apartment ? "есть" : "нету");
	printf("Боб:   счет = %.2f, квартира %s\n", bob.bank_account, bob.has_apartment ? "есть" : "нету");
	RUB alice_total = alice.bank_account + (alice.has_apartment ? alice.apartment_price : 0.0);
	RUB bob_total = bob.bank_account + (bob.has_apartment ? bob.apartment_price : 0.0);
	printf("\nСравнение:\n");
	if (alice_total > bob_total) printf("Лучше вариант Алисы (итого: %.2f руб.)\n", alice_total);
	else if (bob_total > alice_total) printf("Лучше вариант Боба (итого: %.2f руб.)\n", bob_total);
	else printf("Итог одинаковый (%.2f руб.)\n", alice_total);
}
