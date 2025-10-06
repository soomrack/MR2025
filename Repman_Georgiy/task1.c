#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

// RUB - тип для рублей
typedef int RUB;

// --- Структура для даты ---
typedef struct {
    int year;
    int month;
} Date;

// Проверка корректности даты
bool date_is_valid(const Date* date) {
    return date->year >= 0 && date->month >= 1 && date->month <= 12;
}

// Получение следующей даты
Date date_next_month(const Date* date) {
    Date next = *date;
    if (next.month == 12) {
        next.year++;
        next.month = 1;
    } else {
        next.month++;
    }
    return next;
}

// Сравнение дат
bool date_equals(const Date* a, const Date* b) {
    return a->year == b->year && a->month == b->month;
}

bool date_less(const Date* a, const Date* b) {
    if (a->year < b->year) return true;
    if (a->year > b->year) return false;
    return a->month < b->month;
}

bool date_less_equal(const Date* a, const Date* b) {
    return date_equals(a, b) || date_less(a, b);
}

// --- Перечисление для типа жилищных выплат ---
typedef enum {
    HOUSING_RENT,      // Аренда
    HOUSING_MORTGAGE   // Ипотека
} HousingType;

// --- Структура дома ---
typedef struct {
    RUB price;
} House;

// Константы для расчетов
#define ANNUAL_PRICE_INCREASE 0.04
#define RENT_RATE 0.055
#define INTEREST_RATE 0.15
#define MORTGAGE_YEARS 20
#define UTILITY_BILL 4000

// Расчет платежей
RUB house_rent_payment(const House* house) {
    return (RUB)(house->price * RENT_RATE / 12);
}

RUB house_mortgage_payment(const House* house) {
    return (RUB)(house->price / (12 * MORTGAGE_YEARS) + house->price / 12 * INTEREST_RATE);
}

RUB house_equity_payment(const House* house) {
    return house->price / (12 * MORTGAGE_YEARS);
}

void house_raise_price(House* house) {
    house->price = (RUB)(house->price * (1 + ANNUAL_PRICE_INCREASE));
}

// --- Конфигурация персонажа ---
typedef struct {
    char name[50];
    RUB salary;
    RUB initial_capital;
    HousingType housing_type;
    RUB monthly_expenses;
    bool receives_house_equity;
} PersonConfig;

// --- Структура человека ---
typedef struct Person {
    char name[50];
    RUB salary;
    RUB total_capital;
    House* house;
    RUB money_left_in_month;
    PersonConfig config;
} Person;

// Функции для работы с Person
void person_init(Person* person, const PersonConfig* config, House* house) {
    strcpy(person->name, config->name);
    person->salary = config->salary;
    person->total_capital = config->initial_capital;
    person->house = house;
    person->money_left_in_month = 0;
    person->config = *config;
}

void person_receive_salary(Person* person) {
    person->money_left_in_month = person->salary;
}

void person_receive_house_equity(Person* person) {
    if (person->config.receives_house_equity) {
        person->total_capital += house_equity_payment(person->house);
    }
}

void person_pay_housing(Person* person) {
    switch (person->config.housing_type) {
        case HOUSING_RENT:
            person->money_left_in_month -= house_rent_payment(person->house);
            break;
        case HOUSING_MORTGAGE:
            person->money_left_in_month -= house_mortgage_payment(person->house);
            break;
    }
}

void person_pay_utility_bill(Person* person) {
    person->money_left_in_month -= UTILITY_BILL;
}

void person_pay_monthly_expenses(Person* person) {
    person->money_left_in_month -= person->config.monthly_expenses;
}

void person_invest_remaining_money(Person* person) {
    person->total_capital += person->money_left_in_month;
    person->money_left_in_month = 0;
}

void person_add_money(Person* person) {
    person_receive_salary(person);
    person_receive_house_equity(person);
}

void person_spend_money(Person* person) {
    person_pay_housing(person);
    person_pay_utility_bill(person);
    person_pay_monthly_expenses(person);
    person_invest_remaining_money(person);
}

void person_process_month(Person* person) {
    person_add_money(person);
    person_spend_money(person);
}

bool person_has_negative_budget(const Person* person) {
    return person->money_left_in_month < 0;
}

RUB person_total_assets(const Person* person) {
    RUB assets = person->total_capital;
    if (person->config.housing_type == HOUSING_MORTGAGE) {
        assets += person->house->price;
    }
    return assets;
}

// --- Фабричные функции для создания конфигураций ---
PersonConfig person_config_create_alice(RUB salary, RUB capital) {
    PersonConfig config;
    strcpy(config.name, "Alice");
    config.salary = salary;
    config.initial_capital = capital;
    config.housing_type = HOUSING_MORTGAGE;
    config.monthly_expenses = 65000;
    config.receives_house_equity = true;
    return config;
}

PersonConfig person_config_create_bob(RUB salary, RUB capital) {
    PersonConfig config;
    strcpy(config.name, "Bob");
    config.salary = salary;
    config.initial_capital = capital;
    config.housing_type = HOUSING_RENT;
    config.monthly_expenses = 100000;
    config.receives_house_equity = false;
    return config;
}

PersonConfig person_config_create_custom(const char* name, RUB salary, RUB capital,
                                        HousingType housing, RUB expenses, bool gets_equity) {
    PersonConfig config;
    strcpy(config.name, name);
    config.salary = salary;
    config.initial_capital = capital;
    config.housing_type = housing;
    config.monthly_expenses = expenses;
    config.receives_house_equity = gets_equity;
    return config;
}

// --- Структура для симуляции ---
typedef struct {
    Date current_date;
    Date start_date;
    Date end_date;
    Person** persons;
    int persons_count;
    House* house;
} Simulation;

// Инициализация симуляции
bool simulation_init(Simulation* sim, const Date* start, const Date* end,
                    Person** persons, int persons_count, House* house) {
    if (!date_is_valid(start) || !date_is_valid(end)) {
        return false;
    }
    
    sim->start_date = *start;
    sim->end_date = *end;
    sim->current_date = *start;
    sim->persons = persons;
    sim->persons_count = persons_count;
    sim->house = house;
    
    return true;
}

// Запуск симуляции
void simulation_run(Simulation* sim) {
    printf("Начальная цена дома: %d руб.\n", sim->house->price);
    printf("Начало симуляции: %d.%d\n", sim->start_date.month, sim->start_date.year);
    printf("Конец симуляции: %d.%d\n\n", sim->end_date.month, sim->end_date.year);
    
    while (date_less_equal(&sim->current_date, &sim->end_date)) {
        // Повышаем цену дома в конце каждого года
        if (sim->current_date.month == 12) {
            house_raise_price(sim->house);
        }

        // Обрабатываем месяц для всех персонажей
        bool should_stop = false;
        for (int i = 0; i < sim->persons_count; i++) {
            Person* person = sim->persons[i];
            person_process_month(person);
            
            if (person_has_negative_budget(person)) {
                printf("ВНИМАНИЕ: у %s отрицательный бюджет! Симуляция остановлена.\n", 
                       person->name);
                should_stop = true;
                break;
            }
        }
        
        if (should_stop) {
            break;
        }

        sim->current_date = date_next_month(&sim->current_date);
    }
    
    printf("\nКонечная цена дома: %d руб.\n", sim->house->price);
}

// Вывод результатов
void simulation_print_results(const Simulation* sim) {
    printf("\n=== ИТОГИ СИМУЛЯЦИИ ===\n");
    for (int i = 0; i < sim->persons_count; i++) {
        const Person* person = sim->persons[i];
        printf("%s:\n", person->name);
        printf("  - Капитал: %d руб.\n", person->total_capital);
        printf("  - Общие активы: %d руб.\n", person_total_assets(person));
    }
}

// Основная функция
int main() {
    // --- Создаем начальные условия ---
    Date start_date = {2025, 9};
    Date end_date = {2045, 12};
    
    House house = {9000000};
    
    // --- Создаем персонажей ---
    Person persons[3];
    Person* person_ptrs[3];
    
    // Alice (покупает в ипотеку)
    PersonConfig alice_config = person_config_create_alice(200000, 0);
    person_init(&persons[0], &alice_config, &house);
    
    // Bob (снимает жилье)
    PersonConfig bob_config = person_config_create_bob(200000, 0);
    person_init(&persons[1], &bob_config, &house);
    
    // Charlie (кастомный персонаж)
    PersonConfig charlie_config = person_config_create_custom("Charlie", 250000, 500000, 
                                                             HOUSING_RENT, 80000, false);
    person_init(&persons[2], &charlie_config, &house);
    
    // Создаем массив указателей
    for (int i = 0; i < 3; i++) {
        person_ptrs[i] = &persons[i];
    }
    
    // --- Создаем и запускаем симуляцию ---
    Simulation sim;
    if (!simulation_init(&sim, &start_date, &end_date, person_ptrs, 3, &house)) {
        printf("Ошибка: некорректные даты симуляции!\n");
        return 1;
    }
    
    simulation_run(&sim);
    simulation_print_results(&sim);
    
    return 0;
}