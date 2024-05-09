#include "./WordIndex.hpp"
#include <algorithm>
#include <unordered_set>
#include <vector>

namespace searchserver {

WordIndex::WordIndex() : index_{}, num_words_(0) {}

size_t WordIndex::num_words() {
  return num_words_;
}

void WordIndex::record(const string& word, const string& doc_name) {
  if (!index_.contains(word)) {
    num_words_++;
  }
  index_[word][doc_name].rank += 1;
  index_[word][doc_name].doc_name = doc_name;
  doc_to_words_[doc_name].insert(word);
}

vector<Result> WordIndex::lookup_word(const string& word) {
  vector<Result> results;
  for (auto& keyValue : index_[word]) {
    results.push_back(keyValue.second);
  }
  sort(results.begin(), results.end());
  return results;
}

vector<Result> WordIndex::lookup_query(const vector<string>& query) {
  vector<Result> results;
  // for each document, check if contains each word
  // as checking increment count of for Results
  // if doesnt contain, then break
  for (auto& doc : doc_to_words_) {
    auto& set = doc.second;
    auto& doc_name = doc.first;
    int count = 0;
    bool contains = true;
    for (auto& word : query) {
      if (!set.contains(word)) {
        contains = false;
        break;
      }
      count += index_[word][doc_name].rank;
    }
    if (contains) {
      results.push_back(Result(doc_name, count));
    }
  }
  sort(results.begin(), results.end());
  return results;
}

}  // namespace searchserver
