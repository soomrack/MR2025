#pragma once
#include <iostream>
typedef long long RUB;

class Person {
public:
	Person(RUB income, RUB deposit, RUB credit = 0);
	void printInfo();
	void setCreditRate(int rate);
	void increaseCredit();
	void increaseDeposit(int percent);
	void increaseSalary(int inflation);
	void getAndSpendMoney(RUB regularSpend, RUB rent = 0);

private:
	RUB m_deposit;
	RUB m_credit;
	RUB m_income;
	int m_creditRate;
};

