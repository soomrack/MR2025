#include <iostream>

struct person {
    double bank_account;
    double zp;
    double food;
    double rent;
    double mortgage;
    double comm;
    double randomPurchases;


    void income(const int year, const int month) {
        if (month == 10) {
            zp = zp * 1.08;     //индексация зп
        }
        if ((year == 2034) && (month == 5)) {
            zp = zp * 1.5;      //повышение
        }

        bank_account += zp;

    }

    void spendings(const int month) {      //траты
        if (month == 1) {                  //инфляция
            food = food * 1.10;
            comm = comm * 1.08;
            rent = rent * 1.10;
            randomPurchases = randomPurchases * 1.08;
        }
        bank_account -= food;
        bank_account -= rent;
        bank_account -= comm;
        bank_account -= randomPurchases;

    }

    void info() {       //показать состояние банк. аккаунта
        std::cout << bank_account << '\n';
    }
};


person Alice;           //обозначаем людей
person Bob;

void Bob_savings(double homePrice, bool ownsHome, int year, int month) {        //симуляция накоплений боба
    if ((Bob.bank_account >= homePrice + 100000) && (ownsHome == false)) {
        Bob.bank_account -= homePrice;
        ownsHome = true;
        std::cout << "Bob molodets! " << year << ' ' << month << ' ' << '\n';
    }
}

void Alice_mortgage(int homePrice) {
    int rate = 20;      //ставка
    int term = 20;      //кол-во лет
    double mortgage = (homePrice * pow(1 + (rate / 100), term)) / (term * 12);
    Alice.bank_account -= mortgage;
}


void simulation() {     //основная симуляцияя
    int year = 2025, month = 9;
    int homePrice = 15 * 1000 * 1000;
    bool ownsHome = false;

    while (!(year == 2045 && month == 9)) {

        Bob.income(year, month);
        Bob.spendings(month);
        Bob_savings(homePrice, ownsHome, year, month);
        Alice_mortgage(homePrice);
        Alice.income(year, month);
        Alice.spendings(month);

        month++;
        if (month == 13) {
            month = 1;
            homePrice = homePrice * 1.05;
            year++;
        }
    }
}


void Alice_init() {     //вводим начальные условия
    Alice.bank_account = 100000;
    Alice.zp = 100000;
    Alice.food = 30000;
    Alice.comm = 6000;
    Alice.randomPurchases = 30000;
}


void Bob_init() {       //вводим начальные условия
    Bob.bank_account = 100000;
    Bob.zp = 150000;
    Bob.food = 25000;
    Bob.comm = 8000;
    Bob.rent = 60000;
    Bob.randomPurchases = 20000;
}


int main() {        //тело программы
    Bob_init();
    Alice_init();
    simulation();
    Bob.info();
    Alice.info();
}