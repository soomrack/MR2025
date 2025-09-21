#include <iostream>
#include <iomanip>
#include <cmath> 
#include <CTYPE.H>
#include <conio.h> 
#include <cstdlib>
#include <ctime>
#include <io.h>
#include <fcntl.h>
#include <cstring> 
#include <string>
#include <Windows.h>


struct YEAR
{
    int year;
    int month;
};


class Deposit                 
{
private:
    YEAR countperiod;
    const YEAR data = {2025, 9};
    YEAR end;
public:
    int startcap = 5000000; 
    int years = 1;
    float percent = 15.5f;
    int sum = startcap;
    
    void incrofmon();
    void show_inf();
};

void expences(const YEAR& start, int Months, int& Sum)
{
    int exp = 0;
    YEAR current = start;
    static int salary = 65000;
    static int foodmonth = 5000;
    static int monthflat = 35000;
    for (int i = 0; i < Months; i++) {
        
        if (current.month == 10) {
            salary = static_cast<int>(salary * 1.07);
        }
        if (current.year % 2 == 0 && current.month == 9) {
            foodmonth = static_cast<int>(foodmonth * 1.13);
        }
        if (current.year % 3 == 0 && current.month == 9 && current.year != 2025) {
            monthflat = static_cast<int>(monthflat * 1.33);
        }
        exp += salary - foodmonth - monthflat;
        std::cout << current.year << " : " << current.month << " - exp = " << exp << std::endl;
        current.month++;
        if (current.month > 12) {
            current.month = 1;
            current.year++;
        }
    }
    Sum += exp;
    std::cout << current.year << " : " << current.month << " - Sum = " << Sum << std::endl;
}

void Deposit::incrofmon()     //increase of money
{
    end.month = data.month;
    end.year = data.year;
    while (!(end.year == (data.year + years) && end.month == data.month)){
        YEAR period;
        period.month = 3;
        if (end.month % 3 == 0 && (end.month != 0)){
            double quartalpercent = percent / 4.0 / 100.0;
            int afterpercent = sum * quartalpercent;
            sum += afterpercent;
            float cbrate = (percent + 1.5) / 100; //central bank rate
            int nontax = 1000000 * cbrate;
            if (afterpercent > nontax){
                sum = static_cast<int>(sum - 0.13 * (afterpercent - nontax));
            }
            percent -= 0.1f;
            std::cout << end.year << " : " << end.month << " - " << sum << std::endl;
            expences(end, 3, sum);
        }
        end.month++;
        if (end.month > 12){
            end.month = 1;
            end.year++;
        }
        
    }
   
}





void Deposit::show_inf()
{
    std::cout << "общий доход со вклада за " << years << " лет составил " << sum << " рублей" << std::endl;
}


int main()
{
    SetConsoleCP(1251);
    SetConsoleOutputCP(1251);
    setlocale(LC_ALL, "Russian");
    Deposit Maria;
    Maria.incrofmon();
  
    Maria.show_inf();
}
