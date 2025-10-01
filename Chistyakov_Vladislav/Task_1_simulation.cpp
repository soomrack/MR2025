#include <iostream>
#include <cmath>
#include <random>

typedef long int RUB;

std::mt19937 mt{ std::random_device{}() };

struct person {
	RUB bank_account;  //���� � �����
	RUB income;        //��������
	RUB mortgage;      //������� ����� �� �������
	RUB investment;    //����� � �����

	RUB food;          //����� �� ���
	RUB rent;          //������ ��������
	RUB utilities;     //����������� ������

	bool HasApartment; //������� ��������
};

struct person alice;   //���� � �������
struct person bob;     //����� �� �������


//������� ������������ ���������� (����� ��� ������ �������������� �����)
void alice_init() {
	alice.bank_account = 0;
	alice.mortgage = 8 * 1000 * 1000;
	alice.income = 70 * 1000;
	alice.food = 20 * 1000;
	alice.utilities = 4 * 1000;
	alice.HasApartment = true;
}

//������� ������������ ���������� (��� ��� ������� ������ �� ������������� ����)
void bob_init() {
	bob.bank_account = 0;
	bob.investment = 2 * 1000 * 1000;
	bob.income = 70 * 1000;
	bob.food = 20 * 1000;
	bob.rent = 30 * 1000;
	bob.utilities = 4 * 1000;
	bob.HasApartment = false;
}

//��������� ��������
RUB apartment_cost = 10 * 1000 * 1000;

//��������
void inflation(const int& month) {
	if (month == 10) {
		apartment_cost *= 1.07;

		alice.food *= 1.07;
		bob.food *= 1.07;

		bob.rent *= 1.07;

		alice.utilities *= 1.07;
		bob.utilities *= 1.07;
	}
}

void alice_income(const int& year, const int& month) {
	if (month == 10) alice.income *= 1.07;                 //���� �������� �� ��������
	if (year == 2030 && month == 3) alice.income *= 1.5;   //��������� � ���������
	alice.bank_account += alice.income;
}

void bob_income(const int& year, const int& month) {
	if (month == 10) bob.income *= 1.07;                   //���� �������� �� ��������
	if (year == 2030 && month == 3) bob.income *= 1.5;     //��������� � ���������
	bob.bank_account += bob.income;
	bob.investment *= 1 + 0.20 / 12;                       //���������� �������� �� ����� � ����� ��� 20% ������� (������ �����)
}

//���������� ������������ ������� �� �������
int mortgage_payment(long sum, double percent, int months) {
	return (sum * (percent / 12) / (1 - pow(1 + percent / 12, -months - 1)));
}

//������������ ������� �� ���������� ��� �����
void alice_costs() {
	alice.bank_account -= alice.food;
	alice.bank_account -= alice.utilities;
	alice.bank_account -= mortgage_payment(alice.mortgage, 0.06, 12 * 20);
	//����� ���� ������� � ����������, ��� ����� ������� ��� 6%, �� 20 ���
}

//������������ ������� �� ���������� ��� ����
void bob_costs() {
	bob.bank_account -= bob.food;
	if (!bob.HasApartment) bob.bank_account -= bob.rent;       //���� ��� �������� ������� ��������
	else bob.bank_account -= bob.utilities;                    //� ��������� ������ ����������� ������
}

//��������� ����� ���� � ����� (���������� ��� ����� ������� ��������)
void rand_costs() {
	int rand_num = 10 + (mt() % 50);
	alice.bank_account -= rand_num * 1000;
	bob.bank_account -= rand_num * 1000;
}

//��� ������ ����� � ��������
void alice_remain(const int& year, const int& month) {
	if (alice.mortgage >= alice.bank_account) {             //���� ���� �������, �������� � �������
		alice.mortgage -= alice.bank_account;
		alice.bank_account = 0;
	}
	else {													//���� ����� �������, ����� �������� ������� ���������
		if (alice.mortgage < alice.bank_account && alice.mortgage != 0) {
			alice.bank_account -= alice.mortgage;
			alice.mortgage = 0;
			std::cout << "����� �������� �������: " << month << " ����� " << year << " ���\n";
		}                                                   //� ��������� ������� �����
	}
}

//��� ������ ��� � ��������
void bob_remain(const int& year, const int& month) {
	if (!bob.HasApartment && bob.investment < apartment_cost) {         //���� ����� �� ������ �� ������� ���������� ����������
		bob.investment += bob.bank_account;
		bob.bank_account = 0;
	}
	else if (!bob.HasApartment && bob.investment >= apartment_cost) {   //���� �������, �� ������� �� � ��������
		bob.bank_account += bob.investment;
		bob.investment = 0;
		bob.bank_account -= apartment_cost;
		bob.HasApartment = true;
		std::cout << "��� ����� ��������: " << month << " ����� " << year << " ���\n";
	}														   	        //� ��������� ������� �����, ��� �������������� �����
}

void simulation() {
	int year = 2025;
	int month = 9;

	while (!(year == 2045 && month == 9)) {
		inflation(month);

		alice_income(year, month);
		bob_income(year, month);

		alice_costs();
		bob_costs();
		rand_costs();

		alice_remain(year, month);
		bob_remain(year, month);

		++month;
		if (month == 13) {
			month = 1;
			++year;
		}
	}
}

void alice_print() {
	std::cout << "���� ����� � �����: " << alice.bank_account << "\n";
	if (alice.HasApartment) std::cout << "���� ��������\n";
	else std::cout << "��� ��������\n";
}

void bob_print() {
	std::cout << "���� ���� � �����: " << bob.bank_account << "\n";
	if (bob.HasApartment) std::cout << "���� ��������\n";
	else std::cout << "��� ��������\n";
}

int main() {
	setlocale(LC_ALL, "RU");

	alice_init();
	bob_init();

	simulation();

	alice_print();
	bob_print();
}