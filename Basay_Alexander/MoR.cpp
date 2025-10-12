#include <stdio.h>
#include<locale.h>

typedef long long int RUB;


struct Car
{
	RUB petrol;
	RUB tax;
};


struct Home
{
	RUB tax;
	RUB moving;
	RUB deduction;
	RUB contribution;
	RUB price;
};


struct Person 
{
	struct Car car;
	struct Home home;
	RUB bank_account;
	RUB salary;
	RUB investmens;
	RUB monthly_payment;
	RUB food;
	RUB cloth;
	// RUB internet;
	// RUB public_servise;
};


Person JOHN;
Person ANNA;


/* void JOHN_income()
{
	JOHN.bank_account += JOHN.salary;
} */


/* void ANNA_income()
{
	ANNA.bank_account += ANNA.salary;
} */


void JOHN_salary (const int year, const int month) 
{
	if (month == 3) {
		JOHN.salary = JOHN.salary * 1.03; // Indexation
	}

	if (year == 2035 && month == 1) {
		JOHN.salary *= 1.5; // Promotion
	}

	JOHN.bank_account += JOHN.salary;
}


void ANNA_salary(const int year, const int month)
{
	if (month == 3) {
		ANNA.salary = ANNA.salary * 1.03; // Indexation
	}

	if (year == 2035 && month == 1) {
		ANNA.salary *= 1.5; // Promotion
	}

	ANNA.bank_account += ANNA.salary;
}


void JOHN_base_expenses () 
{
	JOHN.bank_account -= JOHN.cloth;
	JOHN.bank_account -= JOHN.food;
/*	JOHN.bank_account -= JOHN.internet;
	JOHN.bank_account -= JOHN.public_servise; */
}


void ANNA_base_expenses()
{
	ANNA.bank_account -= ANNA.cloth;
	ANNA.bank_account -= ANNA.food;
/*	ANNA.bank_account -= ANNA.internet;
	ANNA.bank_account -= ANNA.public_servise; */
}


void JOHN_car (const int month)
{
	if (month == 12) {
		JOHN.bank_account -= JOHN.car.tax;
	}

	JOHN.bank_account -= JOHN.car.petrol;
}


void ANNA_car(const int month)
{
	if (month == 12) {
		ANNA.bank_account -= ANNA.car.tax;
	}

	ANNA.bank_account -= ANNA.car.petrol;
}


void JOHN_investments (const int month)
{
	if (month == 1) {
		JOHN.investmens *= 1.15; // Percentages
	}

	JOHN.investmens += JOHN.salary*0.1;
	JOHN.bank_account -= JOHN.salary * 0.1; // Monthly deductions
}


void ANNA_investments (const int month)
{
	if (month == 1) {
		ANNA.investmens *= 1.15; // Percentages
	}

	ANNA.investmens += ANNA.salary * 0.2;
	ANNA.bank_account -= ANNA.salary * 0.2; // Monthly deductions
}


void JOHN_house_tax (const int month)
{
	if (month == 12) {
		JOHN.bank_account -= JOHN.home.tax;
	}
}


void ANNA_moving (const int year, const int month)
{
	if ((year == 2030 || year == 2040 || year == 2045) && month == 5) {
		ANNA.bank_account -= ANNA.home.moving;
	}
}


void JOHN_repair (const int month) 
{
	if (month == 7) {
		JOHN.bank_account -= JOHN.home.price * 0,005; // 0,5% от стоимости жилья каждый год
	}
}


void JOHN_deducation() 
{
		JOHN.bank_account += JOHN.home.deduction;
}


void JOHN_contridution () 
{
	JOHN.bank_account -= JOHN.home.contribution;
}


void JOHN_house_price (const int month) 
{
	if (month == 12) {
		JOHN.home.price *= 1.08;
	}
}


void JOHN_monthly_payment(const int year, const int month)
{
	JOHN.bank_account -= JOHN.monthly_payment;
}


void ANNA_monthly_payment(const int year, const int month)
{
	if ((year == 2030 || year == 2040 || year == 2045) && month == 5) {
		ANNA.monthly_payment *= 1.5;
	}

	ANNA.bank_account -= ANNA.monthly_payment;
}


void simulation()
{
	int year = 2025;
	int month = 9;

	JOHN_contridution();

	while (!(year == 2050 && month == 9)) {
		JOHN_salary(year, month);
		ANNA_salary(year, month);

		JOHN_base_expenses();
		ANNA_base_expenses();

		JOHN_car(month);
		ANNA_car(month);

		JOHN_investments(month);
		ANNA_investments(month);

		JOHN_house_tax(month);

		ANNA_moving(year, month);

		JOHN_repair(month);

		JOHN_house_price(month);

		JOHN_monthly_payment(year, month);
		ANNA_monthly_payment(year, month);

/*		JOHN_income();
		ANNA_income();  */

		++month;
		if (month == 13) {
			month = 1;
			++year;
		}
	}

	JOHN_deducation();
}


void print_JOHN_info()
{
	JOHN.bank_account += JOHN.investmens;
	printf("John capital = %d RUB\n", JOHN.bank_account);
}


void print_ANNA_info()
{
	ANNA.bank_account += ANNA.investmens;
	printf("Anna capital = %d RUB\n", ANNA.bank_account);
}


void JOHN_init()
{
	JOHN.bank_account = 5000 * 1000;
	JOHN.salary = 400 * 1000;
	JOHN.food = 30000;
	JOHN.cloth = 10000;
	JOHN.car.petrol = 10000;
	JOHN.car.tax = 15000;
	JOHN.home.tax = 30000;
	// JOHN.public_servise = 20000;
	// JOHN.internet = 5000;
	JOHN.investmens = 200 * 1000;
	JOHN.home.deduction = 650*1000;
	JOHN.home.contribution = 5000*1000;
	JOHN.monthly_payment = 280274;
	JOHN.home.price = 15*1000*1000;
}


void ANNA_init()
{
	ANNA.bank_account = 1000 * 1000;
	ANNA.salary = 400 * 1000;
	ANNA.food = 30000;
	ANNA.cloth = 15000;
	ANNA.car.petrol = 20000;
	ANNA.car.tax = 12000;
	// ANNA.public_servise = 25000;
	// ANNA.internet = 5000;
	ANNA.investmens = 200 * 1000;
	ANNA.home.moving = 50000;
	ANNA.monthly_payment = 180*1000;
}


int main()
{
	JOHN_init();

	ANNA_init();

	simulation();

	print_JOHN_info();

	print_ANNA_info();
}