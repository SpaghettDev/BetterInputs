#include <Geode/modify/CCEGLView.hpp>
#include <Geode/cocos/robtop/glfw/glfw3.h>

#include "BetterTextInputNode.hpp"

#include "utils.hpp"

using namespace geode::prelude;

// handles ctrl and shift
// also fixes mouse clicks
struct BetterCCEGLView : Modify<BetterCCEGLView, CCEGLView>
{
	void onGLFWKeyCallback(
		GLFWwindow* window,
		int key,
		int scancode,
		int action,
		int mods
	) {
		if (!g_selectedInput)
			return CCEGLView::onGLFWKeyCallback(window, key, scancode, action, mods);

		// on click, can be held
		if (action != GLFW_RELEASE)
		{
			if (
				!BI::platform::keyDown(BI::PlatformKey::LEFT_CONTROL) &&
				!BI::platform::keyDown(BI::PlatformKey::LEFT_SHIFT)
			) {
				switch (key)
				{
					case GLFW_KEY_ESCAPE:
						if (!BI::geode::get<bool>("alternate-deselect"))
							return g_selectedInput->deselectInput();
						break;

					case GLFW_KEY_BACKSPACE:
					case GLFW_KEY_DELETE:
						return g_selectedInput->onDelete(false, key == GLFW_KEY_DELETE);

					default:
						break;
				}
			}

			switch (key)
			{
				case GLFW_KEY_UP:
					return g_selectedInput->onUpArrowKey(
						BI::platform::keyDown(BI::PlatformKey::LEFT_SHIFT)
					);

				case GLFW_KEY_DOWN:
					return g_selectedInput->onDownArrowKey(
						BI::platform::keyDown(BI::PlatformKey::LEFT_SHIFT)
					);

				case GLFW_KEY_RIGHT:
					return g_selectedInput->onRightArrowKey(
						BI::platform::keyDown(BI::PlatformKey::LEFT_CONTROL),
						BI::platform::keyDown(BI::PlatformKey::LEFT_SHIFT)
					);

				case GLFW_KEY_LEFT:
					return g_selectedInput->onLeftArrowKey(
						BI::platform::keyDown(BI::PlatformKey::LEFT_CONTROL),
						BI::platform::keyDown(BI::PlatformKey::LEFT_SHIFT)
					);

				default:
					break;
			}
		}

		// this is what onGLFWKeyCallback actually does to check for control lol
		if (
			action == GLFW_PRESS &&
			BI::platform::keyDown(BI::PlatformKey::LEFT_CONTROL) &&
			!BI::platform::keyDown(BI::PlatformKey::LEFT_SHIFT)
		) {
			switch (key)
			{
				case GLFW_KEY_A:
					return g_selectedInput->highlightFromToPos(0, -1);

				case GLFW_KEY_INSERT:
				case GLFW_KEY_C:
					return g_selectedInput->onCopy();

				case GLFW_KEY_V:
					return g_selectedInput->onPaste();

				case GLFW_KEY_X:
					return g_selectedInput->onCut();

				case GLFW_KEY_BACKSPACE:
				case GLFW_KEY_DELETE:
					return g_selectedInput->onDelete(true, key == GLFW_KEY_DELETE);

				default:
					break;
			}
		}

		if (action == GLFW_PRESS)
		{
			if (!BI::platform::keyDown(BI::PlatformKey::LEFT_CONTROL))
			{
				switch (key)
				{
					case GLFW_KEY_HOME:
						return g_selectedInput->onHomeKey(
							BI::platform::keyDown(BI::PlatformKey::LEFT_SHIFT)
						);

					case GLFW_KEY_END:
						return g_selectedInput->onEndKey(
							BI::platform::keyDown(BI::PlatformKey::LEFT_SHIFT)
						);

					default:
						break;
				}
			}

			if (
				BI::platform::keyDown(BI::PlatformKey::LEFT_SHIFT) &&
				!BI::platform::keyDown(BI::PlatformKey::LEFT_CONTROL)
			) {
				switch (key)
				{
					case GLFW_KEY_INSERT:
						return g_selectedInput->onPaste();

					default:
						break;
				}
			}

			CCEGLView::onGLFWKeyCallback(window, key, scancode, action, mods);
		}
	}

	// for some odd reason, the cursor's position isnt updated until the 2nd click
	// or not at all in TextAreas
	// this fixes it :D
	void onGLFWMouseCallBack(GLFWwindow* window, int button, int action, int mods)
	{
		CCEGLView::onGLFWMouseCallBack(window, button, action, mods);

		if (!g_selectedInput || button != GLFW_MOUSE_BUTTON_1 || action == GLFW_REPEAT) return;

		if (action == GLFW_PRESS)
		{
			CCSize winSize = CCDirector::sharedDirector()->getWinSize();
			CCPoint mousePos = BI::cocos::getMousePosition();

			// OpenGL's mouse origin is the bottom left
			// CCTouch's mouse origin is top left (because of course it is)
			CCTouch touch{};
			touch.setTouchInfo(0, mousePos.x, winSize.height - mousePos.y);

			g_selectedInput->useUpdateBlinkPos(true);

			// 🥰
			g_selectedInput->ccTouchBegan(&touch, nullptr);
		}
		else
			g_selectedInput->useUpdateBlinkPos(false);
	}
};
