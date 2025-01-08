#include <iostream>
#include <string>
#include <curl/curl.h>
#include <json/json.h>
#include <fstream>
#include <filesystem>

size_t WriteCallback(void* contents, size_t size, size_t nmemb, std::string* userData) {
    size_t totalSize = size * nmemb;
    userData->append((char*)contents, totalSize);
    return totalSize;
}

size_t write_data(void *ptr, size_t size, size_t nmemb, FILE *stream) {
    size_t written = fwrite(ptr, size, nmemb, stream);
    return written;
}

std::string get_filename_from_url(const std::string &url) {
    size_t last_slash_pos = url.find_last_of('/');
    if (last_slash_pos != std::string::npos) {
        return url.substr(last_slash_pos + 1);
    }
    return "qwq.webp";
}

bool download_image(const std::string &url, const std::string &output_dir) {
    CURL *curl;
    CURLcode res;
    bool success = false;

    std::string filename = get_filename_from_url(url);

    if (!std::filesystem::exists(output_dir)) 
        std::filesystem::create_directories(output_dir);


    std::string output_path = output_dir + "/" + filename;

    curl_global_init(CURL_GLOBAL_DEFAULT);
    curl = curl_easy_init();

    if (curl) {
        FILE *fp = fopen(output_path.c_str(), "wb");
        if (fp) {
            curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
            curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_data);
            curl_easy_setopt(curl, CURLOPT_WRITEDATA, fp);

            res = curl_easy_perform(curl);

            if (res == CURLE_OK) {
                std::cout << "Image downloaded successfully: " << output_path << std::endl;
                success = true;
            } else {
                std::cerr << "Curl request failed: " << curl_easy_strerror(res) << std::endl;
            }

            fclose(fp);
        } else {
            std::cerr << "Failed to open file: " << output_path << std::endl;
        }

        curl_easy_cleanup(curl);
    } else {
        std::cerr << "Failed to initialize CURL" << std::endl;
    }

    curl_global_cleanup();

    return success;
}

Json::Value fetchJsonFromUrl(const std::string& url) {
    CURL* curl = curl_easy_init();
    if (!curl) {
        throw std::runtime_error("Failed to initialize CURL");
    }

    std::string jsonResponse;
    CURLcode res;

    curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &jsonResponse);

    res = curl_easy_perform(curl);

    if (res != CURLE_OK) {
        curl_easy_cleanup(curl);
        throw std::runtime_error("CURL request failed: " + std::string(curl_easy_strerror(res)));
    }

    Json::Value root;
    Json::CharReaderBuilder readerBuilder;
    std::string errs;

    std::istringstream iss(jsonResponse);
    if (!Json::parseFromStream(readerBuilder, iss, &root, &errs)) {
        curl_easy_cleanup(curl);
        throw std::runtime_error("Failed to parse JSON: " + errs);
    }

    curl_easy_cleanup(curl);

    return root;
}

void writeJson(Json::Value &root, const std::string &file) {
    Json::StreamWriterBuilder builder;
    builder["emitUTF8"] = true; 

    std::ofstream output(file);
    if (!output.is_open()) {
        throw std::runtime_error("Failed to open file for writing");
    }

    std::unique_ptr<Json::StreamWriter> writer(builder.newStreamWriter());
    writer->write(root, &output);

    output.close();
}

int main(int argc,char *argv[]){
    if (argc != 2) {
        std::cerr << "Usage: " << argv[0] << " <ids>" << std::endl; //testid-7067
        return 1;
    }
    std::string ids = argv[1];
    std::string jsonfilename = ids + ".json";
    std::string url = "https://api.bilibili.com/x/emote/package?ids=" + ids + "&&business=reply";
    try {
        Json::Value root = fetchJsonFromUrl(url);
        writeJson(root, jsonfilename);
        for (const auto& package : root["data"]["packages"]) {
        for (const auto& emote : package["emote"]) {
            download_image(emote["webp_url"].asString(),ids);
        }
    }
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
    }

    return 0;
}
