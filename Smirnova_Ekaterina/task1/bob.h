#ifndef BOB_H
#define BOB_H

#include "person.h"

class Bob : public Person {
private:
    float rent;
    
public:
    Bob(const std::vector<float>& expenses, float rent);
    
    void set_inflation() override;
    bool check_if_can_buy_flat() override;
    
    float get_rent() const;
};

#endif