#include <iostream>
#include <cmath>

using namespace std;

typedef long int RUB;

struct person {
	RUB bank_account;  //���� � �����
	RUB income;        //��������
	RUB mortgage;      //������� ����� �� �������
	RUB investment;    //����� � �����

	RUB food;          //����� �� ���
	RUB rent;          //������ ��������
	RUB utilities;     //����������� ������

	bool apartment;    //������� ��������
};

struct person alice;   //���� � �������
struct person bob;     //����� �� �������


//������� ������������ ���������� (����� ��� ������ �������������� �����)
void alice_init() {
	alice.bank_account = 0;
	alice.mortgage = 8 * 1000 * 1000;
	alice.income = 200 * 1000;
	alice.food = 20 * 1000;
	alice.rent = 30 * 1000;
	alice.utilities = 4 * 1000;
	alice.apartment = true;
}

//������� ������������ ���������� (��� ��� ������� ������ �� ������������� ����)
void bob_init() {
	bob.bank_account = 0;
	bob.investment = 2 * 1000 * 1000;
	bob.income = 200 * 1000;
	bob.food = 20 * 1000;
	bob.rent = 30 * 1000;
	bob.utilities = 4 * 1000;
	bob.apartment = false;
}

//��������� ��������
RUB apartment_cost = 10 * 1000 * 1000;

//��������
void inflation(int month) {
	if (month == 10) apartment_cost *= 1.07;
}

void alice_income(int year, int month) {
	if (month == 10) alice.income *= 1.07;                 //���� �������� �� ��������
	if (year == 2030 && month == 3) alice.income *= 1.5;   //��������� � ���������
	alice.bank_account += alice.income;
}

void bob_income(int year, int month) {
	if (month == 10) {
		bob.income *= 1.07;                                //���� �������� �� ��������
		bob.investment *= 1 + 0.20 / 12;                   //���������� �������� �� ����� � ����� ��� 20% �������
	}
	if (year == 2030 && month == 3) bob.income *= 1.5;     //��������� � ���������
	bob.bank_account += bob.income;
}

//���������� ������������ ������� �� �������
int mortgage_payment(long sum, double percent, int months) {
	return (sum * (percent / 12) / (1 - pow(1 + percent / 12, -months - 1)));
}

//������������ ������� �� ���������� ��� �����
void alice_costs() {
	alice.bank_account -= alice.food;
	alice.bank_account -= alice.utilities;
	alice.bank_account -= mortgage_payment(alice.mortgage,0.06,240);
}

//������������ ������� �� ���������� ��� ����
void bob_costs() {
	bob.bank_account -= bob.food;
	if (!bob.apartment) bob.bank_account -= bob.rent;       //���� ��� �������� ������� ��������
	else bob.bank_account -= bob.utilities;                 //� ��������� ������ ����������� ������
}

//��������� ����� ���� � ����� (���������� ��� ����� ������� ��������)
void rand_costs() {
	srand(time(NULL));
	int rand_num = 10 + rand() % 30;
	alice.bank_account -= rand_num * 1000;
	bob.bank_account -= rand_num * 1000;
}

//��� ������ ����� � ��������
void alice_remain() {
	if (alice.mortgage >= alice.bank_account) {             //���� ���� �������, �������� � �������
		alice.mortgage -= alice.bank_account;
		alice.bank_account = 0;
	}
	else {													//���� ����� �������, ����� �������� ������� ���������
		if (alice.mortgage < alice.bank_account) {
			alice.bank_account -= alice.mortgage;
			alice.mortgage = 0;
		}                                                   //� ��������� ������� �����
	}
}

//��� ������ ��� � ��������
void bob_remain() {
	if (!bob.apartment && bob.investment < apartment_cost) {         //���� ����� �� ������ �� ������� ���������� ����������
		bob.investment += bob.bank_account;
		bob.bank_account = 0;
	}
	else if (!bob.apartment && bob.investment >= apartment_cost) {   //���� �������, �� ������� �� � ��������
		bob.bank_account += bob.investment;
		bob.investment = 0;
		bob.bank_account -= apartment_cost;
		bob.apartment = true;
	}																 //� ��������� ������� �����, ��� �������������� �����
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

		alice_remain();
		bob_remain();

		++month;
		if (month == 13) {
			month = 1;
			++year;
		}
	}
}

void alice_print() {
	cout << "���� � �����: " << alice.bank_account << endl;
	if (alice.apartment) cout << "���� ��������" << endl;
	else cout << "��� ��������" << endl;
}

void bob_print() {
	cout << "���� � �����: " << bob.bank_account << endl;
	if (bob.apartment) cout << "���� ��������" << endl;
	else cout << "��� ��������" << endl;
}

int main() {
	alice_init();
	bob_init();

	simulation();

	setlocale(LC_ALL, "RU");
	alice_print();
	bob_print();
}