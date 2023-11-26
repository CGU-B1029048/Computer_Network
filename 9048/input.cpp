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
        string getHTTPRequestPath() {
            string pth;
            for (int i = 0; i < path.size(); i++) {
                pth.append('/' + path[i]);
            }
            if (!query.empty()) pth.append('?' + query);
            return pth;
        }
};

typedef struct HTTP_Respond_Headers {
    int status_code;
    int Content_length;
    string Content_type;
    string Connection;
} HTTP_Respond_Header;

HTTP_Respond_Header ParseHeaders(string respond_header) {
    HTTP_Respond_Header Header;

    // Extract status code
    size_t status_code_start = respond_header.find("HTTP/1.1 ") + 9;
    size_t status_code_end = respond_header.find("\r\n", status_code_start);
    Header.status_code = stoi(respond_header.substr(status_code_start, status_code_end - status_code_start));

    // Extract Content-Length
    size_t content_length_start = respond_header.find("Content-Length:") + 16;
    size_t content_length_end = respond_header.find("\r\n", content_length_start);
    Header.Content_length = stoi(respond_header.substr(content_length_start, content_length_end - content_length_start));

    // Extract Content-Type
    if (respond_header.find("Content-Type:") != string::npos) {
        size_t content_type_start = respond_header.find("Content-Type:") + 14;
        size_t content_type_end = respond_header.find("\r\n", content_type_start);
        Header.Content_type = respond_header.substr(content_type_start, content_type_end - content_type_start);
    }

    // Extract Connection
    if (respond_header.find("Connection:") != string::npos) {
        size_t connection_start = respond_header.find("Connection:") + 12;
        size_t connection_end = respond_header.find("\r\n", connection_start);
        Header.Connection = respond_header.substr(connection_start, connection_end - connection_start);
    }

    return Header;
}

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
    // resolve hostname to IP address
    struct hostent* hostaddr = gethostbyname(target_url.getHost().c_str());
    if (!hostaddr) {
        cerr << "Failed to resolve hostname: " << target_url.getHost() << endl;
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
    addr.sin_port = htons(target_url.getPort());
    addr.sin_addr = *((struct in_addr *)hostaddr->h_addr);
    if (connect(sockfd, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
        std::cerr << "Failed to Connect to Host" << std::endl;
        close(sockfd);
        return -1;
    }

    // Setup HTTP Request header
    string request = "GET " + target_url.getHTTPRequestPath() + " HTTP/1.1\r\n";
    request += "HOST: " + target_url.getHost() + "\r\n";
    request += "Connection: " + connection_type + "\r\n\r\n";
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
    // Received Headers
    string headers;
    char buffer[1024];
    while (headers.find("\r\n\r\n") == string::npos) {
        n = read(sockfd, buffer, sizeof(buffer));
        if (n < 0) {
            std::cerr << "Failed to received HTTP Response" << std::endl;
            close(sockfd);
            return -1;
        }
        headers.append(buffer, n);
    }

    // Parse Content-Length header to determine body size
    HTTP_Respond_Header Header = ParseHeaders(headers);

    // Receive HTTP Response Body
    string body;
    char page_buffer[1048576];
    while (body.length() < Header.Content_length) {
        n = read(sockfd, page_buffer, sizeof(page_buffer));
        if (n < 0) {
            std::cerr << "Failed to receive HTTP Response Body" << std::endl;
            close(sockfd);
            return -1;
        }

        body.append(buffer, n);
    }

    // Display HTTP Response
    // std::cout << headers << std::endl;
    cout << "status code: " << Header.status_code << endl;
    cout << "Content Len: " << Header.Content_length << endl;
    cout << "Content type: " << Header.Content_type << endl;
    cout << "connection: " << Header.Connection << endl;
    cout << "body: " << endl << body << endl;


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
    //target_url.PrintParsedURL();
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

    return 0;
}