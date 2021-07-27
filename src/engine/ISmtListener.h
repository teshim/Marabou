#ifndef __ISMTLISTENER_H__
#define __ISMTLISTENER_H__

#include "PiecewiseLinearCaseSplit.h"
#include "MString.h"

class SmtCore;
class SmtStackEntry;

enum class SolveEvent
{
    UNSAT = 1,
};

struct ImpliedSplit {
    PiecewiseLinearCaseSplit split;
};
struct SplitInfo {
    String msg;
    PiecewiseLinearCaseSplit split;
    List<SmtStackEntry *> const& smtStack;
};

class ISmtListener {

public:
    virtual List<PiecewiseLinearCaseSplit> impliedSplits(SmtCore & smtCore) const = 0;
    virtual void splitOccurred(SplitInfo const& splitInfo) = 0;
    virtual void onUnsat() = 0;

};

#endif // __ISMTLISTENER_H__