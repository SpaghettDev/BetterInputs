#include <Geode/cocos/cocoa/CCGeometry.h>

struct CharNode
{
    cocos2d::CCPoint position;
    cocos2d::CCPoint centerPosition;
    float widthFromCenter;

    operator cocos2d::CCPoint() { return position; }
};
