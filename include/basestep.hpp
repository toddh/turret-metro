#ifndef _BASE_STEP_HPP_
#define _BASE_STEP_HPP_

#include <limits>

class BaseStep
{
    protected:
    const int maximumPosition = 5000;
    const int minimumPosition = -5000;

private:


public:
    BaseStep();
    ~BaseStep() = default;

    void init(int stepPin, int dirPin, int enPin, int theHallPin, long maxPos, long minPos, long speed, long accel);
    void simulate();
    void hiddenPosition(int location);
    int hiddenPosition();

    bool homeMagnet();

    int currentPosition();
    void currentPosition(int position);
    bool moving();
    void moveAbsTo(int position);
    void moveRel(int delta);

    bool isPastMax();
    bool isPastMin();

    void homePosition(int position);
    int homePosition();
};

#endif
