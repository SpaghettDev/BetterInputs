#include <string>
#include <string_view>

#include <Geode/platform/windows.hpp>

#include <Geode/cocos/cocoa/CCGeometry.h>

namespace BI
{
	namespace utils
	{
		constexpr std::string_view SEPARATORS = " !@#%&*()-_{}[]\\/?.,:;\"";

		inline std::string insertCharAtIndex(const std::string& str, int pos, char toInsert)
		{
			std::string newStr;

			if (str.empty())
				pos = 0;
			else if (pos == -1)
				pos = str.size();

			newStr.reserve(str.size() + 1);
			newStr.resize(str.size() + 1);

			newStr = str;

			newStr.insert(pos, 1, toInsert);

			return newStr;
		}

		inline std::string insertStrAtIndex(const std::string& str, int pos, const std::string_view toInsert)
		{
			std::string newStr;

			if (str.empty())
				pos = 0;
			else if (pos == -1)
				pos = str.size();

			newStr.reserve(str.size() + toInsert.size());
			newStr.resize(str.size() + toInsert.size());

			newStr = str;

			newStr.insert(pos, toInsert);

			return newStr;
		}

		inline int findNextSeparator(std::string_view str, int pos)
		{
			if (pos == -1)
				pos = str.size();

			str = str.substr(pos, str.size());

			std::size_t sepPos = str.find_first_of(SEPARATORS);

			return sepPos == std::string_view::npos ? -1 : (pos + sepPos + 1);
		}

		inline int findPreviousSeparator(std::string_view str, int pos)
		{
			if (pos == -1)
				pos = str.size();

			str = str.substr(0, pos);

			std::string reversedString{ str.rbegin(), str.rend() };

			std::size_t sepPos = reversedString.find_first_of(SEPARATORS);

			return sepPos == std::string::npos ? 0 : (str.size() - sepPos - 1);
		}
	}

	namespace cocos
	{
		inline cocos2d::CCPoint getMousePosition()
		{
			auto* director = cocos2d::CCDirector::sharedDirector();
			auto* gl = director->getOpenGLView();
			auto winSize = director->getWinSize();
			auto frameSize = gl->getFrameSize();
			auto mouse = gl->getMousePosition() / frameSize;

			return cocos2d::CCPoint{ mouse.x, 1.f - mouse.y } * winSize;
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

	namespace windows
	{
		inline bool keyDown(unsigned int vk)
		{
			return GetKeyState(vk) & 0x8000;
		}
	}

	namespace mods
	{
		inline bool isVanillaInput()
		{
			std::uintptr_t readMemory;

			std::memcpy(&readMemory, reinterpret_cast<void*>(geode::base::get() + 0x2F974), 1);

			return readMemory == 0x0475;
		}
	}

	namespace geode
	{
		template<typename T>
		T get(std::string_view value)
		{
			return ::geode::Mod::get()->getSettingValue<bool>(value);
		}
	}
}
