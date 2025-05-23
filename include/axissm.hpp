
// optional: enable FSM structure report in debugger
#undef FFSM2_ENABLE_STRUCTURE_REPORT

#include <ffsm2/machine.hpp>
#include <iostream>

#include <hwstep.hpp>

#include <globals.h>

#define MAX_POSITIVE_NOW_REVERSE 1700 // How far are we willing to go forward before we reverse
									  // This is different than the overall max, which is set for safety.

//------------------------------------------------------------------------------

// events

struct MoveRel
{
	bool ignoreLimits;
	int delta;
};
struct MoveAbs
{
	bool ignoreLimits;
	int position;
};
struct StartHoming
{
};
struct StopMoving
{
};

// data shared between FSM states and outside code
struct Context
{
	HWStep *stepper;
	int edgeLocationA = std::numeric_limits<int>::min();
	int edgeLocationB = std::numeric_limits<int>::min();
	bool desiredMagnet = 0; // Store what transition we are looking for in the magnet. For use by the FSM.
};

struct Payload
{
	int step; // The direction to move, mainly used during homing
	bool desiredMagnet;
};

// convenience typedef

using Config = ffsm2::Config::ContextT<Context &>::PayloadT<Payload>;

using M = ffsm2::MachineT<Config>;

// Macro magic can be invoked to simplify FSM structure declaration
#define S(s) struct s

// state machine structure
using FSM = M::Root<S(On),
					S(Idle),
					S(Moving),
					S(HomeInit),
					S(HomeBackupFirst),
					S(HomeFindLeadingEdge),
					S(HomeFindTrailingEdge),
					S(SetHomeLocation),
					S(Error)>;

#undef S

//------------------------------------------------------------------------------

static_assert(FSM::stateId<Idle>() == 0, "");
static_assert(FSM::stateId<Moving>() == 1, "");
static_assert(FSM::stateId<HomeInit>() == 2, "");
static_assert(FSM::stateId<HomeBackupFirst>() == 3, "");
static_assert(FSM::stateId<HomeFindLeadingEdge>() == 4, "");
static_assert(FSM::stateId<HomeFindTrailingEdge>() == 5, "");
static_assert(FSM::stateId<SetHomeLocation>() == 6, "");
static_assert(FSM::stateId<Error>() == 7, "");

////////////////////////////////////////////////////////////////////////////////

// top-level region in the hierarchy
struct On
	: FSM::State // necessary boilerplate!
{
	// called on state entry
	void enter(Control &control)
	{
		console.println("  On::enter");
	}
};

//------------------------------------------------------------------------------

// sub-states
struct Idle
	: FSM::State
{
	void enter(Control &control)
	{
		console.println("  Idle::enter");
	}

	// state can initiate transitions to _any_ other state
	void update(FullControl &control)
	{
		// console.println("  Idle::Update");
	}

	void react(const MoveRel &event, FullControl &control)
	{
		console.printf("    Idle - react to MoveRel: %d\n", event.delta);
		control.context().stepper->moveRel(event.delta, event.ignoreLimits);
		control.changeWith<Moving>(Payload{0, false});
	}
	void react(const MoveAbs &event, FullControl &control)
	{
		console.printf("    Idle - react to MoveAbs: %d\n", event.position);
		control.context().stepper->moveAbsTo(event.position, event.ignoreLimits);
		control.changeWith<Moving>(Payload{0, false});
	}
	void react(const StartHoming &event, FullControl &control)
	{
		console.println("    Idle - react to StartHoming");
		control.changeWith<HomeInit>(Payload{0, false});
	}
	void react(const StopMoving &event, FullControl &control)
	{
		console.println("    Idle - react to StopMoving");
	}	
};

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

struct Moving
	: FSM::State
{
	void enter(Control &)
	{
		console.println("  Moving::enter");
	}

	void update(FullControl &control)
	{
		// console.println("  Moving::update");
		if (control.context().stepper->moving() == false)
		{
			console.printf("  Moving::done - position: %d magnet: %c\n", control.context().stepper->currentPosition(), control.context().stepper->homeMagnet() ? '1' : '0');
			control.changeWith<Idle>(Payload{0, false});
		}
		control.context().stepper->run();
	}

	void react(const StopMoving &event, FullControl &control)
	{
		console.printf("    Idle - react to Stop\n");
		control.changeWith<Idle>(Payload{0, false});
	}

	template <typename TEvent>
	void react(const TEvent &, FullControl &)
	{
		// No-op for unhandled events
	}
};

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

struct HomeInit
	: FSM::State
{
	void enter(Control &control)
	{
		console.println("    HomeInit::enter");
		control.context().edgeLocationA = std::numeric_limits<int>::min();
		control.context().edgeLocationB = std::numeric_limits<int>::min();
	}

	// We first start looking forward. Always forward.  Either for the front edge if we're on the magnet.
	// Or the back edge if we're not on the magnet.

	void update(FullControl &control)
	{
		// console.println("    HomeInit::update");
		if (control.context().stepper->homeMagnet())
		{
			console.println("      Already on magnet (1)");
			control.context().stepper->moveRel(-400, true); // Back up to get off the magnet.
			// TODO: Consider using something different than moveRel in homing
			control.changeWith<HomeBackupFirst>(Payload{1, false});
		}
		else
		{
			console.println("      Not on magnet (0)");
			control.context().stepper->doHomeStep(1);
			control.changeWith<HomeFindLeadingEdge>(Payload{1, true});
		}

		control.context().stepper->run();
	}

	void react(const StopMoving &event, FullControl &control)
	{
		console.printf("    HomeInit - react to Stop\n");
		control.changeWith<Idle>(Payload{0, false});
	}

	template <typename TEvent>
	void react(const TEvent &, FullControl &)
	{
		// No-op for unhandled events
	}
};

//------------------------------------------------------------------------------

struct HomeBackupFirst
	: FSM::State
{
	void enter(Control &control)
	{
		console.println("    HomeBackupFirst::enter");
	}

	void update(FullControl &control)
	{
		// console.println("    HomeBackupFirst::update");
		if (control.context().stepper->moving() == false)
		{
			console.printf("    HomeBackupFirst - done - position: %d magnet: %c\n", control.context().stepper->currentPosition(), control.context().stepper->homeMagnet() ? '1' : '0');
			control.changeWith<HomeFindLeadingEdge>(Payload{1, true});
		}
		control.context().stepper->run();
	}

	void react(const StopMoving &event, FullControl &control)
	{
		console.printf("    HomeBackupFirst - react to Stop\n");
		// control.changeWith<Idle>(Payload{0, false});
	}

	template <typename TEvent>
	void react(const TEvent &, FullControl &)
	{
		// No-op for unhandled events
	}
};

//------------------------------------------------------------------------------

struct HomeFindLeadingEdge
	: FSM::State
{
	void enter(PlanControl &control)
	{
		console.printf("        HomeFindLeadingEdge::enter\n");
	}

	void update(FullControl &control)
	{
		// console.printf("        HomeFindEdge::update - magnet: %c\n", control.context().stepper->homeMagnet() ? 'M' : 'N');

		if (control.context().stepper->isPastMax() || control.context().stepper->isPastMin())
		{
			console.printf("===================================================================\n");
			console.printf("          >>>> HomeFindLeadingEdge::update - past emergency limits\n");
			console.printf("          >>>> HomeFindLeadingEdge::update - transitioning to error\n");
			console.printf("===================================================================\n");
			control.changeWith<Error>(Payload{0, false});
		}
		else
		{
			if (control.context().stepper->homeMagnet() == 1)
			{
				// We have found the leading edge.

				control.context().edgeLocationA = control.context().stepper->currentPosition();
				console.printf("          >>>> edgeA found: %d\n", control.context().edgeLocationA);
				console.printf("		  >>>> magnet: %d\n", control.context().stepper->homeMagnet());

				control.context().stepper->doLeadingEdgeBump(1);
				control.changeWith<HomeFindTrailingEdge>(Payload{0, false});
			}
			else // Have not found the leading edge.
			{
				// Have we gone too far? Is so then go in reverse.
				if (control.context().stepper->currentPosition() > MAX_POSITIVE_NOW_REVERSE)
				{
					console.printf("          >>>> %d - Too Far, back way up\n", control.context().stepper->currentPosition());
					control.context().stepper->moveRel(-2 * MAX_POSITIVE_NOW_REVERSE, true);
					// TODO: Consider using something different than moveRel in homing
					control.changeWith<HomeBackupFirst>(Payload{0, false});
				}
				else
				{
					control.context().stepper->doHomeStep(1);
				}
			}

			control.context().stepper->run();
		}
	}

	void react(const StopMoving &event, FullControl &control)
	{
		console.printf("    HomeFindEdge - react to Stop\n");
		control.changeWith<Idle>(Payload{0, false});
	}

	template <typename TEvent>
	void react(const TEvent &, FullControl &)
	{
		// No-op for unhandled events
	}
};
//------------------------------------------------------------------------------

struct HomeFindTrailingEdge
	: FSM::State
{
	void enter(Control &control)
	{
		console.printf("        HomeFindTrailingEdge::enter\n");
	}

	void update(FullControl &control)
	{
		// console.printf("        HomeFindTrailingEdge::update - magnet: %c\n", control.context().stepper->homeMagnet() ? '1' : '0');

		if (control.context().stepper->isPastMax() || control.context().stepper->isPastMin())
		{
			console.printf("===================================================================\n");
			console.printf("          >>>> HomeFindTrailingEdge::update - past max or min\n");
			console.printf("          >>>> HomeFindLeadingEdge::update - transitioning to Error\n");
			console.printf("===================================================================\n");
			control.changeWith<Error>(Payload{0, false});
		}
		else
		{
			if (control.context().stepper->moving() == false)
			{

				if (control.context().stepper->homeMagnet() == 0)
				{
					control.context().edgeLocationB = control.context().stepper->currentPosition();

					console.printf("          >>>> both edges found\n");
					console.printf("		  >>>> magnet: %d\n", control.context().stepper->homeMagnet());
					console.printf("          >>>> edgeA: %d\n", control.context().edgeLocationA);
					console.printf("          >>>> edgeB: %d\n", control.context().edgeLocationB);

					int homePosition = (control.context().edgeLocationA + control.context().edgeLocationB) / 2;
					console.printf("          >>>> Home Position: %d\n", homePosition);

					control.context().stepper->moveAbsTo(homePosition);
					control.changeWith<SetHomeLocation>(Payload{0, false});
				}
				else
				{
					control.context().stepper->doHomeStep(1);
					// TODO: Consider using something different than moveRel in homing
				}
			}
		}

		control.context().stepper->run();
	}

	void react(const StopMoving &event, FullControl &control)
	{
		console.printf("    HomeFindTrailingEdge - react to Stop\n");
		control.changeWith<Idle>(Payload{0, false});
	}

	template <typename TEvent>
	void react(const TEvent &, FullControl &)
	{
		// No-op for unhandled events
	}
};

//------------------------------------------------------------------------------

struct SetHomeLocation
	: FSM::State
{
	void enter(Control &control)
	{
		console.println("    SetHomeLocation::enter");
	}

	// We first start looking forward. Always forward.  Either for the front edge if we're on the magnet.
	// Or the back edge if we're not on the magnet.

	void update(FullControl &control)
	{
		if (control.context().stepper->moving() == false)
		{
			console.printf("  SetHomeLocation::done - position: %d magnet: %c\n", control.context().stepper->currentPosition(), control.context().stepper->homeMagnet() ? '1' : '0');
			control.context().stepper->homePosition(0);
			control.changeWith<Idle>(Payload{0, false});
		}
		control.context().stepper->run();
	}

	void react(const StopMoving &event, FullControl &control)
	{
		console.printf("    SetHomeLocation - react to Stop\n");
		// control.changeWith<Idle>(Payload{0, false});
	}

	template <typename TEvent>
	void react(const TEvent &, FullControl &)
	{
		// No-op for unhandled events
	}
};

//------------------------------------------------------------------------------

// another top-level state
struct Error
	: FSM::State
{
	void enter(Control &control)
	{
		console.printf("   Error::enter");
	}

	template <typename TEvent>
	void react(const TEvent &, FullControl &)
	{
		console.printf("   THIS AXIS IS IN ERROR. EVENTS ARE IGNORED. FIx AND REBOOT.");
	}
};
