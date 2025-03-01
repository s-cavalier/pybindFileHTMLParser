#include "HTMLFile.h"
#include <iostream>
#include <list>
#include <functional>
using namespace std;

// chatgpt validate function lol
bool validate(const std::string& str) {
    auto start = str.begin();
    while (start != str.end() && std::isspace(*start)) {
        ++start;
    }

    auto end = str.end();
    do {
        --end;
    } while (start <= end && std::isspace(*end));

    if (start > end) {
        return false;
    }

    std::string trimmed(start, end + 1);
    char* endPtr = nullptr;
    errno = 0; 
    std::strtod(trimmed.c_str(), &endPtr);
    if (errno != 0 || endPtr != trimmed.c_str() + trimmed.size()) {
        return false;
    }

    return true;
}

// BFS Search function
// Postorder might be more useful since leaf nodes are usually the ones with text we want
// but preorder is easier to write non-recursively
const html::ComponentNode* BFSsearch(const std::function<bool(const html::ComponentNode*)>& exit_if, const html::ComponentNode* root) {
    list<const html::ComponentNode*> queue;
    queue.push_back(root);

    while (!queue.empty()) {
        auto top = queue.front();
        queue.pop_front();

        if (exit_if(top)) return top;

        for (const auto &x : top->getChildren()) queue.push_back(x.get());
    }

    return nullptr;
}

int main(int argc, char** argv) {

    if (argc != 2) {
        return 1;
    }

    std::string player = argv[1];
    
    html::HTMLParser parser("parse.html");
    
    const html::ComponentNode* nikolajokic = BFSsearch([&player] (const html::ComponentNode* node) { return node->getText() == player; }, parser.getRoot());
    if (nikolajokic == nullptr) {
        cerr << "Couldn't find player." << endl;
        return 1;
    }

    const html::ComponentNode* betNode = BFSsearch([] (const html::ComponentNode* node) { return validate(node->getText()); }, nikolajokic->getParent());
    
    if (betNode == nullptr) {
        cerr << "Couldn't find the over-under line." << endl;
        return 1;
    }

    cout << "The given player's betting line has been placed at " << betNode->getText() << endl;
    cout << betNode->getRawText() << endl;

    return 0;
}