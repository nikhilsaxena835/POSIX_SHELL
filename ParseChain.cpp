#include "ParseChain.h"
#include <cctype>
#include <iostream>

using namespace std;

// Forward declarations — these utilities are defined in main.cpp
extern vector<string> splitByDelimiter(const string& input, char delimiter);
extern vector<string> tokenizeLine(const string& input);
extern bool stripBackgroundToken(vector<string>& tokens);

// ================================================================
// Stage 1: Split raw input by semicolons into pipeline raw texts
// ================================================================
class SequenceSplitStage : public IParseStage {
protected:
    void process(ParseContext& ctx) override {
        vector<string> parts = splitByDelimiter(ctx.rawInput, ';');
        for (auto& part : parts) {
            PipelineNode pn;
            pn.rawText = part;
            // Placeholder: single command with raw text stored temporarily
            CommandNode cn;
            cn.args.push_back(part);  // raw text in args[0], replaced by later stages
            pn.commands.push_back(cn);
            ctx.sequence.pipelines.push_back(pn);
        }
    }
};

// ================================================================
// Stage 2: Split each pipeline's raw text by pipe characters
// ================================================================
class PipelineSplitStage : public IParseStage {
protected:
    void process(ParseContext& ctx) override {
        for (auto& pipeline : ctx.sequence.pipelines) {
            vector<string> segments = splitByDelimiter(pipeline.rawText, '|');
            pipeline.commands.clear();
            for (auto& seg : segments) {
                CommandNode cn;
                cn.args.push_back(seg);  // raw segment text, tokenized next stage
                pipeline.commands.push_back(cn);
            }
        }
    }
};

// ================================================================
// Stage 3: Tokenize each command's raw text into proper args
// ================================================================
class CommandParseStage : public IParseStage {
protected:
    void process(ParseContext& ctx) override {
        for (auto& pipeline : ctx.sequence.pipelines) {
            for (auto& cmd : pipeline.commands) {
                string raw = cmd.args[0];  // the raw text string
                cmd.args = tokenizeLine(raw);
            }
        }
    }
};

// ================================================================
// Stage 4: Detect & strip background token from last command
// ================================================================
class BackgroundDetectStage : public IParseStage {
protected:
    void process(ParseContext& ctx) override {
        for (auto& pipeline : ctx.sequence.pipelines) {
            if (!pipeline.commands.empty()) {
                pipeline.background = stripBackgroundToken(pipeline.commands.back().args);
            }
        }
    }
};

// ================================================================
// Stage 5: Extract redirection tokens from args into Redirection list
// ================================================================
class RedirectionExtractStage : public IParseStage {
protected:
    void process(ParseContext& ctx) override {
        for (auto& pipeline : ctx.sequence.pipelines) {
            for (auto& cmd : pipeline.commands) {
                vector<string> cleaned;
                for (size_t i = 0; i < cmd.args.size(); ++i) {
                    const string& tok = cmd.args[i];
                    if (tok == ">" || tok == ">>" || tok == "<") {
                        if (i + 1 >= cmd.args.size()) {
                            ctx.valid = false;
                            ctx.error = "Redirection error: missing file";
                            return;
                        }
                        Redirection r;
                        if (tok == "<")       r.type = RedirType::Input;
                        else if (tok == ">")  r.type = RedirType::Output;
                        else                  r.type = RedirType::Append;
                        r.filename = cmd.args[i + 1];
                        cmd.redirections.push_back(r);
                        ++i;  // skip filename
                    } else {
                        cleaned.push_back(tok);
                    }
                }
                cmd.args = cleaned;
            }
        }
    }
};

// ================================================================
// Factory: build the default 5-stage chain
// ================================================================
unique_ptr<IParseStage> buildDefaultParseChain() {
    auto s1 = make_unique<SequenceSplitStage>();
    auto s2 = make_unique<PipelineSplitStage>();
    auto s3 = make_unique<CommandParseStage>();
    auto s4 = make_unique<BackgroundDetectStage>();
    auto s5 = make_unique<RedirectionExtractStage>();

    s4->setNext(move(s5));
    s3->setNext(move(s4));
    s2->setNext(move(s3));
    s1->setNext(move(s2));
    return s1;
}
