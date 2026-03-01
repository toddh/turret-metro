#include <Arduino.h>
#include <iostream> // For debugging/logging purposes
#include <AccelStepper.h>

#include <hwstep.hpp>

#include <globals.h>

#include <elapsedMillis.h>


elapsedMillis timeElapsed;
unsigned long timeStart;

HWStep::HWStep()
{
    stepper = nullptr;
    _name = "Unknown";
    _hallPin = -1;
    _enPin = -1;

    _speed = 0;
    _accel = 0;

    _maxPosition = 0;
    _minPosition = 0;

    _messageTimeStart = timeElapsed;
    _homeStep = 1;
    _leadingEdgeBump = 1;

    _isHomed = false;
    _isError = false;
}

HWStep::~HWStep()
{
    if (stepper != nullptr)
        delete stepper;
}

void HWStep::init(const char* name, int dirPin, int stepPin, int enPin, int theHallPin, long maxPos, long minPos, long leadingEdgeBump, long homeStep, long speed, long accel)
{
    _name = name;
    stepper = new AccelStepper(AccelStepper::DRIVER, stepPin, dirPin);
    // console.printf("    this address: %p for dirPin: %d\n", static_cast<void*>(this), dirPin);
    // console.printf("    accelstepper address: %p\n", static_cast<void*>(stepper));

    _hallPin = theHallPin;

    pinMode(_hallPin, INPUT_PULLUP);
    pinMode(_enPin, INPUT_PULLUP);

    _speed = speed;
    _accel = accel;

    stepper->setMaxSpeed(_speed);
    stepper->setAcceleration(_accel);

    stepper->setEnablePin(enPin);
    stepper->setPinsInverted(false, false, true);

    _maxPosition = maxPos;
    _minPosition = minPos;
    _homeStep = homeStep;
    _leadingEdgeBump = leadingEdgeBump;

    _isHomed = false;
    _isError = false;
}

void HWStep::reInit()
{
    _isHomed = false;
    _isError = false;
    stepper->setCurrentPosition(0);
}

void HWStep::run()
{
    if (stepper->distanceToGo() == 0)
    {
        // console.printf("Time elapsed: %lu\n", (unsigned long)timeElapsed - timeStart));

        stepper->disableOutputs(); // TODO: Maybe put disable Outputs on a delay?
    }
    else
    {
        stepper->run();

        unsigned long difference = timeElapsed - _messageTimeStart;
        if (difference > 2000)
        {
            _messageTimeStart = timeElapsed;
            LOG_HWSTEP_PRINTF("          %s::run -  position: %d, distance to go: %d, magnet: %d\n", _name, stepper->currentPosition(), stepper->distanceToGo(), this->homeMagnet());
        }
    }
}

bool HWStep::homeMagnet()
{
    return !digitalRead(_hallPin);
}

int HWStep::currentPosition()
{
    return stepper->currentPosition();
}

bool HWStep::moving()
{
    return stepper->distanceToGo() != 0;
}

void HWStep::moveAbsTo(int position, bool ignoreLimits)
{
    if (!ignoreLimits)
    {
        if (position >= this->_maxPosition || position <= this -> _minPosition) {
            console.printf("%s::moveAbsTo: MOVE OUTSIDE LIMITS (pos=%d, min=%ld, max=%ld). AXIS IS IN ERROR.\n", _name, position, _minPosition, _maxPosition);
            _isError = true;
            return;
        }
    }

    stepper->moveTo(position);
    stepper->enableOutputs();

    timeStart = timeElapsed;
}

void HWStep::moveRel(int delta, bool ignoreLimits)
{
    if (!ignoreLimits)
    {    
        if (stepper->currentPosition() + delta >= this->_maxPosition || stepper->currentPosition() + delta <= this -> _minPosition)
        {
            console.printf("%s::moveRel: MOVE OUTSIDE LIMITS (target=%ld, min=%ld, max=%ld). AXIS IS IN ERROR.\n", _name, stepper->currentPosition() + delta, _minPosition, _maxPosition);
            _isError = true;
            return;
        }
    }


    stepper->move(delta);
    stepper->enableOutputs();

    timeStart = timeElapsed;
}

bool HWStep::doHomeStep(int direction)
{
    int delta = direction * _homeStep;

    if (stepper->currentPosition() + delta >= this->_maxPosition || stepper->currentPosition() + delta <= this -> _minPosition) {
        LOG_HWSTEP_PRINTF("%s::doHomeStep: MOVE OUTSIDE LIMITS. AXIS IS NOW IN ERROR.\n", _name);
        _isError = true;
        return false;

    }
    else
    {
        stepper->move(delta);
        stepper->enableOutputs();
    }

    timeStart = timeElapsed;

    return true;
}

bool HWStep::doLeadingEdgeBump(int direction)
{
    int delta = direction * _leadingEdgeBump;

    if (stepper->currentPosition() + delta >= this->_maxPosition || stepper->currentPosition() + delta <= this -> _minPosition)
    {
        LOG_HWSTEP_PRINTF("%s::doLeadingEdgeBump: MOVE OUTSIDE LIMITS. AXIS IS NOW IN ERROR.\n", _name);
        _isError = true;
        return false;
    }
    else
    {
        stepper->move(delta);
        stepper->enableOutputs();
    }

    timeStart = timeElapsed;

    return true;
}


bool HWStep::isPastMax()
{
    return stepper->currentPosition() > _maxPosition;
}
bool HWStep::isPastMin()
{
    return stepper->currentPosition() < _minPosition;
}

void HWStep::isError(bool isError)
{
    _isError = isError;

}

void HWStep::homePosition(int position)
{
    stepper->setCurrentPosition(position);
    _isHomed = true;
}
