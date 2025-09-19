#include <iostream>

#define nums_of_persons 2

typedef long long int RUB;

struct Person{
    std::string name;
    RUB bank_account;
    RUB income;
    RUB food;
    RUB arenda;
    RUB car;
    RUB trip;
    RUB credit;
};

struct Person persons[nums_of_persons];


void struct_ini(int name1, std::string name2);
void struct_print(std::string name2);
void delta_print(std::string name1, std::string name2);
void person_income(std::string name2, const int year, const int month);
void person_food(const std::string name2, const int month);
void person_car(const std::string name2);
void person_arenda(const std::string name2, const int month);
void person_credit(const std::string name2, const int month);
void simulation(void);

void Bob_ini(int name1, std::string name2);
void Alice_ini(int name1, std::string name2);


int main(void){

  Bob_ini(0, "Bob");
  Alice_ini(1, "Alice");

  simulation();

  struct_print("Bob");
  struct_print("Alice");
  delta_print("Bob", "Alice");
}


int _get_id_from_name(std::string name2){
  for(int i={nums_of_persons-1}; i>=0; i--){
    if(persons[i].name == name2){return i;}
  }
  return -1;
}


void Bob_ini(int name1, std::string name2){
  persons[name1].name = name2;
  persons[name1].bank_account = 1000*1000;
  persons[name1].income = 200*1000;
  persons[name1].food = 40*1000;
  persons[name1].car = 20*1000;
  persons[name1].arenda = 80*1000;
}


void Alice_ini(int name1, std::string name2){
  persons[name1].name = name2;
  persons[name1].bank_account = 1000*1000;
  persons[name1].income = 200*1000;
  persons[name1].food = 40*1000;
  persons[name1].car = 20*1000;
  persons[name1].credit = 75*1000;
}


void struct_print(std::string name2){
  int name1 = _get_id_from_name(name2);
  std::cout << persons[name1].name;
  std::cout << " bank account = ";
  std::cout << persons[name1].bank_account << " RUB" << std::endl;
}

void delta_print(std::string name1, std::string name2){
  int name3= _get_id_from_name(name1);
  int name4 = _get_id_from_name(name2);
  std::cout << persons[name3].bank_account - persons[name4].bank_account;
  std::cout << std::endl;
}

void person_income(std::string name2, const int year, const int month){
  int name1 = _get_id_from_name(name2);
  if(year == 2030 && month == 10){
    persons[name1].income *= 1.5;
  }
  persons[name1].bank_account += persons[name1].income;
}


void person_food(const std::string name2, const int month){
  int name1 = _get_id_from_name(name2);
  persons[name1].bank_account -= persons[name1].food;
  if ( month == 12){persons[name1].bank_account -= persons[name1].food;}
}


void person_car(const std::string name2){
  int name1 = _get_id_from_name(name2);
  persons[name1].bank_account -= persons[name1].car;
}


void person_arenda(const std::string name2, const int month){
  int name1 = _get_id_from_name(name2);
  if(month == 7){persons[name1].arenda*=1.02;}
  persons[name1].bank_account -= persons[name1].arenda;

}


void person_trip(std::string name2, const int month){
  int name1 = _get_id_from_name(name2);
  persons[name1].bank_account -= persons[name1].trip;
  if(month == 2){
    persons[name1].bank_account -= persons[name1].trip;
  }
}


void person_credit(const std::string name2, const int month){
  static int flag = 0;
  int name1 = _get_id_from_name(name2);
  if(flag == 0){
    persons[name1].bank_account -= 2.5*1000*1000;
    flag += 1;
  }
  persons[name1].bank_account -= persons[name1].credit;
}


void simulation(void){
  int year={2025};
  int month={9};

  while (!(year==2045 && month ==9)){
    person_income("Bob",year,month);
    person_food("Bob",month);
    person_car("Bob");
    person_trip("Bob", month);
    person_arenda("Bob", month);

    person_income("Alice",year,month);
    person_food("Alice",month);
    person_car("Alice");
    person_trip("Alice", month);   
    person_credit("Alice", month);

    if (++month == 13){
      year++;
      month = 1;
    }
  }
}
