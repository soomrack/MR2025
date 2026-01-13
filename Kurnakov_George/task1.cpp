#include <stdio.h>
#include<iostream>

typedef long long int RUB;


struct Home {
    RUB rent;  //аренда квартиры 
    RUB apartment_price;  // цена покупки квартиры 
    RUB mortgage;       // ипотека
    RUB downpayment;    // первоначальный взнос
};


struct Person {
    Home home;         
    RUB bank_deposit;   // накопления на счету
    RUB income;         // зарплата 
    int has_apartment;  // 1 если есть квартира, 0 если нет
    RUB main_bank_account; //второй зарплатный счет
    RUB clothes;//расходы на одежду
    RUB food;//расходы на еду
    RUB entertainment;//расходы на развлечения
    RUB car;//расходы на обслуживание машины     
    RUB machine;
};


struct Person alice;
struct Person bob;

//Общие параметры
double inflation = 0.08;// 8% годовая инфляция
double bank_rate = 0.15;// 15% годовых на вклад в банке
int start_year = 2025;
int start_month = 9;
int end_year = 2045;
int end_month = 9;
double indexing = 0.08;//8% индексация


void alice_init() {
    alice.bank_account = 200000;
    alice.income = 20000;
    alice.mortage = 100000;
    alice.monthly_payment = 50000;
    alice.mortgage_balance = alice.mortage - alice.bank_account;
    alice.investment_acc = 0;
}
void bob_init() {
    bob.bank_account = 200000;
    bob.income = 20000;
    bob.investment_acc = 0;
    bob.house = 1000000;
    
}


//Банк (проценты на вклад раз в месяц)
void apply_bank_interest(struct Person* p) {
    double monthly_rate = bank_rate / 12.0;
    p->bank_deposit *= (1.0 + monthly_rate);
}


void alice_food(int month) {
    if (month == 12) {
        alice.food *= (1.0 + inflation);
    }
    alice.main_bank_account -= alice.food;
}


void alice_clothes(int month) {
    if (month == 1) {
        alice.clothes *= (1.0 + inflation);
    }
    alice.main_bank_account -= alice.clothes;
}


void alice_entertainment(int month) {
    if (month == 1) {
        alice.entertainment *= (1.0 + inflation);
    }
    alice.main_bank_account -= alice.entertainment;
}


void alice_car(int month) {
    if (month == 1) {
        alice.car *= (1.0 + inflation);
    }
    alice.main_bank_account -= alice.car;
}


void alice_income(int month) {
    alice.main_bank_account += alice.income;
    if (month==1) {
        alice.income *= (1.0 + indexing);
    }
}


void alice_mortgage(int month) {
    alice.main_bank_account -= alice.home.mortgage;
}


void alice_transfer_money(int month) {
    alice.bank_deposit += alice.main_bank_account;
    alice.main_bank_account == 0;
}



void bob_food (int month) {
    if (month == 1) {
        bob.food *= (1.0 + inflation);
    }
    if (month == 12) {
        bob.main_bank_account -= 5000;
    }
    bob.main_bank_account -= bob.food;
    }


void bob_clothes(int month) {
    if (month == 1) {
        bob.clothes *= (1.0 + inflation);
    }
    bob.main_bank_account -= bob.clothes;
}


void bob_entertainment(int month) {
    if (month == 1) {
        bob.entertainment *= (1.0 + inflation);
    }
    bob.main_bank_account -= bob.entertainment;
}


void bob_car(int month) {
    if (month == 1) {
        bob.car *= (1.0 + inflation);
    }
    bob.main_bank_account -= bob.car;
}


void bob_income(int month) {
    bob.main_bank_account = 0;
    bob.main_bank_account += bob.income;
    if (month == 1) {
        bob.income *= (1.0 + indexing);
    }
}

void bob_home_rent(int month) {
    if (month == 1) {
        bob.home.rent *= (1.0 + inflation);
    }
     bob.main_bank_account -= bob.home.rent;
}

void bob_transfer_money(int month) {
    bob.bank_deposit += bob.main_bank_account;
    bob.main_bank_account == 0;
}

void bob_availability_of_an_apartament(int year, int month) {
    if (bob.bank_deposit >= bob.home.apartment_price && bob.has_apartment == 0) {
        bob.bank_deposit -= bob.home.apartment_price;
        bob.has_apartment = 1;
        printf("Боб купил квартиру в %d году-%lld месяца\n", year, month);
    }
}



//Симуляция 20 лет
void simulation() {
    int year = start_year;
    int month = start_month;
    while (!(year == end_year && month == end_month)) {
        apply_bank_interest(&alice);
        alice_income(month);
        alice_clothes(month);
        alice_food(month);
        alice_entertainment(month);
        alice_car(month);
        alice_mortgage(month);
        alice_transfer_money(month);

        apply_bank_interest(&bob);
        bob_income(month);
        bob_clothes(month);
        bob_food(month);
        bob_entertainment(month);
        bob_car(month);
        bob_home_rent(month);
        bob_transfer_money(month);
        bob_availability_of_an_apartament(year,month);
       
        // следующий месяц
        month++;
        if (month == 13) {
            year++;
            month = 1;
        }
    }
}




//Результаты
void print_results() {
    printf("\nРЕЗУЛЬТАТЫ ЧЕРЕЗ 20 ЛЕТ:\n\n");

    printf("Алиса: счёт = %lld руб., проживание: %s\n",
        alice.bank_deposit+=alice.machine, alice.has_apartment ? "есть квартира" : "нет квартиры");

    printf("Боб: счёт = %lld руб., проживание: %s\n",
        bob.bank_deposit+=bob.machine, bob.has_apartment ? "есть квартира" : "нет квартиры");
}


int main() {
    setlocale(LC_ALL, "RU");

    alice_init();
    bob_init();

    simulation();
    
    print_results();
    return 0;
}