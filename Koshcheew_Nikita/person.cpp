#include "Person.h"
#include <format>

Person::Person(RUB income)
	: m_income{ income } {
}

void Person::get_dep_percent(int inflation)
{
	m_deposit *= ((100 + static_cast<double>(inflation) * 2) / 100);
}

void Person::increase_salary(int inflation)
{
	m_income *= ((105 + static_cast<double>(inflation))/100);
}

void Person::set_food_spending(RUB prices)
{
	m_food = prices;
}

void Person::set_rent(RUB rent)
{
	m_rent = rent;
}

void Person::set_mortgage_pay(RUB pay)
{
	m_mortgage_pay = pay;
}

void Person::income()
{
	m_deposit += m_income;
}

void Person::buy_food()
{
	m_deposit -= m_food;
}

void Person::pay_rent()
{
	m_deposit -= m_rent;
}

void Person::pay_mortgage()
{
	m_deposit -= m_mortgage_pay;
}

void Person::increase_prices(int inflation)
{
	increase_food_spending(inflation);
	increase_rent(inflation);
}

void Person::print_info()
{
	std::cout << std::format("deposit - {}\n", m_deposit);
}

void Person::buy_flat(RUB cost_of_flat)
{
	m_deposit -= cost_of_flat;
}

void Person::increase_food_spending(int inflation)
{
	m_food *= ((100 + static_cast<double>(inflation))/100);
}

void Person::increase_rent(int inflation)
{
	m_rent *= ((100 + static_cast<double>(inflation)) / 100);
}


