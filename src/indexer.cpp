#include "indexer.h"
#include "app.h"
#include "editor.h"
#include "search.h"
#include "highlighter.h"
#include "util.h"

#include <algorithm>
#include <unistd.h>

#include <algorithm>
#include <cctype>
#include <string>

#define INDEXED_LINE_LENGTH_LIMIT 200

indexer_t::indexer_t()
	: threadId(0)
	, hasInvalidBlocks(false)
{
	memset(&indexingRequests, 0, sizeof(size_t) * INDEX_REQUEST_SIZE);
	run();
}

indexer_t::~indexer_t()
{
	cancel();
}

void indexer_t::addEntry(block_ptr block, std::string prefix)
{
	// need mutex?
	std::transform(prefix.begin(), prefix.end(), prefix.begin(), [](unsigned char c) { return std::tolower(c); });

	block_list& blocks = indexMap[prefix];
	if (std::find(blocks.begin(), blocks.end(), block) == blocks.end()) {
        blocks.push_back(block);
        log("add %s", prefix.c_str());
    }
}

void indexer_t::updateBlock(block_ptr block)
{
	// request indexing service
	if (!block->data) {
		return;
	}

	for(int i=0; i<INDEX_REQUEST_SIZE; i++) {
		if (indexingRequests[i] == 0) {
			indexingRequests[i] = block->lineNumber;
			break;
		}
	}
}

void indexer_t::indexBlock(block_ptr block)
{
	if (!block->data) {
		return;
	}

	std::string text = block->text().substr(0, INDEXED_LINE_LENGTH_LIMIT);

	if (text.length() < 4) {
		return;
	}

	std::vector<search_result_t> result = search_t::instance()->findWords(text);

    for (auto r : result) {
    	if (r.text.length()<3) continue;

    	span_info_t span = spanAtBlock(block->data.get(), r.begin);
    	if (span.state == BLOCK_STATE_COMMENT) {
    		continue;
    	}

    	// log("%s %d", r.text.c_str(), span.colorIndex);

    	addEntry(block, r.text.substr(0, 3));
    }
}

void indexer_t::clear()
{}

std::vector<std::string> indexer_t::findWords(std::string prefix)
{
	std::vector<std::string> res;
    if (prefix.length() < 3) {
        return res;
    }	

    std::map<std::string, int> scoredMap;
	std::transform(prefix.begin(), prefix.end(), prefix.begin(), [](unsigned char c) { return std::tolower(c); });

	block_list& blocks = indexMap[prefix.substr(0, 3)];
	for(auto b : blocks) {
		if (!b->isValid()) {
			hasInvalidBlocks = true;
			continue;
		}
		std::vector<search_result_t> result = search_t::instance()->findWords(b->text());
		for(auto r : result) {
			std::string rprefix = r.text.substr(0, prefix.length());
			std::transform(rprefix.begin(), rprefix.end(), rprefix.begin(), [](unsigned char c) { return std::tolower(c); });
			if (rprefix == prefix) {
				int score = scoredMap[r.text];
				// log("?%s %s %s %d", rprefix.c_str(), prefix.c_str(), r.text.c_str(), score);
				if (score == 0) {
					scoredMap[r.text] = 1;
				}
				// res.push_back(r.text);
			}
		}
	}

	for(std::map<std::string,int>::iterator it = scoredMap.begin(); it != scoredMap.end(); ++it) {
		res.push_back(it->first);
	}

    return res;
}


void* indexerThread(void* arg)
{
    indexer_t* indexer = (indexer_t*)arg;
    editor_t* editor = indexer->editor;

    while (true) {
    	for(int i=0; i<INDEX_REQUEST_SIZE; i++) {
			if (indexer->indexingRequests[i] != 0) {
				block_ptr block = editor->document.blockAtLine(indexer->indexingRequests[i]);
				indexer->indexBlock(block);
				log("indexing %d", indexer->indexingRequests[i]);
				indexer->indexingRequests[i] = 0;
				usleep(100000);
			}
		}

		usleep(500000);
    }

    return NULL;
}

void indexer_t::run()
{
    pthread_create(&threadId, NULL, &indexerThread, this);
}

void indexer_t::cancel()
{
    if (threadId) {
        pthread_cancel(threadId);
        threadId = 0;
    }
}