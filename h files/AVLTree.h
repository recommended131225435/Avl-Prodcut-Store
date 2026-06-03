#pragma once
#include "Node.h"
#include <vector>

 
// generic avl tree keyed on double
// supports duplicate keys (same price / same rating for different products)

class AVLTree {
public:
    AVLTree();
    ~AVLTree();

    void insert(double key, Product* product);
    void remove(double key, Product* product);
    void rangeQuery(double low, double high, std::vector<Product*>& result) const;// O (logn + k)
    void topK(int k, std::vector<Product*>& result) const;
    int size() const;

private:
    AVLNode* root_;
    int      size_;

     
    int      height(AVLNode* n) const;
    int      balanceFactor(AVLNode* n) const;
    void     updateHeight(AVLNode* n);

    //roattions
    AVLNode* rotateRight(AVLNode* y);   
    AVLNode* rotateLeft(AVLNode* x);    
    AVLNode* balance(AVLNode* n);       

    AVLNode* insert(AVLNode* node, double key, Product* product);
    AVLNode* remove(AVLNode* node, double key, Product* product);

    // Finds the in-order successor (smallest node in right subtree)
    AVLNode* minNode(AVLNode* node) const;

    void rangeQuery(AVLNode* node, double low, double high,
                    std::vector<Product*>& result) const;

    // Reverse in-order traversal to collect top-k efficiently
    void topKDescending(AVLNode* node, int k, std::vector<Product*>& result) const;

    void destroyTree(AVLNode* node);
};