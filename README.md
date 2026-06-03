# AVL-Based Product Store

A C++ project that simulates an e-commerce product catalog. It uses a custom multi-index architecture to handle different types of database queries efficiently without wasting memory.

## Core Architecture

Instead of copying product data everywhere, the system stores each product exactly once in a central `std::deque`. This keeps memory addresses stable as the database grows. 

The system then uses lightweight pointers across different data structures to handle specific queries:
* **Price & Rating Indexes:** Custom AVL Trees that allow for fast range queries by pruning irrelevant branches.
* **Popularity Index:** A Max-Heap to track the most popular items.
* **Identity Index:** A Hash Map for instant ID lookups.
* **Baseline Comparison:** A Sorted Vector, kept as a baseline to demonstrate why standard arrays struggle with frequent database updates.

## Performance

By using a reverse in-order traversal on the Rating AVL Tree, this system can extract the top products in O(k) time. Competing baseline structures, like hash maps or brute-force arrays, are forced to execute a full O(n log n) sorting operation to answer the exact same query.

## Tech Stack
* **Language:** C++ 
* **Environment:** Linux (Zorin OS)
* **Benchmarking:** Precision timing via `std::chrono`
* **Data Generation:** Deterministic seeding via `std::mt19937`

## How to Run

Compile the source code using standard GCC in your Linux terminal:

```bash
cd "cpp files"
g++ main.cpp AVLTree.cpp Product.cpp -o ProductStore
./ProductStore
