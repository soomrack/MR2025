#include "person.h"
#include <iostream>

Person::Person(const std::string& name, const std::vector<float>& expenses)
    : name(name), expenses(expenses), incoming(100000), bank_account(0), 
      flat_amount(5000000), is_flat_bought(false) {
}

void Person::set_incoming(float new_incoming) {
    incoming = new_incoming;
}

float Person::calculate_monthly_expenses() const {
    float total = 0;
    for (const auto& expense : expenses) {
        total += expense;
    }
    return total;
}

void Person::set_inflation_impl(float expense_inflation, float flat_inflation) {
    for (auto& expense : expenses) {
        expense *= expense_inflation;
    }
    flat_amount *= flat_inflation;
}

void Person::update_bank_account() {
    float monthly_expenses = calculate_monthly_expenses();
    bank_account += (incoming - monthly_expenses);
}

bool Person::get_is_flat_bought() const {
    return is_flat_bought;
}

float Person::get_bank_account() const {
    return bank_account;
}

float Person::get_flat_amount() const {
    return flat_amount;
}

std::string Person::get_name() const {
    return name;
}