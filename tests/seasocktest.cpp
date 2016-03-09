#include "seasocks/Server.h"
#include "seasocks/PrintfLogger.h"
#include "seasocks/WebSocket.h"
#include "jeayeson/jeayeson.hpp"
#include <set>
#include <sstream>

using namespace seasocks;

struct ChatHandler : WebSocket::Handler {
    std::set<WebSocket *> _conns;

    std::string _data_to_send;

    ChatHandler() : WebSocket::Handler() {
        json_value val{
            {"hello", "world"},
            {"arr", {1.1, 2.2, 3.3}},
            {"person", {{"name", "Tom"}, {"age", 36}, {"weapon", nullptr}}}};

        std::ostringstream str;
        str << val;

        _data_to_send = str.str();
    }

    void onConnect(WebSocket *conn) override { _conns.insert(conn); }

    void onData(WebSocket *, const char *data) override {
        (void)data; // Unused

        for (auto c : _conns) {
            c->send(_data_to_send.c_str());
        }
    }

    void onDisconnect(WebSocket *conn) override { _conns.erase(conn); }
};

void chat() {
    Server s(std::make_shared<PrintfLogger>());
    s.addWebSocketHandler("/chat", std::make_shared<ChatHandler>());
    s.serve("web", 9090);
}

int main() { chat(); }
