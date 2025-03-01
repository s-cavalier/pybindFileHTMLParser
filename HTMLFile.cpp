#include "HTMLFile.h"
#include <cctype>
#include <cstdio>
#include <forward_list>
#include <thread>
#include <iostream>
using namespace html;

// FPARSER

fparser::fparser(const std::string& file, const int& init_size) {
    _file = fopen(file.c_str(), "r");
    endflag = false;
    incr(init_size);
}
fparser::~fparser() {
    fclose(_file);
}
void fparser::shift(const int& shift_size) {
    incr(shift_size);
    if (endflag) return;
    for (int i = 0; i < shift_size; ++i) storage.pop_front();
}
void fparser::incr(const int& size) {
    if (size < 1) throw std::runtime_error("Attempted to increment fparser storage by <1");
    if (size == 1) {
        char c = fgetc(_file);
        if (c == -1) {
            endflag = true;
            return;
        }
        storage.push_back(c);
        return;
    }
    char* appendc = new char[size];
    fread(appendc, 1, size, _file);
    for (int i = 0; i < size; ++i) storage.push_back(appendc[i]);
    delete[] appendc;
}
void fparser::popleft() {
    if (storage.size() < 2) return;
    storage.pop_front();
}
void fparser::flatten() {
    char c = storage.back();
    storage.clear();
    storage.push_back(c);
}
bool fparser::atEOF() const {
    return endflag;
}
bool fparser::operator==(const std::string& other) const {
    if (storage.size() != other.size()) return false;
    for (int i = 0; i < storage.size(); ++i) {
        if (storage[i] != other[i]) return false;
    }
    return true;
}
bool fparser::operator!=(const std::string& other) const {
    return !(*this == other);
}
fparser::operator std::string() const {
    return std::string(storage.begin(), storage.end());
}
const std::deque<char>& fparser::getWindow() const {
    return storage;
}
void fparser::append_until(end_condition comp) {
    while (!(*comp)(storage.back())) storage.push_back(fgetc(_file));
}
const char& fparser::back() const {
    return storage.back();
}
bool fparser::compLast(const std::string& other) const {
    if (storage.size() < other.size()) return false;
    return std::string(storage.begin() + (storage.size() - other.size()), storage.end()) == other;
}

// END OF FPARSER

const bool& ComponentNode::isLocked() const {
    return ptr_lock;
}
std::vector<std::unique_ptr<ComponentNode>>& ComponentNode::accessChildren() {
    if (ptr_lock) throw AttemptedLockNodeAccess(raw);
    return children;
}
ComponentNode* &ComponentNode::accessParent() {
    if (ptr_lock) throw AttemptedLockNodeAccess(raw);
    return parent;
}
std::string& ComponentNode::accessRawText() {
    if (ptr_lock) throw AttemptedLockNodeAccess("penis");
    return raw;
}
bool& ComponentNode::accessIsVoid() {
    if (ptr_lock) throw AttemptedLockNodeAccess("penis");
    
    return void_elem;    
}
const std::vector<std::unique_ptr<ComponentNode>>& ComponentNode::getChildren() const {
    return children;
}
const bool& ComponentNode::isVoid() const {
    return void_elem;
}
const ComponentNode* ComponentNode::getParent() const {
    return parent;
}
const std::string& ComponentNode::getRawText() const {
    return raw;
}
const std::string& ComponentNode::getComponent() const {
    return component;
}
const std::string& ComponentNode::getText() const {
    return text;
}
const std::unordered_map<std::string, std::string>& ComponentNode::getAttributes() const {
    return attributes;
}
const std::string& ComponentNode::getId() const {
    return id;
}
const std::unordered_set<std::string>& ComponentNode::getClasses() const {
    return classes;
}
ComponentNode::ComponentNode() {
    ptr_lock = false;
    parent = nullptr;
    void_elem = false;
}

// We assume component node is a fully parsed text, i.e. it should follow the form
//<'element' attributes... >text</'element'>
//
//TODO: define a way to 'dynamically' generate the innertext s.t. the original rawtext is recreatable (as of previous, any inner node data is lose,
// i.e. <p>pe<strong>nis</strong><p>) loses the 'nis' information when trying to 'regen' the original text. Not sure if this is necessary, though.
void ComponentNode::lock() {
    // Holds current 'word', which is to say it is just some text utilized to parse the input string.
    ptr_lock = true;
    std::string word;
    word.reserve(100);
    // Manage opener tag
    auto it = raw.cbegin();
    ++it;

    while (std::isalpha(*it)) {
        word.push_back(*it);
        ++it;
    }
    component = word;
    word.clear();

    std::string att_value;
    att_value.reserve(100);
    bool inattr = false;
    // Manage attributes
    while (*it != '>') {
        if (inattr && *it != '"') {
            att_value.push_back(*it);
            ++it;
            continue;
        }
        if (inattr && *it == '"') {
            attributes[word] = att_value;
            att_value.clear();
            word.clear();
            inattr = false;
            ++it;
            continue;
        }
        if (std::isspace(*it)) {
            ++it;
            continue;
        }
        if (*it == '=') {
            ++it;
            ++it;
            inattr = true;
            continue;
        }
        word.push_back(*it);
        ++it;
    }

    // use the already-parsed attributes map to find classes & ids
    if (attributes.count("id")) id = attributes[id];
    if (attributes.count("class")) {
        std::string word;
        const std::string& classAttribute = attributes["class"];
        for (const auto &c : classAttribute) {
            if (std::isspace(c) && !word.empty()) {
                classes.insert(word);
                word.clear();
                continue;
            }
            word.push_back(c);
        }
        classes.insert(word);
    }

    if (void_elem) return;

    ++it;

    //Open tag is now closed; we can just enter all text directly into text until *it == '<' (close tag)
    while (*it != '<') {
        text.push_back(*it);
        ++it;
    }

    // We can ignore closing tag, literally no use for it other than to identify end
}

const int& HTMLParser::size() const {
    return _size;
}
bool HTMLParser::empty() const {
    return _size == 0;
}
const ComponentNode* HTMLParser::getRoot() const {
    return root.get();
}


HTMLParser::HTMLParser(const std::string& file) {
    fparser fp(file);
    end_condition isnt_alnum = [] (const char& c) { return !bool(std::isalnum(c)); };
    end_condition end_of_tag = [] (const char& c) { return c == '>'; };

    std::forward_list<ComponentNode*> stack;
    for (fp; !fp.atEOF(); fp.shift()) {
        
        // Encountered tag
        if (fp.back() == '<') {
            fp.incr();
            fp.append_until(isnt_alnum);
            
            // Is a closing tag, </...>
            if (fp.getWindow()[1] == '/') {
                fp.incr();
                fp.append_until(isnt_alnum);
                stack.front()->accessRawText() += fp;
                if (++stack.begin() == stack.end()) stack.front()->lock();
                else {
                    std::thread asyncLock(&ComponentNode::lock, stack.front());
                    asyncLock.detach();
                }
                stack.pop_front();
                fp.flatten();
                continue;
            }

            if (fp.getWindow()[1] == '!') {
                fp.incr();
                fp.append_until(end_of_tag);
                fp.flatten();
                continue;
            }

            // fp should look like <tag> or <tag , so we cutoff start and end and check if void elem
            std::string tag(++fp.getWindow().cbegin(), std::prev(fp.getWindow().cend()));
            if (void_elements.count(tag)) {
                fp.append_until(end_of_tag);
                std::unique_ptr<ComponentNode> newNode = std::make_unique<ComponentNode>();
                newNode->accessRawText() = fp;
                newNode->accessIsVoid() = true;
                newNode->accessParent() = stack.front();
                // we only care to wait on the lock to finish iff lock is the root since we need the root to access all other nodes, otherwise they can be done asynchronously
                if (++stack.begin() == stack.end()) newNode->lock();
                else {
                    std::thread asyncLock(&ComponentNode::lock, newNode.get());
                    asyncLock.detach();
                }
                stack.front()->accessChildren().push_back(std::move(newNode));
                fp.flatten();
                continue;
            }

            // Style and Script tag text have A LOT of bad characters that can cause us to throw errors
            // since they're garaunteed to have no children and don't have any useful information, we just skip them entirely
            if (tag == "style" || tag == "script") {
                fp.flatten();
                fp.incr();
                // fp should be of size two, shift until fp == "</"
                std::string kill("</");
                while (!fp.compLast(kill)) {
                    fp.incr();
                }
                fp.incr();
                fp.append_until(end_of_tag);
                fp.flatten();
                continue;
            }

            // is an opening tag, push to stack
            std::unique_ptr<ComponentNode> newNode = std::make_unique<ComponentNode>();
            newNode->accessRawText() += fp;
            fp.flatten();
            ComponentNode* push = newNode.get();
            if (stack.empty()) root = std::move(newNode);
            else {
                newNode->accessParent() = stack.front();
                stack.front()->accessChildren().push_back(std::move(newNode));
            }
            stack.push_front(push);
            continue;
        }

        if (stack.empty()) continue;
        if (fp.back() != ' ' && std::isspace(fp.back())) continue;

        stack.front()->accessRawText().push_back(fp.back());

    }
}