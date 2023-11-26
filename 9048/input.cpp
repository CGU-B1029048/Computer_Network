#include <iostream>
#include <string>
#include <fstream>
#include <sys/stat.h>
#include <filesystem>

using namespace std;

bool isWritable(const string& filePath) {
  struct stat st;
  int stat_result = stat(filePath.c_str(), &st);

  if (stat_result == 0) {
    if ((st.st_mode & S_IWUSR) || (st.st_mode & S_IWGRP) || (st.st_mode & S_IWOTH)) {
      return true;
    } else {
      return false;
    }
  } else {
    cerr << "Error retrieving file information" << endl;
    return false;
  }
}

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

    // check file accessibility
    filesystem::path output_dir = dir;
    struct stat st;
    if (!filesystem::exists(output_dir))
        cout << "\033[31mOutput Directionary doesn't exist\033[0m" << endl;
    else if (!isWritable(dir))
        cout << "\033[31mOutput Directionary cannot access\033[0m" << endl;

    cout << "target url : " << url << endl;
    cout << "storage directionary : " << dir << endl;
}