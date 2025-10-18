#include <iostream>
#include <istream>
#include <string>
#include <math.h>
#include <stdlib.h>


using namespace std;

float Bob() {
	float stoimost_kv_B, zarp_B, razn_B, pr_of_cl_B, communism_B, food_B, unexp_B, per_dep_B, dep, rdepper, infl, inflr;
	float koshel_B = 0;
	string Bobans;

	cout << "Bob" << endl;
	cout << "Please, enter the price of a flat" << endl;
	cin >> stoimost_kv_B;

	cout << "Please, enter the value of interest rate" << endl;
	cin >> per_dep_B;

	rdepper = ((per_dep_B / 12) / 100);

	cout << "Please, enter the value of your salary" << endl;
	cin >> zarp_B;

	cout << "Please, enter the amount of money You spend on clothes every year" << endl;
	cin >> pr_of_cl_B;

	cout << "Please, enter the amount of money You spend on upkeeping your household every month" << endl;
	cin >> communism_B;

	cout << "Please, enter the amount of money You spend on food every month" << endl;
	cin >> food_B;

	dep = 0;
	for (int l = 0; l <= 239; l++) {

		if (l > 0) {
			if ((l + 1) % 12 == 0) {
				cout << "Did you have any unexpected expenditures during the " << ((l + 1) / 12) << " year? Answer with Yes or nothing" << endl;
				cin >> Bobans;
				if (Bobans == "Yes") {
					cout << "Please, enter the amount of money You spend on it" << endl;
					cin >> unexp_B;
					razn_B = zarp_B - pr_of_cl_B - communism_B - food_B - unexp_B;
				}
				else {
					razn_B = zarp_B - pr_of_cl_B - communism_B - food_B;
				}
				cout << "What was the total percent of inflation of " << ((l + 1) / 12) << " year?" << endl;
				cin >> infl;
				inflr = infl / 100;
				dep = dep * (1 - inflr);
				pr_of_cl_B = pr_of_cl_B + (pr_of_cl_B * inflr);
				communism_B = communism_B + (communism_B * inflr);
				food_B = food_B + (food_B * inflr);
			}
			else {
				razn_B = zarp_B - communism_B - food_B;
			}
			if (dep > 0) {
				dep += dep * rdepper;
				dep += razn_B;
			}
			else {
				dep += razn_B;
			}


		}
		else {
			razn_B = zarp_B - communism_B - food_B;
			dep += razn_B;
		}
	}
	cout << dep << endl;
	return dep;

}

float Alice() {
	// setlocale (LC_ALL, "RU"), коли хотите по-русски
	float stoimost_kv, perv_plat, ishod_ost, mes_pr_st, pr_st, zarp, razn, pr_of_cl, communism, food, unexp, infl, inflr;
	float koshel = 0;
	string text;
	cout << "Alice" << endl;

	cout << "Please, enter the price of a flat" << endl;
	cin >> stoimost_kv;

	cout << "Please, enter the value of entrance fee" << endl;
	cin >> perv_plat;

	ishod_ost = stoimost_kv - perv_plat;

	cout << "Please, enter the value of interest rate" << endl;
	cin >> pr_st;

	mes_pr_st = ((pr_st / 12) / 100);
	float ezh_pl = ishod_ost * ((mes_pr_st * pow(1 + mes_pr_st, 240)) / (pow(1 + mes_pr_st, 240) - 1));

	cout << "Please, enter the value of your salary" << endl;
	cin >> zarp;

	cout << "Please, enter the amount of money You spend on clothes every year" << endl;
	cin >> pr_of_cl;

	cout << "Please, enter the amount of money You spend on upkeeping your household every month" << endl;
	cin >> communism;

	cout << "Please, enter the amount of money You spend on food every month" << endl;
	cin >> food;

	for (int k = 0; k <= 239; k++) {


		if ((k + 1) % 12 == 0 && k > 0) {
			cout << "Did you have any unexpected expenditures during the " << ((k + 1) / 12) << " year? Answer with Yes or some letters" << endl;
			// getline работает странно
			cin >> text;
			if (text == "Yes") {
				cout << "Please, enter the amount of money You spend on it" << endl;
				cin >> unexp;
				razn = zarp - ezh_pl - communism - food - pr_of_cl - unexp;
			}
			else {
				razn = zarp - ezh_pl - communism - food - pr_of_cl;

			}
			cout << "What was the total percent of inflation of " << ((k + 1) / 12) << " year?" << endl;
			cin >> infl;
			inflr = infl / 100;
			koshel = koshel * (1 - inflr);
			pr_of_cl = pr_of_cl + (pr_of_cl * inflr);
			communism = communism + (communism * inflr);
			food = food + (food * inflr);

		}
		else {
			razn = zarp - ezh_pl - communism - food;
		}
		koshel += razn;


	}
	if (koshel < 0) {
		cout << "Alice is Bunkrupt!";
		exit(0);
	}

	koshel += stoimost_kv;
	cout << koshel << endl;
	return koshel;
}

int main() {
	float Beob, Aliece;
	Aliece = Alice();
	Beob = Bob();
	if (Aliece > Beob) {
		cout << "\nAlice has more money" << endl;
	}
	if (Aliece < Beob) {
		cout << "\nBob has more money" << endl;
	}
	if (Aliece == Beob) {
		cout << "\nDraw!" << endl;
	}


	return 0;


}
