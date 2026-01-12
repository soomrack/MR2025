#include <iostream>

typedef int RUB;
typedef int USDT;

struct person {
    RUB bank_account;
    RUB deposit;
    RUB salary;
    RUB food;
    RUB rent;
    RUB mortgage;
    RUB comm;
    RUB random_purchases;
    RUB home_price;
    int stavka;
};

person Alice;           //обозначаем людей
person Bob;

void Alice_income(const int year, const int month) {
    if (month == 10) {
        Alice.salary = Alice.salary * 1.08;     //индексация зп
    }
    if ((year == 2034) && (month == 5)) {
        Alice.salary = Alice.salary * 1.5;      //повышение
    }

    Alice.bank_account += Alice.salary;

}   

void Bob_income(const int year, const int month) {
    if (month == 10) {
        Bob.salary = Bob.salary * 1.1;     //индексация зп
    }
    if ((year == 2034) && (month == 5)) {
        Bob.salary = Bob.salary * 1.6;      //повышение
    }

    Bob.bank_account += Bob.salary;

}

void inflation(const int month) {
    if (month == 1) {
        Bob.home_price = Bob.home_price * 1.05;
    }
}


void Bob_food(const int month) {
    Bob.bank_account -= Bob.food;
    if (month == 1) {
        Bob.food = Bob.food * 1.10;
    }


}

void Bob_rent(const int month) {
    Bob.bank_account -= Bob.rent;
    Bob.bank_account -= Bob.comm;
    if (month == 1) {
        Bob.comm = Bob.comm * 1.08;
        Bob.rent = Bob.rent * 1.10;
    }
}


void Bob_random_purchases(const int month) {
    Bob.bank_account -= Bob.random_purchases;
    if (month == 1) {
        Bob.random_purchases = Bob.random_purchases * 1.10;
    }
}

void Alice_food(const int month) {  
    Alice.bank_account -= Alice.food;
    if (month == 1) {
        Alice.food = Alice.food * 1.10;
    }

}

void Alice_rent(const int month) {
    Alice.bank_account -= Alice.rent;
    Alice.bank_account -= Alice.comm;
    if (month == 1) {
        Alice.comm = Alice.comm * 1.08;
        Alice.rent = Alice.rent * 1.10;
    }
}

void Alice_random_purchases(const int month) {
    Alice.bank_account -= Alice.random_purchases;
    if (month == 1) {
        Alice.random_purchases = Alice.random_purchases * 1.10;
    }
}

void Alice_car(const int month, const int year, RUB car_price) {
    if (year == 2035 && month == 5) {
        Alice.bank_account -= car_price;
    }
}

void Bob_home_purchase(bool owns_home, int year, int month) {
    if ((Bob.deposit >= Bob.home_price) && (owns_home == false)) {
        Bob.deposit -= Bob.home_price;
        owns_home = true;
    }

}

void Alice_mortgage() {
    int rate = 20;      //ставка
    int term = 20;      //кол-во лет
    RUB mortgage = (Alice.home_price * pow(1 + (rate / 100), term)) / (term * 12);
    Alice.bank_account -= mortgage;
}

void Bob_bank_savings(const int month){
    Bob.deposit += Bob.bank_account;
    Bob.bank_account = 0;
    Bob.deposit += Bob.deposit * (Bob.stavka / 12);
}

void Alice_bank_savings(const int month) {
    Alice.deposit += Alice.bank_account;
    Alice.bank_account = 0;
    Alice.deposit += Alice.deposit * (Alice.stavka / 12);
}

void info() {       //показать состояние банк. аккаунта
    std::cout <<  "Bob:" << Bob.deposit << '\n';
    std::cout <<  "Alice:" << Alice.deposit << '\n';
}

void simulation() {     //основная симуляцияя
    int year = 2025, month = 9;
    bool owns_home = false;
    RUB car_price = 2 * 1000 * 1000;

    while (!(year == 2045 && month == 9)) {

        Bob_income(year, month);
        Bob_food(month);
        Bob_rent(month);
        Bob_random_purchases(month);
        Bob_bank_savings(month);
        Bob_home_purchase(owns_home, year, month);
       

        Alice_income(year, month);
        Alice_food(month);
        Alice_rent(month);
        Alice_random_purchases(month);
        Alice_car(month, year, car_price);
        Alice_bank_savings(month);
        Alice_mortgage();

        inflation(month);
        month++;
        if (month == 13) {
            month = 1;
            year++;
        }
    }
}


void Alice_init() {     //вводим начальные условия
    Alice.deposit = 100000;
    Alice.salary = 100000;
    Alice.food = 30000;
    Alice.comm = 6000;
    Alice.random_purchases = 30000;
    Alice.home_price = 20 * 1000 * 1000;
    Alice.stavka = 0.12;
}


void Bob_init() {       //вводим начальные условия
    Bob.deposit = 100000;
    Bob.salary = 150000;
    Bob.food = 25000;
    Bob.comm = 8000;
    Bob.rent = 60000;
    Bob.random_purchases = 5000;
    Bob.home_price = 15 * 1000 * 1000;
    Bob.stavka = 0.12;
}


int main() {        //тело программы
    Bob_init();
    Alice_init();
    simulation();
    info();
}