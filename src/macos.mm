#include <string>

#include <Geode/loader/Log.hpp>
#include <Geode/utils/ObjcHook.hpp>

#define CommentType CommentTypeDummy
#import <Foundation/Foundation.h>
#import <Cocoa/Cocoa.h>
#undef CommentType

#include <Carbon/Carbon.h>
#import <objc/runtime.h>

#include <Geode/modify/CCTouchDispatcher.hpp>
#include <Geode/cocos/platform/mac/CCEventDispatcher.h>
#import <Geode/cocos/platform/mac/EAGLView.h>

#include "BetterTextInputNode.hpp"

#include "types/TouchMessageType.hpp"
#include "utils.hpp"

namespace BI::platform
{
	inline bool keyDown(PlatformKey key, NSEvent* event)
	{
		switch (key)
		{
			case BI::PlatformKey::LEFT_CONTROL:
				return [event modifierFlags] & NSCommandKeyMask;
			case BI::PlatformKey::LEFT_SHIFT:
				return [event modifierFlags] & NSShiftKeyMask;
		}

		return false;
	}
}

#define HOOK_OBJC_METHOD(klass, type, cleanFuncName, funcName) \
	auto cleanFuncName ## Method = class_getInstanceMethod(klass, @selector(funcName)); \
	cleanFuncName ## OIMP = reinterpret_cast<type>(method_getImplementation(cleanFuncName ## Method)); \
	method_setImplementation(cleanFuncName ## Method, reinterpret_cast<IMP>(&cleanFuncName));

using KeyEventType = void(*)(EAGLView*, SEL, NSEvent*);


static KeyEventType keyDownExecOIMP;
void keyDownExec(EAGLView* self, SEL sel, NSEvent* event) {
	if (!g_selectedInput)
		return keyDownExecOIMP(self, sel, event);

	geode::log::debug("key down ({})", [event keyCode]);
	geode::log::debug(
		"isControl: {} isShift: {} isRepeat: {}",
		BI::platform::keyDown(BI::PlatformKey::LEFT_CONTROL, event),
		BI::platform::keyDown(BI::PlatformKey::LEFT_SHIFT, event),
		[event isARepeat]
	);

	// on click, can be held
	if (
		!BI::platform::keyDown(BI::PlatformKey::LEFT_CONTROL, event) &&
		!BI::platform::keyDown(BI::PlatformKey::LEFT_SHIFT, event)
	) {
		switch ([event keyCode])
		{
			case kVK_Escape:
				return g_selectedInput->deselectInput();

			case kVK_Delete:
			case kVK_ForwardDelete:
				return g_selectedInput->onDelete(false, [event keyCode] == kVK_ForwardDelete);

			default:
				break;
		}
	}

	switch ([event keyCode])
	{
		case kVK_RightArrow:
			return g_selectedInput->onRightArrowKey(
				BI::platform::keyDown(BI::PlatformKey::LEFT_CONTROL, event),
				BI::platform::keyDown(BI::PlatformKey::LEFT_SHIFT, event)
			);

		case kVK_LeftArrow:
			return g_selectedInput->onLeftArrowKey(
				BI::platform::keyDown(BI::PlatformKey::LEFT_CONTROL, event),
				BI::platform::keyDown(BI::PlatformKey::LEFT_SHIFT, event)
			);

		default:
			break;
	}

	if (
		![event isARepeat] &&
		!BI::platform::keyDown(BI::PlatformKey::LEFT_CONTROL, event) &&
		BI::platform::keyDown(BI::PlatformKey::LEFT_SHIFT, event)
	) {
		switch ([event keyCode])
		{
			case kVK_Delete:
			case kVK_ForwardDelete:
				return g_selectedInput->onDelete(true, [event keyCode] == kVK_ForwardDelete);

			case 'a': case 'A':
				return g_selectedInput->highlightFromToPos(0, -1);

			case 'c': case 'C':
				return g_selectedInput->onCopy();

			case 'v': case 'V':
				return g_selectedInput->onPaste();

			case 'x': case 'X':
				return g_selectedInput->onCut();

			default:
				break;
		}
	}

	if (![event isARepeat])
	{
		if (!BI::platform::keyDown(BI::PlatformKey::LEFT_CONTROL, event))
		{
			switch ([event keyCode])
			{
				case kVK_Home:
					return g_selectedInput->onHomeKey(
						BI::platform::keyDown(BI::PlatformKey::LEFT_SHIFT, event)
					);

				case kVK_End:
					return g_selectedInput->onEndKey(
						BI::platform::keyDown(BI::PlatformKey::LEFT_SHIFT, event)
					);

				default:
					break;
			}
		}
	}

	// key is probably a regular character, allow CCIMEDispatcher to pick up the event
	keyDownExecOIMP(self, sel, event);
}

static KeyEventType keyUpExecOIMP;
void keyUpExec(EAGLView* self, SEL sel, NSEvent* event) {
	if (!g_selectedInput)
		return keyUpExecOIMP(self, sel, event);

	geode::log::debug("key released ({})", [event keyCode]);
}


// TODO: move to hooking mouseDownExec
// handles mouse clicks
struct BetterTouchDispatcher : geode::Modify<BetterTouchDispatcher, cocos2d::CCTouchDispatcher>
{
	// https://github.com/ninXout/Crystal-Client/blob/7df5a8336ccb852bc984e55dd29ca27bb1741443/src/ImGui/ImGui.cpp#L96
	void touches(cocos2d::CCSet* touches, cocos2d::CCEvent* event, unsigned int type)
	{
		if (!g_selectedInput)
			return cocos2d::CCTouchDispatcher::touches(touches, event, type);

		geode::log::debug("button clicked");

		auto* touch = static_cast<cocos2d::CCTouch*>(touches->anyObject());
		const auto touchPos = touch->getLocation();

		if (type == TouchMessageType::Began)
		{
			cocos2d::CCSize winSize = cocos2d::CCDirector::sharedDirector()->getWinSize();

			// the touch event's origin is bottom left
			cocos2d::CCTouch touch{};
			touch.setTouchInfo(0, touchPos.x, winSize.height - touchPos.y);

			g_selectedInput->useUpdateBlinkPos(true);

			// 🥰
			g_selectedInput->ccTouchBegan(&touch, nullptr);
		}
		else
			g_selectedInput->useUpdateBlinkPos(false);
	}
};


$on_mod(Loaded)
{
	auto eaglView = objc_getClass("EAGLView");

	HOOK_OBJC_METHOD(eaglView, KeyEventType, keyDownExec, keyDownExec:);
	HOOK_OBJC_METHOD(eaglView, KeyEventType, keyUpExec, keyUpExec:);
}
