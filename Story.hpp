#pragma once

#include <cstdint>
#include <string>
#include <vector>

struct Node {
    std::string title;
    std::string body;
    struct Choice {
        std::string text;
        uint32_t next;  // index into the story's node array
    };
    std::vector< Choice > choices;
};

std::vector< Node > make_story();