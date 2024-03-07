#ifndef TTRACKER_H
#define TTRACKER_H

#include "General.h"
//---------------------------------------------------------------------------
class TTracker
{
public:
    TTracker();
    virtual ~TTracker();

    void Init(void);
    void Execute(std::vector<bbox_t>& boxes);
protected:

private:
};
//---------------------------------------------------------------------------
#endif // TTRACKER_H
