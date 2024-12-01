// #ifdef GEODE_IS_MACOS
#if 1

#define CommentType CommentTypeDummy
#import <Foundation/Foundation.h>
#import <Cocoa/Cocoa.h>
#import <CoreGraphics/CoreGraphics.h>
#undef CommentType

#include <Geode/loader/Log.hpp>

#include <Geode/cocos/platform/mac/CCEventDispatcher.h>
#import <Geode/cocos/platform/mac/EAGLView.h>
#import <objc/runtime.h>

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

void empty() {}

template <typename Func>
void createObjcHook(std::string_view className, std::string_view funcName, Func function)
{
	if (auto res = geode::ObjcHook::create(className, funcName, function, &empty); res.isOk())
		static_cast<void>(geode::Mod::get()->claimHook(res.unwrap()));
}


void keyDownExec(EAGLView* self, SEL sel, NSEvent* event) {
	geode::log::debug("received event");

	if (!g_selectedInput)
		[self performSelector:sel withObject:event];

	geode::log::debug("received event 2 with keyCode {}", [event keyCode]);
	geode::log::debug(
		"isControl: {} isShift: {}",
		BI::platform::keyDown(BI::PlatformKey::LEFT_CONTROL, event),
		BI::platform::keyDown(BI::PlatformKey::LEFT_SHIFT, event)
	);
	geode::log::debug("isRepeat: {}", [event isARepeat]);

	// on click, can be held
	if (
		!BI::platform::keyDown(BI::PlatformKey::LEFT_CONTROL, event) &&
		!BI::platform::keyDown(BI::PlatformKey::LEFT_SHIFT, event)
	) {
		switch ([event keyCode])
		{
			case kVK_Escape:
				g_selectedInput->deselectInput();
				break;

			case kVK_Delete:
			case kVK_ForwardDelete:
				g_selectedInput->onDelete(false, [event keyCode] == kVK_ForwardDelete);
				break;

			default:
				break;
		}
	}

	switch ([event keyCode])
	{
		case kVK_RightArrow:
			g_selectedInput->onRightArrowKey(
				BI::platform::keyDown(BI::PlatformKey::LEFT_CONTROL, event),
				BI::platform::keyDown(BI::PlatformKey::LEFT_SHIFT, event)
			);
			break;

		case kVK_LeftArrow:
			g_selectedInput->onLeftArrowKey(
				BI::platform::keyDown(BI::PlatformKey::LEFT_CONTROL, event),
				BI::platform::keyDown(BI::PlatformKey::LEFT_SHIFT, event)
			);
			break;

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
				g_selectedInput->onDelete(true, [event keyCode] == kVK_ForwardDelete);
				break;

			default:
				break;
		}

		/*
		int code = [[event characters] length] > 0
			? [[event characters] characterAtIndex:0]
			: [[event charactersIgnoringModifiers] length] > 0
				? [[event charactersIgnoringModifiers] characterAtIndex:0]
				: 0;
		*/

		int code = 0;
		{
			NSString* s = [event characters];
			code = [s length] > 0 ? [s characterAtIndex:0] : 0;

			if (code == 0)
			{
				s = [event charactersIgnoringModifiers];
				code = [s length] > 0 ? [s characterAtIndex:0] : 0;
			}

			geode::log::debug("key found: '{}' ({})", static_cast<char>(code), code);
		}

		switch (code)
		{
			case 'a': case 'A':
				g_selectedInput->highlightFromToPos(0, -1);
				break;

			case 'c': case 'C':
				g_selectedInput->onCopy();
				break;

			case 'v': case 'V':
				g_selectedInput->onPaste();
				break;

			case 'x': case 'X':
				g_selectedInput->onCut();
				break;

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
					g_selectedInput->onHomeKey(
						BI::platform::keyDown(BI::PlatformKey::LEFT_SHIFT, event)
					);
					break;

				case kVK_End:
					g_selectedInput->onEndKey(
						BI::platform::keyDown(BI::PlatformKey::LEFT_SHIFT, event)
					);
					break;

				default:
					break;
			}
		}

		if (
			!BI::platform::keyDown(BI::PlatformKey::LEFT_SHIFT, event) &&
			!BI::platform::keyDown(BI::PlatformKey::LEFT_CONTROL, event) &&
			[event keyCode] == kVK_Return
		) {
			[self performSelector:sel withObject:event];
		}
	}
}

void keyUpExec(EAGLView* self, SEL sel, NSEvent* event) {
	if (!g_selectedInput)
		[self performSelector:sel withObject:event];
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

		auto* touch = static_cast<cocos2d::CCTouch*>(touches->anyObject());
		const auto touchPos = touch->getLocation();

		if (type == TouchMessageType::Began)
		{
			CCSize winSize = cocos2d::CCDirector::sharedDirector()->getWinSize();

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
	createObjcHook("EAGLView", "keyDownExec:", keyDownExec);
	createObjcHook("EAGLView", "keyUpExec:", keyDownExec);
}

#endif