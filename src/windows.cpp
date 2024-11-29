#ifdef GEODE_IS_WINDOWS

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
						g_selectedInput->deselectInput();
						break;

					case GLFW_KEY_BACKSPACE:
					case GLFW_KEY_DELETE:
						g_selectedInput->onDelete(false, key == GLFW_KEY_DELETE);
						break;

					default:
						break;
				}
			}

			switch (key)
			{
				case GLFW_KEY_RIGHT:
					g_selectedInput->onRightArrowKey(
						BI::platform::keyDown(BI::PlatformKey::LEFT_CONTROL),
						BI::platform::keyDown(BI::PlatformKey::LEFT_SHIFT)
					);
					break;

				case GLFW_KEY_LEFT:
					g_selectedInput->onLeftArrowKey(
						BI::platform::keyDown(BI::PlatformKey::LEFT_CONTROL),
						BI::platform::keyDown(BI::PlatformKey::LEFT_SHIFT)
					);
					break;

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
					g_selectedInput->highlightFromToPos(0, -1);
					break;

				case GLFW_KEY_INSERT:
				case GLFW_KEY_C:
					g_selectedInput->onCopy();
					break;

				case GLFW_KEY_V:
					g_selectedInput->onPaste();
					break;

				case GLFW_KEY_X:
					g_selectedInput->onCut();
					break;

				case GLFW_KEY_BACKSPACE:
				case GLFW_KEY_DELETE:
					g_selectedInput->onDelete(true, key == GLFW_KEY_DELETE);
					break;

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
						g_selectedInput->onHomeKey(
							BI::platform::keyDown(BI::PlatformKey::LEFT_SHIFT)
						);
						break;

					case GLFW_KEY_END:
						g_selectedInput->onEndKey(
							BI::platform::keyDown(BI::PlatformKey::LEFT_SHIFT)
						);
						break;

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
						g_selectedInput->onPaste();
						break;

					default:
						break;
				}
			}

			if (
				!BI::platform::keyDown(BI::PlatformKey::LEFT_SHIFT) &&
				!BI::platform::keyDown(BI::PlatformKey::LEFT_CONTROL) &&
				key == GLFW_KEY_ENTER
			) {
				CCEGLView::onGLFWKeyCallback(window, key, scancode, action, mods);
			}
		}
	}

	// for some odd reason, the cursor's position isnt updated until the 2nd click
	// or not at all in TextAreas
	// this fixes it :D
	void onGLFWMouseCallBack(GLFWwindow* window, int button, int action, int mods)
	{
		CCEGLView::onGLFWMouseCallBack(window, button, action, mods);

		if (!g_selectedInput || button != GLFW_MOUSE_BUTTON_1 || action == 2) return;

		if (action == 1)
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

#endif
