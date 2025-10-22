#include <stdio.h>

typedef long long int RUB;

struct Home {
    RUB bill;           // коммунальные платежи
    RUB value;          // стоимость дома (будет пересчитываться с инфляцией)
    RUB rent;           // арендная плата
    RUB mortgage_sum;   // общая сумма ипотеки
    RUB first_pay;      // первоначальный взнос
    RUB monthly_payment; // ежемесячный платеж
};

struct Person {
    RUB salary;
    RUB account;
    RUB wastes; // food, clothes, etc
    RUB deposit;
    struct Home home;   // информация о жилье
};

struct Person bob;
struct Person alice;

void alice_inite() 
{
    alice.account = 1000 * 1000;
    alice.salary = 300 * 1000;
    alice.wastes = 30 * 1000;
    alice.deposit = 0;

    // Инициализация данных о жилье Алисы (покупка)
    alice.home.bill = 10 * 1000;           // коммунальные платежи
    alice.home.value = 14 * 1000 * 1000;   // начальная стоимость дома
    alice.home.rent = 0;                   // Алиса не платит аренду
    alice.home.mortgage_sum = 14 * 1000 * 1000;
    alice.home.first_pay = 1000 * 1000;
    alice.home.monthly_payment = 217232;
    
    alice.account -= alice.home.first_pay;
}

void bob_inite() 
{
    bob.account = 1000 * 1000;
    bob.salary = 300 * 1000;
    bob.wastes = 50 * 1000;
    bob.deposit = 0;

    // Инициализация данных о жилье Боба (аренда)
    bob.home.bill = 0;                     // коммунальные обычно включены в аренду
    bob.home.value = 0;                    // Боб не владеет недвижимостью
    bob.home.rent = 60 * 1000;             // арендная плата
    bob.home.mortgage_sum = 0;
    bob.home.first_pay = 0;
    bob.home.monthly_payment = 0;
}

void alice_salary(int month) 
{
    if (month == 1) {
        alice.salary *= 1.07; // ежегодное повышение зарплаты на 7%
    }
    alice.account += alice.salary;
}

void alice_print() 
{
    printf("Alice:\n");
    printf("  Savings: %lld rub.\n", alice.deposit);
    printf("  Current account: %lld rub.\n", alice.account);
    printf("  Property value: %lld rub.\n", alice.home.value); // Стоимость недвижимости
    printf("  Total capital: %lld rub.\n\n", alice.deposit + alice.account + alice.home.value);
}

void alice_mortgage() 
{
    alice.account -= alice.home.monthly_payment;
}

void alice_wastes() 
{
    alice.account -= alice.wastes;
}

void alice_bill() 
{
    alice.account -= alice.home.bill;
}

void alice_deposit()
{
    // Ежемесячная капитализация депозита под 20% годовых
    alice.deposit += alice.deposit * (20.0 / 12.0) / 100.0;
    alice.deposit += alice.account;
    alice.account = 0;
}

// Функции для Боба
void bob_print() 
{
    printf("Bob:\n");
    printf("  Savings: %lld rub.\n", bob.deposit);
    printf("  Current account: %lld rub.\n", bob.account);
    printf("  Total capital: %lld rub.\n\n", bob.deposit + bob.account);
}


void bob_salary(int month) 
{
    if (month == 1) {
        bob.salary *= 1.07; // ежегодное повышение зарплаты на 7%
    }
    bob.account += bob.salary;
}

void bob_rent()
{
    bob.account -= bob.home.rent;
}

void bob_wastes() 
{
    bob.account -= bob.wastes;
}

void bob_deposit()
{
    // Ежемесячная капитализация депозита под 20% годовых
    bob.deposit += bob.deposit * (20.0 / 12.0) / 100.0;
    bob.deposit += bob.account;
    bob.account = 0;
}

// Функция для пересчета стоимости недвижимости с учетом инфляции
void update_home_value(int month)
{
    if (month == 1) {
        // Ежегодная инфляция на недвижимость - 5%
        alice.home.value *= 1.05;
        
        // Ежегодное увеличение коммунальных платежей на 7%
        alice.home.bill *= 1.07;
        
        // Ежегодное увеличение арендной платы на 5%
        bob.home.rent *= 1.05;
    }
}

void simulation()
{
    int month = 9;
    int year = 2025;

    while (!((year == 2025 + 30) && (month == 9))) {
        // Доходы, расходы, инвестиции и обновления стоимости
        alice_salary(month);
        alice_mortgage();
        alice_wastes();
        alice_bill();
        alice_deposit();
        update_home_value(month);

        bob_salary(month);
        bob_rent();
        bob_wastes();
        bob_deposit();
        
        month++;
        if (month == 13) {
            month = 1;
            year++;
        }
    }
}

void print_result()
{
    printf("Results after 30 years:\n");
    printf("========================\n");
    
    // Вывод результатов для Боба
    printf("Bob:\n");
    printf("  Savings: %lld rub.\n", bob.deposit);
    printf("  Current account: %lld rub.\n", bob.account);
    printf("  Total capital: %lld rub.\n\n", bob.deposit + bob.account);

    // Вывод результатов для Алисы
    printf("Alice:\n");
    printf("  Savings: %lld rub.\n", alice.deposit);
    printf("  Current account: %lld rub.\n", alice.account);
    printf("  Property value: %lld rub.\n", alice.home.value);
    printf("  Total capital: %lld rub.\n\n", alice.deposit + alice.account + alice.home.value);

    // Итоговое сравнение
    RUB bob_total = bob.deposit + bob.account;
    RUB alice_total = alice.deposit + alice.account + alice.home.value;
    
    printf("FINAL COMPARISON:\n");
    printf("Bob (rent): %lld rub.\n", bob_total);
    printf("Alice (buy): %lld rub.\n", alice_total);
    
    if (alice_total > bob_total) {
        printf("Buying is better by %lld rub.\n", alice_total - bob_total);
    } else {
        printf("Renting is better by %lld rub.\n", bob_total - alice_total);
    }
}

int main()
{
    alice_inite();
    bob_inite();
    simulation();
    print_result();

    return 0;
}