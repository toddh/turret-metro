#ifndef _SIMSTEPHPP_
#define _SIMSTEPHPP_

#include <basestep.hpp>

class SimStep : public BaseStep
{
private:
    int _currentPosition;  // The position reported by the axis
    int _targetPosition;

    int _homePosition = std::numeric_limits<int>::min();    // The position of the home magnet

    int _hiddenPosition;   // The position of the axis, not reported to the user. Used for simulation.
public:
    SimStep();
    ~SimStep();

    void simulate();
    void hiddenPosition(int location) { _hiddenPosition = location; }
    int hiddenPosition() { return _hiddenPosition; }

    bool homeMagnet();

    int currentPosition();
    void currentPosition(int position) { _currentPosition = position; }
    bool moving();
    void moveAbsTo(int position);
    void moveRel(int delta);

    bool isPastMax() { return _currentPosition >= maximumPosition; }
    bool isPastMin() { return _currentPosition <= minimumPosition; }

    void homePosition(int position) { _homePosition = position; }
    int homePosition() { return _homePosition; }
};

#endif
