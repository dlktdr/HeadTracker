#include "boardtype.h"

BoardType::BoardType(QObject *parent) : QObject(parent)
{
    trkset = nullptr;
    alwaccess = false;
}

void BoardType::setTracker(TrackerSettings *t)
{
    trkset = t;
}
