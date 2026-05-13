#ifndef PARSECHAIN_H
#define PARSECHAIN_H

#include <memory>
#include <string>
#include "ParseTypes.h"

struct ParseContext {
    std::string rawInput;
    SequenceNode sequence;
    bool valid = true;
    std::string error;
};

class IParseStage {
public:
    virtual ~IParseStage() = default;
    void setNext(std::unique_ptr<IParseStage> next) { next_ = std::move(next); }
    void handle(ParseContext& ctx) {
        if (!ctx.valid) return;
        process(ctx);
        if (next_ && ctx.valid) next_->handle(ctx);
    }
protected:
    virtual void process(ParseContext& ctx) = 0;
    std::unique_ptr<IParseStage> next_;
};

// Build the default 5-stage parsing chain:
// SequenceSplit -> PipelineSplit -> CommandParse -> BackgroundDetect -> RedirectionExtract
std::unique_ptr<IParseStage> buildDefaultParseChain();

#endif // PARSECHAIN_H
