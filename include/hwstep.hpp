#ifndef _HWSTEPHPP_
#define _HWSTEPHPP_

#include <basestep.hpp>
#include <AccelStepper.h>

struct StepStatus
{
    uint8_t error;
    uint8_t homed;
    uint8_t moving;
    int16_t position;
};

class HWStep : public BaseStep
{

private:

    AccelStepper *stepper;

    const char* _name;
    bool _isError;
    bool _isHomed;
    bool _holdEnabled;
    bool _outputsEnabled;

    int _hallPin;
    int _enPin;
    int _stepPin;
    int _dirPin;

    long _speed;
    long _accel;

    long _maxPosition;
    long _minPosition;
    long _homeStep;
    long _leadingEdgeBump;

    unsigned long _messageTimeStart;

public:
    HWStep();
    ~HWStep();

    void init(const char* name, int stepPin, int dirPin, int enPin, int theHallPin, long maxPos, long minPos, long leadingEdgeBump, long homeStep, long speed, long accel);
    void run();
    void reInit();
    void hiddenPosition(int location);
    int hiddenPosition();

    bool homeMagnet();

    int currentPosition();
    void currentPosition(int position);
    bool moving();
    void moveAbsTo(int position, bool ignoreLimits = false);
    void moveRel(int delta, bool ignoreLimits = false);
    bool doHomeStep(int direction); // Returns true if successful. False if it a limit.
    bool doLeadingEdgeBump(int direction);
    void startHomingMove(int direction); // Begins continuous move toward limit for magnet search
    void stopMove();                     // Immediately cancels current move

    bool isPastMax();
    bool isPastMin();

    void homePosition(int position);
    int homePosition();

    void isError(bool isError);
    bool isError() const { return _isError; }

    void holdEnabled(bool enabled);
    bool holdEnabled() const { return _holdEnabled; }

    const char* name() { return _name; }

    StepStatus status()
    {
        StepStatus status;
        status.error = _isError;
        status.homed = _isHomed;
        status.moving = moving();
        status.position = currentPosition();
        return status;
    }
};

#endif
