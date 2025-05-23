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
}

HWStep::~HWStep()
{
    if (stepper != nullptr)
        delete stepper;
}

void HWStep::init(int dirPin, int stepPin, int enPin, int theHallPin, long maxPos, long minPos, long leadingEdgeBump, long homeStep, long speed, long accel)
{
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
}

void HWStep::run()
{
    if (stepper->distanceToGo() == 0)
    {
        unsigned long difference = timeElapsed - timeStart;
        // console.printf("Time elapsed: %lu\n", difference);

        stepper->disableOutputs(); // TODO: Maybe put disable Outputs on a delay?
    }
    else
    {
        stepper->run();

        unsigned long difference = timeElapsed - _messageTimeStart;
        if (difference > 2000)
        {
            _messageTimeStart = timeElapsed;
            console.printf("          HWStep::run -  position: %d, distance to go: %d, magnet: %d\n", stepper->currentPosition(), stepper->distanceToGo(), this->homeMagnet());
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
            console.printf("moveAbsTo: MOVE OUTSIDE LIMITS. COMMAND IGNORED.\n");
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
            console.printf("moveRel: MOVE OUTSIDE LIMITS. COMMAND IGNORED.\n");
            return;
        }
    }


    stepper->move(delta);
    stepper->enableOutputs();

    timeStart = timeElapsed;
}

void HWStep::doHomeStep(int direction)
{
    int delta = direction * _homeStep;

    if (stepper->currentPosition() + delta >= this->_maxPosition || stepper->currentPosition() + delta <= this -> _minPosition)
        console.printf("doHomeStep: MOVE OUTSIDE LIMITS. COMMAND IGNORED.\n");
    else
    {
        stepper->move(delta);
        stepper->enableOutputs();
    }

    timeStart = timeElapsed;
}

void HWStep::doLeadingEdgeBump(int direction)
{
    int delta = direction * _leadingEdgeBump;

    if (stepper->currentPosition() + delta >= this->_maxPosition || stepper->currentPosition() + delta <= this -> _minPosition)
        console.printf("doLeadingEdgeBump: MOVE OUTSIDE LIMITS. COMMAND IGNORED.\n");
    else
    {
        stepper->move(delta);
        stepper->enableOutputs();
    }

    timeStart = timeElapsed;
}


bool HWStep::isPastMax()
{
    return stepper->currentPosition() > _maxPosition;
}
bool HWStep::isPastMin()
{
    return stepper->currentPosition() < _minPosition;
}
void HWStep::homePosition(int position)
{
    stepper->setCurrentPosition(position);
    _isHomed = true;
}
