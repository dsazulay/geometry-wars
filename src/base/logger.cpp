#include "logger.h"

namespace logger
{
    Level logLevel = Level::Debug;
    bool isInclusiveFilter = true;

    void setFilter(Level level, bool isInclusive)
    {
        logLevel = level;
        isInclusiveFilter = isInclusive;
    }

    bool checkLevel(Level level)
    {
        return (isInclusiveFilter && level >= logLevel) ||
            ((level & logLevel) == level);
    }
}
