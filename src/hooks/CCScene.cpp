#include <Geode/modify/CCScene.hpp>

#include "../BetterTextInputNode.hpp"

// fix layers appearing above selected CCTextInputNodes making ESC key deselect the
// input node instead of closing the alert
struct AlertLayerFix : geode::Modify<AlertLayerFix, cocos2d::CCScene>
{
	struct Fields
	{
		std::size_t m_previous_scene_children_count = 0;
		cocos2d::CCLayer* m_outermost_input_parent;
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

		if (!g_selectedInput)
		{
			m_fields->m_outermost_input_parent = nullptr;
			m_fields->m_previous_scene_children_count = currentChildrenCount;
			return;
		}

		if (
			currentChildrenCount == 1 ||
			currentChildrenCount == m_fields->m_previous_scene_children_count
		)
			return;

		if (!m_fields->m_outermost_input_parent)
		{
			cocos2d::CCLayer* outermostInputParent = static_cast<cocos2d::CCLayer*>(g_selectedInput->getParent());
			while (outermostInputParent->getParent() != this)
				outermostInputParent = static_cast<cocos2d::CCLayer*>(outermostInputParent->getParent());

			m_fields->m_outermost_input_parent = outermostInputParent;
		}

		auto* lastLayer = static_cast<cocos2d::CCLayer*>(this->getChildren()->lastObject());
		const int lastLayerTouchPrio = lastLayer->getTouchPriority();

		if (
			auto handler = cocos2d::CCTouchDispatcher::get()->findHandler(static_cast<cocos2d::CCTouchDelegate*>(g_selectedInput));
			handler &&
			lastLayerTouchPrio != 0 &&
			lastLayerTouchPrio < handler->getPriority()
		)
			g_selectedInput->deselectInput();
		else if (lastLayer->getZOrder() > m_fields->m_outermost_input_parent->getZOrder())
			g_selectedInput->deselectInput();

		m_fields->m_previous_scene_children_count = currentChildrenCount;
	}
};
