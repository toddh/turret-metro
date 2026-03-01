
// optional: enable FSM structure report in debugger
#undef FFSM2_ENABLE_STRUCTURE_REPORT

#include <ffsm2/machine.hpp>
#include <iostream>

#include <hwstep.hpp>

#include <globals.h>

/*

TODO: Determine if this value needs to change from one axis to the next. It was 1700. But for the tilt axis,
I changed it to the value above. I think that the the tilt axis could be around +350 before its past the magnet. And the most
I'd want to forward is another 500 for a max of 850.  The tilt axis wouldn't know that it's at 350,
so perhaps the max positive now reverse should be 850. And the e-stop limit should be 900.  Maybe.AC_COMPCTRL_HYST_HYST100_Val

TODO: Clean-up how limits are set.

Tilt really shouldn't be down that much.

*/


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
struct Reset
{
};


// data shared between FSM states and outside code
struct Context
{
	HWStep *stepper;
	int edgeLocationA = std::numeric_limits<int>::min();
	int edgeLocationB = std::numeric_limits<int>::min();
	bool backedUp = false; // Used to determine if we need to back up after finding the leading edge.
	int maxHomeSearchBeforeReverse = 500;
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
		// console.println("  Idle::enter");
	}

	// state can initiate transitions to _any_ other state
	void update(FullControl &control)
	{
		// console.println("  Idle::Update");
	}

	void react(const MoveRel &event, FullControl &control)
	{
		console.printf("    Idle - react to MoveRel [%s]: %d\n", control.context().stepper->name(), event.delta);
		control.context().stepper->moveRel(event.delta, event.ignoreLimits);
		if (control.context().stepper->isError())
			control.changeWith<Error>(Payload{0, false});
		else
			control.changeWith<Moving>(Payload{0, false});
	}
	void react(const MoveAbs &event, FullControl &control)
	{
		console.printf("    Idle - react to MoveAbs [%s]: %d\n", control.context().stepper->name(), event.position);
		control.context().stepper->moveAbsTo(event.position, event.ignoreLimits);
		if (control.context().stepper->isError())
			control.changeWith<Error>(Payload{0, false});
		else
			control.changeWith<Moving>(Payload{0, false});
	}
	void react(const StartHoming &event, FullControl &control)
	{
		console.println("    Idle - react to StartHoming");
		control.context().stepper->reInit();
		control.changeWith<HomeInit>(Payload{0, false});
	}
	void react(const StopMoving &event, FullControl &control)
	{
		console.println("    Idle - react to StopMoving");
	}	

	template <typename TEvent>
	void react(const TEvent &, FullControl &)
	{
		// No-op for unhandled events
	}	
};

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

struct Moving
	: FSM::State
{
	void enter(Control &)
	{
		// console.println("  Moving::enter");
	}

	void update(FullControl &control)
	{
		// console.println("  Moving::update");
		if (control.context().stepper->moving() == false)
		{
			console.printf("    Moving::done [%s] - position: %d magnet: %c\n", control.context().stepper->name(), control.context().stepper->currentPosition(), control.context().stepper->homeMagnet() ? '1' : '0');
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

/*
Homing states

HomeInit -
  - Enter: Resets edge locations
  - Update: ChangeWith to:
	- HomeBackupFirst if already on magnet (back up to get off the magnet)
	- HomeFindLeadingEdge if not on magnet (start looking for leading edge)

HomeBackupFirst -
  - Enter: No action
  - Update: If moving is done, changeWith to HomeFindLeadingEdge


HomeFindLeadingEdge -
  - Enter: No action
  - Update:
  	- If past max or min, changeWith to Error
	- If on magnet, set edgeLocationA and changeWith to HomeFindTrailingEdge
	- If not on magnet, check if past control.context().maxHomeSearchBeforeReverse
	  - If back the emergency limit, transition to error
	  - If past control.context().maxHomeSearchBeforeReverse, back up and changeWith to HomeBackupFirst
	  - Else, continue moving forward to find leading edge.
 	  - (If doHomeStep returns false, we are past the emergency limits, so changeWith to Error)


HomeFindTrailingEdge -
  - Enter: No action
  - Update:
  	- If isPastMin or isPastMax, changeWith to Error
	- If not on magnet, continue moving forward to find trailing edge
	- If on magnet, set edgeLocationB and changeWith to SetHomeLocation
	- (If doHomeStep returns false, we are past the emergency limits, so changeWith to Error)

SetHomeLocation -
  - Enter: No action
  - Update:
  	- setHomeLocation - set the stepper's current position to zero
	- changeWith to Idle
*/


struct HomeInit
	: FSM::State
{
	void enter(Control &control)
	{
		console.println("    HomeInit::enter");
		control.context().stepper->reInit();
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
			// TODO: Handle potential error. We've gone back too far.
			control.changeWith<HomeBackupFirst>(Payload{1, false});
		}
		else
		{
			console.println("      Not on magnet (0)");
			if (control.context().stepper->doHomeStep(1))
				control.changeWith<HomeFindLeadingEdge>(Payload{1, true});
			else
			{
				console.printf("===================================================================\n");
				console.printf("          >>>> HomeInit::update - past emergency limits\n");
				console.printf("          >>>> HomeInit::update - transitioning to error\n");
				console.printf("===================================================================\n");
				control.changeWith<Error>(Payload{0, false});
			}
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
		// TODO: Handle potential error.
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
				// if (control.context().stepper->currentPosition() > control.context().maxHomeSearchBeforeReverse)
				// {
				// 	console.printf("===================================================================\n");
				// 	console.printf("          >>>> HomeFindLeadingEdge::update - past max home search\n");
				// 	console.printf("          >>>> HomeFindLeadingEdge::update - transitioning to error\n");
				// 	console.printf("===================================================================\n");
				// 	control.changeWith<Error>(Payload{0, false});
				// }
				// else
				if (control.context().stepper->currentPosition() > control.context().maxHomeSearchBeforeReverse)
				{
					if (control.context().backedUp)
					{
						console.printf("          >>>> %d - Too Far, back way up\n", control.context().stepper->currentPosition());
						console.printf("          >>>> BUT, WE'VE ALREADY DONE THIS ONCE. SO GO TO ERROR\n");
						control.changeWith<Error>(Payload{0, false});
					}
					else
					{
						console.printf("          >>>> %d - Too Far, back way up\n", control.context().stepper->currentPosition());
						control.context().stepper->moveRel(-2 * control.context().maxHomeSearchBeforeReverse, true);
						control.context().backedUp = true;
						control.changeWith<HomeBackupFirst>(Payload{0, false});
					}

				}
				else
				{
					if (!control.context().stepper->doHomeStep(1))
					{
						console.printf("===================================================================\n");
						console.printf("          >>>> HomeFindLeadingEdge::update - past emergency limits\n");
						console.printf("          >>>> HomeFindLeadingEdge::update - transitioning to error\n");
						console.printf("===================================================================\n");
						control.changeWith<Error>(Payload{0, false});
					}
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
					if (!control.context().stepper->doHomeStep(1))
					{
						console.printf("===================================================================\n");
						console.printf("          >>>> HomeFindTrailingEdge::update - past emergency limits\n");
						console.printf("          >>>> HomeFindTrailingEdge::update - transitioning to error\n");
						console.printf("===================================================================\n");
						control.changeWith<Error>(Payload{0, false});
					}
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
		console.printf("   *** ERROR STATE ENTERED [%s] ***\n", control.context().stepper->name());
		control.context().stepper->isError(true);
	}

	void react(const Reset &event, FullControl &control)
	{
		console.printf("   *** ERROR STATE CLEARED [%s] ***\n", control.context().stepper->name());
		control.context().stepper->isError(false);
		control.changeWith<Idle>(Payload{0, false});
	}

	template <typename TEvent>
	void react(const TEvent &, FullControl &control)
	{
		console.printf("   *** [%s] IS IN ERROR - ALL COMMANDS IGNORED - SEND CMD_INIT TO RESET ***\n", control.context().stepper->name());
	}
};
