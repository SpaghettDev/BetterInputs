#pragma once

#include <string_view>
#include <utility>

struct HighlightedString
{
	constexpr HighlightedString(std::string_view str, int from = -2, int to = -2)
		: m_from(from == -1 ? str.size() : from),
			m_to(to == -1 ? str.size() : to),
			m_og_str(str)
	{
		update_string_substr();
	}

	constexpr HighlightedString()
		: m_from(-2), m_to(-2), m_og_str(""), str("")
	{}

	constexpr void reset()
	{
		m_from = -2;
		m_to = -2;
		m_og_str = "";
		str = "";
	}

	constexpr void updateStr(std::string_view s)
	{
		m_og_str = s;

		if (m_from != -2 && m_to != -2)
			update_string_substr();
	}

	constexpr void update(std::string_view str, std::pair<std::size_t, std::size_t> pos)
	{
		m_from = pos.first == -1 ? str.size() : pos.first;
		m_to = pos.second == -1 ? str.size() : pos.second;

		m_og_str = str;

		update_string_substr();
	}

	inline constexpr bool isHighlighting() const { return !str.empty(); }

	inline constexpr int getFromPos() const { return m_from == m_og_str.size() ? -1 : m_from; }
	inline constexpr int getToPos() const { return m_to == m_og_str.size() ? -1 : m_to; }
	inline constexpr std::size_t getLength() const { return str.size(); }

public:
	std::string_view str;

private:
	std::string_view m_og_str;
	int m_from = 0;
	int m_to = -1;

	constexpr void update_string_substr()
	{
		str = m_og_str.substr(m_from, m_to - m_from);
	}
};
