#ifndef HTMLFILE_H
#define HTMLFILE_H
#include <vector>
#include <unordered_map>
#include <unordered_set>
#include <memory>
#include <string>
#include <deque>
#include <stdexcept>

typedef bool (*end_condition)(const char&);

namespace html {

    inline static const std::unordered_set<std::string> void_elements = {"area", "base", "br", "col", "embed", "hr", "img", "input", "link", "meta", "param", "source", "track", "wbr"};

    struct AttemptedLockNodeAccess : public std::runtime_error {
        AttemptedLockNodeAccess(const std::string& identifier) : std::runtime_error("Attempted to access locked node: " + identifier) { }
    };


    // To prefer the iterative solution over the recursive solution, there exists functions that allow for reference access to all inner data
    // that will be managed in the wrapper class HTMLParser. However, once the lock() function is called, any access functions will throw
    // AttemptedLockNodeAccess to prevent from writing to the file; we only want to read it.
    // Note that raw will only contain the open tag, innertext, and close tag to prevent from a lot of copying.
    class ComponentNode {
        // Pointers
        std::vector<std::unique_ptr<ComponentNode>> children;
        ComponentNode* parent;

        bool ptr_lock;
        bool void_elem;

        // Data
        std::string raw;
        // Below is undefined and should not be access until close() is called, as we don't have all info until we encounter the closing tag.
        std::string component;
        std::string text;
        std::unordered_map<std::string, std::string> attributes;
        std::string id;
        std::unordered_set<std::string> classes;

    public:
        ComponentNode();

        // Pointer access
        const bool& isLocked() const;
        std::vector<std::unique_ptr<ComponentNode>>& accessChildren();
        ComponentNode* &accessParent();
        std::string& accessRawText();
        bool& accessIsVoid();

        // ALL ComponentNodes should be locked once they are fully constructed; the specific branch upon which this node lies upon is closed.
        void lock();

        // Data
        const std::vector<std::unique_ptr<ComponentNode>>& getChildren() const;
        const ComponentNode* getParent() const;

        const bool& isVoid() const;
        const std::string& getRawText() const;
        const std::string& getComponent() const;
        const std::string& getText() const;
        const std::unordered_map<std::string, std::string>& getAttributes() const;
        const std::string& getId() const;
        const std::unordered_set<std::string>& getClasses() const;
    };

    // Parsing object for fast parsing and to avoid having to increment by exclusively one char
    // Should be considered a 'sliding window' over the HTML file
    // Must always have at least one item, 
    class fparser {
        std::deque<char> storage;
        FILE* _file;
        bool endflag;

    public:
        fparser(const std::string& file, const int& init_size = 1);
        ~fparser();

        // 'shift' the sliding window over one
        void shift(const int& shift_size = 1);
        // append without popping
        void incr(const int& inc_size = 1);
        // popping on left; can throw exception if bad access
        void popleft();
        // drop the entire window except for the storage.back()
        void flatten();
        // run until condition returns true or end of file
        void append_until(end_condition comp);
        
        bool atEOF() const;
        const std::deque<char>& getWindow() const;
        const char& back() const;

        // used for comparsion against strings
        bool operator==(const std::string& other) const;
        bool operator!=(const std::string& other) const;

        // compares from the end of the deque backwards
        bool compLast(const std::string &other) const;

        operator std::string() const;

    };

    class HTMLParser {
        std::unique_ptr<ComponentNode> root; // <html> component
        int _size;

    public:
        // Takes in path to HTML file to parse, converts to a graph of HTML components.
        HTMLParser(const std::string& file);

        const int& size() const;
        bool empty() const;
        const ComponentNode* getRoot() const;
    };
}

#endif