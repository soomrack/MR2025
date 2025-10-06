#pragma once
#include <iostream>
typedef long long RUB;

class Person {
private:
	RUB deposit{ 0 };
	RUB income;
	RUB food{ 0 };
	RUB rent{ 0 };
	RUB mortgage_pay{ 0 };
	RUB bank_account{ 0 };
	RUB communal{ 0 };
public:
	explicit Person(RUB income);

	void set_food_spending(RUB prices);
	void set_mortgage_pay(RUB pay);
	void set_rent(RUB rent);
	void set_communal(RUB price);

	void get_dep_percent(int inflation);
	void get_income(int mont, int inflation);
	void put_money_on_deposit();

	void spend_money(int month, int inflation);

	void buy_flat(RUB cost_of_flat);
	void print_info();

private:
	void increase_salary(int inflation);
	void increase_food_spending(int inflation);
	void increase_rent(int inflation);
	void increase_communal(int inflation);

	void buy_food(int month, int inflation);
	void pay_rent(int month, int inflation);
	void pay_mortgage();
	void pay_communal(int month, int inflation);
};

