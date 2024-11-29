#ifdef GEODE_IS_MACOS

#define CommentType CommentTypeDummy
#import <Foundation/Foundation.h>
#import <Cocoa/Cocoa.h>
#import <CoreGraphics/CoreGraphics.h>
#undef CommentType

#include <Geode/cocos/platform/mac/CCEventDispatcher.h>
#import <Geode/cocos/platform/mac/EAGLView.h>
#import <objc/runtime.h>

#include "types/TouchMessageType.hpp"

#define HOOK_OBJC_METHOD(klass, cleanFuncName, funcName) \
	auto cleanFuncName ## Method = class_getInstanceMethod(objc_getClass(#klass), @selector(funcName)); \
	cleanFuncName ## OIMP = method_getImplementation(cleanFuncName ## Method); \
	method_setImplementation(cleanFuncName ## Method, (EventType<klass>)&funcName);

#define CALL_OIMP(funcName) reinterpret_cast<decltype(&funcName)>(funcName ## OIMP)(self, sel, event)

template <typename T>
using EventType = void(*)(T*, SEL, NSEvent*);


static EventType<EAGLView> keyDownExecOIMP;
void keyDownExec(EAGLView* self, SEL sel, NSEvent* event) {
	if (!g_selectedInput)
		CALL_OIMP(keyDownExec);

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
		int code = [[event characters] length] > 0
			? [[event characters] characterAtIndex:0];
			: [[event charactersIgnoringModifiers] characterAtIndex:0];

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
						BI::platform::keyDown(BI::PlatformKey::LEFT_SHIFT)
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
			CALL_OIMP(keyDownExec);
		}
	}
}

static EventType<EAGLView> keyUpExecOIMP;
void keyUpExec(EAGLView* self, SEL sel, NSEvent* event) {
	if (!g_selectedInput)
		CALL_OIMP(keyUpExec);
}


// TODO: move to hooking mouseDownExec
// handles mouse clicks
struct BetterTouchDispatcher : Modify<BetterTouchDispatcher, CCTouchDispatcher>
{
	// https://github.com/ninXout/Crystal-Client/blob/7df5a8336ccb852bc984e55dd29ca27bb1741443/src/ImGui/ImGui.cpp#L96
	void touches(cocos2d::CCSet* touches, cocos2d::CCEvent* event, unsigned int type)
	{
		if (!g_selectedInput)
			return CCTouchDispatcher::touches(touches, event, type);

		auto* touch = static_cast<CCTouch*>(touches->anyObject());
		const auto touchPos = touch->getLocation();

		if (type == TouchMessageType::Began)
		{
			CCSize winSize = CCDirector::sharedDirector()->getWinSize();

			// the touch event's origin is bottom left
			CCTouch touch{};
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
	HOOK_OBJC_METHOD(EAGLView, keyDownExec, keyDownExec:);
	HOOK_OBJC_METHOD(EAGLView, keyUpExec, keyUpExec:);
}

#endif