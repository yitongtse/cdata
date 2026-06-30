#include <iostream>
using namespace std;

int main ()
{
    string line = "0001 0 3,45,6,78,9";
    cout << line << endl;

    string transId = line.substr(0,4);
    string cashId = line.substr(5,1);

    cout << "Transaction ID: " << transId << endl;
    cout << "Cashier ID: " << cashId << endl;

    int start = 7;
    int stop = 7;
    int count = 0;

    while (stop != string::npos) {
        string item;

        stop = line.find(",", start);
        if (stop == string::npos) {
            item = line.substr(start);
        } else {
            item = line.substr(start, stop-start);
        }

        cout << "Item: " << item << endl;
        start = stop + 1;
        count++;
    }

    cout << "Total " << count << " items." << endl;
}
