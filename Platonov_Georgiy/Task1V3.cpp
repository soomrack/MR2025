#include <iostream>

typedef long int RUB;
int year = 2025;
int month = 4;
int inflation = 12;
const float deposit_interest = 0.02;

struct Person {
 RUB balance;
 RUB income;
 RUB food;
 RUB car;
 RUB clothes;
 RUB emergency;
 RUB rent;
 RUB communal;
 RUB mortgage;
 RUB deposit;
 RUB house_price;
};

Person artyom, boris;

void artyom_init() 
{
 artyom.balance = 2000000;
 artyom.income = 180000;
 artyom.food = 35000;
 artyom.car = 20000;
 artyom.clothes = 8000;
 artyom.emergency = 20000;
 artyom.communal = 5000;
 artyom.mortgage = 40000;
 artyom.house_price = 2000000;
};

// Artyom be like
void artyom_get_paid(const int year, const int month, int inflation) 
{
    if (month == 8) {
        artyom.income *= 1+inflation/100;
    };
    artyom.balance += artyom.income;  
};

void artyom_spend_food(const int month, int inflation) 
{
    if (month == 10) {
        artyom.food *= 1+inflation/100; 
    };
    artyom.balance -= artyom.food;  
}; 

void artyom_spend_car(const int month, int inflation) 
{
    if (month == 10) {
        artyom.car *= 1+inflation/100; 
    };
    artyom.balance -= artyom.car;  
};

void artyom_spend_clothes(const int month, int inflation) 
{
    if (month == 10) {
        artyom.clothes *= 1+inflation/100; 
    };
    artyom.balance -= artyom.clothes;  
};

void artyom_spend_emergency(const int month, int inflation) 
{
    if (month == 10) {
        artyom.emergency *= 1+inflation/100; 
    };
    artyom.balance -= artyom.emergency;  
};

void artyom_spend_communal(const int month, int inflation) 
{
    if (month == 10) {
        artyom.communal *= 1+inflation/100; 
    };
    artyom.balance -= artyom.communal;  
};
void artyom_spend_mortgage(const int month, int inflation) 
{
    artyom.balance -= artyom.mortgage;  
};

void artyom_events(const int year, int month) 
{
    if (month == 4 and year == 2025) {
        artyom.balance -= artyom.house_price; // Купил дом и живёт
    };
    if (month == 8 and year == 2036) {
        artyom.income *= 0; // Сжег офис и был уволен 
    };
    if (month == 9 and year == 2036) {
        artyom.income *= 102000; // Стал дроппером 
    };
    if (month == 2 and year == 2037) {
        artyom.income *= 0; // Попал в тюрягу на 3 года
        artyom.balance -= 200000;   // Платежи по ипотеке делает семья на его деньги
    };
    if (month == 2 and year == 2040)  {
        artyom.income *= 200000; // Вышел из тюряги и снова пошел работать программистом
    };
    };

void artyom_life(int month, int year, int inflation) 
{
    while(!(year == 2045 && month == 1)){
        
        artyom_get_paid(year, month, inflation);
        artyom_spend_car(month, inflation);
        artyom_spend_clothes(month, inflation);
        artyom_spend_communal(month, inflation);
        artyom_spend_emergency(month, inflation);
        artyom_spend_food(month, inflation);
        artyom_spend_mortgage(month, inflation);
        artyom_events(year, month);

        month++;
        if (month == 13){
            month = 1;
            year++;
        };
    }
};

void time_reset()
{
int year = 2025;
int month = 4;
};

// Boris init
void boris_init() 
{
 boris.balance = 2000000;
 boris.income = 180000;
 boris.food = 35000;
 boris.car = 20000;
 boris.clothes = 8000;
 boris.emergency = 20000;
 boris.rent = 25000;
 boris.deposit = 40000;
 boris.house_price = 2000000;
};

void boris_get_paid(const int year, const int month, int inflation) {
    if (month == 8) {
        boris.income *= 1+inflation/100;
    };
    boris.balance += boris.income;  
};

void boris_spend_food(const int month, int inflation) {
    if (month == 10) {
        boris.food *= 1+inflation/100; 
    };
    boris.balance -= boris.food;  
}; 

void boris_spend_car(const int month, int inflation) {
    if (month == 10) {
        boris.car *= 1+inflation/100; 
    };
    boris.balance -= boris.car;  
};

void boris_spend_clothes(const int month, int inflation) {
    if (month == 10) {
        boris.clothes *= 1+inflation/100; 
    };
    boris.balance -= boris.clothes;  
};

void boris_spend_emergency(const int month, int inflation) 
{
    if (month == 10) {
        boris.emergency *= 1+inflation/100; 
    };
    boris.balance -= boris.emergency;  
};

void boris_spend_rent(const int month, int inflation) 
{
    if (month == 10) {
        boris.rent *= 1+inflation/100; 
    };
    boris.balance -= boris.rent;  
};

void boris_house_price_inflate(const int month, int inflation)
{
    if (month == 10) {
        boris.house_price *= 1+inflation/100; 
    };
};

void boris_get_deposit(const int month, int inflation)
{
    boris.deposit *= 1+deposit_interest;  
};

void boris_deposit_bailout(const int month, int inflation)
{
    if (boris.balance < 20000 and boris.deposit >= 20000) {
        boris.balance += 20000;
        boris.deposit -= 20000;
    }
};

void boris_events(int year, int month)
{
    if (month == 8 and year == 2036)   {
        boris.income *= 0; // Сжег офис и был уволен 
    };
    if (month == 9 and year == 2036)  {
        boris.income *= 102000; // Стал дроппером 
    };
    if (month == 2 and year == 2037)  {
        boris.income *= 0; // Попал в тюрягу на 3 года
        boris.balance -= 200000;   // Платежи по ипотеке делает семья на его деньги
    };
    if (month == 2 and year == 2040) {
        boris.income *= 200000; // Вышел из тюряги и снова пошел работать программистом
    };
     if (month == 2 and year == 2044) {
       // boris.balance += 2000000; // Получил наследство от нигерийского прадеда
    };
     if (month == 12 and year == 2044) {
        boris.balance -= boris.house_price; // Купил дом и живёт
    };
    };

void boris_life(int month, int year, int inflation) 
{
    while(!(year == 2045 && month == 1)){

        boris_get_paid(year, month, inflation);
        boris_spend_car(month, inflation);
        boris_spend_clothes(month, inflation);
        boris_spend_rent(month, inflation);
        boris_spend_emergency(month, inflation);
        boris_spend_food(month, inflation);
        boris_get_deposit(month, inflation);
        boris_deposit_bailout(month, inflation);
        boris_house_price_inflate(month,inflation);
        boris_events(year, month);

        month++;
        if (month == 13){
            month = 1;
            year++;
        };
    }
};

void result_cout()
{
    std::cout << "Artyom balance: " << artyom.balance << "\n"; 
    std::cout << "Boris balance: " << boris.balance << "\n";
    std::cout << "Boris Deposit " << boris.deposit << "\n";
};


int main()
{
artyom_init();
boris_init();
artyom_life(month,year,inflation);
time_reset();
boris_life(month,year,inflation);
result_cout();
return 1;
};