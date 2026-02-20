#include "agent.h"
#include "calculator_tool.h"
#include "chat_loop.h"
#include "error.h"
#include "logging_callback.h"
#include "remote_model.h"

#include <cstdio>
#include <memory>

int
main()
{
    try {
        printf("Creating OpenRouter remote model from env...\n");
        auto model = agent_cpp::RemoteModel::create_from_env();

        std::vector<std::unique_ptr<agent_cpp::Tool>> tools;
        tools.push_back(std::make_unique<CalculatorTool>());

        const std::string instructions =
          "You are a helpful assistant. Use the calculator tool for math. "
          "When you call tools, provide valid JSON arguments.";

        std::vector<std::unique_ptr<agent_cpp::Callback>> callbacks;
        callbacks.push_back(std::make_unique<LoggingCallback>());

        agent_cpp::Agent agent(std::move(model), std::move(tools),
                               std::move(callbacks), instructions);

        printf("\nRemote chat ready!\n");
        printf("Set env OPENROUTER_API_KEY, optional OPENROUTER_MODEL.\n");
        printf("Type an empty line to quit.\n\n");

        run_chat_loop(agent);
    } catch (const std::exception& e) {
        fprintf(stderr, "error: %s\n", e.what());
        return 1;
    }

    return 0;
}
