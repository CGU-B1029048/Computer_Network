#include <iostream>
#include <string>
#include <vector>
#include <fstream>
#include <regex>
#include <unistd.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <filesystem>

using namespace std;

/*class URL {
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
};*/

int main() {
    string host, path;
    cout << "enter target host : ";
    cin >> host;

    // resolve hostname to IP address
    struct hostent* hostaddr = gethostbyname(host.c_str());
    if (!hostaddr) {
        cerr << "Failed to resolve hostname: " << host << endl;
        return -1;
    }

    // establish socket connection
    int sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) {
        cerr << "Failed to establish socket" << endl;
        return -1;
    }

    // setup connection info
    struct sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_port = htons(80);
    addr.sin_addr = *((struct in_addr *)hostaddr->h_addr);
    if (connect(sockfd, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
        std::cerr << "Failed to Connect to Host" << std::endl;
        close(sockfd);
        return -1;
    }
    
    cout << "enter path" << endl;
    cin >> path;
    // Setup HTTP Request header
    string request = "GET " + path + " HTTP/1.1\r\n";
    request += "HOST: " + host + "\r\n";
    request += "Connection: close \r\n\r\n";
    //char request[] = "GET /index.html HTTP/1.1\r\nHost: www.example.com\r\nConnection: close\r\n\r\n";
    cout << "request: \n\033[32m" << request << "\033[0m" << endl;

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

