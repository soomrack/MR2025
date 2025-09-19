#include "Person.h"

Person::Person(RUB income, RUB deposit, RUB credit)
	: m_income{ income }, m_deposit{ deposit }, m_credit{ credit } {
}


void Person::printInfo()
{
	std::cout << m_income << " " << m_credit << " " << m_deposit <<'\n';
}


void Person::setCreditRate(int rate)
{
	m_creditRate = rate;
}


void Person::increaseCredit()
{
	if (m_credit > 0) m_credit = m_credit * (100 + m_creditRate) / 100;
}


void Person::increaseDeposit(int percent)
{
	if (m_deposit > 0) m_deposit = m_deposit * (100 + percent) / 100;
}


void Person::increaseSalary(int inflation)
{
	m_income = m_income * (105 + inflation) / 100;
}


void Person::getAndSpendMoney(RUB regularSpend, RUB rent)
{
	if (m_credit > 0) {
		m_credit -= (m_income - regularSpend);

		if (m_credit < 0) {
			m_deposit = -m_credit;
			m_credit = 0;
		}
	}
	else {
		m_deposit += (m_income - regularSpend - rent);
	}
}
