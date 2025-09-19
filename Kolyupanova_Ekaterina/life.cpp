#include <stdio.h>
#include <stdexcept>

typedef int RUB;

struct Date {
    int year;
    int month;
    Date(int y, int m) {
        if (y < 0) {
            throw std::invalid_argument("Год не может быть отрицательным");
        }
        if (m > 12 || m < 1) {
            throw std::invalid_argument("Месяц не может быть больше 12 или отрицательным");
        }
        year = y;
        month = m;
    }
};

class Person
{
public:
    RUB salary;
    RUB capital;
    RUB rashod; 
    RUB payment;

    Person(RUB s, RUB c, RUB r, RUB p)
        : salary(s), capital(c), rashod(r), payment(p) {}
};

class Alice: public Person
{
public:
    Alice(RUB s, RUB c, RUB r, RUB p)
        : Person(s, c, r, p) {}

    void sell_picture() {
        capital += 50*1000;
    }
};
class Bob: public Person
{
public:
    Bob(RUB s, RUB c, RUB r, RUB p)
        : Person(s, c, r, p) {}
    
    void shtraf() {
    capital -= 10*1000;
    }
};


class House{

};

class Simulation
{
private:
    Date current_date;
public:
    Date start_date;
    Date end_date;
    Alice alice;
    Bob bob;
    House house;

    Simulation(Date start, Date end, Alice a, Bob b, House h)
        : start_date(start), end_date(end), alice(a), bob(b), house(h), current_date(start) {}

    void run() {
        current_date = start_date;
        while( !(current_date.year == end_date.year && current_date.month == end_date.month))
        {
            current_date.month ++;
            if (current_date.month == 13)
            {
                current_date.year ++;
                current_date.month = 1;
            }
            alice.capital = alice.capital + alice.salary - alice.rashod - alice.payment;
            bob.capital = bob.capital + bob.salary - bob.rashod - bob.payment;
            if (current_date.year == 2030 && current_date.month == 3)
            {
                alice.sell_picture();
                bob.shtraf();
            }
        

        
        }
    }
    void print_resulte() {
        printf("У Алисы в 2045 капитал: %d руб.\n", alice.capital);
        printf("У Боба в 2045 капитал: %d руб.\n", bob.capital);

    }
};

int main() {
    Date start_date = {2025, 9};
    Date end_date = {2045, 9};
    // Алиса взяла ипотеку: 9 млн руб; 20,5%; 20 лет; 1.5 млн перв.взнос => 130362 руб ежемесячный платеж
    Alice alice(200 * 1000, 0, 50*1000, 130362);
    Bob bob(200 * 1000, 0, 100*1000, 0);
    House house;

    Simulation simulation(
        start_date,
        end_date,
        alice,
        bob,
        house); 

    simulation.run();
    simulation.print_resulte();

    return 0;
}
