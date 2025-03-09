#pragma once

#include <numbers>

#include <Geode/modify/CCTextInputNode.hpp>
#include <Geode/cocos/draw_nodes/CCDrawNode.h>

#include "utils.hpp"

#include "types/CharNodeInfo.hpp"
#include "types/HighlightedString.hpp"
#include "types/InputNodeTextAreaInfo.hpp"

class BetterTextInputNode : public geode::Modify<BetterTextInputNode, CCTextInputNode>
{
public:
	struct Fields
	{
		// the actual string, because we basically rework everything about the input text insertion is broken :D
		// (getString() normally returns the last character or is empty, we hook CCTextFieldTTF::getString to fix this)
		std::string m_string = "";
		std::string m_placeholder_str = "";

		// current cursor position
		int m_pos = -1;

		HighlightedString m_highlighted;
		bool m_is_adding_to_highlight = false;
		std::vector<cocos2d::CCDrawNode*> m_highlights;

		bool m_is_blinking = false;
		float m_time_since_last_input = .0f;

		bool m_use_update_blink_pos = false;
	};

	enum ACTION_TAG : uint8_t
	{
		CURSOR_BLINK
	};


	bool init(float p0, float p1, char const* p2, char const* p3, int p4, char const* p5);

	virtual bool onTextFieldAttachWithIME(cocos2d::CCTextFieldTTF* tField) override;
	virtual bool onTextFieldDetachWithIME(cocos2d::CCTextFieldTTF* tField) override;

	void updateBlinkLabelToChar(int pos);

	void setString(gd::string str);


	void updateCursorBlink(float dt);
	void updateCursorPos(std::size_t pos);

	// key events

	void onRightArrowKey(bool isCtrl, bool isShift);
	void onLeftArrowKey(bool isCtrl, bool isShift);
	void onUpArrowKey(bool isShift);
	void onDownArrowKey(bool isShift);
	void onHomeKey(bool isShift);
	void onEndKey(bool isShift);
	void onDelete(bool isCtrl, bool isDel);
	void onCopy();
	void onPaste();
	void onCut();

	// other events

	void onStringEmpty();

	// getters and setters

	void useUpdateBlinkPos(bool toggle);
	void showTextOrPlaceholder(bool toggle);

	// TODO: maybe export these?
	int getCursorPos();
	const HighlightedString& getHighlighted();


	// rewritten input stuff, helpers and highlighting

	void highlightFromToPos(int from, int to);
	void insertCharAtPos(int pos, char character);
	void insertStrAtPos(int pos, std::size_t len, const std::string& str);
	void deletePos(int pos, bool isDel);

	CharNodeInfo getCharNodePosInfo(std::size_t pos, bool isLeftAnchored);
	CharNodeInfo getCharNodePosInfoAtLine(std::size_t pos, std::size_t line, bool isLeftAnchored);

	InputNodeTextAreaInfo getTextLabelInfoFromPos(std::size_t pos);

	std::size_t getClosestCharIdxToXPos(float posX);
	std::size_t getClosestCharIdxToXPos(float posX, cocos2d::CCLabelBMFont* targetLabel);

	void setAndUpdateString(const std::string& str);

	void updateBlinkLabelToCharForced(int pos);

	void clearHighlight(bool clearStr = true);

	void deselectInput();

	cocos2d::CCDrawNode* appendHighlightNode();
	void removeLastHighlightNode();

	[[nodiscard]] int getAndSetNextPos();
	[[nodiscard]] int getAndSetPreviousPos();


	inline void whileAddingToHighlight(std::function<void()>&& f)
	{
		m_fields->m_is_adding_to_highlight = true;
		f();
		m_fields->m_is_adding_to_highlight = false;
	}

	cocos2d::ccColor4F getHighlightColor()
	{
		cocos2d::ccColor4F col = cocos2d::ccc4FFromccc3B(
			BI::geode::get<cocos2d::ccColor3B>("highlight-color")
		);
		col.a = .5f;
		return col;
	}

	static constexpr float getHighlightOffset(float scale)
	{
		return 5.f * std::pow(scale, std::numbers::e_v<float>);
	}
};


// this works because only one input node can ever be selected
inline BetterTextInputNode* g_selectedInput = nullptr;
