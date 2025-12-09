#pragma once

#include "agent.h"
#include "chat.h"
#include "model.h"
#include <filesystem>
#include <string>
#include <vector>

inline std::vector<llama_token>
build_agent_prompt_tokens(Agent& agent)
{
    Model* model = agent.get_model();
    if (!model) {
        return {};
    }

    std::vector<common_chat_msg> system_messages;
    const std::string& instructions = agent.get_instructions();
    if (!instructions.empty()) {
        common_chat_msg system_msg;
        system_msg.role = "system";
        system_msg.content = instructions;
        system_messages.push_back(system_msg);
    }

    std::vector<common_chat_tool> tool_definitions =
      agent.get_tool_definitions();

    common_chat_templates_inputs inputs;
    inputs.messages = system_messages;
    inputs.tools = tool_definitions;
    inputs.tool_choice = COMMON_CHAT_TOOL_CHOICE_AUTO;
    inputs.add_generation_prompt = false;
    inputs.enable_thinking = false;

    auto params = common_chat_templates_apply(model->get_templates(), inputs);

    return model->tokenize(params.prompt);
}

inline bool
load_or_create_agent_cache(Agent& agent, const std::string& cache_path)
{
    Model* model = agent.get_model();
    if (!model) {
        return false;
    }

    if (std::filesystem::exists(cache_path)) {
        auto cached_tokens = model->load_cache(cache_path);
        if (!cached_tokens.empty()) {
            printf("Loaded prompt cache from '%s' (%zu tokens)\n",
                   cache_path.c_str(),
                   cached_tokens.size());
            return true;
        }
    }

    auto prompt_tokens = build_agent_prompt_tokens(agent);
    if (prompt_tokens.empty()) {
        return true;
    }

    printf("Creating prompt cache at '%s' (%zu tokens)\n",
           cache_path.c_str(),
           prompt_tokens.size());

    // warms the KV cache
    model->generate_from_tokens(prompt_tokens);

    return model->save_cache(cache_path);
}
