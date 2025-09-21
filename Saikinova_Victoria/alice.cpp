#include <stdio.h>
#include <cmath>

typedef long long int RUB;

struct person {
    RUB salary;
    RUB balance;
};

person alice;


void alice_init(RUB salary, RUB balance) {
    alice.salary = salary;
    alice.balance = balance;
}


void alice_print1() {
    printf("ALice took out a mortgage\n");
    printf("ALice has salary = %lld RUB\n", alice.salary);
    printf("ALice has balance = %lld RUB\n\n", alice.balance);
}


void alice_print2() {
    printf("ALice has balance = %lld RUB\n", alice.balance);
    if (alice.balance < 0) {
        printf("ALice starves(\n");
    }
    else {
        printf("ALice`s happy)\n");
    }
}


void alice_promotion(const int year, const int month, double inflation) {
    if ((year - 2025) % 5 == 0 && month == 9 && year != 2025) {
        alice.salary += 10 * 1000;
        printf("salary %lld \n", alice.salary);
    }
    if (month == 12) {
        alice.salary = RUB (double (alice.salary) * (inflation - 0.1));
        printf("sal inf %lld \n", alice.salary);
    }
}


RUB payment_mortgage(RUB amount_mortgage, double mortgage_percentage) {
    double monthly_rate = mortgage_percentage / 12;
    RUB remaining_mortgage = amount_mortgage - 100 * 1000;
    RUB payment = RUB( double(remaining_mortgage) * (monthly_rate * pow((1 + monthly_rate),20) ) );

    printf("payment %lld \n",payment);
    return (payment);
}


RUB expenditure() {
    RUB food = 10 * 1000;
    RUB cosmetic = 1000;
    RUB cat = 2 * 1000;
    RUB communal_flat = 5 * 1000;
    RUB clothes = 3 * 1000;
    RUB relaxation = 5 * 1000;
    RUB self_care = 4 * 1000;
    RUB cost = food + cosmetic + cat + communal_flat + clothes + relaxation + self_care;
    return(cost);
    //printf("balance %lld \n", alice.balance);
}


void calculat(double inflation, RUB amount_mortgage, double mortgage_percentage) {
    int month = 9;
    int year = 2025;

    RUB payment = payment_mortgage(amount_mortgage, mortgage_percentage);
    RUB cost = expenditure();

    while (!(month == 9 && year == 2045)) {

        alice.balance += alice.salary - payment - cost;
        if (month == 12) cost *= inflation;

        alice_promotion(year, month, inflation);

        if (alice.balance < 0) printf("Change jobs %lld \n",alice.balance);

        month ++;
        if (month == 13) {
            year++;
            month = 1;
        }
    }
}


int main() {
    alice_init(120*1000, 500*1000);
    alice_print1();
    calculat(1.3,10*1000*1000,0.13);
    alice_print2();
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
