#include <iostream>
#include <string>
#include <vector>
#include <fstream>
#include <regex>
#include <sys/stat.h>
#include <filesystem>

using namespace std;

class URL {
    private:
        string scheme;
        string host;
        int port = 0;
        vector<string> path;
        string query = "";
        string fragment = "";
    public:
        URL(string input_url) {
            Parse(input_url);
        }
        void Parse(string url_str) {
            // Split the URL by '://'
            size_t pos = url_str.find("://");
            if (pos != string::npos) {
                scheme = url_str.substr(0, pos);
                host = url_str.substr(pos + 3);
            } else {
                cout << "\033[31mMissing scheme/protocol\033[0m" << endl;
                throw invalid_argument("Missing protocol/scheme");
            }
            // Check Protocol
            if (scheme != "http" && scheme != "https") {
                cout << "\033[31mUnsupported or Invalid scheme/protocol\033[33m" << endl;
                cout << "This program only support HTTP abd HTTPS\033[0m" << endl;
                throw invalid_argument("Unsupported or Invalid scheme/protocol");
            }

            // Split the path by '/'
            while ((pos = host.find('/', 0)) != string::npos) {
                path.push_back(host.substr(0, pos));
                host = host.substr(pos + 1);
            }
            path.push_back(host);
            host = path[0];
            path.erase(path.begin(), path.begin() + 1);

            // Split the host and port by ':'
            if ((pos = host.find(':', 0)) != string::npos) {
                port = stoi(host.substr(pos + 1));
                host = host.substr(0, pos);
            }
            // check port
            if (port < 0 || port > 65535) {
                cout << "\033[31mInvalid Port number\033[0m" << endl;
                throw invalid_argument("Invalid Port number");
            }

            // Split the last path component by '?'
            pos = path.back().find('?');
            if (pos != string::npos) {
                query = path.back().substr(pos + 1);
                path.back() = path.back().substr(0, pos);
            }

            // Split the query by '#'
            pos = query.find('#');
            if (pos != string::npos) {
                fragment = query.substr(pos + 1);
                query = query.substr(0, pos);
            }
        }
        void PrintParsedURL() {
            // Print the parsed URL components
            cout << "proc: " << scheme << endl;
            cout << "host: " << host << endl;
            cout << "port: " << port << endl;
            cout << "path: ";
            for (size_t i = 0; i < path.size(); ++i) {
                cout << path[i] << "/";
            }
            cout << endl;
            cout << "query: " << query << endl;
            cout << "fragment: " << fragment << endl;
        }
        string PrintURL() {
            string url;
            url = scheme + "://" + host;
            if (port > 0) url += ':' + to_string(port);
            for (int i = 0; i < path.size(); ++i) {
                url.append('/' + path[i]);
            }
            if (!query.empty()) url +=  '?' + query;
            if (!fragment.empty()) url += '#' + fragment;
            return url;
        }
};

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

bool is_valid_url(const string& url) {
  // Use Regular Expression to validate url format
  regex pattern("^([a-zA-Z]+://)?[a-zA-Z0-9-.]+(:[0-9]+)?/[a-zA-Z0-9-./?%&=]*)$");
  return regex_match(url, pattern);
}

int main(int argc, char *argv[]) {
    string input_url ,dir;
    cout << "Welcome to website downloader\n";

    // argument parsing, usage : <pragram_name> url dir
    if (argc == 3) {
        input_url = argv[1];
        dir = argv[2];
    } else {
        if (argc != 1) cout << "Invalid arguments, enter interactive mode\n";
        cout << "enter target url : ";
        cin >> input_url;
        cout << "enter storage directionary : ";
        cin >> dir;
    }

    // check url format
    // if (!is_valid_url(input_url))
    //     cout << "\033[31mInvalid URL\033[0m" << endl;
    URL target_url(input_url);
    target_url.PrintParsedURL();
    cout << "target url : " << target_url.PrintURL() << endl;


    // check file accessibility
    filesystem::path output_dir = dir;
    struct stat st;
    if (!filesystem::exists(output_dir))
        cout << "\033[31mOutput Directionary doesn't exist\033[0m" << endl;
    else if (!isWritable(dir))
        cout << "\033[31mOutput Directionary cannot access\033[0m" << endl;


    cout << "storage directionary : " << dir << endl;
}