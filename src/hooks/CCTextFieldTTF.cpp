#include <Geode/modify/CCTextFieldTTF.hpp>

#include "BetterTextInputNode.hpp"

// CCTextInputNode::getString is inlined, all it does is call CCTextFieldTTF::getString
struct BetterCCTextFieldTTF : geode::Modify<BetterCCTextFieldTTF, cocos2d::CCTextFieldTTF>
{
	const char* getString()
	{
		if (g_selectedInput)
			return g_selectedInput->m_fields->m_string.c_str();

		return CCTextFieldTTF::getString();
	}
};
