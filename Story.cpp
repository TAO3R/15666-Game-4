#include "Story.hpp"

std::vector< Node > make_story()
{
    enum StoryStage : uint32_t {
        Start,
        Finish,
        Count
    };

    std::vector< Node > story;
    story.resize(Count);

    story[Start] = {
        "This is Start Title",
        "This is a Start Body",
        {
            {"This is Choice A", Finish},
            {"This is choice B", Finish}
        }
    };

    story[Finish] = {
        "This is Finish Title",
        "This is Finish Body",
        {}
    };

    return story;
}