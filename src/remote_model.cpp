#include "remote_model.h"

#include "error.h"

#include <cstdlib>
#include <sstream>
#include <stdexcept>

#define CPPHTTPLIB_OPENSSL_SUPPORT
#include "httplib.h"
#include <nlohmann/json.hpp>

namespace agent_cpp {

using json = nlohmann::json;

namespace {

static std::string
get_env_str(const char* key)
{
    const char* v = std::getenv(key);
    return v ? std::string(v) : std::string();
}

static void
parse_url(const std::string& url, std::string& host, std::string& path)
{
    // Keep scheme in host because httplib::Client accepts it (as used elsewhere
    // in this repo).
    size_t scheme_end = url.find("://");
    size_t host_start = (scheme_end != std::string::npos) ? scheme_end + 3 : 0;

    size_t path_start = url.find('/', host_start);
    if (path_start != std::string::npos) {
        host = url.substr(0, path_start);
        path = url.substr(path_start);
    } else {
        host = url;
        path = "/";
    }
}

static json
to_openai_message(const common_chat_msg& m)
{
    json j;
    j["role"] = m.role;

    // OpenAI-compatible format:
    // - system/user/assistant: content
    // - tool: content + tool_call_id
    // - assistant tool calls: tool_calls
    if (m.role == "tool") {
        j["content"] = m.content;
        if (!m.tool_call_id.empty()) {
            j["tool_call_id"] = m.tool_call_id;
        }
    } else {
        j["content"] = m.content;
    }

    // If assistant message contains tool_calls, convert them.
    if (m.role == "assistant" && !m.tool_calls.empty()) {
        json tcs = json::array();
        for (const auto& tc : m.tool_calls) {
            json t;
            if (!tc.id.empty()) {
                t["id"] = tc.id;
            }
            t["type"] = "function";
            t["function"] = {
                { "name", tc.name },
                { "arguments", tc.arguments },
            };
            tcs.push_back(std::move(t));
        }
        j["tool_calls"] = std::move(tcs);
    }

    return j;
}

static json
to_openai_tools(const std::vector<common_chat_tool>& tools)
{
    json out = json::array();
    for (const auto& t : tools) {
        json fn;
        fn["name"] = t.name;
        fn["description"] = t.description;
        // t.parameters is a JSON string
        try {
            fn["parameters"] = json::parse(t.parameters);
        } catch (...) {
            // If it's not valid JSON, fall back to an empty object.
            fn["parameters"] = json::object();
        }

        json tool;
        tool["type"] = "function";
        tool["function"] = std::move(fn);
        out.push_back(std::move(tool));
    }
    return out;
}

} // namespace

RemoteModel::RemoteModel(Config cfg)
  : cfg_(std::move(cfg))
{
}

std::shared_ptr<RemoteModel>
RemoteModel::create_from_env()
{
    Config cfg;
    cfg.api_key = get_env_str("OPENROUTER_API_KEY");
    if (cfg.api_key.empty()) {
        throw ModelError("OPENROUTER_API_KEY is not set");
    }

    const auto model = get_env_str("OPENROUTER_MODEL");
    if (!model.empty()) {
        cfg.model = model;
    }

    const auto base_url = get_env_str("OPENROUTER_BASE_URL");
    if (!base_url.empty()) {
        cfg.base_url = base_url;
    }

    const auto timeout = get_env_str("OPENROUTER_TIMEOUT_SEC");
    if (!timeout.empty()) {
        try {
            cfg.timeout_sec = std::stoi(timeout);
        } catch (...) {
            // ignore
        }
    }

    return create(cfg);
}

std::shared_ptr<RemoteModel>
RemoteModel::create(const Config& cfg)
{
    return std::shared_ptr<RemoteModel>(new RemoteModel(cfg));
}

common_chat_msg
RemoteModel::generate(const std::vector<common_chat_msg>& messages,
                      const std::vector<common_chat_tool>& tools,
                      const ResponseCallback& callback)
{
    // Build request
    json body;
    body["model"] = cfg_.model;

    json msgs = json::array();
    for (const auto& m : messages) {
        msgs.push_back(to_openai_message(m));
    }
    body["messages"] = std::move(msgs);

    if (!tools.empty()) {
        body["tools"] = to_openai_tools(tools);
        body["tool_choice"] = "auto";
    }

    std::string host;
    std::string base_path;
    parse_url(cfg_.base_url, host, base_path);

    std::string path = base_path;
    if (!path.empty() && path.back() == '/') {
        path.pop_back();
    }
    path += "/chat/completions";

    httplib::Client client(host);
    client.set_read_timeout(cfg_.timeout_sec, 0);
    client.set_connection_timeout(cfg_.timeout_sec, 0);

    httplib::Headers headers = {
        { "Content-Type", "application/json" },
        { "Authorization", std::string("Bearer ") + cfg_.api_key },
        // Optional but recommended by OpenRouter
        { "HTTP-Referer", "https://github.com/blue119/agent.cpp" },
        { "X-Title", "agent.cpp" },
    };

    auto res = client.Post(path.c_str(), headers, body.dump(), "application/json");
    if (!res) {
        throw ModelError(std::string("OpenRouter request failed: ") + httplib::to_string(res.error()));
    }
    if (res->status < 200 || res->status >= 300) {
        std::ostringstream oss;
        oss << "OpenRouter HTTP " << res->status << ": " << res->body;
        throw ModelError(oss.str());
    }

    json resp;
    try {
        resp = json::parse(res->body);
    } catch (const std::exception& e) {
        throw ModelError(std::string("Failed to parse OpenRouter response JSON: ") + e.what());
    }

    // Parse OpenAI-compatible response
    common_chat_msg out;
    out.role = "assistant";

    if (!resp.contains("choices") || resp["choices"].empty()) {
        throw ModelError("OpenRouter response has no choices");
    }

    json msg = resp["choices"][0]["message"];
    if (msg.contains("content") && msg["content"].is_string()) {
        out.content = msg["content"].get<std::string>();
    }

    if (msg.contains("tool_calls") && msg["tool_calls"].is_array()) {
        for (const auto& tc : msg["tool_calls"]) {
            common_chat_tool_call ctc;
            if (tc.contains("id") && tc["id"].is_string()) {
                ctc.id = tc["id"].get<std::string>();
            }
            if (tc.contains("function")) {
                auto fn = tc["function"];
                if (fn.contains("name") && fn["name"].is_string()) {
                    ctc.name = fn["name"].get<std::string>();
                }
                if (fn.contains("arguments") && fn["arguments"].is_string()) {
                    ctc.arguments = fn["arguments"].get<std::string>();
                } else if (fn.contains("arguments") && fn["arguments"].is_object()) {
                    ctc.arguments = fn["arguments"].dump();
                }
            }
            if (!ctc.name.empty()) {
                out.tool_calls.push_back(std::move(ctc));
            }
        }
    }

    if (callback) {
        // Non-streaming: emit once.
        callback(out.content);
    }

    return out;
}

} // namespace agent_cpp
