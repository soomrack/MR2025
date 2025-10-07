//Once David and Alice argued about benefits of mortgage. 
// Unlike David who had just taken morgage Alice was convinced that it is more profitably to rent accomodation and deposit money in bank. 
// Then they managed to conduct a competition: who will have more money on his bank account after 20 years.

#include <stdio.h>

typedef long long int RUB;

struct Person {
    RUB bank_account;
    RUB income;
    RUB rent_price;
    RUB food_price;
    RUB invested_money;
    RUB mortgage;
    RUB other_spendings;
    RUB flat_price;
    RUB all_spendings;
};

Person alice;
Person david;

void alice_init() {
    alice.bank_account = 1000 * 1000;
    alice.income = 300 * 1000;
    alice.rent_price = 60 * 1000;
    alice.food_price = 25 * 1000;
    alice.other_spendings = 30 * 1000;
    alice.invested_money = alice.invested_money = alice.income - (alice.rent_price + alice.food_price + alice.other_spendings);

};

void david_init() {
    david.bank_account = 500 * 1000;
    david.income = 200 * 1000;
    david.flat_price = 20 * 1000 * 1000;  //david's new flat price
    david.food_price = 25 * 1000;
    david.other_spendings = 30 * 1000;
    david.mortgage = 60 * 1000;
    david.invested_money = david.invested_money = david.income - (david.mortgage + david.food_price + david.other_spendings);

};

void alice_print() {
    printf("Alice bank account = %lld rub.\n", alice.bank_account);
}

void david_print() {
    printf("David bank account = %lld rub.\n", david.bank_account);
}

void david_mortgage(const int current_year, const int current_month) {

    int mortgage_start_year = 2025;
    int mortgage_start_month = 9;
    int mortgage_period = 15;

    int mortgage_years_passed = current_year - mortgage_start_year;
    int mortgage_months_passed = mortgage_years_passed * 12 + current_month;

    if (mortgage_months_passed < mortgage_period * 12) {
        david.bank_account -= david.mortgage; //month mortgage payment
    }
}


void alice_income(const int year, const int month) {
    if (year == 2030 && month == 10) {
        alice.income *= 1.5; //promotion
    }
    alice.bank_account += alice.income;
    if (month == 12) {
        alice.bank_account += alice.income; //gained a bonus at work
    }

}

void david_income(const int year, const int month) {
    if (year == 2030 && month == 10) {
        david.income *= 1.5; //promotion
    }
    david.bank_account += david.income;

}


void inflation(RUB* price, int year, int month) {
    if (month == 12) {
        *price *= 1.08;
    }
}

void alice_rent(int year, int month) {
    alice.bank_account -= alice.rent_price;
    inflation(&alice.rent_price, year, month);
}

void alice_food(int year, int month) {
    alice.bank_account -= alice.food_price;
    if (month == 5) {
        alice.bank_account -= 10000; //she tried to improve her diet(Healthy food is more expensive)
    }
    inflation(&alice.food_price, year, month);
}

void david_food(int year, int month) {
    RUB init_bank_account = david.bank_account;
    david.bank_account -= david.food_price;
    if (month == 5) {
        david.bank_account -= 20000; //he bought a lot of caviar this month
    }
    inflation(&david.food_price, year, month);
}

void alice_deposit(int year, int month) {
    alice.bank_account += alice.invested_money * 0.18;
}

void david_deposit(int year, int month) {
    david.bank_account += david.invested_money * 0.18;
}

//void david_deposit

void david_trip(int year, int month) {
    if (month == 6) {
        david.bank_account -= 250000;  //trip to Turkey or Egypt
    }

    if (year == 2035 && month == 12) {
        david.bank_account -= 600000;   //trip to Maldives
    }
}

void david_other_spendings(int year, int month) {
    david.bank_account -= david.other_spendings;
    if (month == 6) {
        david.bank_account -= 10000;  //donated to charity
    }

    if (year == 2035 && month == 12) {
        david.bank_account -= 250000; //presented a ring to his girlfriend   
    }

    if (year == 2035 && month == 12) {
        david.bank_account -= 200000; //bought a new playstation 7
    }
    inflation(&david.other_spendings, year, month);
}

void alice_other_spendings(int year, int month) {
    alice.bank_account -= alice.other_spendings;
    if (month == 6) {
        alice.bank_account -= 20000;  //donated to charity
    }

    if (year == 2035 && month == 12) {
        alice.bank_account -= 250000; //bought new iphone   
    }

    if (year == 2035 && month == 12) {
        alice.bank_account -= 200000; //bought a new dyson
    }
    inflation(&alice.other_spendings, year, month);
}

void alice_trip(int year, int month) {
    if (month == 6) {
        alice.bank_account -= 250000;  //trip to Turkey or Egypt
    }

    if (year == 2035 && month == 12) {
        alice.bank_account -= 600000;   //trip to Seychelles
    }

    if (year == 2040 && month == 1) {  //Cristmas in Japan
        alice.bank_account -= 800000;
    }
}

void david_flat(int year, int month) {
    if (year == 2045 && month == 8)
        david.bank_account += david.flat_price * 2; //due to inflation
}

void simulation() {

    int year = 2025;
    int month = 9;

    while (!(year == 2045 && month == 9)) {

        david_income(year, month);
        david_mortgage(year, month);
        david_food(year, month);
        david_trip(year, month);
        david_other_spendings(year, month);
        david_deposit(year, month);
        david_flat(year, month);


        alice_income(year, month);
        alice_rent(year, month);
        alice_food(year, month);
        alice_deposit(year, month);
        alice_trip(year, month);
        alice_other_spendings(year, month);


        month++;
        if (month == 13) {
            year++;
            month = 1;
        }
    }
}
int main() {
    alice_init();
    david_init();

    simulation();

    alice_print();
    david_print();

    return 0;
}
