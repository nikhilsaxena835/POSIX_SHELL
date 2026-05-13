#ifndef PARSETYPES_H
#define PARSETYPES_H

#include <string>
#include <vector>

enum class RedirType { Input, Output, Append };

struct Redirection {
    RedirType type;
    std::string filename;
};

struct CommandNode {
    std::vector<std::string> args;           // command + arguments (redirect tokens removed)
    std::vector<Redirection> redirections;
};

struct PipelineNode {
    std::string rawText;                     // original text for history recording
    std::vector<CommandNode> commands;
    bool background = false;
};

struct SequenceNode {
    std::vector<PipelineNode> pipelines;
};

#endif // PARSETYPES_H
