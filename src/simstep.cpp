#include <Arduino.h>
#include <iostream> // For debugging/logging purposes

#include <simstep.hpp>

SimStep::SimStep() : BaseStep(),
    _currentPosition(0), _targetPosition(0), _hiddenPosition(0)
{
    std::cout << "SimStep initialized." << std::endl;
}

SimStep::~SimStep()
{
    std::cout << "SimStep destroyed." << std::endl;
}

void SimStep::simulate()
{
    // Simulate the movement of the axis
    if (_currentPosition < _targetPosition)
    {
        _currentPosition++;
        _hiddenPosition++;
    }
    else if (_currentPosition > _targetPosition)
    {
        _currentPosition--;
        _hiddenPosition--;
    }

    _hiddenPosition = _hiddenPosition % 100; // Keep the hidden position within a range (0-99)

    // std::cout << "            Simulate: currentPosition= " << _currentPosition << " hiddenPosition= " << _hiddenPosition << std::endl;

}

bool SimStep::homeMagnet()
{
    // Simulate the effect of the home magnet
    if (_hiddenPosition > 10 && _hiddenPosition < 20)
        return true;
    else
        return false;
}

int SimStep::currentPosition()
{
    return _currentPosition;
}

bool SimStep::moving()
{
    return _currentPosition != _targetPosition;
}

void SimStep::moveAbsTo(int position)
{
    _targetPosition = position;
    Serial.printf("            Recvd command absolute to target position: %d\n", _targetPosition);
}

void SimStep::moveRel(int delta)
{
    _targetPosition = _currentPosition + delta;
    Serial.printf("            Recvd command relative %d to target position: %d\n", delta, _targetPosition);
}
