#include <iostream>
#include <fstream>
#include <sstream>
#include <thread>
#include <vector>
#include <map>
#include <filesystem>
#include <mutex>
#include <netinet/in.h>
#include <unistd.h>

#define PORT 8080
#define BUFFER_SIZE 4096

std::mutex log_mutex;

void log_message(const std::string& message) {
    std::lock_guard<std::mutex> guard(log_mutex);
    std::ofstream log_file("server.log", std::ios::app);
    log_file << message << std::endl;
}

std::string get_content_type(const std::string& path) {
    if (path.ends_with(".html")) return "text/html";
    if (path.ends_with(".css")) return "text/css";
    if (path.ends_with(".js")) return "application/javascript";
    return "text/plain";
}

std::string handle_get_request(const std::string& file_path) {
    if (!std::filesystem::exists(file_path)) {
        return "HTTP/1.1 404 Not Found\r\nContent-Type: text/plain\r\n\r\n404 Not Found";
    }
    std::ifstream file(file_path, std::ios::binary);
    std::stringstream buffer;
    buffer << file.rdbuf();
    return "HTTP/1.1 200 OK\r\nContent-Type: " + get_content_type(file_path) + "\r\n\r\n" + buffer.str();
}

void handle_client(int client_socket) {
    char buffer[BUFFER_SIZE] = {0};
    int bytes_read = read(client_socket, buffer, BUFFER_SIZE);
    if (bytes_read <= 0) {
        close(client_socket);
        return;
    }
    
    std::istringstream request(buffer);
    std::string method, path, protocol;
    request >> method >> path >> protocol;
    
    log_message("Received request: " + method + " " + path);
    
    std::string response;
    if (method == "GET") {
        response = handle_get_request("./www" + path);
    } else {
        response = "HTTP/1.1 405 Method Not Allowed\r\n\r\n";
    }
    
    send(client_socket, response.c_str(), response.size(), 0);
    close(client_socket);
}

int main() {
    int server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd == -1) {
        log_message("Failed to create socket");
        return 1;
    }
    
    sockaddr_in address = {};
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(PORT);
    
    if (bind(server_fd, (struct sockaddr*)&address, sizeof(address)) < 0) {
        log_message("Failed to bind socket");
        return 1;
    }
    
    if (listen(server_fd, 10) < 0) {
        log_message("Listen failed");
        return 1;
    }
    
    log_message("Server running on port " + std::to_string(PORT));
    
    while (true) {
        int client_socket = accept(server_fd, nullptr, nullptr);
        if (client_socket >= 0) {
            std::thread(handle_client, client_socket).detach();
        }
    }
    return 0;
}
