#pragma once

#include <Geode/modify/CCTextInputNode.hpp>

#include "types/CharNode.hpp"
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
		std::vector<cocos2d::extension::CCScale9Sprite*> m_highlights;

		bool m_use_update_blink_pos = false;
	};

	bool init(float p0, float p1, char const* p2, char const* p3, int p4, char const* p5);

	virtual bool onTextFieldAttachWithIME(cocos2d::CCTextFieldTTF* tField) override;
	virtual bool onTextFieldDetachWithIME(cocos2d::CCTextFieldTTF* tField) override;

	void updateBlinkLabelToChar(int pos);

	void setString(gd::string str);

	// bug fixes
	void textAreaCursorFix(std::size_t pos);

	// key events
	void onRightArrowKey(bool isCtrl, bool isShift);
	void onLeftArrowKey(bool isCtrl, bool isShift);
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

	CharNode getCharNodePosInfo(std::size_t pos, bool isLeftAnchored);
	CharNode getCharNodePosInfoAtLine(std::size_t pos, std::size_t line, bool isLeftAnchored);

	InputNodeTextAreaInfo getTextLabelInfoFromPos(std::size_t pos);

	void setAndUpdateString(const std::string& str);

	void updateBlinkLabelToCharForced(int pos);

	void clearHighlight();

	void deselectInput();

	cocos2d::extension::CCScale9Sprite* appendHighlightNode();
	void removeLastHighlightNode();

	[[nodiscard]] int getAndSetNextPos();
	[[nodiscard]] int getAndSetPreviousPos();
};


inline BetterTextInputNode* g_selectedInput = nullptr;
