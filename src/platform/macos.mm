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

namespace BI
{
	namespace platform
	{
		inline bool keyDown(PlatformKey key, NSEvent* event)
		{
			switch (key)
			{
				case BI::PlatformKey::LEFT_CONTROL:
					return [event modifierFlags] & NSEventModifierFlagCommand;
				case BI::PlatformKey::LEFT_SHIFT:
					return [event modifierFlags] & NSEventModifierFlagShift;
				case BI::PlatformKey::LEFT_ALT:
					return [event modifierFlags] & NSEventModifierFlagOption;
			}

			return false;
		}
	}

	namespace cocos
	{
		inline cocos2d::CCPoint getMousePosition(NSEvent* event)
		{
			auto window = [event window];
			auto windowFrame = [window frame];
			auto viewFrame = [[window contentView] frame];
			auto scaleFactor = cocos2d::CCPoint{
				cocos2d::CCDirector::get()->getWinSize()
			} / cocos2d::CCPoint{
				static_cast<float>(viewFrame.size.width),
				static_cast<float>(viewFrame.size.height)
			};
			auto mousePos = [NSEvent mouseLocation];

			return cocos2d::CCPoint{
				static_cast<float>(mousePos.x - windowFrame.origin.x),
				static_cast<float>(mousePos.y - windowFrame.origin.y)
			} * scaleFactor;
		}
	}
}

#define OBJC_SWIZZLE(klass, type, cleanFuncName, funcName) \
	do { \
		auto cleanFuncName ## Method = class_getInstanceMethod(objc_getClass(#klass), @selector(funcName)); \
		cleanFuncName ## OIMP = reinterpret_cast<type>(method_getImplementation(cleanFuncName ## Method)); \
		method_setImplementation(cleanFuncName ## Method, reinterpret_cast<IMP>(&cleanFuncName)); \
		geode::log::debug("Swizzled Objective C Method '" #klass " " #funcName "'"); \
	} while(0)

using key_event_t = void(*)(EAGLView*, SEL, NSEvent*);

static key_event_t keyDownExecOIMP;
void keyDownExec(EAGLView* self, SEL sel, NSEvent* event)
{
	if (!g_selectedInput)
		return keyDownExecOIMP(self, sel, event);

	// on click, can be held
	if (
		!BI::platform::keyDown(BI::PlatformKey::LEFT_CONTROL, event) &&
		!BI::platform::keyDown(BI::PlatformKey::LEFT_SHIFT, event)
	) {
		switch ([event keyCode])
		{
			case kVK_Escape:
				if (!BI::geode::get<bool>("alternate-deselect"))
					return g_selectedInput->deselectInput();
				break;

			case kVK_Delete:
			case kVK_ForwardDelete:
				return g_selectedInput->onDelete(false, [event keyCode] == kVK_ForwardDelete);

			default:
				break;
		}
	}

	switch ([event keyCode])
	{
		case kVK_UpArrow:
			return g_selectedInput->onUpArrowKey(
				BI::platform::keyDown(BI::PlatformKey::LEFT_SHIFT, event)
			);

		case kVK_DownArrow:
			return g_selectedInput->onUpArrowKey(
				BI::platform::keyDown(BI::PlatformKey::LEFT_SHIFT, event)
			);

		case kVK_RightArrow:
			return g_selectedInput->onRightArrowKey(
				BI::platform::keyDown(BI::PlatformKey::LEFT_ALT, event),
				BI::platform::keyDown(BI::PlatformKey::LEFT_SHIFT, event)
			);

		case kVK_LeftArrow:
			return g_selectedInput->onLeftArrowKey(
				BI::platform::keyDown(BI::PlatformKey::LEFT_ALT, event),
				BI::platform::keyDown(BI::PlatformKey::LEFT_SHIFT, event)
			);

		default:
			break;
	}

	if (
		![event isARepeat] &&
		BI::platform::keyDown(BI::PlatformKey::LEFT_CONTROL, event) &&
		!BI::platform::keyDown(BI::PlatformKey::LEFT_SHIFT, event)
	) {
		// https://github.com/WebKit/WebKit/blob/5c8281f146cfbf4b6189b435b80c527f138b829f/Source/WebCore/platform/mac/PlatformEventFactoryMac.mm#L559
		// we use this instead of [event keyCode] because the returned value of keyCode for letters is keyboard locale-specific
		int code = [[event characters] length] > 0
			? [[event characters] characterAtIndex:0]
			: [[event charactersIgnoringModifiers] length] > 0
				? [[event charactersIgnoringModifiers] characterAtIndex:0]
				: 0;

		switch ([event keyCode])
		{
			case kVK_Delete:
			case kVK_ForwardDelete:
				g_selectedInput->onDelete(true, [event keyCode] == kVK_ForwardDelete);
				break;

			default:
				break;
		}

		switch (code)
		{
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

		if (BI::platform::keyDown(BI::PlatformKey::LEFT_CONTROL, event))
		{
			switch ([event keyCode])
			{
				case kVK_LeftArrow:
					return g_selectedInput->onHomeKey(false);

				case kVK_RightArrow:
					return g_selectedInput->onEndKey(false);

				default:
					break;
			}
		}
	}

	// key is probably a regular character, allow CCIMEDispatcher to pick up the event
	keyDownExecOIMP(self, sel, event);
}

static key_event_t keyUpExecOIMP;
void keyUpExec(EAGLView* self, SEL sel, NSEvent* event)
{
	if (g_selectedInput)
		return;

	keyUpExecOIMP(self, sel, event);
}


static key_event_t mouseDownExecOIMP;
void mouseDownExec(EAGLView* self, SEL sel, NSEvent* event)
{
	if (!g_selectedInput)
		return mouseDownExecOIMP(self, sel, event);

	cocos2d::CCSize winSize = cocos2d::CCDirector::sharedDirector()->getWinSize();
	cocos2d::CCPoint mousePos = BI::cocos::getMousePosition(event);

	cocos2d::CCTouch touch{};
	touch.setTouchInfo(0, mousePos.x, winSize.height - mousePos.y);

	g_selectedInput->useUpdateBlinkPos(true);

	// 🥰
	g_selectedInput->ccTouchBegan(&touch, nullptr);
}

static key_event_t mouseUpExecOIMP;
void mouseUpExec(EAGLView* self, SEL sel, NSEvent* event)
{
	if (!g_selectedInput)
		return mouseUpExecOIMP(self, sel, event);

	g_selectedInput->useUpdateBlinkPos(false);
}


// https://github.com/qimiko/click-on-steps/blob/d8a87e93b5407e5f2113a9715363a5255724c901/src/macos.mm#L101
$on_mod(Loaded)
{
	OBJC_SWIZZLE(EAGLView, key_event_t, keyDownExec, keyDownExec:);
	OBJC_SWIZZLE(EAGLView, key_event_t, keyUpExec, keyUpExec:);

	OBJC_SWIZZLE(EAGLView, key_event_t, mouseDownExec, mouseDownExec:);
	OBJC_SWIZZLE(EAGLView, key_event_t, mouseUpExec, mouseUpExec:);
}
