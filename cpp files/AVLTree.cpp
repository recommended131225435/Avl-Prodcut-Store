#include "../h files/AVLTree.h"
#include <algorithm>
#include <stdexcept>


// Constructor / Destructor
AVLTree::AVLTree() : root_(nullptr), size_(0) {}

AVLTree::~AVLTree() { destroyTree(root_); }

void AVLTree::destroyTree(AVLNode* node) {
    if (!node) return;
    destroyTree(node->left);
    destroyTree(node->right);
    delete node;
}

// Height & Balance helpers
int AVLTree::height(AVLNode* n) const 
{

    if (n != nullptr) 
    {
        return n->height;
    } 
    else 
    {
        return 0;
    }
}

int AVLTree::balanceFactor(AVLNode* n) const 
{
    
    if (n == nullptr) 
    {
        return 0;
    }

    int leftHeight = height(n->left);
    int rightHeight = height(n->right);

    return leftHeight - rightHeight;
}

void AVLTree::updateHeight(AVLNode* n) 
{
    if (n == nullptr) 
    {
        return;
    }
    int leftHeight  = height(n->left);
    int rightHeight = height(n->right);

    int tallernode = std::max(leftHeight, rightHeight);

    n->height = 1 + tallernode;
}





AVLNode* AVLTree::rotateRight(AVLNode* y) {
    AVLNode* x  = y->left;
    AVLNode* T2 = x->right;

    x->right = y;
    y->left  = T2;

    updateHeight(y);   // y is now lower → update first
    updateHeight(x);
    return x;          // x is the new sub-root
}

AVLNode* AVLTree::rotateLeft(AVLNode* x) {
    AVLNode* y  = x->right;
    AVLNode* T2 = y->left;

    y->left  = x;
    x->right = T2;

    updateHeight(x);
    updateHeight(y);
    return y;
}

// balance()
// Called on every node on the way back up after insert / remove.
// 4 imbalances that needs teh fix

AVLNode* AVLTree::balance(AVLNode* n) {
    updateHeight(n);
    int bf = balanceFactor(n);

    // heavy on the left side 
    if (bf > 1)
    {
        if (balanceFactor(n->left) < 0)       // LR , double rotation
            n->left = rotateLeft(n->left);
        return rotateRight(n);                 // LL, single rotation
    }

    // heavy on the right side
    if (bf < -1)
    {
        if (balanceFactor(n->right) > 0)       // RL, double rotation
            n->right = rotateRight(n->right);
        return rotateLeft(n);                  // RR, single rotation
    }

    return n;  // already balanced
}


// Public insert

void AVLTree::insert(double key, Product* product)
{
    root_ = insert(root_, key, product);
    ++size_;
}

AVLNode* AVLTree::insert(AVLNode* node, double key, Product* product)
{
    if (!node)
    {
        return new AVLNode(key, product);
    }
    // Duplicate keys are allowed: send them to the right subtree
    if(key < node->key)
    {
        node->left  = insert(node->left,  key, product);
    }
    else{
        node->right = insert(node->right, key, product);
    }
    return balance(node);
}


// Public remove
// We must match BOTH key and pointer because keys can be duplicated.

void AVLTree::remove(double key, Product* product)
{
    root_ = remove(root_, key, product);
    --size_;
}

AVLNode* AVLTree::minNode(AVLNode* node) const
{
    while(node->left)
    {
         node = node->left;
    }
    return node;
}

AVLNode* AVLTree::remove(AVLNode* node, double key, Product* product) 
{

    if (node == nullptr) 
    {
        return nullptr;
    }

    
    if (key < node->key) 
    {
        
        node->left = remove(node->left, key, product);
    } 
    else if (key > node->key) 
    {
        
        node->right = remove(node->right, key, product);
    } 
    else 
    {
        // keys match but we have to check pointer as well
        if (node->data == product) 
        {

            //1) no children
            if (node->left == nullptr && node->right == nullptr) 
            {
                delete node;
                return nullptr;
            }

            // 2)only right child
            if (node->left == nullptr) 
            {
                AVLNode* temp = node->right;
                delete node;
                return temp;
            }

            // 3)only left child
            if (node->right == nullptr) 
            {
                AVLNode* temp = node->left;
                delete node;
                return temp;
            }

            // 4) 2 children
            //we find the inorder successor
            AVLNode* successor = minNode(node->right);

            node->key  = successor->key;
            node->data = successor->data;

            // delete the successor from right subtree
            node->right = remove(node->right, successor->key, successor->data);
        } 
        else 
        {
            // same key but different product duplicate keys
            // search both subtrees
            node->left  = remove(node->left,  key, product);
            node->right = remove(node->right, key, product);
        }
    }

    // STEP 2: rebalance on the way back up
    return balance(node);
}


// Range Query  O(log n + k)
// Prune: if node->key < low, the entire left subtree is also < low → skip it.
//        if node->key > high, the entire right subtree is also > high → skip it.

// public
void AVLTree::rangeQuery(double low, double high, std::vector<Product*>& result) const 
{
    rangeQuery(root_, low, high, result);
}

// private
void AVLTree::rangeQuery(AVLNode* node, double low, double high,
                          std::vector<Product*>& result) const 
{
    if (node == nullptr) 
    {
        return;
    }

    if (node->key >= low) 
    {
        rangeQuery(node->left, low, high, result);
    }

    // the nodes here would be in range so we add em
    if (node->key >= low && node->key <= high) 
    {
        result.push_back(node->data);
    }


    if (node->key <= high) 
    {
        rangeQuery(node->right, low, high, result);
    }
}




void AVLTree::topK(int k, std::vector<Product*>& result) const
{
    result.reserve(k);
    topKDescending(root_, k, result);
}


void AVLTree::topKDescending(AVLNode* node, int k, std::vector<Product*>& result) const 
{
    
    if (node == nullptr) 
    {
        return;
    }

    // stop if we already have k results
    if ((int)result.size() >= k) 
    {
        return;
    }

    // go RIGHT first (largest values are on the right)
    topKDescending(node->right, k, result);

    // add this node if we still need more
    if ((int)result.size() < k) 
    {
        result.push_back(node->data);
    }

    // go LEFT last (smaller values)
    topKDescending(node->left, k, result);
}

int AVLTree::size() const { return size_; }