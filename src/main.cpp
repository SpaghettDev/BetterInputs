#include <string>
#include <string_view>

#include <Geode/modify/CCTextInputNode.hpp>
#include <Geode/modify/CCEGLView.hpp>
#include <Geode/modify/CCIMEDispatcher.hpp>
#include <Geode/modify/CCTextFieldTTF.hpp>

#include <Geode/cocos/robtop/glfw/glfw3.h>

#include "HighlightedString/HighlightedString.hpp"
#include "utils.hpp"

using namespace geode::prelude;

struct BetterTextInputNode;
BetterTextInputNode* g_selectedInput = nullptr;

struct BetterTextInputNode : Modify<BetterTextInputNode, CCTextInputNode>
{
	struct Fields
	{
		// the actual string, because we basically rework everything about the input
		// text insertion is broken :D (getString() normally returns the last character or is empty)
		std::string m_string = "";

		// current cursor position
		int m_pos = -1;

		HighlightedString m_highlighted;
		bool m_is_adding_to_highlight = false;
		cocos2d::extension::CCScale9Sprite* m_highlight = nullptr;

		bool m_use_update_blink_pos = false;
		bool m_get_text_field_str = false;


		// i dont think this ever changes
		const CCPoint m_text_area_initial_cursor_pos = { .0f, -9.f };
	};

	bool init(float p0, float p1, char const* p2, char const* p3, int p4, char const* p5)
	{
		if (!CCTextInputNode::init(p0, p1, p2, p3, p4, p5)) return false;

		m_fields->m_highlight = cocos2d::extension::CCScale9Sprite::create("square.png");
		m_fields->m_highlight->setOpacity(120);
		m_fields->m_highlight->setPosition(this->m_cursor->getPosition() - 1.f);
		m_fields->m_highlight->setAnchorPoint({ .0f, .5f });
		m_fields->m_highlight->setVisible(false);
		m_fields->m_highlight->setID("highlight-sprite"_spr);
		this->addChild(m_fields->m_highlight, 10);

		return true;
	}

	bool onTextFieldAttachWithIME(cocos2d::CCTextFieldTTF* tField)
	{
		g_selectedInput = this;

		// get the current string if the input node already has something (other than the placeholder obviously)
		// TODO: move to somewhere else
		m_fields->m_get_text_field_str = true;
		if (
			const auto textFieldStr = this->m_textField->getString();
			m_fields->m_string.empty() && textFieldStr != m_fields->m_string
		)
		{
			log::debug("brah {}", textFieldStr);
			m_fields->m_string = textFieldStr;
		}
		m_fields->m_get_text_field_str = false;

		return CCTextInputNode::onTextFieldAttachWithIME(tField);
	}

	bool onTextFieldDetachWithIME(cocos2d::CCTextFieldTTF* tField)
	{
		g_selectedInput = nullptr;

		return CCTextInputNode::onTextFieldDetachWithIME(tField);
	}

	void updateBlinkLabelToChar(int pos)
	{
		showTextOrPlaceholder(true);

		if (m_fields->m_use_update_blink_pos)
		{
			m_fields->m_pos = pos;
			m_fields->m_use_update_blink_pos = false;
		}

		CCTextInputNode::updateBlinkLabelToChar(m_fields->m_pos);

		if (m_fields->m_highlighted.isHighlighting() && !m_fields->m_is_adding_to_highlight)
			clearHighlight();
	}


	// rewritten stuff
	void setAndUpdateString(const std::string& str)
	{
		// the position is modified in the call to setString
		const int prevPos = m_fields->m_pos;
		m_fields->m_string = str;

		CCTextInputNode::setString(str);

		m_fields->m_pos = prevPos;
	}

	void updateBlinkLabelToCharForced(int pos)
	{
		m_fields->m_use_update_blink_pos = true;

		this->updateBlinkLabelToChar(pos);

		m_fields->m_use_update_blink_pos = false;
	}

	void clearHighlight()
	{
		m_fields->m_highlight->setVisible(false);

		m_fields->m_highlighted.reset();
	}

	void deselectInput()
	{
		this->onClickTrackNode(false);
		this->m_cursor->setVisible(false);
	}


	// Left and Right arrow events
	void onRightArrow(bool isCtrl, bool isShift)
	{
		if (m_fields->m_highlighted.isHighlighting() && !isShift)
		{
			updateBlinkLabelToCharForced(
				m_fields->m_highlighted.getToPos() == m_fields->m_string.size()
					? -1
					: m_fields->m_highlighted.getToPos()
			);
			clearHighlight();
			return;
		}

		if (m_fields->m_string.empty() || m_fields->m_pos == -1) return;

		if (isCtrl && isShift)
		{
			const int currentPos = m_fields->m_pos;

			m_fields->m_is_adding_to_highlight = true;

			updateBlinkLabelToCharForced(
				BI::utils::findNextSeparator(m_fields->m_string, m_fields->m_pos)
			);

			m_fields->m_is_adding_to_highlight = false;

			if (m_fields->m_highlighted.isHighlighting())
				highlightFromToPos(
					currentPos == m_fields->m_highlighted.getFromPos()
						? m_fields->m_pos
						: m_fields->m_highlighted.getFromPos(),
					currentPos == m_fields->m_highlighted.getFromPos()
						? m_fields->m_highlighted.getToPos()
						: m_fields->m_pos
					);
			else
				highlightFromToPos(currentPos, m_fields->m_pos);

			return;
		}

		if (isShift)
		{
			const int currentPos = m_fields->m_pos;

			m_fields->m_is_adding_to_highlight = true;

			this->updateBlinkLabelToChar(getAndSetNextPos());

			m_fields->m_is_adding_to_highlight = false;

			if (m_fields->m_highlighted.isHighlighting())
				highlightFromToPos(
					currentPos == m_fields->m_highlighted.getFromPos()
						? m_fields->m_pos
						: m_fields->m_highlighted.getFromPos(),
					currentPos == m_fields->m_highlighted.getFromPos()
						? m_fields->m_highlighted.getToPos()
						: m_fields->m_pos
				);
			else
				highlightFromToPos(currentPos, m_fields->m_pos);

			return;
		}

		if (m_fields->m_highlighted.isHighlighting())
			updateBlinkLabelToCharForced(-1);

		if (isCtrl)
			return updateBlinkLabelToCharForced(
				BI::utils::findNextSeparator(m_fields->m_string, m_fields->m_pos)
			);

		this->updateBlinkLabelToChar(getAndSetNextPos());
	}

	void onLeftArrow(bool isCtrl, bool isShift)
	{
		if (m_fields->m_highlighted.isHighlighting() && !isShift)
		{
			updateBlinkLabelToCharForced(
				m_fields->m_highlighted.getFromPos() == m_fields->m_string.size()
					? -1
					: m_fields->m_highlighted.getFromPos()
			);
			clearHighlight();
			return;
		}

		if (m_fields->m_string.empty() || m_fields->m_pos == 0) return;

		if (isCtrl && isShift)
		{
			const int currentPos = m_fields->m_pos;

			m_fields->m_is_adding_to_highlight = true;

			updateBlinkLabelToCharForced(
				BI::utils::findPreviousSeparator(m_fields->m_string, m_fields->m_pos)
			);

			m_fields->m_is_adding_to_highlight = false;

			if (m_fields->m_highlighted.isHighlighting())
				highlightFromToPos(
					currentPos == m_fields->m_highlighted.getToPos()
						? m_fields->m_pos
						: m_fields->m_highlighted.getToPos(),
					currentPos == m_fields->m_highlighted.getToPos()
						? m_fields->m_highlighted.getFromPos()
						: m_fields->m_pos
				);
			else
				highlightFromToPos(m_fields->m_pos, currentPos);

			return;
		}

		if (isShift)
		{
			const int currentPos = m_fields->m_pos;

			m_fields->m_is_adding_to_highlight = true;

			this->updateBlinkLabelToChar(getAndSetPreviousPos());

			m_fields->m_is_adding_to_highlight = false;

			if (m_fields->m_highlighted.isHighlighting())
			{
				if (currentPos == -1)
					return highlightFromToPos(m_fields->m_highlighted.getFromPos(), m_fields->m_pos);

				highlightFromToPos(
					currentPos == m_fields->m_highlighted.getToPos()
						? m_fields->m_pos
						: m_fields->m_highlighted.getToPos(),
					currentPos == m_fields->m_highlighted.getToPos()
						? m_fields->m_highlighted.getFromPos()
						: m_fields->m_pos
				);
			}
			else
				highlightFromToPos(m_fields->m_pos, currentPos);

			return;
		}

		if (m_fields->m_highlighted.isHighlighting())
			updateBlinkLabelToCharForced(0);

		if (isCtrl)
			return updateBlinkLabelToCharForced(
				BI::utils::findPreviousSeparator(m_fields->m_string, m_fields->m_pos)
			);

		this->updateBlinkLabelToChar(getAndSetPreviousPos());
	}


	// other events
	void onDelete(bool isDel)
	{
		deletePos(m_fields->m_pos, isDel);
	}

	void onCopy()
	{
		if (!m_fields->m_highlighted.isHighlighting()) return;

		clipboard::write(m_fields->m_highlighted.str.data());
	}

	void onPaste()
	{
		const std::string& clipboardText = clipboard::read();
		const int pos = m_fields->m_pos == -1 ? m_fields->m_string.size() : m_fields->m_pos;

		log::debug("from: {} len: {} text: \"{}\"", m_fields->m_highlighted.getFromPos(), m_fields->m_highlighted.getLength(), clipboardText);

		if (m_fields->m_highlighted.isHighlighting())
			insertStringAtPos(m_fields->m_highlighted.getFromPos(), m_fields->m_highlighted.getLength(), clipboardText);
		else
		{
			setAndUpdateString(BI::utils::insertStrAtIndex(m_fields->m_string, m_fields->m_pos, clipboardText));

			updateBlinkLabelToCharForced(pos + clipboardText.size() == m_fields->m_string.size() ? -1 : pos + clipboardText.size());
		}
	}

	void onCut()
	{
		if (!m_fields->m_highlighted.isHighlighting()) return;

		clipboard::write(m_fields->m_highlighted.str.data());

		insertStringAtPos(
			m_fields->m_highlighted.getFromPos(),
			m_fields->m_highlighted.getLength(),
			""
		);

		if (m_fields->m_string.empty())
			showTextOrPlaceholder(false);
	}


	// Getters and Setters
	void useUpdateBlinkPos(bool toggle)
	{
		m_fields->m_use_update_blink_pos = toggle;
	}
	void showTextOrPlaceholder(bool toggle)
	{
		if (this->m_placeholderLabel)
			this->m_placeholderLabel->setVisible(toggle);
		else
			this->m_textArea->setVisible(toggle);
	}

	int getPos() { return m_fields->m_pos; }


	// rewritten input stuff and highlighting
	/**
	 * @brief Highlights from `from` to `to` by setting the highlight's content width
	 * 
	 * @param from
	 * @param to
	 */
	void highlightFromToPos(int from, int to)
	{
		if (m_fields->m_string.empty()) return;

		if ((from == -1 ? m_fields->m_string.size() : from) > (to == -1 ? m_fields->m_string.size() : to))
			std::swap(from, to);

		if (from == to)
			return clearHighlight();

		if (this->m_placeholderLabel)
			m_fields->m_highlight->setScaleY(
				3.f * this->m_placeholderLabel->getScale() + .4f
			);
		else
			m_fields->m_highlight->setScaleY(
				2.5f * this->m_textArea->getScale()
			);

		CCPoint startPos = getCharNodeSpacePos(0, false);

		m_fields->m_highlighted.update(m_fields->m_string, { from, to });

		m_fields->m_highlight->setPositionX(getCharNodeSpacePos(from, true).x - 1.f);
		m_fields->m_highlight->setContentWidth(
			std::abs(
				m_fields->m_highlight->getPositionX() - getCharNodeSpacePos(to == -1 ? m_fields->m_string.size() - 1 : to, to != -1).x
			)
		);
		m_fields->m_highlight->setVisible(true);
	}

	/**
	 * @brief Inserts `char` at `pos`
	 * 
	 * @param pos
	 * @param character
	 */
	void insertCharAtPos(int pos, char character)
	{
		if (
			!BI::geode::get<bool>("allow-any-character") &&
			BI::mods::isVanillaInput() &&
			std::string_view(this->m_allowedChars).find(character) == std::string_view::npos
			) return;

		if (m_fields->m_highlighted.isHighlighting())
			insertStringAtPos(
				m_fields->m_highlighted.getFromPos(),
				m_fields->m_highlighted.getLength(),
				""
			);

		setAndUpdateString(
			BI::utils::insertCharAtIndex(m_fields->m_string, pos, character)
		);

		this->updateBlinkLabelToChar(getAndSetNextPos());
	}

	/**
	 * @brief Inserts `str` at `pos` replacing `len` characters
	 * 
	 * @param pos
	 * @param len
	 * @param str
	 */
	void insertStringAtPos(int pos, std::size_t len, const std::string_view str)
	{
		log::debug("before inputting str: {}", m_fields->m_string);
		m_fields->m_string.replace(pos, len, str);
		log::debug("after inputting str: {}", m_fields->m_string);

		setAndUpdateString(m_fields->m_string);

		updateBlinkLabelToCharForced(
			(m_fields->m_highlighted.getFromPos() + str.length()) == m_fields->m_string.length()
				? -1
				: m_fields->m_highlighted.getFromPos() + str.length()
		);
	}

	/**
	 * @brief Deletes `pos`
	 * 
	 * @param pos
	 * @param isDel is the del key responsible for this delete event
	 */
	void deletePos(int pos, bool isDel)
	{
		if (
			!m_fields->m_highlighted.isHighlighting() &&
			(
				m_fields->m_string.empty() ||
				(isDel && pos == -1) ||
				(!isDel && pos == 0)
			)
		) return;

		if (m_fields->m_highlighted.isHighlighting())
		{
			insertStringAtPos(
				m_fields->m_highlighted.getFromPos(),
				m_fields->m_highlighted.getLength(),
				""
			);

			if (m_fields->m_string.empty())
			{
				updateBlinkLabelToCharForced(-1);
				showTextOrPlaceholder(false);

				if (this->m_textArea)
					this->m_cursor->setPosition(m_fields->m_text_area_initial_cursor_pos);
			}

			return;
		}

		if (pos == -1)
			pos = m_fields->m_string.length();

		if (isDel)
			pos++;

		// log::debug("deleting pos {} in str \"{}\"", pos, m_fields->m_string);
		setAndUpdateString(m_fields->m_string.erase(pos - 1, 1));
		// log::debug("becoming \"{}\"", m_fields->m_string);

		// log::debug("pos was {}", m_fields->m_pos);
		if (isDel)
			updateBlinkLabelToCharForced(m_fields->m_pos == m_fields->m_string.length() ? -1 : m_fields->m_pos);
		else
			updateBlinkLabelToCharForced(pos - 1 == m_fields->m_string.length() ? -1 : pos - 1);
		// log::debug("pos is now {}", m_fields->m_pos);

		if (m_fields->m_string.empty())
		{
			if (this->m_textArea)
				this->m_cursor->setPosition(m_fields->m_text_area_initial_cursor_pos);

			showTextOrPlaceholder(false);
		}
	}

	/**
	 * @brief Gets the character's position relative to the the parent CCTextInputNode
	 * 
	 * This position is anchored to the left (or right) of the actual position, and is not the center of the character.
	 * e.g.:
	 * 
	 * This square is a CCFontSprite
	 * 
	 * +---+
	 * # * #
	 * +---+
	 * 
	 * *: the actual position of the character
	 * #: the position returned by this function
	 * 
	 * @param pos
	 * @param isLeftAnchored
	 * @return CCPoint
	 */
	CCPoint getCharNodeSpacePos(unsigned int pos, bool isLeftAnchored)
	{
		if (pos == -1)
			pos = m_fields->m_string.length();

		if (this->m_placeholderLabel)
		{
			auto charNode = static_cast<CCNode*>(
				this->m_placeholderLabel->getChildren()->objectAtIndex(pos)
			);

			CCPoint charNodeNodeSpacePos = this->convertToNodeSpace(
				this->m_placeholderLabel->convertToWorldSpace(charNode->getPosition())
			);

			// log::debug("char at pos {} has position {{ {} }}", pos, this->convertToNodeSpace(
			// 	this->m_placeholderLabel->convertToWorldSpace(charNode->getPosition())
			// ));
			// log::debug("charNode {{ {} }}", charNode->getPosition());

			const float offset = ((charNode->getContentWidth() * this->m_placeholderLabel->getScaleX()) / 2);
			return {
				isLeftAnchored ? (charNodeNodeSpacePos.x - offset) : (charNodeNodeSpacePos.x + offset),
				charNodeNodeSpacePos.y
			};
		}
		else
		{
			auto textAreaLabels = CCArrayExt<CCLabelBMFont*>(this->m_textArea->m_label->getChildren());
			CCLabelBMFont* targetLabel;

			for (auto* label : textAreaLabels)
			{
				std::size_t labelStringSize = std::string_view(label->getString()).length();

				if (labelStringSize >= pos)
				{
					targetLabel = label;
					break;
				}

				pos -= labelStringSize;
			}

			auto targetLabelLetters = CCArrayExt<CCNode*>(targetLabel->getChildren());

			// TODO: update
			return this->convertToNodeSpace(
				targetLabel->convertToWorldSpace(targetLabelLetters[pos]->getPosition())
			) - 5.f;
		}
	}

	/**
	 * @brief Gets and sets the next cursor postion
	 * To be used when calling updateBlinkLabelToChar
	 * 
	 * @return int 
	 */
	[[nodiscard]] int getAndSetNextPos()
	{
		if (const auto& str = m_fields->m_string; !str.empty())
		{
			if (m_fields->m_pos != -1)
			{
				if (m_fields->m_pos + 1 == str.length())
					m_fields->m_pos = -1;
				else if (m_fields->m_pos < str.length())
					m_fields->m_pos++;
			}
		}
		else
			m_fields->m_pos = 0;

		return m_fields->m_pos;
	}

	/**
	 * @brief Gets and sets the previous cursor postion
	 * To be used when calling updateBlinkLabelToChar
	 * 
	 * @return int 
	 */
	[[nodiscard]] int getAndSetPreviousPos()
	{
		if (const auto& str = m_fields->m_string; !str.empty())
		{
			if (m_fields->m_pos != 0)
			{
				if (m_fields->m_pos == -1)
					m_fields->m_pos = str.length() - 1;
				else
					m_fields->m_pos--;
			}
		}
		else
			m_fields->m_pos = 0;

		return m_fields->m_pos;
	}
};

struct BetterCCTextFieldTTF : Modify<BetterCCTextFieldTTF, CCTextFieldTTF>
{
	// CCTextInputNode::getString is inlined, all it does is call CCTextFieldTTF::getString
	const char* getString()
	{
		if (g_selectedInput && !g_selectedInput->m_fields->m_get_text_field_str)
			return g_selectedInput->m_fields->m_string.c_str();

		return CCTextFieldTTF::getString();
	}
};


// this pretty much reworks everything when a CCTextInputNode is interacted with,
// from text input to deletion and even selection


// backspace and del is handled in dispatchDeleteBackward/dispatchDeleteForward then in CCEGLView::onGLFWKeyCallback
// same goes for all the other characters (dispatchInsertText then CCEGLView::onGLFWKeyCallback)
// except for ctrl and shift, which are only handled in CCEGLView::onGLFWKeyCallback
struct BetterCCIMEDispatcher : Modify<BetterCCIMEDispatcher, CCIMEDispatcher>
{
	void dispatchDeleteBackward()
	{
		if (g_selectedInput) return;

		CCIMEDispatcher::dispatchDeleteBackward();
	}

	void dispatchDeleteForward()
	{
		if (g_selectedInput) return;

		CCIMEDispatcher::dispatchDeleteForward();
	}

	void dispatchInsertText(const char* text, int len, cocos2d::enumKeyCodes keyCode)
	{
		if (g_selectedInput)
			return g_selectedInput->insertCharAtPos(g_selectedInput->getPos(), text[0]);

		CCIMEDispatcher::dispatchInsertText(text, len, keyCode);
	}
};

// handles ctrl and shift
// also handles mouse clicks
struct BetterCCEGLView : Modify<BetterCCEGLView, CCEGLView>
{
	void onGLFWKeyCallback(
		GLFWwindow* window,
		int key,
		int scancode,
		// 0 released, 1 clicked, 2 held
		int action,
		int mods
	) {
		if (!g_selectedInput)
			return CCEGLView::onGLFWKeyCallback(window, key, scancode, action, mods);

		// log::debug("key: {}", key);

		if (action != 0)
		{
			switch (key)
			{
				case GLFW_KEY_ESCAPE:
					g_selectedInput->deselectInput();
					break;

				case GLFW_KEY_HOME:
					g_selectedInput->updateBlinkLabelToCharForced(0);
					break;

				case GLFW_KEY_END:
					g_selectedInput->updateBlinkLabelToCharForced(-1);
					break;

				case GLFW_KEY_BACKSPACE:
				case GLFW_KEY_DELETE:
					g_selectedInput->onDelete(key == GLFW_KEY_DELETE);
					break;

				case GLFW_KEY_RIGHT:
					g_selectedInput->onRightArrow(
						BI::windows::keyDown(VK_CONTROL),
						BI::windows::keyDown(VK_SHIFT)
					);
					break;

				case GLFW_KEY_LEFT:
					g_selectedInput->onLeftArrow(
						BI::windows::keyDown(VK_CONTROL),
						BI::windows::keyDown(VK_SHIFT)
					);
					break;

				default:
					break;
			}
		}

		// this is what onGLFWKeyCallback actually does to check for control lol
		if (action == 1 && BI::windows::keyDown(VK_CONTROL))
		{
			switch (key)
			{
				case GLFW_KEY_A:
					g_selectedInput->highlightFromToPos(0, -1);
					break;

				case GLFW_KEY_C:
					g_selectedInput->onCopy();
					break;

				case GLFW_KEY_V:
					g_selectedInput->onPaste();
					break;

				case GLFW_KEY_X:
					g_selectedInput->onCut();
					break;

				default:
					break;
			}
		}
	}

	// for some odd reason, the cursor's position isnt updated until the 2nd click
	// this fixes it :D
	void onGLFWMouseCallBack(GLFWwindow* window, int button, int action, int mods)
	{
		CCEGLView::onGLFWMouseCallBack(window, button, action, mods);

		if (!g_selectedInput || button != GLFW_MOUSE_BUTTON_1) return;

		CCSize winSize = CCDirector::sharedDirector()->getWinSize();
		CCPoint mousePos = BI::cocos::getMousePosition();

		// OpenGL's mouse origin is the bottom left
		// CCTouch's mouse origin is top left (because of course it is)
		CCTouch touch{};
		touch.setTouchInfo(0, mousePos.x, winSize.height - mousePos.y);

		// log::debug("mouse pos: {}", mousePos);
		// log::debug("touch pos: {}", CCPoint{ mousePos.x, winSize.height - mousePos.y });

		g_selectedInput->useUpdateBlinkPos(true);

		// 🥰
		g_selectedInput->ccTouchBegan(&touch, nullptr);
	}
};
