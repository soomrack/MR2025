#include <iostream>
#include <math.h>
#include <vector>
#include "alice.h"
#include "bob.h"
#include <iomanip>

using namespace std;

float calculate_mortgage_payment(float loan_amount, 
    float annual_rate, int years) 
    {
    float monthly_rate = annual_rate / 12 / 100;
    int months = years * 12;
    return loan_amount * monthly_rate * pow(1 + monthly_rate, months) / (pow(1 + monthly_rate, months) - 1);
}

int main() {
    float food = 20000.0;
    float car = 15000.0;
    float entertainment = 10000.0;
    float rent = 30000.0;
    
    vector<float> alice_expenses = {food, car, 
        entertainment};
    vector<float> bob_expenses = {food, car, 
        entertainment};
    
    cout << fixed << setprecision(2);

    Alice alice(alice_expenses);
    Bob bob(bob_expenses, rent);
    
    float initial_payment = alice.get_flat_amount() * 0.2;
    float mortgage_amount = alice.get_flat_amount() - initial_payment;
    float mortgage_payment = calculate_mortgage_payment(mortgage_amount, 10.0f, 20);

    cout << "Начальные условия:" << endl;
    cout << "Стоимость квартиры: " << alice.get_flat_amount() << " руб." << endl;
    cout << "Первоначальный взнос: " << initial_payment << " руб." << endl;
    cout << "Ежемесячный платеж по ипотеке: " << mortgage_payment << " руб." << endl;
    cout << "Аренда Боба: " << rent << " руб." << endl << endl;

    int start_year = 2025;
    int end_year = 2070;
    int alice_buy_year = 0;
    int alice_mortgage_paid_year = 0;
    int bob_buy_year = 0;

    for (int year = start_year; year <= end_year; year++) {
        cout << "Год " << year << ":" << endl;
        
        for (int month = 1; month <= 12; month++) {
            alice.update_bank_account_with_mortgage();
            bob.update_bank_account();
            
            if (!alice.get_has_mortgage()) {
                if (alice.check_if_can_buy_flat()) {
                    alice_buy_year = year;
                    alice.set_mortgage(mortgage_payment, 20);
                    cout << "Алиса купила квартиру в " << year << " году" << endl;
                }
            } else {
                if (alice.is_mortgage_paid() && alice_mortgage_paid_year == 0) {
                    alice_mortgage_paid_year = year;
                }
            }

            if (!bob.get_is_flat_bought()) {
                if (bob.check_if_can_buy_flat()) {
                    bob_buy_year = year;
                    cout << "Боб купил квартиру в " << year << " году" << endl;
                }
            }
        }

        alice.set_inflation();
        bob.set_inflation();

        float income_increase = pow(1.01, year - start_year + 1);
        alice.set_incoming(100000 * income_increase);
        bob.set_incoming(100000 * income_increase);

        cout << "Алиса: " << alice.get_bank_account() << " руб.";
        if (alice.get_has_mortgage() && !alice.is_mortgage_paid()) {
            cout << " (ипотека: " << alice.get_mortgage_years_left() << " лет осталось)";
        }
        cout << endl;
        
        cout << "Боб: " << bob.get_bank_account() << " руб." << endl;

        if (alice.is_mortgage_paid() && bob.get_is_flat_bought()) {
            cout << "Оба купили квартиры" << endl;
            break;
        }
    }

    cout << endl << "Итог:" << endl;
    if (alice_buy_year > 0) {
        cout << "Алиса купила квартиру в: " << alice_buy_year << " году" << endl;
    } else {
        cout << "Алиса не смогла купить квартиру" << endl;
    }
    
    if (alice_mortgage_paid_year > 0) {
        cout << "Алиса выплатила ипотеку в: " << alice_mortgage_paid_year << " году" << endl;
        cout << "Всего выплачено по ипотеке: " << alice.get_total_mortgage_paid() << " руб." << endl;
    } else if (alice.get_has_mortgage()) {
        cout << "Алиса еще выплачивает ипотеку (осталось лет: " << alice.get_mortgage_years_left() << ")" << endl;
    }
    
    if (bob_buy_year > 0) {
        cout << "Боб купил квартиру в: " << bob_buy_year << " году" << endl;
    } else {
        cout << "Боб не смог купить квартиру" << endl;
    }
    
    cout << "Сбережения Алисы: " << alice.get_bank_account() << " руб." << endl;
    cout << "Сбережения Боба: " << bob.get_bank_account() << " руб." << endl;

    return 0;
}