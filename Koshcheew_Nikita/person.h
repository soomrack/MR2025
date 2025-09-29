#pragma once
#include <iostream>
typedef long long RUB;

class Person {
public:
	explicit Person(RUB income);
	void get_dep_percent(int inflation);
	void increase_salary(int inflation);
	void set_food_spending(RUB prices);
	void set_mortgage_pay(RUB pay);
	void set_rent(RUB rent);
	void income();
	void buy_food();
	void pay_rent();
	void pay_mortgage();
	void increase_prices(int inflation);
	void buy_flat(RUB cost_of_flat);
	void print_info();
private:
	RUB m_deposit{0};
	RUB m_income;
	RUB m_food{0};
	RUB m_rent{0};
	RUB m_mortgage_pay{ 0 };

	void increase_food_spending(int inflation);
	void increase_rent(int inflation);
};

