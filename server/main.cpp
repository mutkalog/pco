#include <httplib.h>
#include <nlohmann/json.hpp>


int main()
{
    using namespace httplib;
    Server server;
    using json = nlohmann::ordered_json;

    server.Get("/manifest", [](const Request& req, Response& res) {
        std::cout << req.method << std::endl;
        std::cout << req.path << std::endl;
        std::cout << req.body << std::endl;
        std::cout << req.get_param_value("id") << std::endl;
        std::cout << req.get_param_value("place") << std::endl;
        std::cout << req.get_param_value("type") << std::endl;

        json jsonbody;
        jsonbody["release"] = {{"version", "1.2.0"},
                               {"type", "Raspberry Pi 4"},
                               {"timestamp", "2025-11-12T10:23:00Z"},
                               {"platform", "linux"},
                               {"arch", "x86_64"}};

        jsonbody["files"] = json::array({
            {
             {"path", "/opt/myapp/test"},
             {"hash", {
                       {"algo", "sha256"},
                       {"value", "abcd12318712673abcd..."}
                      }
             }
            },
            {
             {"path", "/opt/myapp/config.yml"},
             {"hash", {
                       {"algo", "sha256"},
                       {"value", "beef5678..."}
                      }
             }
            }
        });

        jsonbody["dependencies"] = {
            "/lib/x86_64-linux-gnu/libgcc_s.so.1",
            "/lib64/ld-linux-x86-64.so.2",
            "/lib/x86_64-linux-gnu/libm.so.6",
            "/lib/x86_64-linux-gnu/libc.so.6",
            "/lib/x86_64-linux-gnu/libstdc++.so.6"
        };

        res.status = 200;
        std::string body = jsonbody.dump();
        res.set_content(body, "application/json");
    });

    server.listen("0.0.0.0", 8080);
}
