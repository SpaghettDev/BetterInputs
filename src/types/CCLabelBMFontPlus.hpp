#include <Geode/cocos/label_nodes/CCLabelBMFont.h>

class CCLabelBMFontPlus : public cocos2d::CCLabelBMFont
{
public:
	[[deprecated("Not Implemented")]] char* atlasNameFromFntFile(const char* fntFile)
	{
		return nullptr;
	}

	[[deprecated("Not Implemented")]] int kerningAmountForFirst(unsigned short first, unsigned short second)
	{
		return -1;
	}

	float getLetterPosXLeft(cocos2d::CCSprite* characterSprite, float p1, bool p2)
	{
		float width = characterSprite->getContentWidth();
		if (p2 && width <= .0f)
			width = p1 * 1.5f;

		const cocos2d::CCPoint& charPos = characterSprite->getPosition();
		return -(((width * this->m_fScaleX) * characterSprite->getAnchorPoint().x) - (charPos.x * this->m_fScaleX));
	}

	float getLetterPosXRight(cocos2d::CCSprite* characterSprite, float p1, bool p2)
	{
		float width = characterSprite->getContentWidth();
		if (p2 && width <= .0f)
			width = p1 * 1.5f;

		const cocos2d::CCPoint& charPos = characterSprite->getPosition();
		return ((width * this->m_fScaleX) * characterSprite->getAnchorPoint().x) + (charPos.x * this->m_fScaleX);
	}
};
