#include <stdio.h>
#include <math.h>

typedef long long int Money; // В нашем случае валюта - рубли

// Константы задаем
const double INFLATION = 0.07;
const int START_MONTH = 9;  
const int START_YEAR = 2025;  
const int YEARS = 20;  

// Структуры

struct Mortgage { // Ипотека
    double rate; // Ставка
    Money payment; // Платеж
};

struct Car { // Машина
    Money cost; // Стоимость
    Money insurance; // Страховка
    Money maintenance; // Обслуживание
    Money fuel; // Толпливо
    Money parking; // Парковка
    int purchase_year; // Год покупки
};

struct Human { // Человек
    Money salary; // Зарплата 
    Money account; // Банковский счет
    double bank_percent; // Процент в банке
    Money wastes; // Расходы ежемесячные
    Money rent; // Аренда
    Money communal_payments; // Коммунальные платежи
    Money flat_value; // Рыночная стоимость квартиры
    Money mortgage_debt; // Остаток долга по ипотеке
    struct Mortgage mortgage; // Параметры ипотеки
    struct Car car; // Информация о машине
    Money garage_value; // Стоимость гаража
    Money initial_savings; // Первоначальные сбережения
};                             

struct Human Alice;
struct Human Bob;

void calculate_mortgage() {
    double monthly_rate = Alice.mortgage.rate / 12; // Расчет месячной процентной ставки
    int months = YEARS * 12; // Количество месяцев ипотеки
    double coefficient = (monthly_rate * pow(1 + monthly_rate, months)) / 
                        (pow(1 + monthly_rate, months) - 1); // Формула аннуитетного коэффициента
    // Ипотека на сумму квартиры минус первоначальный взнос
    Money mortgage_amount = Alice.flat_value - Alice.initial_savings; // Сумма кредита
    Alice.mortgage.payment = mortgage_amount * coefficient; // Ежемесячный платеж
    Alice.mortgage_debt = mortgage_amount; // Начальный долг
//printf("Alice: mortgage payment = %lld rub/month, debt = %lld rub\n", 
      // Alice.mortgage.payment, Alice.mortgage_debt);
//  
}

void Alice_init() {
    Alice.initial_savings = 800000;     // 800к накоплений
    Alice.flat_value = 8000000;         // Квартира 8 млн 
    Alice.account = 100000;             // 100к на руках после взноса
    Alice.salary = 150000;              // Зарплата 150к
    Alice.bank_percent = 0.06;          // Процент в банке 6 %
    Alice.wastes = 40000;               // 40к на еду и базовые расходы
    Alice.communal_payments = 8000;     // 8к коммунальные платежи
    Alice.mortgage.rate = 0.09;         // Годовая ставка по ипотеке
    Alice.garage_value = 0;
    Alice.car.cost = 0;
    
    calculate_mortgage();
}

void Bob_init() {
    Bob.account = 50000;               // На счете у Боба
    Bob.salary = 170000;               // Зарплата 170к
    Bob.bank_percent = 0.06;           // Процент в банке 6 %
    Bob.wastes = 50000;                // Базовые расходы 50к
    Bob.rent = 35000;                  // Аренда 35к (скромнее)
    Bob.communal_payments = 3000;      // Часть комуналки включена в аренду
    Bob.flat_value = 0;
    Bob.garage_value = 0;
    Bob.car.cost = 0;
}

void Alice_salary(const int year, const int month) {
    Alice.account += Alice.salary;
    
    // Индексация раз в 2 года
    if (month == 1 && year % 2 == 0 && year > START_YEAR) {
        Alice.salary += Alice.salary * 0.03;
    }
}

void Alice_deposite(const int year, const int month) {
    // Проценты на весь остаток
    if (month % 3 == 0) {  // Раз в квартал
        Alice.account += Alice.account * (Alice.bank_percent / 12 * 3);
    }
}
void Alice_expenses(const int year, const int month) {
    // Базовые расходы
    Money basic_expenses = Alice.wastes + Alice.communal_payments;
    if (Alice.account >= basic_expenses) {
        Alice.account -= basic_expenses;
    }
    
    // Ипотечный платеж 
    if (Alice.account >= Alice.mortgage.payment) {
        Alice.account -= Alice.mortgage.payment;
        Alice.mortgage_debt -= Alice.mortgage.payment;
    }
    
    // Инфляция расходов раз в год
    if (month == 1 && year > START_YEAR) {
        Alice.wastes += Alice.wastes * 0.05; // Ниже общей инфляции
        Alice.communal_payments += Alice.communal_payments * 0.07;
    }
}

void Alice_flat_expenses(const int year, const int month) {
    // Квартирные расходы только если есть деньги
    if (month == 6 && (year == 2030 || year == 2035) && Alice.account > 100000) {
 //       printf("Alice: major repair -150,000 rub\n");
        Alice.account -= 150000;
    }
    
    // Мелкие домашние расходы
    if (month == 8 && Alice.account > 50000) {
        Alice.account -= 15000;
    }
}

void Alice_life_events(const int year, const int month) {
    // Отпуск только если есть деньги
    if (month == 7 && Alice.account > 100000) {
      //  printf("Alice: vacation -50,000 rub\n");
        Alice.account -= 50000;
    }
    
    // Мелкие непредвиденные расходы
    if (month == 3 && year % 3 == 0 && Alice.account > 30000) {
        Alice.account -= 20000;
    }
}

void Bob_salary(const int year, const int month) { 
    Bob.account += Bob.salary; // Начисление зарплаты
    
    if (month == 1 && year % 2 == 0 && year > START_YEAR) { // Индексация
        Bob.salary += Bob.salary * 0.03;
    }
}

void Bob_deposite(const int year, const int month) { // Начисление процентов по вкладу
    if (month % 3 == 0) {
        Bob.account += Bob.account * (Bob.bank_percent / 12 * 3);
    }
}

void Bob_expenses(const int year, const int month) {
    // Базовые расходы
    Money basic_expenses = Bob.wastes + Bob.rent + Bob.communal_payments;
    if (Bob.account >= basic_expenses) {
        Bob.account -= basic_expenses;
    }
    
    // Расходы на машину если есть
    if (Bob.car.cost > 0 && Bob.account > Bob.car.fuel + Bob.car.parking) {
        Bob.account -= Bob.car.fuel + Bob.car.parking;
        if (month == 12 && Bob.account > Bob.car.insurance) {
            Bob.account -= Bob.car.insurance;
        }
        if (month == 6 && Bob.account > Bob.car.maintenance) {
            Bob.account -= Bob.car.maintenance;
        }
    }
    
    // Инфляция
    if (month == 1 && year > START_YEAR) {
        Bob.wastes += Bob.wastes * 0.05;
        Bob.rent += Bob.rent * 0.07;
    }
}

void Bob_buys_car(const int year, const int month) { // Боб покупает машину
    if (year == 2028 && month == 3 && Bob.car.cost == 0 && Bob.account > 700000) {
//        printf("Bob buys used car for 600,000 rub\n");
        Bob.account -= 600000;
        Bob.car.cost = 600000;
        Bob.car.insurance = 20000;     // Дешевле страховка
        Bob.car.maintenance = 15000;   // Дешевле ТО
        Bob.car.fuel = 5000;           // Меньше бензин
        Bob.car.parking = 2000;
        Bob.car.purchase_year = year;
    }
}
void Bob_buys_garage(const int year, const int month) { // Покупка гаража
    if (year == 2033 && month == 9 && Bob.garage_value == 0 && Bob.account > 400000) {
   //     printf("Bob buys garage for 300,000 rub\n");
        Bob.account -= 300000;
        Bob.garage_value = 300000;
    }
}

void Bob_life_events(const int year, const int month) {
    // Отпуск
    if (month == 7 && Bob.account > 100000) {
      //  printf("Bob: vacation -60,000 rub\n");
        Bob.account -= 60000;
    }
    
    // Развлечения поскромнее
    if (month % 2 == 0 && Bob.account > 30000) {
        Bob.account -= 8000;
    }
}

void property_taxes(const int year, const int month) {
    if (month == 12) {
        // Налог на имущество Алисы (только если есть деньги)
        if (Alice.account > 10000) {
            Money alice_tax = Alice.flat_value * 0.001;
            if (alice_tax > Alice.account) alice_tax = Alice.account / 2;
            Alice.account -= alice_tax;
        }
        
        // Транспортный налог Боба
        if (Bob.car.cost > 0 && Bob.account > 5000) {
            Money bob_transport_tax = 3000; // Фиксированный
            Bob.account -= bob_transport_tax;
        }
    }
}

void adjust_for_inflation(const int year) {
    // Стоимость недвижимости растет
    if (year == 2030 || year == 2040) {
        Alice.flat_value += Alice.flat_value * 0.08;
        if (Bob.garage_value > 0) {
            Bob.garage_value += Bob.garage_value * 0.05;
        }
    }
}

void print_results() {
    printf("\n=== РЕЗУЛЬТАТЫ ЗА %d ЛЕТ ===\n", YEARS);
    
    // У Алисы остался долг по ипотеке 
    Money alice_equity = Alice.flat_value - Alice.mortgage_debt;
    if (alice_equity < 0) alice_equity = 0;
    
    printf("Элис (ипотека):\n");
    printf("  Накопления: %lld руб\n", Alice.account);
    printf("  Стоимость квартиры: %lld руб\n", Alice.flat_value);
    printf("  Остаток долга: %lld руб\n", Alice.mortgage_debt);
    printf("  Чистая стоимость: %lld руб\n", alice_equity);
    printf("  ОБЩИЙ КАПИТАЛ: %lld руб\n\n", Alice.account + alice_equity);
    
    printf("Боб (аренда):\n");
    printf("  Накопления: %lld руб\n", Bob.account);
    printf("  Автомобиль: %lld руб\n", Bob.car.cost);
    printf("  Гараж: %lld руб\n", Bob.garage_value);
    printf("  ОБЩИЙ КАПИТАЛ: %lld руб\n\n", Bob.account + Bob.car.cost + Bob.garage_value);
    if ((Alice.account + alice_equity) > (Bob.account + Bob.car.cost + Bob.garage_value)){
        printf(" В итоге жизнь лучше у Элис");
    }
    else {
        printf(" В итоге жизнь лучше у Боба");
    }
}

void simulation() {
    int month = START_MONTH;
    int year = START_YEAR;

    printf("Симуляция жизни двух людей\n\n");

    while (!(year == START_YEAR + YEARS && month == 9)) {
        // Сначала зарплата
        Alice_salary(year, month);
        Bob_salary(year, month);
        
        // Потом проценты
        Alice_deposite(year, month);
        Bob_deposite(year, month);
        
        // Затем расходы
        Alice_expenses(year, month);
        Bob_expenses(year, month);
        
        // События жизни
        Alice_flat_expenses(year, month);
        Alice_life_events(year, month);
        Bob_life_events(year, month);
        Bob_buys_car(year, month);
        Bob_buys_garage(year, month);
        
        // Годовые события
        if (month == 12) {
            property_taxes(year, month);
            adjust_for_inflation(year);
        }

        // Следующий месяц
        month++;
        if (month > 12) {
            year++;
            month = 1;
        }
    }
}

int main() {
    Alice_init();
    Bob_init();

    simulation();
    print_results();

    return 0;
}