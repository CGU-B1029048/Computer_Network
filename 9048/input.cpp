#include <iostream>
#include <string>
#include <vector>
#include <fstream>
#include <regex>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
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
                cout << "This program only support HTTP and HTTPS url, and request will only be sent in HTTP\033[0m" << endl;
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
        int getPort() {
            if (port > 0 && port < 65536) {
                return port;
            } else if (port == 0) {
                return 80;
            } else {
                return -1;
            }
        }
        string getHost() {
            return host;
        }
        string getPath() {
            string pth;
            pth.append(path[0]);
            for (int i = 1; i < path.size(); i++) {
                pth.append('/' + path[i]);
            }
            return pth;
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

int HTTP_protocol(URL& target_url, string connection_type) {
    // establish socket connection
    int sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) {
        cerr << "Failed to establish socket" << endl;
        return -1;
    }

    // Setup Connection info
    struct sockaddr_in addr;
    bzero(&addr,sizeof(addr));
    addr.sin_family = PF_INET;
    addr.sin_port = htons(target_url.getPort());
    addr.sin_addr.s_addr = inet_addr(target_url.getHost().c_str());
    if (connect(sockfd, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
        std::cerr << "Failed to Connect to Host" << std::endl;
        close(sockfd);
        return -1;
    }

    // Setup HTTP Request header
    string request = "GET " + target_url.getPath() + " HTTP/1.1\r\n";
    request += "HOST: " + target_url.getHost() + "\r\n";
    request += "Connection: " + connection_type + "\r\n\r\n";
    //char request[] = "GET /index.html HTTP/1.1\r\nHost: www.example.com\r\nConnection: close\r\n\r\n";

    // Send HTTP Request
    int n = send(sockfd, request.c_str(), strlen(request.c_str()), 0);
    if (n < 0) {
        std::cerr << "Failed to sent HTTP Request" << std::endl;
        close(sockfd);
        return -1;
    }

    // Received HTTP Response
    char buffer[1048576];
    n = read(sockfd, buffer, sizeof(buffer));
    if (n < 0) {
        std::cerr << "Failed to received HTTP Response" << std::endl;
        close(sockfd);
        return -1;
    }

    // Display HTTP Response
    std::cout << buffer << std::endl;

    // Close socket Connection
    close(sockfd);
    return 0;
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
    cout << "\033[34mtarget url : \033[0m" << target_url.PrintURL() << endl;


    // check file accessibility
    filesystem::path output_dir = dir;
    struct stat st;
    if (!filesystem::exists(output_dir))
        cout << "\033[31mOutput Directionary doesn't exist\033[0m" << endl;
    else if (!isWritable(dir))
        cout << "\033[31mOutput Directionary cannot access\033[0m" << endl;

    cout << "\033[34mstorage directionary : \033[0m" << dir << endl;

    // send request
    string connection_type = "close";
    HTTP_protocol(target_url, connection_type);
}