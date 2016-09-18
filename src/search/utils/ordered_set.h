#ifndef UTILS_ORDERED_SET_H
#define UTILS_ORDERED_SET_H

#include <cassert>
#include <unordered_set>
#include <vector>

namespace utils {
/*
  Combine vector and unordered_set to store a set of elements, ordered
  by insertion time.
*/
template <typename T>
class OrderedSet {
    std::vector<T> ordered_items;
    std::unordered_set<T> unordered_items;

public:
    bool empty() const {
        assert(unordered_items.size() == ordered_items.size());
        return ordered_items.empty();
    }

    void clear() {
        ordered_items.clear();
        unordered_items.clear();
        assert(empty());
    }

    void add(const T &item) {
        auto result = unordered_items.insert(item);
        bool inserted = result.second;
        if (inserted) {
            ordered_items.push_back(item);
        }
        assert(unordered_items.size() == ordered_items.size());
    }

    std::vector<T> extract_ordered_items() {
        std::vector<T> items = std::move(ordered_items);
        std::unordered_set<T>().swap(unordered_items);
        assert(empty());
        return items;
    }

    std::unordered_set<T> extract_unordered_items() {
        std::vector<T>().swap(ordered_items);
        std::vector<T> items = std::move(unordered_items);
        assert(empty());
        return items;
    }

    std::pair<std::vector<T>, std::unordered_set<T>> extract_collections() {
        auto collections = make_pair(
            std::move(ordered_items), std::move(unordered_items));
        assert(empty());
        return collections;
    }
};
}

#endif
