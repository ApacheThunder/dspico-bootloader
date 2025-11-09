#include <nds.h>
#include "Environment.h"

u32 Environment::_flags;

void Environment::Initialize()
{
    _flags = ENVIRONMENT_FLAGS_NONE;
    if (isDSiMode())
    {
        _flags |= ENVIRONMENT_FLAGS_DSI_MODE;
    }
}