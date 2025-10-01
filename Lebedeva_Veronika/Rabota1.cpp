#include <iostream>
#include <cctype>
#include <Windows.h>
#include <cmath>

typedef int RUB;

struct Date
{
    int year;
    int month;
};


struct Deposit
{
    RUB monthflat_rent_start = 50000;
    float year_percent_deposit = 15.5f;
    RUB monthflat_rent = monthflat_rent_start;
    bool calculating_deposit = false;
};


struct Mortgage
{
    RUB monthly_payment = 0;
    RUB total_expenses_per_month = 0;
    RUB overpayment = 0;
    float year_percent_mortgage = 23.0f;
    bool calculating_mortgage = false;
};


class FinancialModel                 
{
private:
    Deposit deposit;
    Mortgage mortgage;
    Date countperiod;
    const Date data = {2025, 9};
    Date current;
    RUB salary = 220000;
    RUB foodmonth_start = 30000;
    RUB flatprice = 10890000;
    RUB flat_price_after = 0;
public:
    RUB startcap = 2188890;
    int years = 20;
    RUB bank_account = startcap;
    RUB foodmonth = foodmonth_start;
    RUB money_on_card = 0;

    void salary_inflation(Date Cur);
    void food_inflation(Date Cur);
    void rent_flat_inflation(Date Cur);
    void crediting_to_bank_account();
    void increase_of_money_with_deposit();
    void show_results();
    void deposit_simulation();
    void set_mortgage_monthly_payment();
    void count_month_expences();
    void count_total_expences();
    void mortgage_simulation();
    void count_overpayment();
    void inflation_flat_price();
};
 

void FinancialModel::salary_inflation(Date Cur)
{
    if (Cur.month == 10) {
        salary = static_cast<int>(salary * 1.07);
    }
    money_on_card += salary;
}


void FinancialModel::food_inflation(Date Cur)
{
    if (Cur.year % 2 == 0 && Cur.month == 9) {
        foodmonth = static_cast<int>(foodmonth * 1.13);
    }
    money_on_card -= foodmonth;
}


void FinancialModel::rent_flat_inflation(Date Cur)
{
    if (Cur.year % 3 == 0 && Cur.month == 9 && Cur.year != 2025) {
        deposit.monthflat_rent = static_cast<int>(deposit.monthflat_rent * 1.33);
    }
    money_on_card -= deposit.monthflat_rent;
}


void FinancialModel::deposit_simulation()
{
    int period_deposit = 3;
    if (current.month % period_deposit == 0 && (current.month != 0)) {
        double quartal_percent = deposit.year_percent_deposit / 4.0 / 100.0;
        RUB after_percent = bank_account * quartal_percent;
        bank_account += after_percent;
        float cbrate = (deposit.year_percent_deposit + 1.5) / 100; //central bank rate
        RUB nontax = 1000000 * cbrate;
        if (after_percent > nontax){
            bank_account = static_cast<int>(bank_account - 0.13 * (after_percent - nontax));
        }
        deposit.year_percent_deposit -= 0.1f;
    }
}


void FinancialModel::crediting_to_bank_account()
{
    bank_account += money_on_card;
    money_on_card = 0;
}


void FinancialModel::increase_of_money_with_deposit()    
{
    current.month = data.month;
    current.year = data.year;
    while (!(current.year == (data.year + years) && current.month == data.month)) {
        deposit_simulation();
        salary_inflation(current);
        food_inflation(current);
        rent_flat_inflation(current);
        crediting_to_bank_account();

        current.month++;
        if (current.month > 12) {
            current.month = 1;
            current.year++;
        } 
    }
    deposit.calculating_deposit = true;
}


void FinancialModel::set_mortgage_monthly_payment()
{
    RUB down_payment = startcap;
    RUB sum_of_credit = flatprice - down_payment;
    float month_percent = mortgage.year_percent_mortgage / 12 / 100;
    int period_mortgage = years * 12;
    mortgage.monthly_payment = static_cast<RUB>(sum_of_credit * 
        (month_percent * pow((1 + month_percent), period_mortgage)) / (pow((1 + month_percent), (period_mortgage)) - 1));
    mortgage.calculating_mortgage = true;
}


void FinancialModel::count_month_expences()
{
    mortgage.total_expenses_per_month += foodmonth + mortgage.monthly_payment;
}


void FinancialModel::count_total_expences()
{
    foodmonth = foodmonth_start;
    current.month = data.month;
    current.year = data.year;
    while (!(current.year == (data.year + years) && current.month == data.month)) {
        food_inflation(current);
        count_month_expences();

        current.month++;
        if (current.month > 12) {
            current.month = 1;
            current.year++;
        }
    }      
}


void FinancialModel::inflation_flat_price()
{
    RUB inflation_flat = flatprice;
    current.month = data.month;
    current.year = data.year;
    while (!(current.year == (data.year + years) && current.month == data.month)) {
        if (current.year % 2 == 0 && current.month == 9) {
            inflation_flat = static_cast<int>(flat_price_after * 0.06);
        }
        flat_price_after += inflation_flat;
        inflation_flat = 0;
        current.month++;
        if (current.month > 12) {
            current.month = 1;
            current.year++;
        }
    }
}


void FinancialModel::count_overpayment()
{
    mortgage.overpayment = mortgage.total_expenses_per_month - flat_price_after;
}


void FinancialModel::mortgage_simulation()
{
    set_mortgage_monthly_payment();
    count_total_expences();
    inflation_flat_price();
    count_overpayment();
    mortgage.calculating_mortgage = true;
}


void FinancialModel::show_results()
{
    if (deposit.calculating_deposit && !mortgage.calculating_mortgage) {
        std::cout << "ВКЛАД" << std::endl;
        std::cout << "общий доход со вклада за " << years << " лет составил " << bank_account << " рублей" << std::endl;
    }
    if (mortgage.calculating_mortgage && !deposit.calculating_deposit) {
        std::cout << "ИПОТЕКА" << std::endl;
        std::cout << "месячный платеж при ипотеке = " << mortgage.monthly_payment << " рублей" << std::endl;
        std::cout << "суммарные траты при ипотеке " << mortgage.total_expenses_per_month << " рублей" << std::endl;
        std::cout << "переплата составила (с учетом изменения цены на квартиру) " 
            << mortgage.overpayment << " рублей" << std::endl;
        std::cout << "стоимость квартиры спустя 20 лет с учетом инфляции " << flat_price_after << " рублей" << std::endl;
    }
}


int main()
{
    SetConsoleCP(1251);
    SetConsoleOutputCP(1251);
    setlocale(LC_ALL, "Russian");

    FinancialModel Maria;
    FinancialModel Fedor;

    Maria.increase_of_money_with_deposit();
    Fedor.mortgage_simulation();
    
    Maria.show_results();
    Fedor.show_results();
}
