#include <iostream>
#include <iomanip>
#include <cmath>
#include <vector>

using namespace std;

class FinancialSimulator {
private:
    
    double initialApartmentPrice;    
    double downPaymentPercent;       
    double mortgageRate;             
    double investmentReturn;         
    double apartmentGrowthRate;      
    int years;                       

    double bobRent;                  
    double aliceMaintenance;         

    double bobSalary;                
    double aliceSalary;              

public:
    FinancialSimulator(double price, double downPayment, double mortgage,
        double investment, double growth, int years,
        double rent, double maintenance, double bobSal, double aliceSal) {
        initialApartmentPrice = price;
        downPaymentPercent = downPayment;
        mortgageRate = mortgage;
        investmentReturn = investment;
        apartmentGrowthRate = growth;
        this->years = years;
        bobRent = rent;
        aliceMaintenance = maintenance;
        bobSalary = bobSal;
        aliceSalary = aliceSal;
    }

    
    double calculateMonthlyPayment(double principal, double annualRate, int years) {
        double monthlyRate = annualRate / 12 / 100;
        int months = years * 12;
        return principal * monthlyRate * pow(1 + monthlyRate, months) /
            (pow(1 + monthlyRate, months) - 1);
    }

    
    void simulateAlice() {
        

        double downPayment = initialApartmentPrice * downPaymentPercent / 100;
        double mortgageAmount = initialApartmentPrice - downPayment;
        double monthlyPayment = calculateMonthlyPayment(mortgageAmount, mortgageRate, years);
        double totalPaid = downPayment;
        double currentApartmentPrice = initialApartmentPrice;
        double remainingDebt = mortgageAmount;

        for (int year = 1; year <= years; year++) {
            
            currentApartmentPrice *= (1 + apartmentGrowthRate / 100);

            double yearlyPayment = monthlyPayment * 12;
            totalPaid += yearlyPayment + aliceMaintenance * 12;

            double yearlyInterest = remainingDebt * mortgageRate / 100;
            double yearlyPrincipal = yearlyPayment - yearlyInterest;
            remainingDebt -= yearlyPrincipal;

            if (remainingDebt < 0) remainingDebt = 0;

            double netWorth = currentApartmentPrice - remainingDebt;

        }

    }

    
    void simulateBob() {
       

        double currentSavings = 0;
        double currentApartmentPrice = initialApartmentPrice;
        double monthlySavings = (bobSalary - bobRent) * 0.3; 

        double totalRentPaid = 0;
        for (int year = 1; year <= years; year++) {
            
            currentApartmentPrice *= (1 + apartmentGrowthRate / 100);
            double yearlySavings = monthlySavings * 12;
            double investmentIncome = currentSavings * investmentReturn / 100;
            currentSavings += yearlySavings + investmentIncome;
            totalRentPaid += bobRent * 12;

        }

    }

    
    void compareStrategiesDetailed() {
        
        double finalApartmentPrice = initialApartmentPrice * pow(1 + apartmentGrowthRate / 100, years);

        double downPayment = initialApartmentPrice * downPaymentPercent / 100;
        double mortgageAmount = initialApartmentPrice - downPayment;
        double monthlyPayment = calculateMonthlyPayment(mortgageAmount, mortgageRate, years);
        double aliceTotalPaid = downPayment + (monthlyPayment * 12 * years) + (aliceMaintenance * 12 * years);
        double aliceNetWorth = finalApartmentPrice;
        double aliceFinancialResult = aliceNetWorth - aliceTotalPaid;

        double monthlySavings = (bobSalary - bobRent) * 0.3;
        double bobNetWorth = 0;
        double yearlySavings = monthlySavings * 12;

        for (int year = 0; year < years; year++) {
            bobNetWorth += yearlySavings;
            bobNetWorth *= (1 + investmentReturn / 100);
        }

        double bobTotalRent = bobRent * 12 * years;
        double bobFinancialResult = bobNetWorth - bobTotalRent;

        double netWorthDifference = aliceNetWorth - bobNetWorth;
        double financialResultDifference = aliceFinancialResult - bobFinancialResult;
        double totalCostDifference = aliceTotalPaid - bobTotalRent;

        double aliceCurrentPrice = initialApartmentPrice;
        double aliceCurrentDebt = mortgageAmount;
        double bobCurrentSavings = 0;

        for (int year = 1; year <= years; year++) {
            
            aliceCurrentPrice *= (1 + apartmentGrowthRate / 100);

            
            double yearlyPayment = monthlyPayment * 12;
            double yearlyInterest = aliceCurrentDebt * mortgageRate / 100;
            double yearlyPrincipal = yearlyPayment - yearlyInterest;
            aliceCurrentDebt -= yearlyPrincipal;
            if (aliceCurrentDebt < 0) aliceCurrentDebt = 0;

            double aliceCurrentNetWorth = aliceCurrentPrice - aliceCurrentDebt;

            double bobYearlySavings = monthlySavings * 12;
            double bobInvestmentIncome = bobCurrentSavings * investmentReturn / 100;
            bobCurrentSavings += bobYearlySavings + bobInvestmentIncome;

            double difference = aliceCurrentNetWorth - bobCurrentSavings;

           
        }

        cout << "\nFINAL CONCLUSION:" << endl;
        if (financialResultDifference > 0) {
            cout << "Alice's mortgage strategy is better by "
                << setprecision(3) << fixed << financialResultDifference << " RUB" << endl;
            cout << "This represents a " << (financialResultDifference / abs(bobFinancialResult)) * 100
                << "% advantage over Bob's strategy" << endl;
        }
        else {
            cout << "Bob's saving strategy is better by "
                << setprecision(3) << fixed << - financialResultDifference  << " RUB" << endl;
            cout << "This represents a " << (-financialResultDifference / abs(aliceFinancialResult)) * 100
                << "% advantage over Alice's strategy" << endl;
        }
    }

    
    void runCompleteAnalysis() {
        simulateAlice();
        simulateBob();
        compareStrategiesDetailed();
    }
};

int main() {
    
    double apartmentPrice = 10000000;      
    double downPayment = 20;               
    double mortgageRate = 15.0;            
    double investmentReturn = 12.0;        
    double apartmentGrowth = 10.0;         
    int simulationYears = 10;              

    double bobRent = 40000;                
    double aliceMaintenance = 10000;       
    double bobSalary = 150000;             
    double aliceSalary = 150000;           

    FinancialSimulator simulator(apartmentPrice, downPayment, mortgageRate,
        investmentReturn, apartmentGrowth, simulationYears,
        bobRent, aliceMaintenance, bobSalary, aliceSalary);

    simulator.runCompleteAnalysis();

    return 0;
}