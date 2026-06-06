#pragma once

#include "token.hpp"

#include <vector>
#include <string>
#include <unordered_map>

class Tree
{
public:
    struct Node
    {
        Node *parent;
        std::vector<Node *> child;

        std::string name;
        std::vector<std::string> arg;

        Node(std::string name, Node *parent);
        void print(int ofs) const;
        std::vector<Token> get_tokens(std::vector<Token> &tok) const;
        ~Node();
    };

private:
    Node *beg;
    Node *cur;

    int node_traversal(Node *cur);
    bool constant_folding_node(Node *cur, std::unordered_map<std::string, long long> &know, std::unordered_map<std::string, long long> &array_know);
    bool branch_cutting_node(Node *cur);
    bool identity_simplification_node(Node *cur);
    bool loop_unrolling_node(Node *cur, std::unordered_map<std::string, long long> &know);
    bool assignment_combining_node(Node *cur);
    bool array_access_lowering_node(Node *cur, const std::unordered_map<std::string, std::string> &array_types);
    void collect_variable_lifetimes_node(Node *cur, std::unordered_map<std::string, std::vector<std::string>> &type_by_name, std::unordered_map<std::string, Node *> &create_by_name, std::unordered_map<std::string, Node *> &last_use);
    void release_dead_variables_node();
    void varible_counter_node(Node *cur, std::unordered_map<std::string, int> &total_usage);
    bool varible_cleaner_node(Node *cur, std::unordered_map<std::string, int> &total_usage);
    bool neg_simplification_node(Node *cur, std::unordered_map<std::string, Node *> &last_use);

public:
    Tree();
    Node *get_beg() const;
    void append(const Token &tok);
    void traversal();
    bool constant_folding();
    bool branch_cutting();
    bool identity_simplification();
    bool loop_unrolling();
    bool assignment_combining();
    bool array_access_lowering();
    void release_dead_variables();
    bool varible_cleaner();
    bool neg_simplification();
    void print() const;
    std::vector<Token> get_token() const;
};
