#include "simulator.hpp"

#include <cstdlib>
#include <algorithm>
#include <cerrno>
#include <cstring>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>

#if defined(_WIN32)
#include <winsock2.h>
#include <ws2tcpip.h>
using Socket = SOCKET;
static bool invalidSocket(Socket s) { return s == INVALID_SOCKET; }
static void closeSocket(Socket s) { closesocket(s); }
#else
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>
using Socket = int;
static bool invalidSocket(Socket s) { return s < 0; }
static void closeSocket(Socket s) { close(s); }
#endif

static std::string decode(const std::string& text) {
    std::string out;
    for (size_t i = 0; i < text.size(); ++i) {
        if (text[i] == '+' ) out.push_back(' ');
        else if (text[i] == '%' && i + 2 < text.size()) {
            char* end = nullptr;
            const auto hex = text.substr(i + 1, 2);
            const auto code = std::strtol(hex.c_str(), &end, 16);
            if (end && *end == '\0') { out.push_back(static_cast<char>(code)); i += 2; }
            else out.push_back(text[i]);
        } else out.push_back(text[i]);
    }
    return out;
}

static std::string param(const std::string& query, const std::string& key) {
    std::istringstream items(query);
    std::string item;
    while (std::getline(items, item, '&')) {
        const auto eq = item.find('=');
        if (eq != std::string::npos && decode(item.substr(0, eq)) == key) return decode(item.substr(eq + 1));
    }
    return {};
}

static void respond(Socket client, int status, const std::string& type, const std::string& body) {
    const std::string reason = status == 200 ? "OK" : "Bad Request";
    std::ostringstream head;
    head << "HTTP/1.1 " << status << ' ' << reason << "\r\nContent-Type: " << type
         << "\r\nContent-Length: " << body.size()
         << "\r\nCache-Control: no-store\r\nConnection: close\r\nAccess-Control-Allow-Origin: null\r\n\r\n";
    const auto response = head.str() + body;
    size_t sentTotal = 0;
    while (sentTotal < response.size()) {
        const auto chunk = std::min<size_t>(response.size() - sentTotal, 4096);
        const int sent = send(client, response.data() + sentTotal, static_cast<int>(chunk), 0);
        if (sent <= 0) break;
        sentTotal += static_cast<size_t>(sent);
    }
}

int main(int argc, char** argv) {
    int port = 8765;
    if (argc > 1 && std::string(argv[1]) == "--port" && argc > 2) port = std::atoi(argv[2]);
    if (port < 1024 || port > 65535) { std::cerr << "端口范围应为 1024-65535。\n"; return 2; }

#if defined(_WIN32)
    WSADATA wsa{};
    if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0) { std::cerr << "无法初始化网络服务。\n"; return 2; }
#endif
    Socket server = socket(AF_INET, SOCK_STREAM, 0);
    if (invalidSocket(server)) { std::cerr << "无法创建本地服务端口。\n"; return 2; }
    int reuse = 1;
    setsockopt(server, SOL_SOCKET, SO_REUSEADDR, reinterpret_cast<const char*>(&reuse), sizeof(reuse));
    sockaddr_in address{};
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
    address.sin_port = htons(static_cast<unsigned short>(port));
    if (bind(server, reinterpret_cast<sockaddr*>(&address), sizeof(address)) < 0) {
#if defined(_WIN32)
        const int bindError = WSAGetLastError();
        const bool addressInUse = bindError == WSAEADDRINUSE;
#else
        const int bindError = errno;
        const bool addressInUse = bindError == EADDRINUSE;
#endif
        if (argc == 1 && addressInUse) {
            address.sin_port = 0;
            if (bind(server, reinterpret_cast<sockaddr*>(&address), sizeof(address)) == 0) {
                socklen_t addressLength = sizeof(address);
                if (getsockname(server, reinterpret_cast<sockaddr*>(&address), &addressLength) == 0)
                    port = ntohs(address.sin_port);
            } else {
#if defined(_WIN32)
                std::cerr << "默认端口被占用，且系统未分配可用本机端口（Winsock " << WSAGetLastError() << "）。\n";
#else
                std::cerr << "默认端口被占用，且系统未分配可用本机端口：" << std::strerror(errno) << "。\n";
#endif
                closeSocket(server);
                return 2;
            }
        } else {
#if defined(_WIN32)
            std::cerr << "端口 " << port << " 无法绑定（Winsock " << bindError << "）。\n";
#else
            std::cerr << "端口 " << port << " 无法绑定：" << std::strerror(bindError) << "。\n";
#endif
            closeSocket(server);
            return 2;
        }
    }
    if (listen(server, 16) < 0) {
        std::cerr << "端口 " << port << " 无法监听：" << std::strerror(errno) << "。\n";
        closeSocket(server);
        return 2;
    }

    Simulator simulator;
    std::cout << "智能门禁交互式验证平台已启动：http://127.0.0.1:" << port
              << "\n仅监听本机。按 Ctrl+C 结束。\n" << std::flush;
    for (;;) {
        Socket client = accept(server, nullptr, nullptr);
        if (invalidSocket(client)) continue;
        char buffer[8192]{};
        const int count = recv(client, buffer, sizeof(buffer) - 1, 0);
        if (count <= 0) { closeSocket(client); continue; }
        std::istringstream request(std::string(buffer, static_cast<size_t>(count)));
        std::string method, target, version;
        request >> method >> target >> version;
        const auto qmark = target.find('?');
        const std::string path = target.substr(0, qmark);
        const std::string query = qmark == std::string::npos ? "" : target.substr(qmark + 1);
        if (method == "GET" && path == "/api/state") respond(client, 200, "application/json; charset=utf-8", simulator.stateJson());
        else if (method == "GET" && path == "/api/action") {
            const auto name = param(query, "name");
            const auto value = param(query, "value");
            static const std::string allowed[] = {"button", "l2-press", "l2-release", "l2-bounce", "l2-tone", "card", "door-open", "door-close", "remote", "serial", "wifi", "mqtt", "power-cut", "rfid-fail", "rfid-recover", "clear", "tick", "speed", "sleep", "watchdog", "heap", "reset"};
            bool valid = false;
            for (const auto& item : allowed) if (name == item) valid = true;
            if (!valid) respond(client, 400, "text/plain; charset=utf-8", "未知操作");
            else {
                try { simulator.action(name, value); respond(client, 200, "application/json; charset=utf-8", simulator.stateJson()); }
                catch (...) { respond(client, 400, "text/plain; charset=utf-8", "参数格式错误"); }
            }
        } else if (method == "GET" && (path == "/" || path == "/index.html")) {
            std::ifstream file("desktop/web/index.html", std::ios::binary);
            std::ostringstream html;
            html << file.rdbuf();
            if (file) respond(client, 200, "text/html; charset=utf-8", html.str());
            else respond(client, 200, "text/html; charset=utf-8", "<h1>请从项目根目录启动程序</h1>");
        } else respond(client, 400, "text/plain; charset=utf-8", "Not found");
        closeSocket(client);
    }
    closeSocket(server);
    return 0;
}
