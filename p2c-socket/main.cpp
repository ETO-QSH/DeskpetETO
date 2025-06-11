#include <iostream>
#include <WS2tcpip.h>
#include <string>
#include <thread>
#include <chrono>
#include "json.hpp"

#pragma comment(lib, "Ws2_32.lib")

using json = nlohmann::json;

class ExeClient {
private:
    SOCKET connectSocket = 0;
    bool connected = false;

public:
    ~ExeClient() {
        if (connectSocket != 0) {
            closesocket(connectSocket);
            WSACleanup();
        }
    }

    bool connect_to_server(const std::string& ip, int port) {
        WSADATA wsaData;
        int iResult = WSAStartup(MAKEWORD(2, 2), &wsaData);
        if (iResult != 0) {
            std::cerr << "WSAStartup failed: " << iResult << std::endl;
            return false;
        }

        struct addrinfo* result = nullptr;
        struct addrinfo hints{};
        ZeroMemory(&hints, sizeof(hints));
        hints.ai_family = AF_UNSPEC;
        hints.ai_socktype = SOCK_STREAM;
        hints.ai_protocol = IPPROTO_TCP;

        iResult = getaddrinfo(ip.c_str(), std::to_string(port).c_str(), &hints, &result);
        if (iResult != 0) {
            std::cerr << "getaddrinfo failed: " << iResult << std::endl;
            WSACleanup();
            return false;
        }

        connectSocket = socket(result->ai_family, result->ai_socktype, result->ai_protocol);
        if (connectSocket == INVALID_SOCKET) {
            std::cerr << "Socket creation failed: " << WSAGetLastError() << std::endl;
            freeaddrinfo(result);
            WSACleanup();
            return false;
        }

        iResult = connect(connectSocket, result->ai_addr, (int)result->ai_addrlen);
        if (iResult == SOCKET_ERROR) {
            std::cerr << "Connection failed: " << WSAGetLastError() << std::endl;
            closesocket(connectSocket);
            connectSocket = INVALID_SOCKET;
            freeaddrinfo(result);
            WSACleanup();
            return false;
        }

        freeaddrinfo(result);
        connected = true;
        std::cout << "Connected to server!" << std::endl;

        // 启动线程监听服务器消息
        std::thread(&ExeClient::listen_for_commands, this).detach();

        return true;
    }

    void send_request_to_gui(const std::string& request) const {
        if (connected) {
            send(connectSocket, request.c_str(), request.size(), 0);
            std::cout << "Sent to server: " << request << std::endl;
        } else {
            std::cerr << "Not connected to server" << std::endl;
        }
    }

    void send_json_request(const json& json_data) const {
        if (connected) {
            std::string json_str = json_data.dump();
            send(connectSocket, json_str.c_str(), json_str.size(), 0);
            std::cout << "Sent JSON to server: " << json_str << std::endl;
        } else {
            std::cerr << "Not connected to server" << std::endl;
        }
    }

    void listen_for_commands() {
        char buffer[1024];
        while (connected) {
            int recvResult = recv(connectSocket, buffer, 1024, 0);
            if (recvResult > 0) {
                std::string response(buffer, recvResult);
                std::cout << "Received from server: " << response << std::endl;

                try {
                    // 尝试解析 JSON
                    json json_data = json::parse(response);
                    handle_json(json_data);
                } catch (json::parse_error& e) {
                    // 处理非 JSON 消息
                    std::cout << "Received non-JSON message: " << response << std::endl;
                }
            } else if (recvResult == 0) {
                std::cout << "Connection closed by server" << std::endl;
                connected = false;
                closesocket(connectSocket);
                WSACleanup();
                break;
            } else {
                std::cerr << "recv failed: " << WSAGetLastError() << std::endl;
                connected = false;
                closesocket(connectSocket);
                WSACleanup();
                break;
            }
        }
    }

    static void handle_json(const json& json_data) {
        if (json_data.contains("type") && json_data["type"] == "command") {
            if (json_data.contains("command")) {
                std::string command = json_data["command"];
                if (command == "do_something") {
                    if (json_data.contains("data")) {
                        const json& data = json_data["data"];
                        std::cout << "Received command with data: " << data.dump() << std::endl;
                    }
                    trigger_function();
                }
            }
        }
    }

    static void trigger_function() {
        std::cout << "Function triggered by Python signal!" << std::endl;
        // 在这里添加你想要执行的逻辑
    }
};

int main() {
    ExeClient client;
    if (client.connect_to_server("127.0.0.1", 12345)) {
        // 模拟 exe 发送请求的场景
        std::this_thread::sleep_for(std::chrono::seconds(2));
        client.send_request_to_gui("GET_DATA");

        std::this_thread::sleep_for(std::chrono::seconds(2));
        client.send_request_to_gui("COMMAND: Do something");

        // 发送 JSON 请求
        std::this_thread::sleep_for(std::chrono::seconds(2));
        json json_data = {
            {"type", "request"},
            {"request", "function"},
            {"function", "trigger_function"}
        };
        client.send_json_request(json_data);
    } else {
        std::cerr << "Failed to connect to server" << std::endl;
    }

    std::cout << "Press Enter to exit...";
    std::cin.get();
    return 0;
}