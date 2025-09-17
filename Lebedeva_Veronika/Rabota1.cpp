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
using namespace std;
struct YEAR
{
    int month;
    int year;
};
class Deposit                 //вклад в сбере
{
private:
    YEAR countperiod;
public:
    double startcap = 2188890; //то что мы изначально имеем
    int years = 20;
    float percent = 15.5f;
    double sum = startcap;
    float max = (percent + 1.5) / 100;
    double nontax = 1000000 * max;
    void incrofmon()     //вклад + траты
    {
        countperiod.month = 0;
        countperiod.year = 0;
        while (countperiod.year <= years)
        {
            YEAR period;
            period.month = 3;
            if (countperiod.month % 3 == 0 && countperiod.month != 0)
            {
                double sumbef = sum;
                double quartalpercent = percent / 4.0 / 100.0;
                double afterpercent = sum * quartalpercent;
                sum += afterpercent;
                percent -= 0.1f;
                double sumaf = sum;
                double incom = sumaf - sumbef;
                if (incom > nontax)
                {
                    sum -= 0.13 * nontax;
                }
            }
            
            countperiod.month ++;
            if (countperiod.month >= 12)
                {
                countperiod.month = 0;
                countperiod.year++;
                }
        }
    }
    void expences()
    {
        double exp = 0;
        for (int monthex = 0; monthex < (years * 12); monthex ++)
        {
            int salary = 220000; //зарплата
            int foodmonth = 30000;     //в месяц траты иные
            int montapp = 50000;    //на квартиру в месяц
            exp += salary - foodmonth - montapp;
        }
        sum += exp;
    }
    void get_inf()
    {
        cout << "общий доход со вклада за " << years << " лет составил " << setw(10) << fixed << setprecision(2) << sum << " рублей" << endl;
    }
};
int main()
{
    SetConsoleCP(1251);
    SetConsoleOutputCP(1251);
    setlocale(LC_ALL, "Russian");
    Deposit Maria;
    Maria.incrofmon();
    Maria.expences();
    Maria.get_inf();
}