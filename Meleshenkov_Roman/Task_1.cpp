// Подключение библиотеки для ввода-вывода
#include <stdio.h>


typedef long long int RUB;

// Структура для хранения информации о человеке (его финансов)
struct Person {
    RUB bank_account;         // Деньги на банковском счету (в рублях)
    RUB income;               // Ежемесячный доход (в рублях)
    RUB cash;                 // Наличные деньги в кошельке (в рублях)
    RUB pocket_cash;          // Сколько наличных оставлять себе каждый месяц (в рублях)
    RUB spending;             // Прочие расходы (в рублях)
    RUB car_spending;         // Расходы на машину (в рублях)
    RUB food_spending;        // Расходы на еду (в рублях)
    RUB mortgage_spending;    // Платеж по ипотеке (в рублях) - только для Алисы
    RUB rent_spending;        // Плата за аренду (в рублях) - только для Боба
    RUB house_cost;           // Стоимость дома (в рублях) - только для Боба
    RUB trip_spending;        // Расходы на отпуск (в рублях)
    RUB car_accident_cost;    // Стоимость ремонта после аварии (в рублях) - только для Боба
    float annual_percent;     // Годовая процентная ставка по вкладу (например, 0.08 = 8%)
    bool has_percent_increase;// Флаг: true если есть повышенная процентная ставка
    bool mortgage_closed;     // Флаг: true если ипотека выплачена - только для Алисы
};

// Создаем двух персонажей: Алису и Боба
struct Person alice;
struct Person bob;

// Функция для начальной настройки Алисы
void alice_init() {
    alice.bank_account = 3 * 1000 * 1000;     // 3 000 000 рублей = 3 миллиона
    alice.income = 200 * 1000;                // 200 000 рублей в месяц
    alice.cash = 0;                           // Начинает с 0 наличных
    alice.pocket_cash = 5000;                 // Оставляет себе 5 000 рублей наличными
    alice.spending = 50000;                   // 50 000 рублей прочих расходов
    alice.car_spending = 40000;               // 40 000 рублей на машину
    alice.food_spending = 40000;              // 40 000 рублей на еду
    alice.mortgage_spending = 100 * 1000;     // 100 000 рублей платеж по ипотеке
    alice.trip_spending = 60000;              // 60 000 рублей на отпуск
    alice.annual_percent = 0.08;              // 8% годовых по вкладу
    alice.has_percent_increase = false;       // Пока нет повышенной ставки
    alice.mortgage_closed = false;            // Ипотека еще не выплачена
}

// Функция для начальной настройки Боба
void bob_init() { 
    bob.bank_account = 3 * 1000 * 1000;       // 3 000 000 рублей = 3 миллиона
    bob.income = 220 * 1000;                  // 220 000 рублей в месяц (на 20к больше чем у Алисы)
    bob.cash = 0;                             // Начинает с 0 наличных
    bob.pocket_cash = 5000;                   // Оставляет себе 5 000 рублей наличными
    bob.spending = 50000;                     // 50 000 рублей прочих расходов
    bob.car_spending = 40000;                 // 40 000 рублей на машину
    bob.food_spending = 40000;                // 40 000 рублей на еду
    bob.rent_spending = 60000;                // 60 000 рублей за аренду (меньше чем ипотека Алисы)
    bob.house_cost = 12 * 1000 * 1000;        // 12 000 000 рублей стоит дом, который хочет купить
    bob.trip_spending = 60000;                // 60 000 рублей на отпуск
    bob.car_accident_cost = 100 * 1000;       // 100 000 рублей на ремонт после аварии
    bob.annual_percent = 0.08;                // 8% годовых по вкладу
    bob.has_percent_increase = false;         // Пока нет повышенной ставки
}

// Функция для вывода итоговых результатов
void persons_print() {
    // Выводим сколько денег у Боба на счету (уже в рублях)
    printf("Итог: Боб = %lld руб.\n", bob.bank_account);
    
    // Выводим сколько денег у Алисы на счету (уже в рублях)
    printf("Итог: Алиса = %lld руб.\n", alice.bank_account);
    
    // Определяем, у кого больше денег
    if (alice.bank_account > bob.bank_account) {
        printf("Алиса богаче на %lld руб.\n", alice.bank_account - bob.bank_account);
    } else if (bob.bank_account > alice.bank_account) {
        printf("Боб богаче на %lld руб.\n", bob.bank_account - alice.bank_account);
    } else {
        printf("У них одинаковое количество денег\n");
    }
}

// Функция для обработки дохода Алисы в определенный месяц и год
void alice_income(const int year, const int month) {
    // Проверяем, не октябрь ли 2030 года (когда Алиса получает повышение)
    if (year == 2030 && month == 10) {
        alice.income = alice.income * 15 / 10; // Увеличиваем доход на 50% (умножаем на 1.5)
        printf("Октябрь 2030: Алиса получила повышение! Новый доход: %lld руб./мес\n", alice.income);
    }
    // Добавляем доход к наличным деньгам
    alice.cash += alice.income;
}

// Функция для обработки ипотечных платежей Алисы
void alice_mortgage(const int year, const int month) {
    // Проверяем, не выплачена ли уже ипотека
    if (!alice.mortgage_closed) {
        // Если это сентябрь 2025 года - делаем первоначальный взнос
        if (year == 2025 && month == 9) {
            alice.bank_account -= 2 * 1000 * 1000; // Снимаем 2 миллиона со счета
            printf("Сентябрь 2025: Алиса внесла первоначальный взнос 2 000 000 руб.\n");
        }
        else {
            // В остальные месяцы платим ежемесячный платеж из наличных
            alice.cash -= alice.mortgage_spending;
        }
        // Проверяем, не сентябрь ли 2045 года (когда ипотека выплачена)
        if (year == 2045 && month == 9) {
            alice.mortgage_closed = true; // Отмечаем, что ипотека выплачена
            printf("Сентябрь 2045: Алиса выплатила ипотеку!\n");
        }
    }
}

// Функция для обработки прочих расходов Алисы
void alice_spending(const int month) {
    // Начинаем с базовой суммы расходов
    RUB spending = alice.spending;
    
    // В феврале тратим меньше (возможно, из-за отпуска или экономии)
    if (month == 2) {
        spending -= 7 * 1000; // Уменьшаем на 7000 рублей
    }
    
    // В июле тратим больше (возможно, сезонные расходы)
    if (month == 7) {
        spending += 5 * 1000; // Увеличиваем на 5000 рублей
    }
    
    // В августе тоже тратим больше
    if (month == 8) {
        spending += 2 * 1000; // Увеличиваем на 2000 рублей
    }
    
    // Вычитаем расходы из наличных денег
    alice.cash -= spending;
}

// Функция для расходов на еду Алисы
void alice_food() {
    // Просто вычитаем фиксированную сумму на еду
    alice.cash -= alice.food_spending;
}

// Функция для расходов на машину Алисы
void alice_car() {
    // Вычитаем фиксированную сумму на обслуживание машины
    alice.cash -= alice.car_spending;
}

// Функция для отпускных расходов Алисы
void alice_trip(const int month) {
    // Проверяем, не июль ли (месяц отпуска)
    if (month == 7) {
        // Снимаем деньги на отпуск со счета (не из наличных)
        alice.bank_account -= alice.trip_spending;
    }
}

// Функция для управления вкладом Алисы
void alice_deposit() {
    // Проверяем, если счет больше 3 миллионов и нет повышенной ставки
    if (!alice.has_percent_increase && alice.bank_account > 3 * 1000 * 1000) {
        alice.annual_percent += 0.005; // Добавляем 0.5% к ставке
        alice.has_percent_increase = true; // Отмечаем, что ставка повышена
    }
    // Если ставка повышена, но счет упал ниже 3 миллионов
    else if (alice.has_percent_increase && alice.bank_account < 3 * 1000 * 1000) {
        alice.annual_percent -= 0.005; // Убираем бонусную ставку
        alice.has_percent_increase = false; // Отмечаем, что ставка обычная
    }
    
    // Начисляем проценты за месяц (делим годовую ставку на 12 месяцев)
    // Приводим результат к типу RUB (long long int)
    alice.bank_account = (RUB)(alice.bank_account * (1.0 + alice.annual_percent / 12.0));
    
    // Добавляем к счету наличные (кроме тех, что оставляем в кармане)
    alice.bank_account += alice.cash - alice.pocket_cash;
    
    // Оставляем только pocket_cash наличными
    alice.cash = alice.pocket_cash;
}

// Функция для обработки дохода Боба в определенный месяц и год
void bob_income(const int year, const int month) {
    // Проверяем, не июль ли 2027 года (когда Боб получает повышение)
    if (year == 2027 && month == 7) {
        bob.income = bob.income * 13 / 10; // Увеличиваем доход на 30% (умножаем на 1.3)
        printf("Июль 2027: Боб получил повышение! Новый доход: %lld руб./мес\n", bob.income);
    }
    // Добавляем доход к наличным деньгам
    bob.cash += bob.income;
}

// Функция для обработки арендной платы Боба
void bob_rent(const int year, const int month) {
    // Проверяем, не август ли 2045 года (когда Боб покупает дом)
    if (year == 2045 && month == 8) {
        bob.bank_account -= bob.house_cost; // Снимаем 12 миллионов за дом
        printf("Август 2045: Боб купил дом за %lld руб.\n", bob.house_cost);
    }
    else {
        // В остальные месяцы платим аренду из наличных
        bob.cash -= bob.rent_spending;
    }
}

// Функция для обработки прочих расходов Боба
void bob_spending(const int month) {
    // Начинаем с базовой суммы расходов
    RUB spending = bob.spending;
    
    // В феврале тратим меньше
    if (month == 2) {
        spending -= 7 * 1000; // Уменьшаем на 7000 рублей
    }
    
    // В июле тратим больше
    if (month == 7) {
        spending += 5 * 1000; // Увеличиваем на 5000 рублей
    }
    
    // В августе тоже тратим больше
    if (month == 8) {
        spending += 2 * 1000; // Увеличиваем на 2000 рублей
    }
    
    // Вычитаем расходы из наличных денег
    bob.cash -= spending;
}

// Функция для расходов на еду Боба
void bob_food() {
    // Просто вычитаем фиксированную сумму на еду
    bob.cash -= bob.food_spending;
}

// Функция для расходов на машину Боба
void bob_car(const int year, const int month) {
    // Проверяем, не июль ли 2034 года (когда происходит авария)
    if (year == 2034 && month == 7) {
        bob.bank_account -= bob.car_accident_cost; // Снимаем деньги на ремонт со счета
        printf("Июль 2034: У Боба проишествие! Ремонт обошелся в %lld руб.\n", bob.car_accident_cost);
    }
    // Ежемесячно платим за обслуживание машины из наличных
    bob.cash -= bob.car_spending;
}

// Функция для отпускных расходов Боба
void bob_trip(const int month) {
    // Проверяем, не июль ли (месяц отпуска)
    if (month == 7) {
        // Снимаем деньги на отпуск со счета (не из наличных)
        bob.bank_account -= bob.trip_spending;
    }
}

// Функция для управления вкладом Боба
void bob_deposit() {
    // Проверяем, если счет больше 3 миллионов и нет повышенной ставки
    if (!bob.has_percent_increase && bob.bank_account > 3 * 1000 * 1000) {
        bob.annual_percent += 0.005; // Добавляем 0.5% к ставке
        bob.has_percent_increase = true; // Отмечаем, что ставка повышена
    }
    // Если ставка повышена, но счет упал ниже 3 миллионов
    else if (bob.has_percent_increase && bob.bank_account < 3 * 1000 * 1000) {
        bob.annual_percent -= 0.005; // Убираем бонусную ставку
        bob.has_percent_increase = false; // Отмечаем, что ставка обычная
    }
    
    // Начисляем проценты за месяц (делим годовую ставку на 12 месяцев)
    // Приводим результат к типу RUB (long long int)
    bob.bank_account = (RUB)(bob.bank_account * (1.0 + bob.annual_percent / 12.0));
    
    // Добавляем к счету наличные (кроме тех, что оставляем в кармане)
    bob.bank_account += bob.cash - bob.pocket_cash;
    
    // Оставляем только pocket_cash наличными
    bob.cash = bob.pocket_cash;
}

// Основная функция симуляции
void simulation() {
    int year = 2025;  // Начинаем с 2025 года
    int month = 9;    // Начинаем с сентября (9-й месяц)
    
    printf("Начало симуляции...\n");
    printf("Сентябрь 2025: Стартовые условия:\n");
    printf("- Алиса: доход 200 000 руб., ипотека 100 000 руб./мес\n");
    printf("- Боб: доход 220 000 руб., аренда 60 000 руб./мес\n");
    printf("Симуляция на 20 лет (сентябрь 2025 - сентябрь 2045)\n\n");
    
    // Цикл выполняется, пока не наступит сентябрь 2045 года
    while (!(year == 2045 && month == 9)) {
        // Обрабатываем финансы Алисы за месяц
        alice_income(year, month);
        alice_mortgage(year, month);
        alice_spending(month);
        alice_food();
        alice_car();
        alice_trip(month);
        alice_deposit();
        
        // Обрабатываем финансы Боба за месяц
        bob_income(year, month);
        bob_rent(year, month);
        bob_spending(month);
        bob_food();
        bob_car(year, month);
        bob_trip(month);
        bob_deposit();
        
        // Переходим к следующему месяцу
        month++;
        
        // Если месяц больше 12 (декабрь), переходим к следующему году
        if (month == 13) {
            year++;
            month = 1; // Начинаем с января
            
            // Каждый год выводим промежуточные результаты (опционально)
            if (year % 5 == 0) { // Каждые 5 лет
                printf("\n--- Промежуточные результаты за %d год ---\n", year);
                printf("Алиса: %lld руб. на счету\n", alice.bank_account);
                printf("Боб: %lld руб. на счету\n", bob.bank_account);
                if (alice.mortgage_closed) {
                    printf("Алиса: ипотека выплачена\n");
                } else {
                    printf("Алиса: ипотека еще не выплачена\n");
                }
            }
        }
    }
    
    printf("\nСимуляция завершена (20 лет прошло)\n");
}

// Главная функция, с которой начинается выполнение программы
int main() {
    // Инициализируем Алису и Боба (задаем начальные условия)
    alice_init();
    bob_init();
    
    // Запускаем симуляцию на 20 лет
    simulation();
    
    // Выводим итоговые результаты
    printf("\n========== ИТОГИ СИМУЛЯЦИИ ==========\n");
    persons_print();
    printf("====================================\n");
    
    // Объясняем, что означает результат
    printf("\nЧто это значит:\n");
    printf("- Указана сумма на банковском счету\n");
    printf("- У Алисы дом уже оплачен (ипотека выплачена)\n");
    printf("- У Боба дом куплен за 12 млн\n");
    
    return 0; // Программа завершилась успешно
}
