#include <iostream>
#include <cmath>
#include <random>

typedef long int RUB;

std::mt19937 mt{ std::random_device{}() };

struct person {
	RUB bank_account;  //Счёт в банке
	RUB income;        //Зарплата
	RUB mortgage;      //Остаток суммы по ипотеке
	RUB investment;    //Вклад в банке

	RUB food;          //Траты на еду
	RUB rent;          //Аренда квартиры
	RUB utilities;     //Комунальные услуги

	bool HasApartment; //Наличие квартиры
};

struct person alice;   //Берёт в ипотеку
struct person bob;     //Копит на вкладах


//Заранее выставленные переменные (Алиса уже внесла первоначальный взнос)
void alice_init() {
	alice.bank_account = 0;
	alice.mortgage = 8 * 1000 * 1000;
	alice.income = 70 * 1000;
	alice.food = 20 * 1000;
	alice.utilities = 4 * 1000;
	alice.HasApartment = true;
}

//Заранее выставленные переменные (Боб уже полодил деньги на накопительный счёт)
void bob_init() {
	bob.bank_account = 0;
	bob.investment = 2 * 1000 * 1000;
	bob.income = 70 * 1000;
	bob.food = 20 * 1000;
	bob.rent = 30 * 1000;
	bob.utilities = 4 * 1000;
	bob.HasApartment = false;
}

//Стоимость квартиры
RUB apartment_cost = 10 * 1000 * 1000;

//Инфляция
void inflation(const int& month) {
	if (month == 10) {
		apartment_cost *= 1.07;

		alice.food *= 1.07;
		bob.food *= 1.07;

		bob.rent *= 1.07;

		alice.utilities *= 1.07;
		bob.utilities *= 1.07;
	}
}

void alice_income(const int& year, const int& month) {
	if (month == 10) alice.income *= 1.07;                 //Учёт инфляции на зарплату
	if (year == 2030 && month == 3) alice.income *= 1.5;   //Повышение в должности
	alice.bank_account += alice.income;
}

void bob_income(const int& year, const int& month) {
	if (month == 10) bob.income *= 1.07;                   //Учёт инфляции на зарплату
	if (year == 2030 && month == 3) bob.income *= 1.5;     //Повышение в должности
	bob.bank_account += bob.income;
	bob.investment *= 1 + 0.20 / 12;                       //Начисление процетов на вклад в банке под 20% годовых (каждый месяц)
}

//Вычисление ежемесячного платеже по ипотеке
int mortgage_payment(long sum, double percent, int months) {
	return (sum * (percent / 12) / (1 - pow(1 + percent / 12, -months - 1)));
}

//Обязательные затраты на проживание для Алисы
void alice_costs() {
	alice.bank_account -= alice.food;
	alice.bank_account -= alice.utilities;
	alice.bank_account -= mortgage_payment(alice.mortgage, 0.06, 12 * 20);
	//Алиса берёт ипотеку с донорством, так чтобы процент был 6%, на 20 лет
}

//Обязательные затраты на проживание для Боба
void bob_costs() {
	bob.bank_account -= bob.food;
	if (!bob.HasApartment) bob.bank_account -= bob.rent;       //Пока нет квартиры снимает арендует
	else bob.bank_account -= bob.utilities;                    //С квартирой платит комунальные услуги
}

//Случайные траты Боба и Алисы (одинаковые для более точного подсчёта)
void rand_costs() {
	int rand_num = 10 + (mt() % 50);
	alice.bank_account -= rand_num * 1000;
	bob.bank_account -= rand_num * 1000;
}

//Что делает Алиса с остатком
void alice_remain(const int& year, const int& month) {
	if (alice.mortgage >= alice.bank_account) {             //Пока есть ипотека, погашает её заранее
		alice.mortgage -= alice.bank_account;
		alice.bank_account = 0;
	}
	else {													//Если суммы хватает, чтобы погасить ипотеку полностью
		if (alice.mortgage < alice.bank_account && alice.mortgage != 0) {
			alice.bank_account -= alice.mortgage;
			alice.mortgage = 0;
			std::cout << "Алиса погасила ипотеку: " << month << " месяц " << year << " год\n";
		}                                                   //В остальных случаюх копит
	}
}

//Что делает Боб с остатком
void bob_remain(const int& year, const int& month) {
	if (!bob.HasApartment && bob.investment < apartment_cost) {         //Пока денег на вкладе не хватает продолжает вкладывать
		bob.investment += bob.bank_account;
		bob.bank_account = 0;
	}
	else if (!bob.HasApartment && bob.investment >= apartment_cost) {   //Если накопил, то снимает всё и покупает
		bob.bank_account += bob.investment;
		bob.investment = 0;
		bob.bank_account -= apartment_cost;
		bob.HasApartment = true;
		std::cout << "Боб купил квартиру: " << month << " месяц " << year << " год\n";
	}														   	        //В остальных случаюх копит, без накопительного счёта
}

void simulation() {
	int year = 2025;
	int month = 9;

	while (!(year == 2045 && month == 9)) {
		inflation(month);

		alice_income(year, month);
		bob_income(year, month);

		alice_costs();
		bob_costs();
		rand_costs();

		alice_remain(year, month);
		bob_remain(year, month);

		++month;
		if (month == 13) {
			month = 1;
			++year;
		}
	}
}

void alice_print() {
	std::cout << "Счёт Алисы в банке: " << alice.bank_account << "\n";
	if (alice.HasApartment) std::cout << "Есть квартира\n";
	else std::cout << "Нет квартиры\n";
}

void bob_print() {
	std::cout << "Счёт Боба в банке: " << bob.bank_account << "\n";
	if (bob.HasApartment) std::cout << "Есть квартира\n";
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