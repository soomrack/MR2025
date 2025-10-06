#include "Person.h"
#include <format>


Person::Person(RUB income)
	: income{ income } {
}


void Person::get_dep_percent(int inflation)
{
	deposit *= ((102.0 + inflation) / 100.0);
}


void Person::increase_salary(int inflation)
{
	income *= ((105.0 + inflation)/100.0);
}


void Person::set_food_spending(RUB prices)
{
	food = prices;
}


void Person::set_rent(RUB price)
{
	rent = price;
}


void Person::set_communal(RUB price)
{
	communal = price;
}


void Person::set_mortgage_pay(RUB pay)
{
	if (pay > income / 2) std::cout << pay <<" Слишком большой платеж\n";
	mortgage_pay = pay;
}


void Person::get_income(int month, int inflation)
{
	if (month == 8) increase_salary(inflation);
	bank_account += income;
}


void Person::put_money_on_deposit()
{
	deposit += bank_account;
	bank_account = 0;
}


void Person::spend_money(int month, int inflation)
{
	buy_food(month, inflation);
	pay_mortgage();
	pay_rent(month, inflation);
	pay_communal(month, inflation);
}


void Person::buy_food(int month, int inflation)
{
	if (month == 12) increase_food_spending(inflation);
	bank_account -= food;
}


void Person::pay_rent(int month, int inflation)
{
	if (month == 12) increase_rent(inflation);
	bank_account -= rent;
}


void Person::pay_mortgage()
{
	bank_account -= mortgage_pay;
}


void Person::pay_communal(int month, int inflation)
{
	if (month == 12) increase_communal(inflation);
	bank_account -= communal;
}


void Person::print_info()
{
	std::cout << std::format("deposit - {}\n", deposit);
}


void Person::buy_flat(RUB cost_of_flat)
{
	deposit -= cost_of_flat;
}


void Person::increase_food_spending(int inflation)
{
	food *= ((100.0 + inflation)/100.0);
}


void Person::increase_rent(int inflation)
{
	rent *= ((100.0 + inflation) / 100.0);
}


void Person::increase_communal(int inflation)
{
	communal *= ((100.0 + inflation) / 100.0);
}

