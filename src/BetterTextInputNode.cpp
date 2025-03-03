#include <Geode/modify/CCTextInputNode.hpp>

#include "BetterTextInputNode.hpp"

#include "types/CCLabelBMFontPlus.hpp"

#include "utils.hpp"

using namespace geode::prelude;

cocos2d::CCRect createRectFromPoints(
	const cocos2d::CCPoint& bottomLeft,
	const cocos2d::CCPoint& bottomRight,
	const cocos2d::CCPoint& topRight,
	const cocos2d::CCPoint& topLeft
) {
	float minX = std::min({
		bottomLeft.x, bottomRight.x, topRight.x, topLeft.x
	});
	float minY = std::min({
		bottomLeft.y, bottomRight.y, topRight.y, topLeft.y
	});
	float maxX = std::max({
		bottomLeft.x, bottomRight.x, topRight.x, topLeft.x
	});
	float maxY = std::max({
		bottomLeft.y, bottomRight.y, topRight.y, topLeft.y
	});

	return { minX, minY, maxX - minX, maxY - minY };
}


bool BetterTextInputNode::init(float p0, float p1, char const* p2, char const* p3, int p4, char const* p5)
{
	if (!CCTextInputNode::init(p0, p1, p2, p3, p4, p5)) return false;

	m_fields->m_placeholder_str = p2;

	appendHighlightNode();

	this->m_cursor->setScaleX(1.5f);
	this->schedule(schedule_selector(BetterTextInputNode::updateCursorBlink));

	return true;
}

bool BetterTextInputNode::onTextFieldAttachWithIME(cocos2d::CCTextFieldTTF* tField)
{
	g_selectedInput = this;

	return CCTextInputNode::onTextFieldAttachWithIME(tField);
}

bool BetterTextInputNode::onTextFieldDetachWithIME(cocos2d::CCTextFieldTTF* tField)
{
	g_selectedInput = nullptr;

	if (m_fields->m_string.empty())
		onStringEmpty();

	return CCTextInputNode::onTextFieldDetachWithIME(tField);
}

void BetterTextInputNode::updateBlinkLabelToChar(int pos)
{
	m_fields->m_time_since_last_input = .0f;

	showTextOrPlaceholder(true);

	if (m_fields->m_use_update_blink_pos)
		m_fields->m_pos = pos;

	this->m_textField->m_uCursorPos = m_fields->m_pos;

	CCTextInputNode::updateBlinkLabelToChar(m_fields->m_pos);

	updateCursorPos(m_fields->m_pos);

	if (m_fields->m_highlighted.isHighlighting() && !m_fields->m_is_adding_to_highlight)
		clearHighlight();
}

void BetterTextInputNode::setString(gd::string str)
{
	setAndUpdateString(str);
}


void BetterTextInputNode::updateCursorBlink(float dt)
{
	if (!this->m_cursor) return;

	if (!this->m_cursor->isVisible())
	{
		this->m_fields->m_is_blinking = false;
		m_fields->m_time_since_last_input = .0f;
		return this->m_cursor->stopActionByTag(ACTION_TAG::CURSOR_BLINK);
	}

	m_fields->m_time_since_last_input += dt;

	if (this->m_fields->m_is_blinking)
	{
		if (m_fields->m_time_since_last_input < .3f)
		{
			this->m_cursor->pauseSchedulerAndActions();
			this->m_cursor->setOpacity(255);
		}
		else
			this->m_cursor->resumeSchedulerAndActions();

		return;
	}

	CCRepeatForever* blinkAction;

	if (BI::geode::get<bool>("alternate-cursor-blink"))
		blinkAction = CCRepeatForever::create(
			CCSequence::create(
				CCFadeTo::create(.0f, 0),
				CCDelayTime::create(.5f),
				CCFadeTo::create(.0f, 255),
				CCDelayTime::create(.5f),
				nullptr
			)
		);
	else
		blinkAction = CCRepeatForever::create(
			CCSequence::create(
				CCFadeOut::create(.5f),
				CCFadeIn::create(.5f),
				CCDelayTime::create(.1f),
				nullptr
			)
		);
	blinkAction->setTag(ACTION_TAG::CURSOR_BLINK);

	this->m_cursor->runAction(blinkAction);

	this->m_fields->m_is_blinking = true;
}

// fix space character being quirky and being positioned in the bottom left of the last character
// which makes this function position the cursor sometimes be in the middle of the previous character
// or sometimes somewhere not even close
// also it sometimes just positions the cursor somewhere completely wrong regardless of the character being
// a space or not, which we fix below (i think CCTextInputNode is very ass)
void BetterTextInputNode::updateCursorPos(std::size_t pos)
{
	if (m_fields->m_string.empty() || !this->m_cursor) return;

	int cursorPos = m_fields->m_pos == -1 ? m_fields->m_string.length() : m_fields->m_pos;

	if (auto placeholderLabel = static_cast<CCLabelBMFontPlus*>(this->m_placeholderLabel))
	{
		if (m_fields->m_string.empty())
			return this->m_cursor->setPosition({ 2.f, -1.f });

		float cursorXPos;
		if (m_fields->m_pos != -1)
		{
			auto childChar = static_cast<CCFontSprite*>(placeholderLabel->getChildren()->objectAtIndex(cursorPos));
			float charPosX = placeholderLabel->getLetterPosXLeft(childChar, this->m_fontValue2, this->m_isChatFont);
			cursorXPos = charPosX + (this->m_fontValue1 * placeholderLabel->getScaleX());
		}
		else
			cursorXPos = placeholderLabel->getScaledContentWidth() + (2.f * placeholderLabel->getScaleX());

		float anchorOffsetX = placeholderLabel->getContentWidth() * (1.f - placeholderLabel->getAnchorPoint().x);
		float offset = anchorOffsetX * placeholderLabel->getScale();

		CCPoint cursorPos{ cursorXPos - (placeholderLabel->getScaledContentWidth() - offset), -1.f };

		this->m_cursor->setPosition(placeholderLabel->getPosition() + cursorPos);
		this->m_cursor->setAnchorPoint({ .5f, placeholderLabel->getAnchorPoint().y });
	}
	else
	{
		if (m_fields->m_string.empty())
			return this->m_cursor->setPosition({ .0f, -9.f });

		auto charInfo = getCharNodePosInfo(m_fields->m_pos, m_fields->m_pos != -1);

		float labelYPos = this->convertToNodeSpace(
			this->m_textArea->m_label->convertToWorldSpace(charInfo.label->getPosition())
		).y + (charInfo.label->getContentHeight() / 2.f);

		this->m_cursor->setAnchorPoint({ .5f, .45f });
		this->m_cursor->setPosition({ charInfo.position.x, labelYPos });
	}
}


// key events

void BetterTextInputNode::onRightArrowKey(bool isCtrl, bool isShift)
{
	if (!m_fields->m_highlighted.isHighlighting() && (m_fields->m_string.empty() || m_fields->m_pos == -1)) return;

	if (m_fields->m_highlighted.isHighlighting() && !isShift)
	{
		updateBlinkLabelToCharForced(
			m_fields->m_highlighted.getToPos<false>() == m_fields->m_string.length()
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

void BetterTextInputNode::onLeftArrowKey(bool isCtrl, bool isShift)
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

void BetterTextInputNode::onHomeKey(bool isShift)
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

void BetterTextInputNode::onEndKey(bool isShift)
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

void BetterTextInputNode::onDelete(bool isCtrl, bool isDel)
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

void BetterTextInputNode::onCopy()
{
	if (!m_fields->m_highlighted.isHighlighting()) return;

	clipboard::write(m_fields->m_highlighted.str.data());
}

void BetterTextInputNode::onPaste()
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

void BetterTextInputNode::onCut()
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

void BetterTextInputNode::onStringEmpty()
{
	std::string&& newString = "";

	if (g_selectedInput)
		newString = "";
	else
		newString = m_fields->m_placeholder_str;

	if (this->m_placeholderLabel)
		this->m_placeholderLabel->setString(newString.c_str());
	else
		this->m_textArea->setString(newString.c_str());
	updateBlinkLabelToCharForced(-1);
	showTextOrPlaceholder(true);
}


// getters and setters

void BetterTextInputNode::useUpdateBlinkPos(bool toggle)
{
	m_fields->m_use_update_blink_pos = toggle;
}

void BetterTextInputNode::showTextOrPlaceholder(bool toggle)
{
	if (this->m_placeholderLabel)
		this->m_placeholderLabel->setVisible(toggle);
	else
		this->m_textArea->setVisible(toggle);
}

int BetterTextInputNode::getCursorPos() { return m_fields->m_pos; }
const HighlightedString& BetterTextInputNode::getHighlighted() { return m_fields->m_highlighted; }


// rewritten input stuff, helpers and highlighting
/**
 * @brief Highlights from `from` to `to` by setting the highlight's content width
 * 
 * @param from
 * @param to
 */
void BetterTextInputNode::highlightFromToPos(int from, int to)
{
	if (m_fields->m_string.empty()) return;

	if ((from == -1 ? m_fields->m_string.length() : from) > (to == -1 ? m_fields->m_string.length() : to))
		std::swap(from, to);

	if (from == to)
		return clearHighlight();

	auto highlightColor = getHighlightColor();
	m_fields->m_highlighted.update(m_fields->m_string, { from, to });

	if (auto label = static_cast<CCLabelBMFontPlus*>(this->m_placeholderLabel))
	{
		auto highlight = m_fields->m_highlights[0];
		auto fromCharInfo = getCharNodePosInfo(from, true);
		auto toCharInfo = getCharNodePosInfo(to, to != -1);

		float topY = fromCharInfo.position.y - getHighlightOffset(label->getScale())
			+ fromCharInfo.sprite->getScaledContentHeight() / 2.f;
		float bottomY = toCharInfo.position.y - getHighlightOffset(label->getScale())
			- toCharInfo.sprite->getScaledContentHeight() / 2.f;

		CCRect rect = createRectFromPoints(
			{ fromCharInfo.position.x, bottomY },
			{ toCharInfo.position.x, bottomY },
			{ toCharInfo.position.x, topY },
			{ fromCharInfo.position.x, topY }
		);
		// we can get away by just lying about the height
		rect.size.height = label->getScaledContentHeight();

		highlight->clear();
		highlight->drawRect(rect, highlightColor, .0f, highlightColor);
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

		std::size_t targetFrom = from;
		std::size_t targetTo = to == -1 ? m_fields->m_string.length() - 1 : to;

		bool hasHighlightedStart = false;
		bool hasHighlightedEnd = false;

		for (std::size_t line = 0; auto* label : textAreaLabels)
		{
			const std::size_t labelStrLen = std::string_view{ label->getString() }.length() - 1;
			const auto& highlight = m_fields->m_highlights[line];

			highlight->clear();

			if (hasHighlightedStart && hasHighlightedEnd)
				continue; // dont break in case other highlights are visible

			if (!hasHighlightedStart)
			{
				if (targetFrom <= labelStrLen)
				{
					const auto& textLabelInfo = getTextLabelInfoFromPos(
						m_fields->m_highlighted.isHighlighting()
							? m_fields->m_highlighted.getToPos<false>()
							: m_fields->m_pos
					);
					hasHighlightedStart = true;

					CharNodeInfo fromCharInfo = getCharNodePosInfoAtLine(targetFrom, line, true);
					CharNodeInfo toCharInfo;

					// in case we're highlighting from and to the next label/the same label
					if (targetTo > labelStrLen)
					{
						toCharInfo = getCharNodePosInfoAtLine(labelStrLen, line, false);

						if (targetTo == labelStrLen + 1)
							hasHighlightedEnd = true;
					}
					else
					{
						// if selection ends at the end of the current label,
						// use the right of last character. else set to left of target character
						if (textLabelInfo.numCharsFromLabelStart == std::string_view{ textLabelInfo.label->getString() }.length() - 1)
							toCharInfo = getCharNodePosInfoAtLine(
								textLabelInfo.numCharsFromLabelStart,
								textLabelInfo.line,
								false
							);
						else
							toCharInfo = getCharNodePosInfoAtLine(
								targetTo,
								line,
								true
							);

						hasHighlightedEnd = true;
					}

					float topY = fromCharInfo.position.y - 3.f
						+ fromCharInfo.sprite->getScaledContentHeight() / 2.f;
					float bottomY = toCharInfo.position.y - 3.f
						- toCharInfo.sprite->getScaledContentHeight() / 2.f;

					CCRect rect = createRectFromPoints(
						{ fromCharInfo.position.x, bottomY },
						{ toCharInfo.position.x, bottomY },
						{ toCharInfo.position.x, topY },
						{ fromCharInfo.position.x, topY }
					);
					// we can get away by just lying about the height
					rect.size.height = label->getScaledContentHeight();

					highlight->drawRect(rect, highlightColor, .0f, highlightColor);
				}
			}
			else if (!hasHighlightedEnd)
			{
				CharNodeInfo fromCharInfo = getCharNodePosInfoAtLine(0, line, true);
				CharNodeInfo toCharInfo;

				// again, check if we're highlighting to the current label or afterwards
				if (targetTo > labelStrLen)
					toCharInfo = getCharNodePosInfoAtLine(labelStrLen, line, to != -1);
				else
				{
					toCharInfo = getCharNodePosInfoAtLine(targetTo, line, to != -1);
					hasHighlightedEnd = true;
				}

				float topY = fromCharInfo.position.y - 3.f
					+ fromCharInfo.sprite->getScaledContentHeight() / 2.f;
				float bottomY = toCharInfo.position.y - 3.f
					- toCharInfo.sprite->getScaledContentHeight() / 2.f;

				CCRect rect = createRectFromPoints(
					{ fromCharInfo.position.x, bottomY },
					{ toCharInfo.position.x, bottomY },
					{ toCharInfo.position.x, topY },
					{ fromCharInfo.position.x, topY }
				);
				// we can get away by just lying about the height
				rect.size.height = label->getScaledContentHeight();

				highlight->drawRect(rect, highlightColor, .0f, highlightColor);
			}

			targetFrom -= labelStrLen + 1;
			targetTo -= labelStrLen + 1;
			line++;
		}
	}
}

/**
 * @brief Inserts `character` at `pos`
 * 
 * @param pos
 * @param character
 */
void BetterTextInputNode::insertCharAtPos(int pos, char character)
{
	if (
		!BI::geode::get<bool>("allow-any-character") &&
		std::string_view(this->m_allowedChars).find(character) == std::string_view::npos
	)
		return;

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
void BetterTextInputNode::insertStrAtPos(int pos, std::size_t len, const std::string& str)
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
void BetterTextInputNode::deletePos(int pos, bool isDel)
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
 * #: the position returned by this function, depending on isLeftAnchored
 * 
 * @param pos
 * @param isLeftAnchored
 * @return CharNode
 */
CharNodeInfo BetterTextInputNode::getCharNodePosInfo(std::size_t pos, bool isLeftAnchored)
{
	std::size_t cursorPos = pos == -1 ? m_fields->m_string.length() - 1 : pos;

	if (auto label = static_cast<CCLabelBMFontPlus*>(this->m_placeholderLabel))
	{
		auto charNode = static_cast<CCFontSprite*>(
			label->getChildren()->objectAtIndex(cursorPos)
		);

		CCPoint charPos{
			(isLeftAnchored
				? label->getLetterPosXLeft(charNode, this->m_fontValue2, this->m_isChatFont)
				: label->getLetterPosXRight(charNode, this->m_fontValue2, true)
			) - label->getScaledContentWidth() * label->getAnchorPoint().x,
			label->getPositionY()
		};

		// 23.5f is the average content height of a CCFontSprite
		if (m_fields->m_string[cursorPos] == ' ')
			charNode->setContentHeight(23.5f);

		return {
			charNode,
			label,
			charPos
		};
	}
	else
	{
		CCLabelBMFontPlus* targetLabel;

		for (auto* label : CCArrayExt<CCLabelBMFont*>(this->m_textArea->m_label->getChildren()))
		{
			std::size_t labelStringSize = std::string_view(label->getString()).length() - 1;

			if (labelStringSize >= cursorPos)
			{
				targetLabel = static_cast<CCLabelBMFontPlus*>(label);
				break;
			}

			cursorPos -= labelStringSize + 1;
		}

		auto charNode = static_cast<CCFontSprite*>(
			targetLabel->getChildren()->objectAtIndex(cursorPos)
		);

		CCPoint charPos{
			(isLeftAnchored
				? targetLabel->getLetterPosXLeft(charNode, this->m_fontValue2, this->m_isChatFont)
				: targetLabel->getLetterPosXRight(charNode, this->m_fontValue2, true)
			) - targetLabel->getScaledContentWidth() * targetLabel->getAnchorPoint().x,
			charNode->getPositionY()
		};

		if (m_fields->m_string[pos] == ' ')
		{
			charNode->setContentHeight(9.f);

			if (isLeftAnchored)
				charPos.x = charNode->getPositionX();
		}

		return {
			charNode,
			targetLabel,
			this->convertToNodeSpace(
				targetLabel->convertToWorldSpace(charPos)
			)
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
CharNodeInfo BetterTextInputNode::getCharNodePosInfoAtLine(std::size_t pos, std::size_t line, bool isLeftAnchored)
{
	auto targetLabel = static_cast<CCLabelBMFontPlus*>(
		this->m_textArea->m_label->getChildren()->objectAtIndex(line)
	);

	if (pos == -1)
		pos = std::string_view(targetLabel->getString()).length() - 1;

	auto charNode = static_cast<CCFontSprite*>(
		targetLabel->getChildren()->objectAtIndex(pos)
	);

	float labelYPos = this->convertToNodeSpace(
		this->m_textArea->m_label->convertToWorldSpace(targetLabel->getPosition())
	).y + (targetLabel->getScaledContentHeight() / 2.f);
	float charXPos = (
		isLeftAnchored
			? targetLabel->getLetterPosXLeft(charNode, this->m_fontValue2, this->m_isChatFont)
			: targetLabel->getLetterPosXRight(charNode, this->m_fontValue2, this->m_isChatFont)
		) - targetLabel->getScaledContentWidth() * targetLabel->getAnchorPoint().x;

	if (m_fields->m_string[pos] == ' ')
	{
		charNode->setContentHeight(9.f);

		if (isLeftAnchored)
			charXPos = charNode->getPositionX();
	}

	CCPoint charNodePos = this->convertToNodeSpace(
		targetLabel->convertToWorldSpace({ charXPos, .0f })
	);

	return {
		charNode,
		targetLabel,
		{ charNodePos.x, labelYPos }
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
InputNodeTextAreaInfo BetterTextInputNode::getTextLabelInfoFromPos(std::size_t pos)
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
void BetterTextInputNode::setAndUpdateString(const std::string& str)
{
	// the position is modified in the call to setString
	int prevPos = m_fields->m_pos;
	m_fields->m_string = str;

	if (!BI::geode::get<bool>("allow-any-character"))
		std::erase_if(
			m_fields->m_string,
			[&](char c) -> bool {
				return std::string_view(this->m_allowedChars).find(c) == std::string_view::npos;
			}
		);

	if (
		!BI::geode::get<bool>("bypass-length-check") &&
		this->m_maxLabelLength != 0 &&
		m_fields->m_string.length() >= this->m_maxLabelLength
	) {
		m_fields->m_string = m_fields->m_string.substr(0, this->m_maxLabelLength);
		m_fields->m_string.resize(this->m_maxLabelLength);

		// sorry...
		if (m_fields->m_pos >= m_fields->m_string.length())
			prevPos = -1;
	}

	CCTextInputNode::setString(str);

	m_fields->m_pos = prevPos;

	if (str.empty())
		onStringEmpty();
}

/**
 * @brief Since we hook updateBlinkLabelToChar and never call it with the supplied `pos` parameter,
 * the only way to set the cursor position is by setting the `m_pos` member manually
 * 
 * @param pos
 */
void BetterTextInputNode::updateBlinkLabelToCharForced(int pos)
{
	m_fields->m_use_update_blink_pos = true;

	if (pos >= m_fields->m_string.length())
		pos = -1;

	this->updateBlinkLabelToChar(pos);

	m_fields->m_use_update_blink_pos = false;
}

/**
 * @brief Hides all the highlight sprites and clears the `m_highlighted` member
 */
void BetterTextInputNode::clearHighlight()
{
	for (auto* highlight : m_fields->m_highlights)
		highlight->clear();

	m_fields->m_highlighted.reset();
}

/**
 * @brief Deselects an input node and resets its string to the placeholder
 */
void BetterTextInputNode::deselectInput()
{
	showTextOrPlaceholder(true);

	this->onClickTrackNode(false);
	this->m_cursor->setVisible(false);
}

/**
 * @brief Adds a highlight node to the parent CCTextInputNode
 * 
 * @return CCDrawNode*
 */
CCDrawNode* BetterTextInputNode::appendHighlightNode()
{
	auto highlight = CCDrawNode::create();

	// highlight->setBlendFunc({ GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA });
	highlight->setID(fmt::format("highlight-sprite-{}"_spr, m_fields->m_highlights.size() + 1));
	this->addChild(highlight, 10);

	m_fields->m_highlights.emplace_back(highlight);

	return highlight;
}

/**
 * @brief Removes the last highlight node from the parent
 */
void BetterTextInputNode::removeLastHighlightNode()
{
	m_fields->m_highlights.back()->clear();
	m_fields->m_highlights.back()->removeFromParent();
	m_fields->m_highlights.pop_back();
}

/**
 * @brief Gets and sets the next cursor postion
 * To be used when calling updateBlinkLabelToChar
 * 
 * @return int
 */
[[nodiscard]] int BetterTextInputNode::getAndSetNextPos()
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
[[nodiscard]] int BetterTextInputNode::getAndSetPreviousPos()
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
