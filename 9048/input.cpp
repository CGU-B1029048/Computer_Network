#include <iostream>
#include <string>

using namespace std;

int main(int argc, char *argv[]) {
    string url ,dir;
    // argument parse : pragram_name url dir
    if (argc == 3) {
        url = argv[1];
        dir = argv[2];
    } else {
        if (argc != 1) cout << "Invalid arguments, enter interactive mode\n";
        cout << "enter target url : ";
        cin >> url;
        cout << "enter storage directionary : ";
        cin >> dir;
    }

    cout << "target url : " << url << endl;
    cout << "storage directionary : " << dir << endl;
}