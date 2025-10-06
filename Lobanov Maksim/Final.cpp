#include <cstdio> 

typedef int RUB;

const double INFLATION_RATE = 1.05; // 5% ежегодной инфляции - нужно еще прописать инфляцию

const double INDEXICATION_RATE = 1.07;


//Структура жилья
struct Apartment {
    RUB apart_price;
    RUB rent;
    RUB bills;
};


struct Person {
    RUB bank_account;
    RUB income;
    RUB food;
    RUB clothes;
    RUB mortgage;
    bool apart; //Наличие квартиры (True - есть, False - ее нет);
};


Person alice;
Person bob;
Apartment alice_home;
Apartment bob_home;

// Инициализация квартирного вопроса Алисы
void alice_home_init() {
    alice_home.rent = 50000;
    alice_home.apart_price = 20 * 1000 * 1000;
    alice_home.bills = 5000;
}


// Инициализация квартирного вопроса Боба
void bob_home_init() {
    bob_home.bills = 5000;
}


// Инициализация Алисы
void alice_init() {
    alice.bank_account = 1000 * 1000;
    alice.income = 200 * 1000;
    alice.food = 30000;
    alice.clothes = 10000;
    alice.apart = false;
}


// Инициализация Боба
void bob_init() {
    bob.bank_account = 2000 * 1000;
    bob.income = 150 * 1000;
    bob.food = 25000;
    bob.clothes = 20000;
    bob.mortgage = 100 * 1000;
}


// Траты на еду Алисы с учетом инфляции
void alice_spend_on_food(int month) {
    if (month == 10) {
        alice.food *=  INFLATION_RATE;  // Инфляция 5%
}
    alice.bank_account -= alice.food;
}


// Траты на еду Боба с учетом инфляции
void bob_spend_on_food(int month) {
    if (month == 10) {
        bob.food *=  INFLATION_RATE;  // Инфляция 5%
}
    bob.bank_account -= bob.food;
}


//Траты на одежду Алисы с учетом инфляции
void alice_spend_on_clothes(int month) {
        if (month == 10) {
        alice.clothes *=  INFLATION_RATE;  // Инфляция 5%
}
    alice.bank_account -= alice.clothes;
}


//Траты на одежду Боба с учетом инфляции
void bob_spend_on_clothes(int month) {
        if (month == 10) {
        bob.clothes *=  INFLATION_RATE;  // Инфляция 5%
}
    bob.bank_account -= bob.clothes;
}


// Начисление доходов с учётом индексации и премий у Алисы
void alice_add_income(int year, const int month) {
    if (month == 10) {
        alice.income = static_cast<RUB>(alice.income * INDEXICATION_RATE);  // Индексация на 7%
    }
    if (year == 2030 && month == 3) {
         alice.income = static_cast<RUB>(alice.income * 1.5);   // Повышение
    }
    alice.bank_account += alice.income;
}


// Начисление доходов с учётом индексации и премий у Боба
void bob_add_income(int year, const int month) {
    if (month == 10) {
        bob.income = static_cast<RUB>(bob.income * INDEXICATION_RATE);  // Индексация на 7%
    }
    if (year == 2030 && month == 3) {
         bob.income = static_cast<RUB>(bob.income * 2);   // Повышение
    }
    bob.bank_account += bob.income;
}


// Выплаты ипотеки
void pay_mortgage() {
    bob.bank_account -= bob.mortgage;
}


// Оплата налогов и ЖКХ Алисы
void alice_pay_bills(int month) {
    if (month == 10) {
            alice_home.bills *=  INFLATION_RATE;
    }
    alice.bank_account -= alice_home.bills;
}


// Оплата налогов и ЖКХ Боба
void bob_pay_bills(int month) {
    if (month == 10) {
            bob_home.bills *=  INFLATION_RATE;
    }
    bob.bank_account -= bob_home.bills;
}


// Покупка квартиры
void alice_buying_apart(){
    if ((alice.bank_account >= alice_home.apart_price)&&(not(alice.apart))){
        alice.bank_account -= alice_home.apart_price;
        alice.apart = true;
    }

}


//Платит аренду, пока нет квартиры
void alice_rent(int month){
    if (not(alice.apart))
    {
        if (month == 10) {
            alice_home.rent *=  INFLATION_RATE;
        }
        alice.bank_account-=alice_home.rent;
    }
    
}

//Увеличение цены на квартиру
void reprice_apart(int month){
    if (month == 10) {
        alice_home.apart_price *=  INFLATION_RATE;  // Инфляция 5%
}
}



void simulate() {
    int year = 2025;
    int month = 9;

    while (!(year == 2045 && month == 9)) {
        alice_add_income(year, month);
        alice_spend_on_food(month);
        alice_spend_on_clothes(month);
        alice_rent(month);
        alice_buying_apart();
        reprice_apart(month);
        alice_pay_bills(month);

        bob_add_income(year, month);
        bob_spend_on_food(month);
        bob_spend_on_clothes(month);
        pay_mortgage(); 
        bob_pay_bills(month);



        ++month;
        if (month == 13) {
            month = 1;
            ++year;
        }
    }
}


void print_results() {
    printf("Alice capital = %d RUB\n", alice.bank_account);
    printf("Bob capital   = %d RUB\n", bob.bank_account);
}


int main() {

    alice_init();  
    bob_init();  

    simulate();

    print_results();

    return 0;
}
