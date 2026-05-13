#ifndef ICOMMAND_H
#define ICOMMAND_H

#include "ShellContext.h"
#include "ExecContext.h"

class ICommand {
public:
    virtual ~ICommand() = default;
    virtual int execute(ShellContext& ctx, ExecContext& exec) = 0;
};

#endif // ICOMMAND_H
