#include <cctype>
#include <iostream>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#include "qute/iterator.h"
#include "qute/query_parser.h"

const std::vector<std::string> kMarkTwainQuotes = {
    "A man is never more truthful than when he acknowledges himself a liar.",
    "I don't give a damn for a man that can only spell a word one way.",
    "The human race has one really effective weapon, and that is laughter.",
    "Loyalty to petrified opinion never yet broke a chain or freed a human soul.",
};

using Index = std::unordered_map<std::string, std::vector<qute::DocId>>;

Index buildIndex() {
  Index index;
  for (qute::DocId id = 0; id < kMarkTwainQuotes.size(); ++id) {
    const auto& quote = kMarkTwainQuotes[id];
    std::unordered_set<std::string> words;
    for (size_t b = 0; b < quote.size(); ++b) {
      size_t e = b;
      while (e < quote.size() && isalpha(quote[e])) {
        ++e;
      }
      if (b < e) {
        words.insert(quote.substr(b, e - b));
      }
      b = e;
    }
    for (const auto& word : words) {
      index[word].push_back(id);
    }
  }
  return index;
}

const auto kIndex = buildIndex();

class IteratorFactory : public qute::QueryParser::IteratorFactory {
  qute::IteratorPtr getIteratorForTerm(const std::string& term) override {
    auto itr = kIndex.find(term);
    return itr == kIndex.end() ? qute::Iterator::getEmptyIterator()
                               : qute::Iterator::getVectorIterator(itr->second);
  }
};

int main() {
  IteratorFactory iteratorFactory;
  qute::QueryParser queryParser(iteratorFactory);
  // Search for quotes having
  //     "man" and "liar"
  //   or having "human" but without "weapon".
  const std::string query = R"(
    (or (and tag:man_liar man liar )
        (diff tag:human-weapon human weapon)
    )
  )";
  auto itr = queryParser.getIterator(query);
  std::cout << "Search results:" << std::endl;
  for (; itr->valid(); itr->next()) {
    std::cout << "  " << kMarkTwainQuotes[itr->value()];
    auto tags = itr->getTags();
    if (!tags.empty()) {
      std::cout << " (" << tags[0] << ")";
    }
    std::cout << std::endl;
  }
  // Output
  /* 
   * Search results:
   *   A man is never more truthful than when he acknowledges himself a liar. (man_liar)
   *   Loyalty to petrified opinion never yet broke a chain or freed a human soul. (human-weapon)
   * 
   */
  return 0;
}
