#include "Story.hpp"

//Lines are kept short on purpose: the renderer does not wrap, and at 60px on a
//1280-wide window a body line runs out of room somewhere past ~40 characters.

std::vector< Node > make_story()
{
    enum StoryStage : uint32_t {
        Dark,
        Phone,
        Feel,
        Waited,
        Car,
        Cars,
        Log,
        Tunnel,
        Door,
        KnownDoor,
        Forced,
        Shaft,
        Count
    };

    std::vector< Node > story;
    story.resize(Count);

    story[Dark] = {
        "Darkness",
        "You wake up between the rails.",
        {
            {"Light your phone", Phone},
            {"Feel for the wall", Feel},
            {"Stay still and wait", Waited}
        }
    };

    story[Phone] = {
        "One Bar of Light",
        "The car ahead is torn open.",
        {
            {"Climb toward it", Car}
        }
    };

    story[Feel] = {
        "Cold Steel",
        "Your hand finds a torn edge.",
        {
            {"Pull yourself up", Car}
        }
    };

    story[Waited] = {
        "The Second Train",
        "It does not slow down.",
        {
            {"Start over", Dark}
        }
    };

    story[Car] = {
        "The Split Car",
        "The driver's seat is empty.",
        {
            {"Head down the tunnel", Tunnel},
            {"Walk back through the cars", Cars}
        }
    };

    story[Cars] = {
        "Car Three",
        "A logbook lies in the aisle.",
        {
            {"Read it", Log},
            {"Leave it and go forward", Tunnel}
        }
    };

    story[Log] = {
        "The Log",
        "Signal read CLEAR. Someone forced it.",
        {
            {"Pocket the door code: 4471", KnownDoor}
        }
    };

    story[Tunnel] = {
        "The Tunnel",
        "Emergency lamps, every ten steps.",
        {
            {"Find the service door", Door}
        }
    };

    story[Door] = {
        "Service Door",
        "A keypad. You have no code.",
        {
            {"Go back for answers", Cars},
            {"Force it open", Forced}
        }
    };

    story[KnownDoor] = {
        "Service Door",
        "4471. The lock gives.",
        {
            {"Climb the ladder", Shaft}
        }
    };

    story[Forced] = {
        "The Door Holds",
        "So does the tunnel. Not for long.",
        {
            {"Start over", Dark}
        }
    };

    story[Shaft] = {
        "Daylight",
        "You surface knowing who did it.",
        {
            {"Start over", Dark}
        }
    };

    return story;
}
