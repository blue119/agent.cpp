#pragma once

#include "model.h"
#include <memory>
#include <string>

namespace agent_cpp {

// RemoteModel: OpenAI-compatible chat completions client (OpenRouter).
//
// Environment variables:
// - OPENROUTER_API_KEY (required)
// - OPENROUTER_MODEL (optional; default: "openai/gpt-4.1-mini")
// - OPENROUTER_BASE_URL (optional; default: "https://openrouter.ai/api/v1")
//
// Notes:
// - Non-streaming implementation (the callback, if provided, is called once).
// - Tool calling is supported via OpenAI-compatible `tools`.
class RemoteModel : public IModel
{
  public:
    struct Config
    {
        std::string base_url = "https://openrouter.ai/api/v1";
        std::string api_key;
        std::string model = "openai/gpt-4.1-mini";
        int timeout_sec = 120;
    };

    static std::shared_ptr<RemoteModel> create_from_env();
    static std::shared_ptr<RemoteModel> create(const Config& cfg);

    common_chat_msg generate(const std::vector<common_chat_msg>& messages,
                             const std::vector<common_chat_tool>& tools,
                             const ResponseCallback& callback = nullptr) override;

    [[nodiscard]] bool supports_prompt_cache() const override { return false; }

  private:
    explicit RemoteModel(Config cfg);

    Config cfg_;
};

} // namespace agent_cpp
