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
#include <ctime>

using namespace std;

// print colorful text
string print_color(string color) {
    int color_code = 0;
    if (color == "red") {
            return "\033[31m";
    } else if (color =="green") {
            return "\033[32m";
    } else if (color =="yellow") {
            return "\033[33m";
    } else if (color =="blue") {
            return "\033[34m";
    } else {
            return "\033[0m";
    }
}

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
                cout << print_color("red") << "Missing scheme/protocol" << print_color("yellow") << endl;
                cout << "Please enter complete url, including \"http://\"" << print_color("default") << endl;
                throw invalid_argument("Missing protocol/scheme");
            }
            // Check Protocol
            if (scheme != "http" && scheme != "https") {
                cout << print_color("red") << "Unsupported or Invalid scheme/protocol" << print_color("yellow") << endl;
                cout << "This program only support HTTP and HTTPS url, and request will only be sent in HTTP" << print_color("default") << endl;
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
                cout << print_color("red") << "Invalid Port number" << print_color("default") << endl;
                throw invalid_argument("Invalid Port number");
            }
            if (port != 80 && port != 0)
                cout << print_color("yellow") << "This program currently only support 80 port, will auto change to use port 80 to connect" << print_color("default") << endl;

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
                return 80;
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
    string full_respond_header;
} HTTP_Respond_Header;

HTTP_Respond_Header ParseHeaders(string& respond_header) {
    HTTP_Respond_Header Header;
    size_t field_start, field_end;
    
    // Extract status code
    field_start = respond_header.find("HTTP/1.1 ") + 9;
    field_end = respond_header.find("\r\n", field_start);
    Header.status_code = stoi(respond_header.substr(field_start, field_end - field_start));

    // Extract Content-Length
    if (respond_header.find("Content-Length:") != string::npos) {
        field_start = respond_header.find("Content-Length:") + 16;
        field_end = respond_header.find("\r\n", field_start);
        Header.Content_length = stoi(respond_header.substr(field_start, field_end - field_start));
    }

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

    // Store Full Header
    Header.full_respond_header = respond_header.substr(0,respond_header.find("\r\n\r\n"));

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
    cerr << print_color("red") << "Error retrieving file information" << print_color("default") << endl;
    return false;
  }
}

bool is_valid_url(const string& url) {
  // Use Regular Expression to validate url format
  regex pattern("^https?://[\\w\\.-]+(:\\d+)?(/\\s*)?$");
  return regex_match(url, pattern);
}

int HTTP_protocol(string host, string path, int port, string connection_type, string& body, HTTP_Respond_Header& Header, bool print_request) {
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
    // request example = "GET /index.html HTTP/1.1\r\nHost: www.example.com\r\nConnection: close\r\n\r\n";
    string request = "GET " + path + " HTTP/1.1\r\n";
    request += "HOST: " + host + "\r\n";
    request += "Connection: " + connection_type + "\r\n\r\n";
    // print request
    if (print_request) {
        cout << "\nrequest: \n" << print_color("green") << request << print_color("default") << endl;
    }
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

    // create directionary according to file path if not exist
    for (int i = 0; i < file_dir_path.size()-1; i++) {
        fp /= file_dir_path[i];
        if (!filesystem::exists(fp)) filesystem::create_directory(fp);
    }
    // cout << "\nwriting data in : " << fp / file_dir_path.back() << endl;
    // write data in
    outfile.open(fp / file_dir_path.back());
    outfile << file_data << endl;
    outfile.close();
}

int main(int argc, char *argv[]) {
    clock_t start_t, end_t;
    string input_url ,dir;
    cout << "\nWelcome to website downloader\n";

    // argument parsing, usage : <pragram_name> url dir
    if (argc == 3) {
        input_url = argv[1];
        dir = argv[2];
    } else {
        if (argc != 1) cout << print_color("yellow") << "Invalid arguments, " << print_color("default") << "enter interactive mode\n";
        cout << "enter target url : ";
        cin >> input_url;
        cout << "enter storage directionary : ";
        cin >> dir;
    }

    // // check url format
    // if (!is_valid_url(input_url)) {
    //     cout << "\033[33mInvalid URL\033[0m" << endl;
    //     return -1;
    // }

    // validate and decode url, create  into an URL object
    URL target_url(input_url);
    // target_url.PrintParsedURL();
    cout << print_color("blue") << "\ntarget url : " << print_color("default") << target_url.PrintURL() << endl;


    // check file accessibility
    filesystem::path output_dir = dir;
    struct stat st;
    if (!filesystem::exists(output_dir)) {
        cout << print_color("yellow") << "Output Directionary doesn't exist" << print_color("default") << "\nPlease retry or change output directionary" << endl;
        return -1;
    } else if (!isWritable(dir)) {
        cout << print_color("yellow") << "Output Directionary cannot access" << print_color("default") << "\nPlease retry or change output directionary" << endl;
        return -1;
    }

    cout << print_color("blue") << "storage directionary : " << print_color("default") << dir << endl;

    // send request
    string connection_type = "close";
    string website_body;
    HTTP_Respond_Header respond_Header;
    start_t = clock();
    int status_code = HTTP_protocol(target_url.getHost(), target_url.getHTTPRequestPath(), target_url.getPort(), connection_type, website_body, respond_Header, true);
    
    // check response code
    if (status_code >= 500 && status_code < 600) {
        cout << "\033[33mServer Error\033[0m : error code \033[33m" << status_code << "\033[0m\nPlease try again later" << endl;
        return -1;
    } else if (status_code >= 400 && status_code < 500) {
        if (status_code == 404) cout << "\033[33mPage not found" << endl;
        else cout << "\033[33mClient Side Error\033[0m : error code \033[33m" << status_code << "\033[0m\nPlease check your request" << endl;
        return -1;
    } else if (status_code >= 300 && status_code < 400) {
        cout << "\033[33mRedirected\033[0m : error code \033[33m" << status_code << "\033[0m\nPlease update your request link" << endl;
        return -1;
    } else if (status_code != 200) {
        cout << "error code \033[33m" << status_code << "\033[0m\nPlease try again" << endl;
        return -1;
    }
    cout << "response: " << print_color("green") << endl << respond_Header.full_respond_header << print_color("default") << endl << endl;

    // find all image url
    vector<string> image_urls, image_path;
    image_urls = Extract_image(website_body, target_url.getHost());

    // store webpage in file
    store_webpage(output_dir / target_url.getHost(), target_url.getPath(), website_body);

    // Download images
    string pictures;
    HTTP_Respond_Header pic_Header;
    int image_total_size = 0;

    cout << "Downloding Image Content now, total of " << print_color("blue") << image_urls.size() << print_color("default") << " images." << endl; 
    for (int i = 0, pos = 0; i < image_urls.size(); i++) {
        // print current downloading content
        cout << endl;
        cout << print_color("green") << "GET" << print_color("default") << " Image " << i+1 << ". Path : " << print_color("green") << image_urls[i] << print_color("default") << endl;

        // send request and get picture
        clock_t pic_start_t = clock();
        int pic_status_code = HTTP_protocol(target_url.getHost(), '/' + image_urls[i], target_url.getPort(), connection_type, pictures, pic_Header, false);
        clock_t pic_end_t = clock();

        // check image download status
        if (pic_status_code != 200) cout << print_color("yellow") << "Download Picture content failed" << print_color("default") << endl;
        else  {
            cout << "estimated time : " << print_color("blue") << (float)(difftime(pic_end_t, pic_start_t))/CLOCKS_PER_SEC << print_color("default") << " sec" << endl;
            cout << "Download Picture content " << i+1 << " successful" << endl;
        }
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

        // print image statistics.
        cout << "image size : " << print_color("blue") << pic_Header.Content_length/1000 << print_color("default") << " kb" << endl; 
        cout << "download progress " << print_color("blue") << i+1 << " / " << image_urls.size() << print_color("default") << endl;

        // clear data after storage
        image_total_size += pic_Header.Content_length;
        image_path.clear();
    }
    end_t = clock();

    // print stat
    cout << endl;
    cout << "url status: " << print_color("blue") << "OK" << print_color("default") << endl;
    cout << "Current webpage length : " << print_color("blue") << respond_Header.Content_length << print_color("default") << " byte" << endl;
    cout << "file size total : " << print_color("blue") << image_total_size/1000 << print_color("default") << " kb" << endl; 
    cout << "file count : " << print_color("blue") << image_urls.size() + 1 << print_color("default") << "" << endl;
    cout << "total time : " << print_color("blue") << (float)(difftime(end_t, start_t))/CLOCKS_PER_SEC << print_color("default") << " sec" << endl;

    return 0;
}