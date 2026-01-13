#ifndef ALICE_H
#define ALICE_H

#include "person.h"

class Alice : public Person {
private:
    float mortgage_payment;
    int mortgage_months_left;
    float total_mortgage_paid;
    bool has_mortgage;
    
public:
    Alice(const std::vector<float>& expenses);
    
    void set_inflation() override;
    bool check_if_can_buy_flat() override;
    
    void set_mortgage(float payment, int years);
    bool pay_mortgage();
    bool is_mortgage_paid() const;
    int get_mortgage_years_left() const;
    float get_total_mortgage_paid() const;
    bool get_has_mortgage() const;
    
    void update_bank_account_with_mortgage();
};

#endif