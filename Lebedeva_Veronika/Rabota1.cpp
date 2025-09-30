#include <iostream>
#include <cctype>
#include <Windows.h>


typedef int RUB;


struct Date
{
    int year;
    int month;
};


class FinancialModelDeposit                 
{
private:
    Date countperiod;
    const Date data = {2025, 9};
    Date current;
    RUB salary = 220000;
    RUB monthflat = 50000;
    RUB foodmonth = 30000;
public:
    RUB startcap = 2188890;
    int years = 20;
    float percent = 15.5f;
    RUB bank_account = startcap;
    RUB money_on_card = 0;
    void Salary(Date Cur);
    void Food(Date Cur);
    void Flat(Date Cur);
    void increase_of_money();
    void show_results();
    void deposit();
};
 

void FinancialModelDeposit::Salary(Date Cur)
{
    if (Cur.month == 10) {
        salary = static_cast<int>(salary * 1.07);
    }
    money_on_card += salary;
}


void FinancialModelDeposit::Food(Date Cur)
{
    if (Cur.year % 2 == 0 && Cur.month == 9) {
        foodmonth = static_cast<int>(foodmonth * 1.13);
    }
    money_on_card -= foodmonth;
}


void FinancialModelDeposit::Flat(Date Cur)
{
    if (Cur.year % 3 == 0 && Cur.month == 9 && Cur.year != 2025) {
        monthflat = static_cast<int>(monthflat * 1.33);
    }
    money_on_card -= monthflat;
}


void FinancialModelDeposit::deposit()
{
    Date period;
    period.month = 3;
    if (current.month % period.month == 0 && (current.month != 0)) {
        double quartalpercent = percent / 4.0 / 100.0;
        RUB afterpercent = bank_account * quartalpercent;
        bank_account += afterpercent;
        float cbrate = (percent + 1.5) / 100; //central bank rate
        RUB nontax = 1000000 * cbrate;
        if (afterpercent > nontax){
            bank_account = static_cast<int>(bank_account - 0.13 * (afterpercent - nontax));
        }
        percent -= 0.1f;
        std::cout << current.year << " : " << current.month << " - " << bank_account << std::endl;
    }
}


void FinancialModelDeposit::increase_of_money()    
{
    current.month = data.month;
    current.year = data.year;
    while (!(current.year == (data.year + years) && current.month == data.month)) {
        deposit();
        Salary(current);
        Food(current);
        Flat(current);
        bank_account += money_on_card;
        money_on_card = 0;
        current.month++;
        if (current.month > 12) {
            current.month = 1;
            current.year++;
        } 
    }
}


void FinancialModelDeposit::show_results()
{
    std::cout << "общий доход со вклада за " << years << " лет составил " << bank_account << " рублей" << std::endl;
}


int main()
{
    SetConsoleCP(1251);
    SetConsoleOutputCP(1251);
    setlocale(LC_ALL, "Russian");
    FinancialModelDeposit Maria;
    Maria.increase_of_money();
    Maria.show_results();
}
