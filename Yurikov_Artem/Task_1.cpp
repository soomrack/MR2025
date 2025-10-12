#include <stdio.h>
#include <stdbool.h>

typedef long long int CURRENCY;

struct Residence {
    CURRENCY property_value;
    CURRENCY loan_balance;
    CURRENCY initial_payment;
    CURRENCY monthly_rent;
};

struct Vehicle {
    CURRENCY vehicle_cost;
    CURRENCY fuel_cost;
    CURRENCY maintenance;
    CURRENCY coverage_cost;
};

struct Individual {
    CURRENCY savings;
    CURRENCY salary;
    
    Residence housing;
    
    CURRENCY groceries;
    CURRENCY apparel;
    CURRENCY vacation;
    
    Vehicle automobile;
};

Individual person_a;
Individual person_b;

void initialize_person_a() {
    person_a.savings = 20000000;
    person_a.salary = 1350000;

    person_a.housing.property_value = 70000000;
    person_a.housing.initial_payment = 20000000;
    person_a.housing.loan_balance = 50000000;
    
    person_a.groceries = 30000;
    person_a.apparel = 20000;
    person_a.vacation = 160000;

    person_a.automobile.vehicle_cost = 1500000;
    person_a.automobile.fuel_cost = 15000;
    person_a.automobile.maintenance = 7000;
    person_a.automobile.coverage_cost = 30000;
}

void initialize_person_b() {
    person_b.savings = 30000000;
    person_b.salary = 1650000;

    person_b.housing.monthly_rent = 45000;

    person_b.groceries = 30000;
    person_b.apparel = 10000;
    person_b.vacation = 180000;

    person_b.automobile.vehicle_cost = 2500000;
    person_b.automobile.fuel_cost = 25000;
    person_b.automobile.maintenance = 10000;
    person_b.automobile.coverage_cost = 7000;
}

void display_person_a() {
    printf("Person A savings = %lld руб.\n", person_a.savings);
    printf("Person A property value = %lld руб.\n", person_a.housing.property_value);
    printf("Person A vehicle cost = %lld руб.\n", person_a.automobile.vehicle_cost);
    printf("\n");
}

void display_person_b() {
    printf("Person B savings = %lld руб.\n", person_b.savings);
    printf("Person B vehicle cost = %lld руб.\n", person_b.automobile.vehicle_cost);
}

void process_income_a(int current_year, int current_month) {
    if (current_month == 10) {
        person_a.salary = (person_a.salary * 105) / 100;
    }
    if (current_year == 2035 && current_month == 10) {
        person_a.salary = (person_a.salary * 135) / 100;
    }
    person_a.savings += person_a.salary;
}

void process_income_b(int current_year, int current_month) {
    if (current_month == 10) {
        person_b.salary = (person_b.salary * 105) / 100;
    }
    if ((current_year == 2030 && current_month == 10) || 
        (current_year == 2040 && current_month == 10)) {
        person_b.salary = (person_b.salary * 125) / 100;
    }
    person_b.savings += person_b.salary;
}

void process_expenses_a(int current_year, int current_month) {
    if (current_month == 1) {
        person_a.groceries = (person_a.groceries * 107) / 100;
        person_a.apparel = (person_a.apparel * 107) / 100;
    }
    person_a.savings -= person_a.groceries;
    person_a.savings -= person_a.apparel;
}

void process_expenses_b(int current_year, int current_month) {
    if (current_month == 1) {
        person_b.groceries = (person_b.groceries * 107) / 100;
        person_b.apparel = (person_b.apparel * 107) / 100;
        person_b.housing.monthly_rent = (person_b.housing.monthly_rent * 107) / 100;
    }
    person_b.savings -= person_b.groceries;
    person_b.savings -= person_b.apparel;
}

void process_vacation_a(int current_month) {
    if ((current_month == 8) && (person_a.savings > person_a.vacation + 1000000)) {
        person_a.savings -= person_a.vacation;
    }
    if (current_month == 1) {
        person_a.vacation = (person_a.vacation * 107) / 100;
    }
}

void process_vacation_b(int current_month) {
    if ((current_month == 6 || current_month == 12) && 
        (person_b.savings > person_b.vacation + 1000000)) {
        person_b.savings -= person_b.vacation;
    }
    if (current_month == 1) {
        person_b.vacation = (person_b.vacation * 107) / 100;
    }
}

void process_vehicle_a(int current_year, int current_month) {
    static bool accident_occurred = false;
    static CURRENCY accident_cost = 1500000;
    
    if (current_year % 5 == 0 && current_month == 12) {
        accident_occurred = true;
    }
    
    if (accident_occurred && person_a.savings >= accident_cost + 200000) {
        person_a.savings -= accident_cost;
        accident_cost = (accident_cost * 120) / 100;
        accident_occurred = false;
    }

    if (current_month == 1) {
        person_a.automobile.vehicle_cost = (person_a.automobile.vehicle_cost * 92 * 107) / 10000;
        person_a.automobile.fuel_cost = (person_a.automobile.fuel_cost * 107) / 100;
        person_a.automobile.maintenance = (person_a.automobile.maintenance * 107) / 100;
        person_a.savings -= person_a.automobile.coverage_cost;
    }

    static char vehicle_upgrade_count = 0;
    if (current_year >= 2036 && vehicle_upgrade_count == 0 && 
        person_a.savings >= 16000000) {
        person_a.automobile.vehicle_cost += 15000000;
        person_a.savings -= 15000000;
        vehicle_upgrade_count++;
    }
    
    if (!accident_occurred) {
        person_a.savings -= person_a.automobile.fuel_cost;
        person_a.savings -= person_a.automobile.maintenance;
    } else {
        person_a.savings -= person_a.automobile.fuel_cost / 3;
    }
}

void process_vehicle_b(int current_year, int current_month) {
    if (current_month == 1) {
        person_b.automobile.vehicle_cost = (person_b.automobile.vehicle_cost * 95 * 107) / 10000;
        person_b.automobile.fuel_cost = (person_b.automobile.fuel_cost * 107) / 100;
        person_b.automobile.maintenance = (person_b.automobile.maintenance * 107) / 100;
        person_b.savings -= person_b.automobile.coverage_cost;
    }

    static char vehicle_upgrade_count = 0;
    if ((current_year >= 2031 && vehicle_upgrade_count == 0 && 
         person_b.savings > 11000000) ||
        (current_year >= 2041 && vehicle_upgrade_count == 1 && 
         person_b.savings > 11000000)) {
        person_b.automobile.vehicle_cost += 10000000;
        person_b.savings -= 10000000;
        vehicle_upgrade_count++;
    }

    person_b.savings -= person_b.automobile.fuel_cost;
    person_b.savings -= person_b.automobile.maintenance;
}

void process_housing_a(int current_month) {
    CURRENCY monthly_payment = 550540;
    person_a.savings -= monthly_payment;
    if (current_month == 1) {
        person_a.housing.property_value = (person_a.housing.property_value * 107) / 100;
    }
}

void process_housing_b(int current_year, int current_month) {
    if ((current_year == 2030 && current_month == 11) || 
        (current_year == 2040 && current_month == 11)) {
        person_b.housing.monthly_rent = (person_b.housing.monthly_rent * 125) / 100;
    }
    person_b.savings -= person_b.housing.monthly_rent;
}

void apply_interest_a() {
    person_a.savings = (person_a.savings * 1005) / 1000;
}

void apply_interest_b() {
    person_b.savings = (person_b.savings * 1005) / 1000;
}

void verify_funds(int current_year, int current_month) {
    if (person_a.savings < 0 || person_b.savings < 0) {
        printf("%d, %d \n", current_year, current_month);
        printf("Person A savings = %lld \n", person_a.savings);
        printf("Person B savings = %lld \n", person_b.savings);
        printf("\n");
    }
}

void run_simulation() {
    int current_year = 2025;
    int current_month = 9;

    while (!(current_year == 2045 && current_month == 9)) {
        process_income_a(current_year, current_month);
        process_housing_a(current_month);
        process_expenses_a(current_year, current_month);
        process_vacation_a(current_month);
        process_vehicle_a(current_year, current_month);
        apply_interest_a();

        process_income_b(current_year, current_month);
        process_housing_b(current_year, current_month);
        process_expenses_b(current_year, current_month);
        process_vacation_b(current_month);
        process_vehicle_b(current_year, current_month);
        apply_interest_b();

        verify_funds(current_year, current_month);

        current_month++;
        if (current_month > 12) {
            current_year++;
            current_month = 1;
        }
    }
}

int main() {
    initialize_person_a();
    initialize_person_b();
    
    run_simulation();

    display_person_a();
    display_person_b();
    
    return 0;
}
