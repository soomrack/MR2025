#include <stdio.h>
#include <stdexcept>
#include <iostream>

typedef int RUB;

class Date {
public:
    int year;
    int month;

public:
    Date(int year, int month);
};


Date::Date(int year, int month): year(year), month(month) {
            if (year < 0) {
                throw std::invalid_argument("Год не может быть отрицательным"); //лучше не писать, Или ловушка для исключений
            }
            if (month > 12 || month < 1) {
                throw std::invalid_argument("Месяц не может быть больше 12 или отрицательным");
            }
        }

class Person
{
public:
    RUB bank_account;
    RUB rashod;

    Person(RUB c, RUB r)
        : bank_account(c), rashod(r) {}
};

class Alice: public Person
{
public:
    Alice(RUB c, RUB r)
        : Person(c, r) {}

    void sell_picture(const int year, const int month) {
        if (year == 2030 && month == 3)
            {
                bank_account += 50*1000;
            }
    }
    void salary(){
        bank_account += 200*1000; // зарплата
    }
    void payment() {
        bank_account -= 130362; // платеж за ипотеку
    }
    void food(){
        bank_account -= 40*1000; 
    }
    void home(){
        bank_account -= 5*1000; // комунальные+связь
    }

    void income(Date current_date) {
        salary();
        sell_picture(current_date.year, current_date.month);
    }

    void spend(Date current_date) {
        payment();
        food();
        home();
    }
};
class Bob: public Person
{
public:
    Bob(RUB c, RUB r)
        : Person(c, r) {}
    
    void shtraf(const int year, const int month) {
         if (year == 2030 && month == 3)
            {
                bank_account -= 10*1000;
            }
    }
    void salary(){
        bank_account += 200*1000;
    }
    void home(){
        bank_account -= 45*1000 + 5*000;
    }
    void food(){
        bank_account -= 40*1000;
    }

    void income(Date current_date){
        salary();
    }
    void spend(Date current_date){
        shtraf(current_date.year, current_date.month);
        food();
        home();
    }
};


class Simulation
{
public:
    Date current_date;
    Date start_date;
    Date end_date;
    Alice alice;
    Bob bob;

    Simulation(Date start, Date end, Alice a, Bob b)
        : start_date(start), end_date(end), alice(a), bob(b), current_date(start) {}

    void run() {
        current_date = start_date;
        while( !(current_date.year == end_date.year 
                && current_date.month == end_date.month)){
            current_date.month++;
            alice.spend(current_date);
            bob.spend(current_date);
            alice.income(current_date);
            bob.income(current_date);
            

            if (current_date.month == 13){
                current_date.year ++;
                current_date.month = 1;
            }
        }
    }

    void print_resulte() {
        printf("У Алисы в 2045 счет в банке: %d руб.\n", alice.bank_account);
        printf("У Боба в 2045 счет в банке: %d руб.\n", bob.bank_account);

    }
};

int main() {
    try{
        Date start_date = {2025, 9};
        Date end_date = {2045, 9};
        Alice alice(0, 0);
        Bob bob(0, 0);

        Simulation simulation(
            start_date,
            end_date,
            alice,
            bob); 

        simulation.run();
        simulation.print_resulte();
    }
    
    catch (const std::invalid_argument& e) {
        printf("Ошибка аргумента %s \n", e.what());
    }
    catch (const std::exception& e) {
        printf("Общая ошибка %s \n", e.what());
    }
}
