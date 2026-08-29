#include <bits/stdc++.h>

struct Node {
    std::string s;
    Node* left;
    Node* right;


    Node(): left(nullptr), right(nullptr) {}
    Node(std::string str): s(str), left(nullptr), right(nullptr) {}
    Node(std::string str, Node* l, Node* r) : s(str), left(l), right(r) {}

    ~Node() {
        delete left;
        delete right;
        s = "";
    }

    bool isNodeValid() {
        if (s == "~") {
            if (!right) return false;
        }
        if (s != "~" && s != "&" && s != "|" && s != "^" && s != ">") {
            if (!right || !left) return false;
        }
        return true;
    }
};

class Formula {
private:
    Node* root;

    bool isOp(char x) {
        if (x == '~' || x == '&' || x == '^' || x == '|' || x == '>') return true;
        return false;
    }
    
    void printParseTree(Node* root) {
        if (!root) {
            std::cout << "";
            return;
        }

        printParseTree(root->left);
        std::cout << " : " << root->s << " : ";

        printParseTree(root->right);
    }
    
public:
    Formula(): root(nullptr) {}
    Formula(Node* root): root(root) {}

    Formula(std::string s) {
        root = buildParseTree(s);
    }

    Node* buildParseTree(std::string &s) {
        int n = s.size();

        if (s[0] == '~') {
            std::string str = s.substr(2, n - 3);
            Node* l = buildParseTree(str);

            return new Node("~", l, nullptr);
        }


        if (s[0] == '(') {
            bool noOp = true;
            for (char x : s) {
                if (isOp(x)) noOp = false;
            }

            if (noOp) {
                return new Node(s);
            }

            int ct = 1, idx = -1;
            for (int i = 1; i < n; i++) {
                if (s[i] == '(') ct++;
                else if (s[i] == ')') ct--;

                if (ct == 0) {
                    idx = i + 1;
                    break;
                }
            }

            std::string leftStr = s.substr(1, idx - 2);
            std::string rightStr = s.substr(idx + 2, n - idx - 3);
            Node* l = buildParseTree(leftStr);
            Node* r = buildParseTree(rightStr);

            std::string op(1, s[idx]);
            return new Node(op, l, r);
        }
        
        return new Node(s);
    }

    void printTree() {
        printParseTree(root);
        std::cout << '\n';
    }

    int evaluate() {

    }
};

int main() {
    std::string s;
    std::getline(std::cin, s);

    s.erase(std::remove(s.begin(), s.end(), ' '), s.end());
    Formula f = Formula(s);
    f.printTree();
}