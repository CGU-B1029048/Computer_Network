#include <iostream>
#include <fstream>
#include <filesystem>
#include <string>
#include <vector>
#include <regex>

using namespace std;

typedef struct HTTP_Respond_Headers {
    int status_code;
    int Content_length;
    string Content_type;
    string Connection;
} HTTP_Respond_Header;


// class HTTP_Respond {
// private:
//     HTTP_Respond_Header Header;
//     string body;
//     vector<string> images_list;
// public:
//     HTTP_Respond(string res_body) {

//     }
// };

vector<string> Extract_image(string body) {
    vector<string> urls;
    size_t image_tag_start = 0, image_tag_end;
    while (body.find("<img", image_tag_start) != string::npos) {
        image_tag_start = body.find("<img", image_tag_start);
        image_tag_end = body.find(">", image_tag_start);
        urls.push_back(body.substr(image_tag_start, image_tag_end - image_tag_start + 1));
        image_tag_start++;
    }

    for (int i = 0; i < urls.size(); i++) {
        image_tag_start = urls[i].find("src=") + 5; 
        image_tag_end = urls[i].find('\"', image_tag_start);
        urls[i] = urls[i].substr(image_tag_start, image_tag_end - image_tag_start);
    }

    return urls;
}

int main () {
    string host = "www.cgu.tw";
    string website = "<html>\n<head>\n<title>My Webpage</title>\n</head>\n<body>\n<img src=http://www.cgu.tw\"image1.jpg\" alt=\"Image 1\">\n<img src=\"image2.jpg\" alt=\"Image 2\">\n<p>This is some text with images.</p>\n</body>\n</html>";

    vector<string> url;
    
    url = Extract_image(website);

    for (int i = 0; i < url.size(); i++) 
        cout << url[i] << endl;

    return 0;
}