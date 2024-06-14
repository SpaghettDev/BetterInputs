#include <string>
#include <string_view>

#include <Geode/modify/CCTextInputNode.hpp>
#include <Geode/modify/CCEGLView.hpp>
#include <Geode/modify/CCIMEDispatcher.hpp>

#include <Geode/modify/CCTextFieldTTF.hpp>
#include <Geode/modify/CCScene.hpp>
#include <Geode/binding/LoadingCircle.hpp>

#include <Geode/cocos/robtop/glfw/glfw3.h>

#include "types/HighlightedString.hpp"
#include "types/CharNode.hpp"
#include "types/InputNodeTextAreaInfo.hpp"
#include "utils.hpp"

using namespace geode::prelude;

struct BetterTextInputNode;
BetterTextInputNode* g_selectedInput = nullptr;

struct BetterTextInputNode : Modify<BetterTextInputNode, CCTextInputNode>
{
	struct Fields
	{
		// the actual string, because we basically rework everything about the input text insertion is broken :D
		// (getString() normally returns the last character or is empty, we hook CCTextFieldTTF::getString to fix this)
		std::string m_string = "";

		// current cursor position
		int m_pos = -1;

		HighlightedString m_highlighted;
		bool m_is_adding_to_highlight = false;
		std::vector<cocos2d::extension::CCScale9Sprite*> m_highlights;

		bool m_use_update_blink_pos = false;
		bool m_get_text_field_str = false;

		std::string m_placeholder;
	};

	bool init(float p0, float p1, char const* p2, char const* p3, int p4, char const* p5)
	{
		m_fields->m_placeholder = p2;

		if (!CCTextInputNode::init(p0, p1, p2, p3, p4, p5)) return false;

		appendHighlightNode();

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
			m_fields->m_string = textFieldStr;
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
			m_fields->m_pos = pos;

		this->m_textField->m_uCursorPos = m_fields->m_pos;

		CCTextInputNode::updateBlinkLabelToChar(m_fields->m_pos);

		if (this->m_textArea)
			textAreaCursorFix(m_fields->m_pos);

		if (m_fields->m_highlighted.isHighlighting() && !m_fields->m_is_adding_to_highlight)
			clearHighlight();
	}

	void setString(gd::string str)
	{
		setAndUpdateString(str);
	}


	// bug fixes
	void textAreaCursorFix(std::size_t pos)
	{
		if (m_fields->m_string.empty()) return;

		// fix space character being quirky and being positioned in the bottom left of the last character
		// which makes this function position the cursor sometimes be in the middle of the previous character
		// or sometimes somewhere not even close
		// also it sometimes just positions the cursor somewhere completely wrong regardless of the character being
		// a space or not, which we fix below (i think CCTextInputNode is very ass)
		if (m_fields->m_string.length() >= 2)
		{
			// ???
			if (m_fields->m_pos > m_fields->m_string.length() - 1)
				m_fields->m_pos = -1;

			// what's to the right of the cursor
			int pos = m_fields->m_pos == -1 ? m_fields->m_string.length() - 1 : m_fields->m_pos;
			const auto& cursorTextLabelInfo = getTextLabelInfoFromPos(m_fields->m_pos);

			if (pos != 0 && m_fields->m_string[pos - 1] == ' ')
			{
				float targetXPos;

				// if the space character isnt the last in the current label, set the position to the left of the next character
				if (cursorTextLabelInfo.numCharsFromLabelStart != std::string_view(cursorTextLabelInfo.label->getString()).length() - 1)
					targetXPos = getCharNodePosInfo(pos, m_fields->m_pos != -1).position.x;
				else
				{
					const auto& secondToLastCharNode = getCharNodePosInfo(pos - 2, false);
					targetXPos = secondToLastCharNode.position.x + (secondToLastCharNode.widthFromCenter * 2);
				}

				this->m_cursor->setPositionX(targetXPos);
			}
			else
			{
				if (m_fields->m_pos == -1)
				{
					float targetXPos;

					if (m_fields->m_string[pos] == ' ')
					{
						const auto& secondToLastCharNode = getCharNodePosInfo(pos - 1, false);

						targetXPos = secondToLastCharNode.position.x + (secondToLastCharNode.widthFromCenter * 2);
					}
					else
						targetXPos = getCharNodePosInfo(-1, false).position.x;

					this->m_cursor->setPositionX(targetXPos);
				}
				else
					this->m_cursor->setPositionX(
						getCharNodePosInfo(
							cursorTextLabelInfo.numCharsFromStart - (m_fields->m_pos != 0),
							m_fields->m_pos == 0
						).position.x
					);
			}
		}
	}


	// key events
	void onRightArrowKey(bool isCtrl, bool isShift)
	{
		if (!m_fields->m_highlighted.isHighlighting() && (m_fields->m_string.empty() || m_fields->m_pos == -1)) return;

		if (m_fields->m_highlighted.isHighlighting() && !isShift)
		{
			updateBlinkLabelToCharForced(
				m_fields->m_highlighted.getToPos(false) == m_fields->m_string.length()
					? -1
					: m_fields->m_highlighted.getToPos()
			);
			clearHighlight();
			return;
		}

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

	void onLeftArrowKey(bool isCtrl, bool isShift)
	{
		if (!m_fields->m_highlighted.isHighlighting() && (m_fields->m_string.empty() || m_fields->m_pos == 0)) return;

		if (m_fields->m_highlighted.isHighlighting() && !isShift)
		{
			updateBlinkLabelToCharForced(
				m_fields->m_highlighted.getFromPos() == m_fields->m_string.length()
					? -1
					: m_fields->m_highlighted.getFromPos()
			);
			clearHighlight();
			return;
		}

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

	void onHomeKey(bool isShift)
	{
		if (m_fields->m_string.empty() || m_fields->m_pos == 0) return;

		if (this->m_textArea)
		{
			const auto& cursorTextLabelInfo = getTextLabelInfoFromPos(m_fields->m_pos);

			if (cursorTextLabelInfo.numCharsFromLabelStart == 0) return;

			if (isShift)
				highlightFromToPos(
					m_fields->m_highlighted.isHighlighting()
						? m_fields->m_highlighted.getToPos()
						: cursorTextLabelInfo.numCharsFromStart - cursorTextLabelInfo.numCharsFromLabelStart - 1,
					m_fields->m_pos
				);
			else
				updateBlinkLabelToCharForced(
					cursorTextLabelInfo.numCharsFromStart - cursorTextLabelInfo.numCharsFromLabelStart - 1
				);
		}
		else
		{
			if (isShift)
				highlightFromToPos(
					0,
					m_fields->m_highlighted.isHighlighting()
						? m_fields->m_highlighted.getToPos()
						: m_fields->m_pos
				);
			else
				updateBlinkLabelToCharForced(0);
		}
	}

	void onEndKey(bool isShift)
	{
		if (m_fields->m_pos == m_fields->m_string.length() - 1)
			updateBlinkLabelToCharForced(-1);

		if (m_fields->m_string.empty() || m_fields->m_pos == -1) return;

		if (this->m_textArea)
		{
			const auto& cursorTextLabelInfo = getTextLabelInfoFromPos(m_fields->m_pos);

			int targetPos = m_fields->m_pos + (
				std::string_view(cursorTextLabelInfo.label->getString()).length() - cursorTextLabelInfo.numCharsFromLabelStart
			) - 1;

			if (
				targetPos - 1 == cursorTextLabelInfo.numCharsFromLabelStart &&
				this->m_textArea->m_label->getChildrenCount() >= cursorTextLabelInfo.line
			) {
				// get the end of the next label
				targetPos = targetPos + std::string_view(
					static_cast<CCLabelBMFont*>(
						this->m_textArea->m_label->getChildren()->objectAtIndex(cursorTextLabelInfo.line + 1)
					)->getString()
				).length();

				if (isShift)
					highlightFromToPos(
						m_fields->m_highlighted.isHighlighting()
							? m_fields->m_highlighted.getFromPos()
							: m_fields->m_pos,
						targetPos
					);
				else
					updateBlinkLabelToCharForced(targetPos);
			}
			else
			{
				if (isShift)
					highlightFromToPos(m_fields->m_pos, targetPos == m_fields->m_string.length() ? -1 : targetPos);
				else
					updateBlinkLabelToCharForced(targetPos == m_fields->m_string.length() ? -1 : targetPos);
			}
		}
		else
		{
			if (isShift)
				highlightFromToPos(
					m_fields->m_highlighted.isHighlighting()
						? m_fields->m_highlighted.getFromPos()
						: m_fields->m_pos,
					-1
				);
			else
				updateBlinkLabelToCharForced(-1);
		}
	}

	void onDelete(bool isCtrl, bool isDel)
	{
		if (isCtrl)
		{
			if (!m_fields->m_highlighted.isHighlighting())
			{
				if (m_fields->m_pos == (isDel ? -1 : 0)) return;
				if (isDel)
					onRightArrowKey(true, true);
				else
					onLeftArrowKey(true, true);
			}

			deletePos(m_fields->m_pos, true);

			return;
		}

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
		const int pos = m_fields->m_pos == -1 ? m_fields->m_string.length() : m_fields->m_pos;

		if (m_fields->m_highlighted.isHighlighting())
			insertStrAtPos(m_fields->m_highlighted.getFromPos(), m_fields->m_highlighted.getLength(), clipboardText);
		else
		{
			setAndUpdateString(BI::utils::insertStrAtIndex(m_fields->m_string, m_fields->m_pos, clipboardText));

			updateBlinkLabelToCharForced(pos + clipboardText.size() == m_fields->m_string.length() ? -1 : pos + clipboardText.size());
		}
	}

	void onCut()
	{
		if (!m_fields->m_highlighted.isHighlighting()) return;

		clipboard::write(m_fields->m_highlighted.str.data());

		insertStrAtPos(
			m_fields->m_highlighted.getFromPos(),
			m_fields->m_highlighted.getLength(),
			""
		);

		if (m_fields->m_string.empty())
			onStringEmpty();
	}


	// other events
	void onStringEmpty()
	{
		if (this->m_placeholderLabel)
			this->m_placeholderLabel->setString("");
		else
			this->m_textArea->setString("");
		updateBlinkLabelToCharForced(-1);
		showTextOrPlaceholder(false);
	}


	// getters and setters
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

	// TODO: maybe export these?
	int getCursorPos() { return m_fields->m_pos; }
	const HighlightedString& getHighlighted() { return m_fields->m_highlighted; }


	// rewritten input stuff, helpers and highlighting
	/**
	 * @brief Highlights from `from` to `to` by setting the highlight's content width
	 * 
	 * @param from
	 * @param to
	 */
	void highlightFromToPos(int from, int to)
	{
		if (m_fields->m_string.empty()) return;

		const CCPoint startPos = getCharNodePosInfo(0, false);

		if ((from == -1 ? m_fields->m_string.length() : from) > (to == -1 ? m_fields->m_string.length() : to))
			std::swap(from, to);

		if (from == to)
			return clearHighlight();

		m_fields->m_highlighted.update(m_fields->m_string, { from, to });

		if (this->m_placeholderLabel)
		{
			m_fields->m_highlights[0]->setPositionX(getCharNodePosInfo(from, true).position.x - 1.f);
			m_fields->m_highlights[0]->setContentWidth(
				std::abs(
					m_fields->m_highlights[0]->getPositionX() - getCharNodePosInfo(to, to != -1).position.x
				)
			);
			m_fields->m_highlights[0]->setScaleY(
				3.f * this->m_placeholderLabel->getScale() + .4f
			);
			m_fields->m_highlights[0]->setVisible(true);
		}
		else
		{
			auto textAreaLabels = CCArrayExt<CCLabelBMFont*>(this->m_textArea->m_label->getChildren());

			std::size_t talSize = textAreaLabels.size();
			std::size_t highlightsSize = m_fields->m_highlights.size();

			if (talSize != highlightsSize)
			{
				if (talSize > highlightsSize)
					while (m_fields->m_highlights.size() < talSize)
						appendHighlightNode();
				else
					while (m_fields->m_highlights.size() > talSize)
						removeLastHighlightNode();
			}

			// beware! very freaky code below
			{
				std::size_t line = 0;
				std::size_t targetFrom = from;
				std::size_t targetTo = to == -1 ? m_fields->m_string.length() - 1 : to;

				bool hasHighlightedStart = false;
				bool hasHighlightedEnd = false;

				for (auto* label : textAreaLabels)
				{
					const std::size_t labelStrLen = std::string_view(label->getString()).length() - 1;
					const auto& highlightSprite = m_fields->m_highlights[line];

					// label is anchored to bottom left :D
					highlightSprite->setPositionY(
						this->convertToNodeSpace(
							this->m_textArea->m_label->convertToWorldSpace(label->getPosition())
						).y + (label->getContentHeight() / 2)
					);
					highlightSprite->setScaleY(2.3f * this->m_textArea->getScale());
					highlightSprite->setVisible(false);

					if (hasHighlightedStart && hasHighlightedEnd)
						continue; // dont break in case other highlights are visible

					if (!hasHighlightedStart)
					{
						if (targetFrom <= labelStrLen)
						{
							const auto& textLabelInfo = getTextLabelInfoFromPos(
								m_fields->m_highlighted.isHighlighting()
									? m_fields->m_highlighted.getToPos(false)
									: m_fields->m_pos
							);
							hasHighlightedStart = true;

							highlightSprite->setPositionX(getCharNodePosInfoAtLine(targetFrom, line, true).position.x);
							highlightSprite->setVisible(true);

							// in case we're highlighting from and to the next label/the same label
							if (targetTo > labelStrLen)
							{
								float targetXPos;

								// fix space character being quirky and being positioned in the bottom left of the last character :D
								if (std::string_view(label->getString()).back() == ' ')
								{
									const auto& secondToLastCharNode = getCharNodePosInfoAtLine(
										labelStrLen - 1,
										line,
										false
									);
									targetXPos = secondToLastCharNode.position.x + (secondToLastCharNode.widthFromCenter * 2);
								}
								else
									targetXPos = getCharNodePosInfoAtLine(labelStrLen, line, false).position.x;

								highlightSprite->setContentWidth(std::abs(highlightSprite->getPositionX() - targetXPos));

								if (targetTo == labelStrLen + 1)
									hasHighlightedEnd = true;
							}
							else
							{
								// if selection ends at the end of the current label,
								// use the right of last character. else set to left of target character
								if (textLabelInfo.numCharsFromLabelStart == std::string_view(textLabelInfo.label->getString()).length() - 1)
									highlightSprite->setContentWidth(
										std::abs(
											highlightSprite->getPositionX() - getCharNodePosInfoAtLine(
												textLabelInfo.numCharsFromLabelStart,
												textLabelInfo.line,
												false
											).position.x
										)
									);
								else
									highlightSprite->setContentWidth(
										std::abs(
											highlightSprite->getPositionX() - getCharNodePosInfoAtLine(
												targetTo,
												line,
												true
											).position.x
										)
									);

								hasHighlightedEnd = true;
							}
						}
					}
					else if (!hasHighlightedEnd)
					{
						highlightSprite->setPositionX(getCharNodePosInfoAtLine(0, line, true).position.x);

						// again, check if we're highlighting to the current label or afterwards
						if (targetTo > labelStrLen)
						{
							float targetXPos;

							if (std::string_view(label->getString()).back() == ' ')
							{
								const auto& secondToLastCharNode = getCharNodePosInfoAtLine(
									labelStrLen - 1,
									line,
									false
								);
								targetXPos = secondToLastCharNode.position.x + (secondToLastCharNode.widthFromCenter * 2);
							}
							else
								targetXPos = getCharNodePosInfoAtLine(labelStrLen, line, to != -1).position.x;

							highlightSprite->setContentWidth(
								std::abs(
									highlightSprite->getPositionX() - targetXPos
								)
							);

							highlightSprite->setVisible(true);
						}
						else
						{
							highlightSprite->setContentWidth(
								std::abs(
									highlightSprite->getPositionX() - getCharNodePosInfoAtLine(targetTo, line, to != -1).position.x
								)
							);

							// selecting from a line to the last character in another one in which the last character is space
							// *and* followed by another line causes the last highlight to become visible
							highlightSprite->setVisible(
								std::string_view(
									static_cast<CCLabelBMFont*>(
										this->m_textArea->m_label->getChildren()->objectAtIndex(line - 1)
									)->getString()
								).back() != ' ' ||
								getTextLabelInfoFromPos(m_fields->m_highlighted.getToPos()).numCharsFromLabelStart != 0
							);

							hasHighlightedEnd = true;
						}
					}

					targetFrom -= labelStrLen + 1;
					targetTo -= labelStrLen + 1;
					line++;
				}
			}
		}
	}

	/**
	 * @brief Inserts `character` at `pos`
	 * 
	 * @param pos
	 * @param character
	 */
	void insertCharAtPos(int pos, char character)
	{
		if (
			!BI::geode::get<bool>("allow-any-character") &&
			BI::gd::isVanillaInput() &&
			std::string_view(this->m_allowedChars).find(character) == std::string_view::npos
			) return;

		const bool wasHighlighting = m_fields->m_highlighted.isHighlighting();

		if (wasHighlighting)
			insertStrAtPos(
				m_fields->m_highlighted.getFromPos(),
				m_fields->m_highlighted.getLength(),
				""
			);

		setAndUpdateString(
			BI::utils::insertCharAtIndex(m_fields->m_string, wasHighlighting ? m_fields->m_pos : pos, character)
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
	void insertStrAtPos(int pos, std::size_t len, const std::string& str)
	{
		const std::size_t endPos = m_fields->m_highlighted.getFromPos() + str.length();

		m_fields->m_string.replace(pos, len, str.data());

		setAndUpdateString(m_fields->m_string);

		updateBlinkLabelToCharForced(endPos == m_fields->m_string.length() ? -1 : endPos);
	}

	/**
	 * @brief Deletes the character at `pos`
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
			insertStrAtPos(
				m_fields->m_highlighted.getFromPos(),
				m_fields->m_highlighted.getLength(),
				""
			);

			if (m_fields->m_string.empty())
				onStringEmpty();

			return;
		}

		if (pos == -1)
			pos = m_fields->m_string.length();

		if (isDel)
			pos++;

		setAndUpdateString(m_fields->m_string.erase(pos - 1, 1));

		if (isDel)
			updateBlinkLabelToCharForced(m_fields->m_pos == m_fields->m_string.length() ? -1 : m_fields->m_pos);
		else
			updateBlinkLabelToCharForced(pos - 1 == m_fields->m_string.length() ? -1 : pos - 1);

		if (m_fields->m_string.empty())
			onStringEmpty();
	}

	/**
	 * @brief Gets the character's position info relative to the the parent CCTextInputNode
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
	 * @return CharNode
	 */
	CharNode getCharNodePosInfo(std::size_t pos, bool isLeftAnchored)
	{
		if (pos == -1)
			pos = m_fields->m_string.length() - 1;

		if (this->m_placeholderLabel)
		{
			auto charNode = static_cast<CCNode*>(
				this->m_placeholderLabel->getChildren()->objectAtIndex(pos)
			);

			CCPoint charNodeNodeSpacePos = this->convertToNodeSpace(
				this->m_placeholderLabel->convertToWorldSpace(charNode->getPosition())
			);

			const float offset = (charNode->getContentWidth() * this->m_placeholderLabel->getScaleX()) / 2;
			return {
				{
					isLeftAnchored ? (charNodeNodeSpacePos.x - offset) : (charNodeNodeSpacePos.x + offset),
					charNodeNodeSpacePos.y
				},
				charNodeNodeSpacePos,
				offset
			};
		}
		else
		{
			CCLabelBMFont* targetLabel;

			for (auto* label : CCArrayExt<CCLabelBMFont*>(this->m_textArea->m_label->getChildren()))
			{
				std::size_t labelStringSize = std::string_view(label->getString()).length() - 1;

				if (labelStringSize >= pos)
				{
					targetLabel = label;
					break;
				}

				pos -= labelStringSize + 1;
			}

			auto charNode = static_cast<CCNode*>(targetLabel->getChildren()->objectAtIndex(pos));

			CCPoint charNodeNodeSpacePos = this->convertToNodeSpace(
				targetLabel->convertToWorldSpace(charNode->getPosition())
			);

			const float offset = (charNode->getContentWidth() * this->m_textArea->getScaleX()) / 2;
			return {
				{
					isLeftAnchored ? (charNodeNodeSpacePos.x - offset) : (charNodeNodeSpacePos.x + offset),
					charNodeNodeSpacePos.y
				},
				charNodeNodeSpacePos,
				offset
			};
		}
	}

	/**
	 * @brief Gets the character's position at a certain line relative to the the parent CCTextInputNode
	 * Should only be called if m_textArea isn't nullptr.
	 * 
	 * @ref getChatNodeSpacePos "Refer to that function for more information about what this returns"
	 * 
	 * @param pos
	 * @param line
	 * @param isLeftAnchored
	 * @return CharNode
	 */
	CharNode getCharNodePosInfoAtLine(std::size_t pos, std::size_t line, bool isLeftAnchored)
	{
		auto targetLabel = static_cast<CCLabelBMFont*>(
			this->m_textArea->m_label->getChildren()->objectAtIndex(line)
		);

		if (pos == -1)
			pos = std::string_view(targetLabel->getString()).length() - 1;

		auto charNode = static_cast<CCNode*>(
			targetLabel->getChildren()->objectAtIndex(pos)
		);

		CCPoint charNodeNodeSpacePos = this->convertToNodeSpace(
			targetLabel->convertToWorldSpace(charNode->getPosition())
		);

		const float offset = (charNode->getContentWidth() * this->m_textArea->getScaleX()) / 2;
		return {
			{
				isLeftAnchored ? (charNodeNodeSpacePos.x - offset) : (charNodeNodeSpacePos.x + offset),
				charNodeNodeSpacePos.y
			},
			charNodeNodeSpacePos,
			offset
		};
	}

	/**
	 * @brief Gets info about the label from a certain position
	 * (line number, distance from beginning of label, from beginning of TextArea, and the label itself)
	 *
	 * @param pos
	 * 
	 * @return InputNodeTextAreaInfo
	 */
	InputNodeTextAreaInfo getTextLabelInfoFromPos(std::size_t pos)
	{
		if (pos == -1)
			pos = m_fields->m_string.length() - 1;

		if (this->m_textArea)
		{
			CCLabelBMFont* targetLabel = nullptr;
			std::size_t targetLabelPos = pos;
			std::size_t line = 0;

			for (auto* label : CCArrayExt<CCLabelBMFont*>(this->m_textArea->m_label->getChildren()))
			{
				const std::size_t labelStrLen = std::string_view(label->getString()).length();

				if (targetLabelPos <= labelStrLen)
				{
					targetLabel = label;
					break;
				}
				else
				{
					targetLabelPos -= labelStrLen;
					line++;
				}
			}

			return {
				targetLabel,
				line,
				pos,
				targetLabelPos - 1
			};
		}

		return {
			this->m_placeholderLabel,
			0,
			pos,
			pos
		};
	}

	/**
	 * @brief Sets the internal m_string to `str` while retaining cursor position
	 * 
	 * @param str
	 */
	void setAndUpdateString(const std::string& str)
	{
		// the position is modified in the call to setString
		const int prevPos = m_fields->m_pos;
		m_fields->m_string = str;

		CCTextInputNode::setString(str);

		m_fields->m_pos = prevPos;
	}

	/**
	 * @brief Since we hook updateBlinkLabelToChar and never call it with the supplied `pos` parameter,
	 * the only way to set the cursor position is by setting the `m_pos` member manually
	 * 
	 * @param pos
	 */
	void updateBlinkLabelToCharForced(int pos)
	{
		m_fields->m_use_update_blink_pos = true;

		this->updateBlinkLabelToChar(pos);

		m_fields->m_use_update_blink_pos = false;
	}

	/**
	 * @brief Hides all the highlight sprites and clears the `m_highlighted` member
	 */
	void clearHighlight()
	{
		for (auto* highlight : m_fields->m_highlights)
			highlight->setVisible(false);

		m_fields->m_highlighted.reset();
	}

	/**
	 * @brief Deselects an input node and resets its string to the placeholder
	 */
	void deselectInput()
	{
		if (this->m_placeholderLabel)
			this->m_placeholderLabel->setString(m_fields->m_placeholder.c_str());
		else
			this->m_textArea->setString(m_fields->m_placeholder.c_str());

		showTextOrPlaceholder(true);

		this->onClickTrackNode(false);
		this->m_cursor->setVisible(false);
	}

	/**
	 * @brief Adds a highlight node to the parent CCTextInputNode
	 * 
	 * @return cocos2d::extension::CCScale9Sprite*
	 */
	cocos2d::extension::CCScale9Sprite* appendHighlightNode()
	{
		auto highlight = cocos2d::extension::CCScale9Sprite::create("square.png");

		highlight->setOpacity(120);
		highlight->setPosition(
			this->m_placeholderLabel
				? CCPoint{this->m_cursor->getPosition() - 1.f}
				: CCPoint{ .0f, .0f }
		);
		highlight->setAnchorPoint({ .0f, .5f });
		highlight->setVisible(false);
		highlight->setID(fmt::format(GEODE_MOD_ID "/highlight-sprite-{}", m_fields->m_highlights.size() + 1));
		this->addChild(highlight, 10);

		m_fields->m_highlights.emplace_back(highlight);

		return highlight;
	}

	/**
	 * @brief Removes the last highlight node from the parent
	 */
	void removeLastHighlightNode()
	{
		m_fields->m_highlights.back()->removeFromParent();
		m_fields->m_highlights.pop_back();
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
			m_fields->m_pos = -1;

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
			m_fields->m_pos = -1;

		return m_fields->m_pos;
	}
};

// CCTextInputNode::getString is inlined, all it does is call CCTextFieldTTF::getString
struct BetterCCTextFieldTTF : Modify<BetterCCTextFieldTTF, CCTextFieldTTF>
{
	const char* getString()
	{
		if (g_selectedInput && !g_selectedInput->m_fields->m_get_text_field_str)
			return g_selectedInput->m_fields->m_string.c_str();

		return CCTextFieldTTF::getString();
	}
};

// fix layers appearing above selected CCTextInputNodes making ESC key deselect the
// input node instead of closing the alert
struct AlertLayerFix : Modify<AlertLayerFix, CCScene>
{
	struct Fields
	{
		std::size_t m_previous_scene_children_count = 0;
		CCLayer* m_outermost_input_parent;
	};

	static CCScene* create()
	{
		auto ret = CCScene::create();

		// inspired by what HappyTextures by Alphalaneous does
		ret->schedule(schedule_selector(AlertLayerFix::onUpdateTick), .1f);

		return ret;
	}

	void onUpdateTick(float)
	{
		const std::size_t currentChildrenCount = this->getChildrenCount();

		if (
			!g_selectedInput ||
			currentChildrenCount == 1 ||
			currentChildrenCount == m_fields->m_previous_scene_children_count
		)
			return;

		if (!m_fields->m_outermost_input_parent)
		{
			CCLayer* outermostInputParent = static_cast<CCLayer*>(g_selectedInput->getParent());
			while (outermostInputParent->getParent() != this)
				outermostInputParent = static_cast<CCLayer*>(outermostInputParent->getParent());

			m_fields->m_outermost_input_parent = outermostInputParent;
		}

		if (static_cast<CCLayer*>(this->getChildren()->lastObject())->getTouchPriority() > m_fields->m_outermost_input_parent->getTouchPriority())
			g_selectedInput->deselectInput();

		m_fields->m_previous_scene_children_count = currentChildrenCount;
	}
};


#ifdef GEODE_IS_WINDOWS
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
			return g_selectedInput->insertCharAtPos(g_selectedInput->getCursorPos(), text[0]);

		CCIMEDispatcher::dispatchInsertText(text, len, keyCode);
	}
};

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
				}
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
