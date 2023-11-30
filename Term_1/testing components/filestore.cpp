#include <string>
#include <filesystem>
#include <iostream>

using namespace std;

int main() {
    string src = "test/pest.txt";
    string long_dir = "aest1/bestt2/ctt3/dmages4/emage.txt5";
    string filename = "test.txt";
    vector<string> image_urls, image_path;
    image_urls.push_back(src);
    image_urls.push_back(long_dir);
    image_urls.push_back(filename);

    size_t pos = 0;
    for (int i = 0; i < image_urls.size(); i++) {    
        string tmp = image_urls[i];
        cout << tmp << endl;
        // Split the path by '/'
        while ((pos = tmp.find('/', 0)) != string::npos) {
            image_path.push_back(tmp.substr(0, pos));
            tmp = tmp.substr(pos + 1);
        }
        image_path.push_back(tmp);
        // print image_path
        cout << "image_path = [";
        for (int i = 0; i < image_path.size() ; i++) {
            cout << '\"' << image_path[i] << '\"';
            if (i < image_path.size()-1) cout << ',';
        }
        cout << ']' << endl << endl;
        // clear path
        image_path.clear();
    }

    return 0;
}