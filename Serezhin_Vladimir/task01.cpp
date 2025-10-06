#include <iostream>

typedef long long int YUAN;

struct Person {
	YUAN bank_account;
	YUAN income;
	YUAN saving_account;
	YUAN saving_payment;
	int retire_year, retire_month;
};

struct House {
	YUAN price;
	YUAN mortgage;
	YUAN taxes;
	YUAN stuff;
	bool flag_own;
};

struct Car {
	YUAN price;
	YUAN petrol;
	YUAN maintenance; // техобслуживание машины
	YUAN fines;
	bool flag_own;
};

struct Entertainment {
	YUAN inflation;
	YUAN beverages;
	YUAN food;
	YUAN public_places;
};


Person lixia_Person, bob_Person;

House lixia_House, bob_House; 

Car lixia_Car, bob_Car; 

Entertainment lixia_Entertainment, bob_Entertainment;

int current_year = 2025, current_month = 10;


void lixia_init()
{
	lixia_Person.bank_account = 5000000;
	lixia_Person.income = 30000;
	lixia_Person.retire_year = 2060;
	lixia_Person.retire_month = 10;
	lixia_Person.saving_account = 0;
	lixia_Person.saving_payment = 10000;

	lixia_House.price = 100000;
	lixia_House.mortgage = 8000;
	lixia_House.taxes = 1000;
	lixia_House.stuff = 300;
	lixia_House.flag_own = false;

	lixia_Car.price = 0;        // Лися не покупает машину
	lixia_Car.petrol = 0;
	lixia_Car.maintenance = 0;
	lixia_Car.fines = 0;
	lixia_Car.flag_own = false;

	lixia_Entertainment.beverages = 100;
	lixia_Entertainment.food = 2000;
	lixia_Entertainment.public_places = 1000;
	lixia_Entertainment.inflation = 0.1;
}

void bob_init()
{
	bob_Person.bank_account = 300000;
	bob_Person.income = 20000;
	bob_Person.retire_year = 2060;
	bob_Person.retire_month = 10;
	bob_Person.saving_account = 0;
	bob_Person.saving_account = 0;
	bob_Person.saving_payment = 3000;

	bob_House.price = 0;        // Боб не покупает дом
	bob_House.mortgage = 0;
	bob_House.taxes = 0;
	bob_House.stuff = 0;
	bob_House.flag_own = false;

	bob_Car.price = 200000;     
	bob_Car.petrol = 200;
	bob_Car.maintenance = 1000;
	bob_Car.fines = 100;
	bob_Car.flag_own = false;

	bob_Entertainment.beverages = 100;
	bob_Entertainment.food = 2000;
	bob_Entertainment.public_places = 1000;
	bob_Entertainment.inflation = 0.1;
}


void lixia_output()
{
	if (lixia_Person.bank_account < 0) {
		std::cout << "Damn! Lixia is a bankrupt. Money has gone down the drain on beverages. "
			<< "Lixia's bank account is " << lixia_Person.bank_account;
	}
	else {
		std::cout << "Lixia's bank account in " << lixia_Person.retire_year << " is equal to "
			<< lixia_Person.bank_account << " yuan" << std::endl;
	}
}

void bob_output()
{
	if (bob_Person.bank_account < 0) {
		std::cout << "Damn! Bob is a bankrupt. Money has gone down the drain on beverages. "
			<< "Bob's bank account is " << bob_Person.bank_account;
	}
	else {
		std::cout << "Bob's bank account in " << bob_Person.retire_year << " is equal to "
			<< bob_Person.bank_account << " yuan" << std::endl;
	}
}


void lixia_income()
{
	if (current_month == 1) {
		lixia_Person.income *= 1.1; // Yearly salary's increasing
	}
	if (current_year == 2030 && current_month == 9) {
		lixia_Person.income *= 1.5; // Promotion for her company's loyalty
	}
	lixia_Person.bank_account += lixia_Person.income;
}

void bob_income()
{
	if (current_month == 1) {
		bob_Person.income *= 1.2; // Yearly salary's increasing
	}
	if (current_year == 2030 && current_month == 9) {
		bob_Person.income *= 1.5; // Promotion for her company's loyalty
	}
	bob_Person.bank_account += bob_Person.income;
}


void lixia_saving() {
	lixia_Person.saving_account += lixia_Person.saving_payment;
	lixia_Person.income -= lixia_Person.saving_payment;
}

void bob_saving() {
	bob_Person.saving_account += bob_Person.saving_payment;
	bob_Person.income -= bob_Person.saving_payment;
}


void lixia_house()
{
	if (lixia_Person.saving_account >= lixia_House.price && !lixia_House.flag_own) {
		lixia_House.flag_own = true;
		lixia_Person.saving_account -= lixia_House.price;
	}

	if (lixia_House.flag_own) {
		lixia_Person.bank_account -= lixia_House.stuff;
		lixia_Person.bank_account -= lixia_House.taxes;
	}
}

void bob_house()
{
	if (bob_Person.saving_account >= bob_House.price && !bob_House.flag_own) {
		bob_House.flag_own = true;
		bob_Person.saving_account -= bob_House.price;
	}

	if (bob_House.flag_own) {
		bob_Person.bank_account -= bob_House.stuff;
		bob_Person.bank_account -= bob_House.taxes;
	}
}


void lixia_car() 
{
	if (lixia_Person.saving_account >= lixia_Car.price && !lixia_Car.flag_own) {
		lixia_Car.flag_own = true;
		lixia_Person.saving_account -= lixia_Car.price;
	}

	if (lixia_Car.flag_own) {
		lixia_Person.bank_account -= lixia_Car.fines;
		lixia_Person.bank_account -= lixia_Car.petrol;
	}

	if (lixia_Car.flag_own && current_month == 6) {         // Т/О каждые полгода
		lixia_Person.bank_account -= lixia_Car.maintenance;
	}       
}

void bob_car()
{
	if (bob_Person.saving_account >= bob_Car.price && !bob_Car.flag_own) {
		bob_Car.flag_own = true;
		bob_Person.saving_account -= bob_Car.price;
	}

	if (bob_Car.flag_own) {
		bob_Person.bank_account -= bob_Car.fines;
		bob_Person.bank_account -= bob_Car.petrol;
	}

	if (bob_Car.flag_own && current_month == 12) {          // Т/О каждый год
		bob_Person.bank_account -= bob_Car.maintenance;
	}
}


void lixia_entertainment()
{
	if (current_month == 13) {                              // ежегодная инфляция на продукты  
		lixia_Entertainment.food += lixia_Entertainment.food * lixia_Entertainment.inflation;
		lixia_Entertainment.beverages += lixia_Entertainment.beverages * lixia_Entertainment.inflation;
		lixia_Entertainment.public_places += lixia_Entertainment.public_places * lixia_Entertainment.inflation;
	}

	lixia_Person.bank_account -= lixia_Entertainment.food;
	lixia_Person.bank_account -= lixia_Entertainment.beverages;
	lixia_Person.bank_account -= lixia_Entertainment.public_places;
}

void bob_entertainment()
{
	if (current_month == 13) {                              // ежегодная инфляция на продукты  
		bob_Entertainment.food += bob_Entertainment.food * bob_Entertainment.inflation;
		bob_Entertainment.beverages += bob_Entertainment.beverages * bob_Entertainment.inflation;
		bob_Entertainment.public_places += bob_Entertainment.public_places * bob_Entertainment.inflation;
	}
	bob_Person.bank_account -= bob_Entertainment.food;
	bob_Person.bank_account -= bob_Entertainment.beverages;
	bob_Person.bank_account -= bob_Entertainment.public_places;
}


void simulation()
{
	while (!(current_year == lixia_Person.retire_year && current_month == lixia_Person.retire_month)) {
		lixia_income();
		lixia_saving();
		lixia_house();
		lixia_car();
		lixia_entertainment();

		current_month++;
		if (current_month == 13) {
			current_year++;
			current_month = 1;
		}
	}

	while (!(current_year == bob_Person.retire_year && current_month == bob_Person.retire_month)) {
		bob_income();
		bob_saving();
		bob_house();
		bob_car();
		bob_entertainment();

		current_month++;
		if (current_month == 13) {
			current_year++;
			current_month = 1;
		}
	}
}


int main()
{
	lixia_init();
	bob_init();

	simulation();

	lixia_output();
	bob_output();
	return 0;
}
