#pragma once
#include "Product.h"

// AVLNode 
// one ndoe in avl holds
// key: for query by price or rating
// data: pointer back to the single Product object
// height: for balancing the avltree
struct AVLNode {
    double   key;        // sort key (price OR rating) to keep it generic so we dont have to write the separate class for each query ( price or rating )
    Product* data;       // its the pointer to the actual product
    int      height;
    AVLNode* left;
    AVLNode* right;

    AVLNode(double key, Product* data)
        : key(key), data(data), height(1), left(nullptr), right(nullptr) {}
};