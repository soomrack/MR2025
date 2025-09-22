#include <stdio.h>
#include <cmath>

typedef long long int RUB;

struct person {
    RUB salary;
    RUB balance;
    RUB cost;
    RUB payment;
};

person alice;
person bob;

RUB first_flat_cost;
RUB second_flat_cost;


void init(RUB salary, RUB balance) {
    alice.salary = salary;
    alice.balance = balance;
    bob.salary = salary;
    bob.balance = balance;
}


void print1() {
    printf("ALice took out a mortgage\n");
    printf("ALice has salary = %lld RUB\n", alice.salary);
    printf("ALice has balance = %lld RUB\n\n", alice.balance);

    printf("Bob rented an apartment\n");
    printf("Bob has salary = %lld RUB\n", bob.salary);
    printf("Bob has balance = %lld RUB\n\n", bob.balance);
}


void print2() {
    printf("\nALice has balance = %lld RUB\n", alice.balance);
    if (alice.balance < 0) {
        printf("ALice starves(\n");
    }
    else {
        printf("ALice`s happy)\n");
    }

    printf("\nBob has balance = %lld RUB\n", bob.balance);
    if (bob.balance < 20 * 1000 * 1000) {
        printf("Bob starves( \n", bob.balance);
    }
    else printf("Bob will buy an apartment)\n");
}


void inflation_do(const int year, const int month, double inflation) {
    if ((year - 2025) % 5 == 0 && month == 9 && year != 2025) {
        alice.salary += 10 * 1000;
        bob.salary += 13 * 1000;
        /*printf("salary %lld \n", alice.salary);*/
    }
    if (month == 12) {
        alice.salary = RUB (double (alice.salary) * (inflation - 0.1));
        bob.salary = RUB (double(bob.salary) * (inflation - 0.1));
     
        alice.cost *= inflation;
        bob.cost *= inflation;
    }
    if (month == 12) {
        
    }

}


void payment_mortgage(RUB amount_mortgage, double mortgage_percentage) {
    double monthly_rate = mortgage_percentage / 12;
    RUB remaining_mortgage = amount_mortgage - 100 * 1000;
    alice.payment = RUB( double(remaining_mortgage) * (monthly_rate * pow((1 + monthly_rate),20) ) );
    /*printf("payment %lld \n",payment);*/
}


void payment_rent_flat(RUB first_flat, RUB second_flat) {
    first_flat_cost = first_flat;
    second_flat_cost = second_flat;
}
;


void payment_rent(const int year) {
    if (year < 2035) bob.payment = first_flat_cost;
    else bob.payment = second_flat_cost;
}

void expenditure() {
    RUB food = 10 * 1000;
    RUB cosmetic = 1000;
    RUB cat = 2 * 1000;
    RUB communal_flat = 5 * 1000;
    RUB clothes = 3 * 1000;
    RUB relaxation = 5 * 1000;
    RUB self_care = 4 * 1000;
    RUB dog = 5 * 1000;
    RUB computer_game = 1000;
    RUB car = 4 * 1000;
    alice.cost = food + cosmetic + cat + communal_flat + clothes + relaxation + self_care;
    bob.cost = food + car + communal_flat + clothes / 2 + computer_game + relaxation/2 + dog;
}


void calculate(double inflation) {
    int month = 9;
    int year = 2025;

    expenditure();

    while (!(month == 9 && year == 2045)) {

        alice.balance += alice.salary - alice.payment - alice.cost;
        bob.balance += bob.salary - bob.payment - bob.cost;

        inflation_do(year, month, inflation);

        if (alice.balance < 0) printf("alice need a part-time job %lld, month %lld, year %lld\n",
                    alice.balance, month, year);

        month ++;
        if (month == 13) {
            year++;
            month = 1;
        }
    }
}


int main() {
    init(120*1000, 580*1000);
    print1();
    payment_mortgage(10 * 1000 * 1000, 0.13);
    payment_rent_flat(45 * 1000, 70 * 1000);
    calculate(1.3);
    print2();
}


// Запуск программы: CTRL+F5 или меню "Отладка" > "Запуск без отладки"
// Отладка программы: F5 или     меню "Отладка" > "Запустить отладку"

// Советы по началу работы 
//   1. В окне обозревателя решений можно добавлять файлы и управлять ими.
//   2. В окне Team Explorer можно подключиться к системе управления версиями.
//   3. В окне "Выходные данные" можно просматривать выходные данные сборки и другие сообщения.
//   4. В окне "Список ошибок" можно просматривать ошибки.
//   5. Последовательно выберите пункты меню "Проект" > "Добавить новый элемент", чтобы создать файлы кода, или "Проект" > "Добавить существующий элемент", чтобы добавить в проект существующие файлы кода.
//   6. Чтобы снова открыть этот проект позже, выберите пункты меню "Файл" > "Открыть" > "Проект" и выберите SLN-файл.
