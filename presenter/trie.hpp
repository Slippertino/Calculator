#pragma once

#include <queue>
#include <vector>
#include <unordered_map>
#include <unordered_set>
#include <string>
#include <memory>
#include <iterator>

template<typename CharT>
class Trie {

	using string_t = std::basic_string<CharT, std::char_traits<CharT>>;

	struct Node;
	using NodePtr = std::unique_ptr<Node>;

	struct Node {
		std::unordered_map<CharT, NodePtr> transitions;
		bool word = false;
	};

public:
	Trie() : root_{ std::make_unique<Node>() }
	{ }

	template<
		std::forward_iterator ForwIt,
		typename = std::enable_if_t<std::is_same_v<string_t, typename std::iterator_traits<ForwIt>::value_type>>
	>
	Trie(ForwIt begin, ForwIt end) : Trie() {
		while (begin != end)
			add(*begin++);
	}

	Trie& add(const string_t& str) {
		auto node = root_.get();
		for (auto& ch : str) {
			if (node->transitions.find(ch) == node->transitions.end())
				node->transitions.insert({ ch, std::make_unique<Node>() });
			node = node->transitions[ch].get();
		}
		node->word = true;
		return *this;
	}

	std::pair<string_t, bool> find_nearest_far(const string_t& str) const {
		string_t res;
		auto node = root_.get();
		for (auto& ch : str) {
			if (node->transitions.find(ch) == node->transitions.end())
				return { res, false };
			node = node->transitions[ch].get();
			res += ch;
		}

		while (!node->word && node->transitions.size() == 1) {
			auto ch = (*node->transitions.begin()).first;
			res += ch;
			node = node->transitions[ch].get();
		}

		return { res, node->word };
	}

	~Trie() {
		std::queue<NodePtr> q;
		q.push(std::move(root_));
		while (!q.empty()) {
			auto cur = std::move(q.front());
			q.pop();

			for (auto& next : cur->transitions) 
				q.push(std::move(next.second));

			cur.reset();
		}
	}

private:
	NodePtr root_;
};