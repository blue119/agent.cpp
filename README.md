# agent.cpp

Building blocks for agents in C++.

## Why this repo?

- There is a wide variety of agent frameworks available, but the most popular ones implemented in Python and/or TypeScript.

    We are pushing the direction of small, specialized, **local** agents, where C++ is a much better fit.

- Everyone is still figuring out what features and/or patterns work best for agentic systems.

    The building blocks provided here are flexible enough to implement most (all?) of the currently popular features and/or patterns
    and adapt to new trends.
    If you think "*that could be a callback of a tool*", you are probably right.

- Frameworks justify their existence (and/or fundraising) by providing built-in features and patterns.

    You are "paying" (with latency, memory and/or code maintenance) for features that your system doesn't need.
    Implementing those features by yourself is usually not that much effort, with the benefit of having full control and flexibility.

## Building Blocks

We define an `agent` by 5 building blocks:

- [Agent Loop](./#agent-loop)
- [Callbacks](./#callbacks)
- [Instructions](./#instructions)
- [Model](./#model)
- [Tools](./#tools)

## Examples

- [Memory](./examples/memory/README.md)
    Use `tools` that allow an agent to store and retrieve relevant information across conversations.

- [Opentelemetry Tracing](./examples/tracing/README.md)
    Use `callbacks` collect a record of the steps of the agent loop.

- [Shell execution with human-in-the-loop](./examples/memory/README.md)
    Allow an agent to write shell scripts to perform multiple actions instead of calling different tools.
    Use `callbacks` for human-in-the-loop interactions.

- [Context Engineering](./examples/context-engineering/README.md)
    Use `callbacks` to manipulate the context between iterations of the agent loop.

## Agent Loop

In the current LLM (Large Language Models) world, and `agent` is usually a simple loop that intersperses `Model Calls` and `Tool Executions`, until a stop condition is met:

```mermaid
graph TD
    User([User Input]) --> Model[Model Call]
    Model --> Decision{Stop Condition Met?}
    Decision -->|Yes| End([End])
    Decision -->|No| Tool[Tool Execution]
    Tool --> Model
```

There are different ways to implement the stop condition.
By default we let the agent decide by generating an output *without* tool executions.
You can implement additional stop conditions via callbacks.

## Callbacks

Callbacks allow you to insert logic deterministically before and/or after specific points in the loop:

## Tools

Current Models can only generate static content like text (let's leave)
A tool is a convention established between the user and the LLM,
