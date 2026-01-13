#include <stdio.h>
#include <stdlib.h>

/* Global define initial data */
#define INITIAL_MONEY (1000 * 1000)
#define ALICE_INCOME (200 * 1000)
#define BOB_INCOME (199 * 1000)
#define SEMEN_INCOME (228 * 100)

typedef long int RUB;

typedef enum {
    BANK_TINKOFF,
    BANK_SBER
} BankType;

typedef struct {
    RUB money;
    RUB income;
    RUB rent;
    RUB food;
    RUB trip;
    RUB car_cost;
    RUB car_expense;
    RUB mortgage_payment;
    RUB flat_price_mortgage;
    
    RUB pocket_money;
    RUB medical_expense;
    RUB sber_balance;
    RUB tinkoff_balance;
    RUB total_capital;
} Person;


void person_init(Person *person);
void simulation(void);
void print_results(void);

Person alice;
Person bob;

/* Person init */

void person_init(Person *person) {
    person->money = INITIAL_MONEY;
    person->income = 0;
    person->rent = 0;
    person->food = 0;
    person->trip = 0;
    person->car_cost = 0;
    person->car_expense = 0;
    person->mortgage_payment = 0;
    person->flat_price_mortgage = 0;
    person->pocket_money = 0;
    person->medical_expense = 0;
    person->sber_balance = 0;
    person->tinkoff_balance = 0;
    person->total_capital = 0;
}

/* Person init */

void alice_init(void) {
    person_init(&alice);
    alice.income = ALICE_INCOME;
    alice.rent = 75 * 1000;
    alice.food = 30000;
    alice.trip = 150 * 1000;
    alice.car_cost = 5000 * 1000;
    alice.car_expense = 20 * 1000;
}

void bob_init(void) {
    person_init(&bob);
    bob.income = BOB_INCOME;
    bob.flat_price_mortgage = 25000 * 1000;
    bob.food = 25000;
    bob.trip = 150000;
    bob.mortgage_payment = 115000;
}

void semen_init(void) {
    person_init(&semen);
    semen.income = SEMEN_INCOME;
    semen.rent = 75 * 1000;
    semen.food = 20000;
    semen.trip = 150 * 1000;
}


/* Utility functions */

RUB rand_range(RUB min, RUB max) {
    if (max <= min) return min;
    return min + rand() % (max - min + 1);
}

void random_pocket_spend(Person *person) {
    RUB spend = rand_range(1000, 10000);
    if (spend > person->pocket_money) {
        spend = person->pocket_money;
    }
    person->pocket_money -= spend;
    person->money -= spend;
}

void random_medical_expense(Person *person) {
    int chance = rand() % 100;
    if (chance < 15) {
        RUB med = rand_range(5000, 50000);
        if (med > person->money) {
            med = person->money;
        }
        person->money -= med;
        person->medical_expense = med;
    } else {
        person->medical_expense = 0;
    }
}

void refill_pocket_money(Person *person) {
    RUB refill = person->income * 0.01;
    if (refill > person->money) {
        refill = person->money;
    }
    person->money -= refill;
    person->pocket_money += refill;
}

/* Bank operation */

void deposit_to_bank(Person *person, RUB amount, BankType bank) {
    if (person->money < amount) {
        amount = person->money;
    }
    person->money -= amount;
    
    if (bank == BANK_TINKOFF) {
        person->tinkoff_balance += amount;
    } else {
        person->sber_balance += amount;
    }
}

void apply_interest(RUB *balance, double rate) {
    double monthly_rate = rate / 12.0;
    *balance += (RUB)(*balance * monthly_rate);
}

/* Alice operations */

void alice_income(int year, int month) {
    if (year == 2030 && month == 10) {
        alice.income = (RUB)(alice.income * 1.5);
    }
    alice.money += alice.income;
}

void alice_rent(int year, int month) {
    if (year > 2025 && month == 1) {
        alice.rent = (RUB)(alice.rent * 1.05);
    }
    alice.money -= alice.rent;
}

void alice_food(int year, int month) {
    if (year > 2025 && month == 1) {
        alice.food = (RUB)(alice.food * 1.03);
    }
    alice.money -= alice.food;
}

void alice_trip(int year, int month) {
    if (month == 8) {
        alice.money -= alice.trip;
    }
}

void alice_car(int year, int month) {
    if (month == 5 && year == 2034) {
        alice.money -= alice.car_cost;
    }
    if (year > 2034 || (year == 2034 && month > 5)) {
        alice.money -= alice.car_expense;
    }
}

/* Bob operations */

void bob_income(int year, int month) {
    if (year == 2030 && month == 10) {
        bob.income = (RUB)(bob.income * 1.5);
    }
    bob.money += bob.income;
}

void bob_mortgage(int year, int month) {
    bob.money -= bob.mortgage_payment;
}

void bob_food(int year, int month) {
    if (year > 2025 && month == 1) {
        bob.food = (RUB)(bob.food * 1.03);
    }
    bob.money -= bob.food;
}

void bob_trip(int year, int month) {
    if (month == 8) {
        bob.money -= bob.trip;
    }
}

/* Semen operations */

void semen_income(int year, int month) {
    if (year == 2030 && month == 10) {
        semen.income = (RUB)(semen.income * 1.5);
    }
    semen.money += semen.income;
}

void semen_rent(int year, int month) {
    if (year > 2025 && month == 1) {
        semen.rent = (RUB)(semen.rent * 1.05);
    }
    semen.money -= semen.rent;
}

void semen_food(int year, int month) {
    if (year > 2025 && month == 1) {
        semen.food = (RUB)(semen.food * 1.03);
    }
    semen.money -= semen.food;
}

void semen_trip(int year, int month) {
    if (month == 8) {
        semen.money -= semen.trip;
    }
}


/* Data printing */

void print_person(const Person *person, const char *name, const char *asset) {
    printf("============\n");
    printf("%s money = %lld rub.\n", name, person->money);
    printf("%s Tinkoff deposit = %lld rub.\n", name, person->tinkoff_balance);
    printf("%s Sberbank deposit = %lld rub.\n", name, person->sber_balance);
    printf("%s has %s.\n", name, asset);
    printf("============\n");
}

void bob_print(void) {
    print_person(&bob, "Bob", "a flat");
}

void alice_print(void) {
    print_person(&alice, "Alice", "a car");
}

void semen_print(void) {
	print_person(&semen, "Semen", "a flat");
}

void comparing_print(void) {
    bob.total_capital = bob.money + bob.tinkoff_balance + bob.sber_balance + bob.flat_price_mortgage;
    alice.total_capital = alice.money + alice.tinkoff_balance + alice.sber_balance + alice.car_cost;
    semen.total_capital = semen.money + semen.tinkoff_balance + semen.sber_balance + semen.flat_price_mortgage;
    
    printf("Bob's total capital: %d rub.\n", bob.total_capital);
    printf("Alice's total capital: %d rub.\n", alice.total_capital);
    printf("Semen's total capital: %d rub.\n", semen.total_capital);
    

    struct {
        const char *name;
        RUB capital;
    } people[3] = {
        {"Alice", alice.total_capital},
        {"Bob", bob.total_capital},
        {"Semen", semen.total_capital}
    };
    

    for (int i = 0; i < 2; i++) {
        for (int j = i + 1; j < 3; j++) {
            if (people[i].capital < people[j].capital) {

                const char *temp_name = people[i].name;
                RUB temp_capital = people[i].capital;
                
                people[i].name = people[j].name;
                people[i].capital = people[j].capital;
                
                people[j].name = temp_name;
                people[j].capital = temp_capital;
            }
        }
    }
    

    const char *best = people[0].name;
    const char *worst = people[2].name;
    
    
    if (people[0].capital == people[1].capital && people[1].capital == people[2].capital) {
        printf("All have the same capital - equal life!\n");
    }
    else if (people[0].capital == people[1].capital) {
        printf("%s and %s share the best life, and %s has the worst\n", 
               people[0].name, people[1].name, worst);
    }
    else if (people[1].capital == people[2].capital) {
        printf("%s has the best life, and %s and %s share the worst\n", 
               best, people[1].name, people[2].name);
    }
    else {
        printf("%s has the best life, and %s has the worst\n", best, worst);
    }
}

/* Simulation */

void simulation(void) {
    int year = 2025;
    int month = 9;

    while (!(year == 2045 && month == 9)) {
        /* Alice */
        alice_income(year, month);
        alice_rent(year, month);
        alice_food(year, month);
        alice_car(year, month);
        alice_trip(year, month);
        refill_pocket_money(&alice);
        random_pocket_spend(&alice);
        random_medical_expense(&alice);
        deposit_to_bank(&alice, alice.income * 0.1, BANK_TINKOFF);
        deposit_to_bank(&alice, alice.income * 0.1, BANK_SBER);
        apply_interest(&alice.tinkoff_balance, 0.03);
        apply_interest(&alice.sber_balance, 0.02);

        /* Bob */
        bob_income(year, month);
        bob_mortgage(year, month);
        bob_food(year, month);
        bob_trip(year, month);
        refill_pocket_money(&bob);
        random_pocket_spend(&bob);
        random_medical_expense(&bob);
        deposit_to_bank(&bob, bob.income * 0.2, BANK_TINKOFF);
        deposit_to_bank(&bob, bob.income * 0.1, BANK_SBER);
        apply_interest(&bob.tinkoff_balance, 0.02);
        apply_interest(&bob.sber_balance, 0.02);
		
		/* Semen */
		semen_income(year, month);
        semen_rent(year, month);
        semen_food(year, month);
        semen_trip(year, month);
        refill_pocket_money(&semen);
        random_pocket_spend(&semen);
        random_medical_expense(&semen);
        deposit_to_bank(&semen, semen.income * 0.1, BANK_SBER);
        apply_interest(&semen.sber_balance, 0.02);
		
        month++;
        if (month == 13) {
            year++;
            month = 1;
        }
    }
}

int main(void) {
    srand(1239);

    alice_init();
    bob_init();
	semen_init();
	
    simulation();

    bob_print();
    alice_print();
    comparing_print();
    
    return 0;
}