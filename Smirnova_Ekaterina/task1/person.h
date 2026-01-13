#ifndef PERSON_H
#define PERSON_H

#include <string>
#include <vector>

class Person {
protected:
    std::string name;
    std::vector<float> expenses;
    float incoming;
    float bank_account;
    float flat_amount;
    bool is_flat_bought;
    
    void set_inflation_impl(float expense_inflation, float flat_inflation);
    
public:
    Person(const std::string& name, const std::vector<float>& expenses);
    virtual ~Person() = default;
    
    void set_incoming(float new_incoming);
    float calculate_monthly_expenses() const;
    virtual void set_inflation() = 0; // Чисто виртуальный метод
    virtual void update_bank_account();
    virtual bool check_if_can_buy_flat() = 0; // Чисто виртуальный метод
    
    bool get_is_flat_bought() const;
    float get_bank_account() const;
    float get_flat_amount() const;
    std::string get_name() const;
};

#endif