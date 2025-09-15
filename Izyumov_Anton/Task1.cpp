#include <stdio.h>
#include <string.h>


typedef long long int RUB;


typedef struct {
    const char* name;
    RUB bank_account;
    RUB income;
    RUB spending;
    RUB car_spending;
    RUB food_spending;
    RUB mortgage_spending;
    RUB rent_spending;
    int has_mortgage;
    int mortgage_closed;
} Person;


Person alice{
    "Alice",
    3 * 1000 * 1000, // start bank account
    200 * 1000,      // income
    50 * 1000,       // spending
    40 * 1000,       // car spending
    40 * 1000,       // food spending
    100 * 1000,      // mortgage month spending
    0,               // rent spending
    1,               // has mortgage
    0                // mortgage not closed
};


Person bob{
    "Bob",
    3 * 1000 * 1000, // the same start
    200 * 1000,
    50 * 1000,
    40 * 1000,
    40 * 1000,
    0,               // has no mortgage spending
    60 * 1000,       // rent spending
    0,               // has no mortgage
    0
};


Person* find_richest(Person* persons, const int n) {
    if (n <= 0) return NULL;

    Person* richest = &persons[0];
    for (int i = 1; i < n; i++) {
        if (persons[i].bank_account > richest->bank_account) {
            richest = &persons[i];
        }
    }
    return richest;
}


void persons_print(Person* persons, const int n) {
    for (int i = 0; i < n; i++) {
        printf("%s bank account = %lld rub.\n", persons[i].name, persons[i].bank_account);
    }
    Person* richest = find_richest(persons, n);
    if (richest != NULL) {
        printf("%s is in the best situation wtih %lld rub.\n", richest->name, richest->bank_account);
    }
}


void person_income(Person* p, const int year, const int month) {
    if (strcmp(p->name, "Alice") == 0 && year == 2030 && month == 10) {
        p->income *= 1.5; // Alice promotion
    }
    if (strcmp(p->name, "Bob") == 0 && year == 2027 && month == 7) {
        p->income *= 1.3; // Bob promotion
    }
    p->bank_account += p->income;
}


void person_housing(Person* p, const int year, const int month) {
    if (p->has_mortgage && !p->mortgage_closed) {
        if (year == 2025 && month == 9) {
            p->bank_account -= 2 * 1000 * 1000; // Alice initial payment
        }
        else {
            p->bank_account -= p->mortgage_spending;
        }
        if (year == 2045 && month == 9) {
            p->mortgage_closed = 1;
        }
    }
    else if (p->rent_spending > 0) {
        p->bank_account -= p->rent_spending;
    }
}


void person_spending(Person* p, const int month) {
    RUB spending = p->spending;
    if (month == 2) {
        spending -= 7 * 1000;
    }
    if (month == 7) {
        spending += 5 * 1000;
    }
    if (month == 8) {
        spending += 2 * 1000;
    }
    p->bank_account -= spending;
}


void person_food(Person* p) {
    p->bank_account -= p->food_spending;
}


void person_car(Person* p, const int year, const int month) {
    if (strcmp(p->name, "Bob") == 0 && year == 2034 && month == 7) {
        p->bank_account -= 100 * 1000; // Bob's car accident
    }
    p->bank_account -= p->car_spending;
}


void person_trip(Person* p, const int month) {
    if (month == 7) {
        p->bank_account -= 60 * 1000;
    }
}


void simulation(Person* persons, const int n) {
    int year = 2025;
    int month = 9;

    while (!(year == 2045 && month == 9)) {
        for (int i = 0; i < n; i++) {
            person_income(&persons[i], year, month);
            person_housing(&persons[i], year, month);
            person_spending(&persons[i], month);
            person_food(&persons[i]);
            person_car(&persons[i], year, month);
            person_trip(&persons[i], month);
        }

        month++;
        if (month == 13) {
            year++;
            month = 1;
        }
    }
}


int main() {
    Person persons[] = { alice, bob };
    int n = sizeof(persons) / sizeof(persons[0]);

    simulation(persons, n);

    persons_print(persons, n);
}
