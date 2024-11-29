#include <string>
#include <string_view>

#ifdef GEODE_IS_WINDOWS
#include <Geode/cocos/robtop/glfw/glfw3.h>

#include <WinUser.h> // virtual keys
#endif

#include <Geode/cocos/cocoa/CCGeometry.h>

namespace BI
{
	namespace utils
	{
		constexpr std::string_view SEPARATORS = " !@#%&*()-_{}[]\\/?.,:;\"";

		inline const std::string& insertCharAtIndex(std::string& str, int pos, char toInsert)
		{
			if (str.empty())
				pos = 0;
			else if (pos == -1)
				pos = str.size();

			std::string curStr{std::move(str)};

			if (curStr.capacity() < curStr.size() + 1)
			{
				curStr.reserve(curStr.size() * 2);
				curStr.resize(curStr.size());
			}

			str = std::move(curStr);

			str.insert(pos, 1, toInsert);

			return str;
		}

		inline const std::string& insertStrAtIndex(std::string& str, int pos, const std::string_view toInsert)
		{
			if (str.empty())
				pos = 0;
			else if (pos == -1)
				pos = str.size();

			std::string curStr{std::move(str)};

			if (curStr.capacity() < curStr.size() + toInsert.size())
			{
				str.reserve((curStr.size() + toInsert.size()) * 2);
				str.resize(curStr.size() + toInsert.size());
			}

			str = std::move(curStr);

			str.insert(pos, toInsert);

			return str;
		}


		inline int findNextSeparator(std::string_view str, int pos)
		{
			if (pos == -1)
				pos = str.size();

			str = str.substr(pos, str.size());

			std::size_t sepPos = str.find_first_of(SEPARATORS);

			// | is the pos
			// example|.com -> example.|com
			if (pos + sepPos == pos)
				return pos + 1;

			return sepPos == std::string_view::npos ? -1 : (pos + sepPos);
		}

		inline int findPreviousSeparator(std::string_view str, int pos)
		{
			if (pos == -1)
				pos = str.size();

			str = str.substr(0, pos);

			std::string reversedString{ str.rbegin(), str.rend() };

			std::size_t sepPos = reversedString.find_first_of(SEPARATORS);

			// | is the pos
			// example.|com -> example|.com
			if (str.size() - sepPos == pos)
				return pos - 1;

			return sepPos == std::string::npos ? 0 : (str.size() - sepPos);
		}
	}

	namespace cocos
	{
		inline cocos2d::CCPoint getMousePosition()
		{
#ifdef GEODE_IS_WINDOWS
			auto* director = cocos2d::CCDirector::sharedDirector();
			auto* gl = director->getOpenGLView();
			auto winSize = director->getWinSize();
			auto frameSize = gl->getFrameSize();
			auto mouse = gl->getMousePosition() / frameSize;

			return cocos2d::CCPoint{ mouse.x, 1.f - mouse.y } * winSize;
#elif defined(GEODE_IS_MACOS)
			return geode::cocos::getMousePos();
#endif
		}

		inline bool isPositionInNode(cocos2d::CCNode* node, const cocos2d::CCPoint& pos)
		{
			auto worldPos = node->getParent()->convertToWorldSpace(node->getPosition());
			auto size = node->getScaledContentSize();

			auto rect = cocos2d::CCRect {
				worldPos.x - size.width / 2,
				worldPos.y - size.height / 2,
				size.width,
				size.height
			};

			return rect.containsPoint(pos);
		}
	}

	enum class PlatformKey
	{
		LEFT_CONTROL,
		LEFT_SHIFT
	};
	namespace platform
	{
#ifdef GEODE_IS_WINDOWS
		inline bool keyDown(PlatformKey key)
		{
			switch (key)
			{
				case BI::PlatformKey::LEFT_CONTROL:
					return GetKeyState(VK_CONTROL) & 0x8000;
				case BI::PlatformKey::LEFT_SHIFT:
					return GetKeyState(VK_SHIFT) & 0x8000;
			}

			return false;
		}
#endif
	}

	namespace geode
	{
		template<typename T>
		T get(std::string_view value)
		{
			return ::geode::Mod::get()->getSettingValue<T>(value);
		}
	}
}
