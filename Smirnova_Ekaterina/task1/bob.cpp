#include "bob.h"

Bob::Bob(const std::vector<float>& expenses, float rent)
    : Person("Bob", expenses), rent(rent) {
    this->expenses.push_back(rent);
}

void Bob::set_inflation() {
    set_inflation_impl(1.01, 1.02);
    rent *= 1.01;
}

bool Bob::check_if_can_buy_flat() {
    if (flat_amount <= bank_account && !is_flat_bought) {
        is_flat_bought = true;
        bank_account -= flat_amount;
        return true;
    }
    return false;
}

float Bob::get_rent() const {
    return rent;
}