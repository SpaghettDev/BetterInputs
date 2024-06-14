#include <Geode/cocos/label_nodes/CCLabelBMFont.h>

struct InputNodeTextAreaInfo
{
    cocos2d::CCLabelBMFont* label;
    std::size_t line;
    std::size_t numCharsFromStart;
    std::size_t numCharsFromLabelStart;
};
