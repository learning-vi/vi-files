#include "e.h"

void
inheritance()
{
    if (getenv(E_INHERIT)){
        inherit = 1;
    }

    if (getenv(E_SAFE_INHERIT)){
        safe_inherit = 1;
    }

    return;
}
