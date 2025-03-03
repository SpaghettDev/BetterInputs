#pragma once

#include <Geode/cocos/cocoa/CCGeometry.h>
#include <Geode/cocos/robtop/sprite_nodes/CCFontSprite.h>
#include <Geode/cocos/label_nodes/CCLabelBMFont.h>

struct CharNodeInfo
{
	cocos2d::CCFontSprite* sprite;
	cocos2d::CCLabelBMFont* label;

	cocos2d::CCPoint position;
	cocos2d::CCPoint worldPos;

	operator cocos2d::CCPoint() { return position; }
};
