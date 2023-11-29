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
        vector<string> getPath() {
            return path;
        }
};

typedef struct HTTP_Respond_Headers {
    int status_code;
    int Content_length;
    string Content_type;
    string Connection;
} HTTP_Respond_Header;

HTTP_Respond_Header ParseHeaders(string& respond_header) {
    HTTP_Respond_Header Header;
    size_t field_start, field_end;

    // Extract status code
    field_start = respond_header.find("HTTP/1.1 ") + 9;
    field_end = respond_header.find("\r\n", field_start);
    Header.status_code = stoi(respond_header.substr(field_start, field_end - field_start));

    // Extract Content-Length
    field_start = respond_header.find("Content-Length:") + 16;
    field_end = respond_header.find("\r\n", field_start);
    Header.Content_length = stoi(respond_header.substr(field_start, field_end - field_start));

    // Extract Content-Type
    if (respond_header.find("Content-Type:") != string::npos) {
        field_start = respond_header.find("Content-Type:") + 14;
        field_end = respond_header.find("\r\n", field_start);
        Header.Content_type = respond_header.substr(field_start, field_end - field_start);
    }

    // Extract Connection
    if (respond_header.find("Connection:") != string::npos) {
        field_start = respond_header.find("Connection:") + 12;
        field_end = respond_header.find("\r\n", field_start);
        Header.Connection = respond_header.substr(field_start, field_end - field_start);
    }

    return Header;
}

vector<string> Extract_image(string& body, string host) {
    vector<string> urls;
    size_t image_tag_start = 0, image_tag_end, i = 0;
    while (body.find("<img", image_tag_start) != string::npos) {
        // find <img> tag
        image_tag_start = body.find("<img", image_tag_start);
        image_tag_end = body.find(">", image_tag_start);

        // extract src from <img>
        image_tag_start = body.find("src=", image_tag_start) + 5; 
        image_tag_end = body.find('\"', image_tag_start);
        
        urls.push_back(body.substr(image_tag_start, image_tag_end - image_tag_start));

        // check link is in relative path
        if (urls[i].find("http://") != string::npos || urls[i].find("https://") != string::npos) {
            // Extract path and query components
            size_t path_start = urls[i].find("//") + 2;
            size_t path_end = urls[i].find('?', path_start);
            if (path_end == string::npos) {
                path_end = urls[i].size();
            }

            // Extract path
            urls[i] = urls[i].substr(path_start, path_end - path_start);
            urls[i] = urls[i].substr(host.length() + 1);            
        }

        if (urls[i].at(0) == '/') urls[i].erase(0,1);

        // Update website with relative URL
        body.replace(image_tag_start, image_tag_end - image_tag_start, urls[i]);

        image_tag_start++;
        i++;
    }

    return urls;
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

int HTTP_protocol(string host, string path, int port, string connection_type, string& body, HTTP_Respond_Header& Header) {
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
    addr.sin_port = htons(port);
    addr.sin_addr = *((struct in_addr *)hostaddr->h_addr);
    if (connect(sockfd, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
        std::cerr << "Failed to Connect to Host" << std::endl;
        close(sockfd);
        return -1;
    }

    // Setup HTTP Request header
    string request = "GET " + path + " HTTP/1.1\r\n";
    request += "HOST: " + host + "\r\n";
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
    string respond;
    char buffer[1024];
    do {
        n = read(sockfd, buffer, sizeof(buffer));
        if (n < 0) {
            std::cerr << "Failed to received HTTP Response" << std::endl;
            close(sockfd);
            return -1;
        }
        if (n > 0) {
            respond.append(buffer, n);
        }
    } while (n > 0);

    // Parse Content-Length header to determine body size
    Header = ParseHeaders(respond);

    // cout << "status code: " << Header.status_code << endl;
    // cout << "Content Len: " << Header.Content_length << endl;
    // cout << "Content type: " << Header.Content_type << endl;
    // cout << "connection: " << Header.Connection << endl;

    // Receive HTTP Response Body
    body = respond.substr(respond.find("\r\n\r\n") + 4, Header.Content_length);

    // Display HTTP Response
    // std::cout << headers << std::endl;
    // cout << "body: " << endl << body << endl;


    // Close socket Connection
    close(sockfd);
    return Header.status_code;
  }

void store_webpage(filesystem::path src_dir, vector<string> file_dir_path, string file_data) {
    filesystem::path fp = src_dir;
    ofstream outfile;
    
    // create a directionary with name of hostname if not exist
    if (!filesystem::exists(fp)) filesystem::create_directory(fp);
    cout << fp << endl;

    // create directionary according to file path if not exist
    for (int i = 0; i < file_dir_path.size()-1; i++) {
        fp /= file_dir_path[i];
        if (!filesystem::exists(fp)) filesystem::create_directory(fp);
        cout << fp << endl;
    }
    cout << "writing data in : " << fp / file_dir_path.back() << endl;
    // write data in
    outfile.open(fp / file_dir_path.back());
    outfile << file_data << endl;
    outfile.close();
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
    string website_body;
    HTTP_Respond_Header respond_Header;
    int status_code = HTTP_protocol(target_url.getHost(), target_url.getHTTPRequestPath(), target_url.getPort(), connection_type, website_body, respond_Header);
    // cout << "body: " << endl << website_body << endl;

    // find all image url
    vector<string> image_urls, image_path;
    image_urls = Extract_image(website_body, target_url.getHost());

    // store webpage in file
    store_webpage(output_dir / target_url.getHost(), target_url.getPath(), website_body);

    // Download images
    string pictures;
    HTTP_Respond_Header pic_Header;
    for (int i = 0, pos = 0; i < image_urls.size(); i++) {
        // send request and get picture
        int pic_status_code = HTTP_protocol(target_url.getHost(), '/' + image_urls[i], target_url.getPort(), connection_type, pictures, pic_Header);

        // decode image path
        string tmp = image_urls[i];
        // // Split the path by '/'
        while ((pos = tmp.find('/', 0)) != string::npos) {
            image_path.push_back(tmp.substr(0, pos));
            tmp = tmp.substr(pos + 1);
        }
        image_path.push_back(tmp);

        // store picture
        store_webpage(output_dir / target_url.getHost(), image_path, pictures);

        // clear data after storage
        image_path.clear();
    }
    // for (int i = 0; i < image_urls.size(); i++) 
    //     cout << image_urls[i] << endl;


    // site.open(filepath, ios::out);
    // if (site.is_open()) {
    //     site.write(website.c_str(), sizeof(website));
    //     site.close();
    // } else {
    //     std::cerr << "Error opening file: " << filepath << std::endl;
    // }
    return 0;
}