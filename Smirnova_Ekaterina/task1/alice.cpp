#include "alice.h"

Alice::Alice(const std::vector<float>& expenses)
    : Person("Alice", expenses), mortgage_payment(0), mortgage_months_left(0),
      total_mortgage_paid(0), has_mortgage(false) {
}

void Alice::set_inflation() {
    set_inflation_impl(1.01, 1.02);
}

bool Alice::check_if_can_buy_flat() {
    float required_amount = flat_amount * 0.2;
    if (required_amount <= bank_account && !is_flat_bought) {
        is_flat_bought = true;
        bank_account -= required_amount;
        return true;
    }
    return false;
}

void Alice::set_mortgage(float payment, int years) {
    mortgage_payment = payment;
    mortgage_months_left = years * 12;
    total_mortgage_paid = 0;
    has_mortgage = true;
    expenses.push_back(payment);
}

bool Alice::pay_mortgage() {
    if (mortgage_months_left > 0) {
        mortgage_months_left--;
        total_mortgage_paid += mortgage_payment;
        return true;
    }
    return false;
}

bool Alice::is_mortgage_paid() const {
    return mortgage_months_left == 0;
}

int Alice::get_mortgage_years_left() const {
    return mortgage_months_left / 12;
}

float Alice::get_total_mortgage_paid() const {
    return total_mortgage_paid;
}

bool Alice::get_has_mortgage() const {
    return has_mortgage;
}

void Alice::update_bank_account_with_mortgage() {
    if (has_mortgage && !is_mortgage_paid()) {
        pay_mortgage();
    }
    Person::update_bank_account();
}