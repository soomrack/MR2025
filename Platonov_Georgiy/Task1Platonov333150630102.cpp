#include <iostream> 
#include <string>
#include <cmath>
#include <iomanip>

typedef long int ruble;  // RUB

const int mortgage_interest = 12/100; //    месячный
const int deposit_interest = 1/100; //  месячный

class Worker {
private:
    std::string name;

    class BankData
    {
    public:
        BankData(ruble bd_house_cost, ruble bd_mortgage, ruble bd_deposit, ruble bd_rent, ruble bd_communal, ruble bd_mortgage_payment)
        {
            house_cost = bd_house_cost;
            mortgage = bd_mortgage;
            deposit = bd_deposit;
            rent = bd_rent;
            communal = bd_communal;
            mortgage_payment = bd_mortgage_payment;
        }
        ruble house_cost;
        ruble mortgage;             // void alice_food(inflation, month)
        ruble deposit;
        ruble rent;
        ruble communal;
        ruble mortgage_payment;
    };

    BankData bank_data{0,0,0,0,0,0};

public:
    Worker(const std::string w_name, ruble w_balance, ruble w_salary, ruble w_spending, 
        ruble w_emergency_spending, ruble w_house_cost = 0, ruble w_mortgage = 0, ruble w_deposit = 0,
        ruble w_rent = 0, ruble w_communal = 0, ruble w_mortgage_payment = 0)
    {
        balance = w_balance;
        salary = w_salary; 
        spending = w_spending;
        emergency_spending = w_emergency_spending;
        bank_data = BankData(w_house_cost, w_mortgage, w_deposit, w_rent, w_communal, w_mortgage_payment);
    }

    ruble balance;
    ruble salary;
    ruble spending;
    ruble emergency_spending;



    void calculate(const int years, const int yearly_inflation)
    {  
        ruble profit;
        ruble deficit;
        for(int yrs = 1; yrs <= years; yrs++)
        {
            for(int months = 1; months <= 12; months++)
            {
                profit = balance + salary * (1+ yearly_inflation * (yrs-1) / 100);
                deficit = (1+ yearly_inflation * (yrs-1) / 100) * // можно заменить на коэффициент 1+%_инфляции
                  (spending + emergency_spending + bank_data.communal + bank_data.rent) + bank_data.mortgage_payment;
                balance = profit - deficit;
                bank_data.deposit = bank_data.deposit * deposit_interest;
                std::cout << "Month: " << months << " Year: " << yrs << " Current Balance: " << balance << "\n";
            };
        }
    };

};

//  чюпеп

int main(){
Worker artem{"Artem",228000,228000,150000,40000,0,0,100000,30000,0,0};
artem.calculate(12,4);
}