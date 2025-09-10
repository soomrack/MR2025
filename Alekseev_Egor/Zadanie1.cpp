#include <iostream>

int main()
{   
    //Общие начальные данные
    const int flatPrice = 7000; //Стоимость квартиры
    int capital = 100; //Начальный капитал после первоначального взноса/платы за аренду
    const int inflation = 0.08; //Величина инфляции

    //Параметры, необходимые для описания жизни А
    const int period = 20; //Срок ипотеки
    const int credRate = 0.24; //Ипотечная ставка
    const int initialPayment = 1500; //Размер первоначального платежа
    const int monthlyPay = 112; //Размер ежемесячного платежа

    //Параметры, необходимые для описания жизни Б
    const int depRate = 0.13; //Ставка по вкладу
    const int monthlyAdd = 52; //Ежемесячное пополнение вклада
    const int rentalCost = 60; //Ежемесячная плата за аренду жилья
    int deposit = initialPayment; //Средства на депозите

    //Траты, общие для А и Б
    const int wage = 150; //Зарплата
    const int food = 28; //Ежемесячные расходы на еду
    const int utilityFee = 6; //Ежемесячная плата за коммунальные услуги
    const int clothes = 7; //ГОДОВЫЕ расходы на одежду
    const int unpredicted = 20; //ГОДОВЫЕ траты на непредвиденные расходы

    int capitalA = capital;
    int capitalB = capital;
    int month = 0;
    while (month<=period*12)
    {   
        deposit = (deposit + monthlyAdd)*(depRate/12);
        capitalA = capitalA + wage - food - utilityFee - (clothes+unpredicted)/12 - monthlyPay;
        capitalB = capitalB + wage - food - utilityFee - (clothes+unpredicted)/12 - rentalCost;
        month++;
    }
    std::cout << "Капитал А через 20 лет: " << capitalA*1000 << " рублей" << std::endl;
    std::cout << "Капитал Б через 20 лет: " << (capitalB - flatPrice)*1000 << " рублей"; 