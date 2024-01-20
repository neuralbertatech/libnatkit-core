#include <regex>
#include <vector>

#include <libnatkit-core.hpp>

namespace nat::core {

// A RawStream is a Stream that has not been read from yet, so any meta information has
// not been derived yet
	std::optional<std::unique_ptr<RawStream>> RawStream::create(const std::vector<BasicTopicInformation>& topics) {
		if (topics.size() == 0) {
			return {};
		}
		const auto id = topics[0].id;
		for (const auto& topic : topics) {
			if (topic.id != id) {
				return {};
			}
		}
		return std::unique_ptr<RawStream>(new RawStream(id, topics));
	}

	std::optional<std::unique_ptr<RawStream>> RawStream::create(std::vector<std::unique_ptr<BasicTopicInformation>>&& topics) {
	  if (topics.size() == 0) {
			return {};
		}
		const auto id = topics[0]->id;
		for (const auto& topic : topics) {
			if (topic->id != id) {
				return {};
			}
		}
		return std::unique_ptr<RawStream>(new RawStream(id, std::move(topics)));

	}

	std::string RawStream::toPrettyString() const {
            std::string string = "RawStream: [";
	    for (int i = 0; i < std::ssize(topics); ++i) {
		string.append("\n    ");
		string.append(topics[i]->toString());
		if (i+1 < std::ssize(topics)) {
		  string.append(",");
		} else {
			string.append("\n]");
		}
	    }

	    return string;
	}

	std::string RawStream::toString() const {
		return std::regex_replace(toPrettyString(), std::regex("\n *"), " ");
	}

	bool RawStream::addTopic(const BasicTopicInformation& topic) {
		if (topic.id != id) {
			return false;
		} else {
			topics.emplace_back(std::make_unique<BasicTopicInformation>(topic));
			return true;
		}
	}

	uint64_t RawStream::getId() const {
		return id;
	}

}
