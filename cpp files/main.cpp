#include "../h files/Product.h"
#include "../h files/AVLTree.h"

#include <iostream>
#include <vector>
#include <deque>
#include <queue>
#include <algorithm>
#include <chrono>
#include <iomanip>
#include <random>
#include <string>
#include <sstream>
#include <unordered_map>


using Clock = std::chrono::high_resolution_clock;
using Ms    = std::chrono::duration<double, std::milli>;
using Ns    = std::chrono::duration<double, std::nano>;

static double elapsedMs(Clock::time_point t0)
{
    return Ms(Clock::now() - t0).count();
}

static double elapsedNs(Clock::time_point t0)
{
    return Ns(Clock::now() - t0).count();
}


using PopPair = std::pair<int, Product*>;
using MaxHeap = std::priority_queue<PopPair>;


class ProductStore
{

public:

    void insert(int id, const std::string& name,
                double price, double rating, int popularity)
    {
        products_.emplace_back(id, name, price, rating, popularity);

        Product* p = &products_.back();

        priceIndex_.insert(p->price,         p);
        ratingIndex_.insert(p->rating,       p);
        popularityHeap_.push({p->popularity, p});

        //for the list to sort itwself
        //lower bound does bin search to place the element in the right space
        auto pos = std::lower_bound(
            sortedByPrice_.begin(), sortedByPrice_.end(), p,
            [](Product* a, Product* b){ return a->price < b->price; });
        sortedByPrice_.insert(pos, p);

        hashMap_[p->id] = p;
    }


    // AVL update 
    void update(int id, double newPrice, double newRating)
    {
        Product* p = findById(id);

        if (!p)
        {
            std::cerr << "Product " << id << " not found.\n";
            return;
        }

        priceIndex_.remove(p->price,   p);
        ratingIndex_.remove(p->rating, p);

        p->price  = newPrice;
        p->rating = newRating;

        priceIndex_.insert(p->price,   p);
        ratingIndex_.insert(p->rating, p);

        rebuildHeap();

        // re-sort sorted vector since price changed
        std::sort(sortedByPrice_.begin(), sortedByPrice_.end(),
                  [](Product* a, Product* b){ return a->price < b->price; });
    }


    // AVL queries 
    std::vector<Product*> rangeByPrice(double low, double high) const
    {
        std::vector<Product*> res;
        priceIndex_.rangeQuery(low, high, res);
        return res;
    }

    std::vector<Product*> topKByRating(int k) const
    {
        std::vector<Product*> res;
        ratingIndex_.topK(k, res);
        return res;
    }

    std::vector<Product*> topKByPopularity(int k) const
    {
        std::vector<Product*> res;
        MaxHeap tmp = popularityHeap_;
        for (int i = 0; i < k && !tmp.empty(); ++i)
        {
            res.push_back(tmp.top().second);
            tmp.pop();
        }
        return res;
    }

    int count() const { return (int)products_.size(); }


    // Brute Force 
    std::vector<Product*> bruteForceRange(double low, double high) const
    {
        std::vector<Product*> res;
        for (auto& p : products_)
            if (p.price >= low && p.price <= high)
                res.push_back(const_cast<Product*>(&p));
        return res;
    }

    std::vector<Product*> bruteForceTopKRating(int k) const
    {
        std::vector<Product*> all;
        for (auto& p : products_)
            all.push_back(const_cast<Product*>(&p));
        std::sort(all.begin(), all.end(),
                  [](Product* a, Product* b){ return a->rating > b->rating; });
        if ((int)all.size() > k) all.resize(k);
        return all;
    }

    std::vector<Product*> bruteForceTopKPopularity(int k) const
    {
        std::vector<Product*> all;
        for (auto& p : products_)
            all.push_back(const_cast<Product*>(&p));
        std::sort(all.begin(), all.end(),
                  [](Product* a, Product* b){ return a->popularity > b->popularity; });
        if ((int)all.size() > k) all.resize(k);
        return all;
    }

    // brute force update = linear scan only, no index to manage
    void bruteForceUpdate(int id, double newPrice, double newRating)
    {
        for (auto& p : products_)
            if (p.id == id)
            {
                p.price  = newPrice;
                p.rating = newRating;
                break;
            }
    }


    // Sorted Vector + Binary Search 
    std::vector<Product*> sortedVecRange(double low, double high) const
    {
        std::vector<Product*> res;
        auto it = std::lower_bound(
            sortedByPrice_.begin(), sortedByPrice_.end(), low,
            [](Product* p, double val){ return p->price < val; });
        while (it != sortedByPrice_.end() && (*it)->price <= high)
        {
            res.push_back(*it);
            ++it;
        }
        return res;
    }


    std::vector<Product*> sortedVecTopKRating(int k) const
    {
        std::vector<Product*> all = sortedByPrice_;
        std::sort(all.begin(), all.end(),
                  [](Product* a, Product* b){ return a->rating > b->rating; });
        if ((int)all.size() > k) all.resize(k);
        return all;
    }

    std::vector<Product*> sortedVecTopKPopularity(int k) const
    {
        std::vector<Product*> all = sortedByPrice_;
        std::sort(all.begin(), all.end(),
                  [](Product* a, Product* b){ return a->popularity > b->popularity; });
        if ((int)all.size() > k) all.resize(k);
        return all;
    }

    
    //   step 1 linear scan to find and modify the product (O(n))
    //   step 2  full re-sort of sortedByPrice_ since price changed (O(n log n))

    void sortedVecUpdate(int id, double newPrice, double newRating)
    {
        // step 1: find and change
        for (auto& p : products_)
            if (p.id == id)
            {
                p.price  = newPrice;
                p.rating = newRating;
                break;
            }
        // step 2: re-sort (this was missisahi h ng before)
        std::sort(sortedByPrice_.begin(), sortedByPrice_.end(),
                  [](Product* a, Product* b){ return a->price < b->price; });
    }


    // Hash Map 
    //  hash map is keyed on product ID only.
    // Range and top-K have no ordering to exploit, so they fall back to a
    // full scan ,same asymptotic cost as brute force (O(n)).
    std::vector<Product*> hashMapRange(double low, double high) const
    {
        std::vector<Product*> res;
        for (auto& kv : hashMap_)
            if (kv.second->price >= low && kv.second->price <= high)
                res.push_back(kv.second);
        return res;
    }

    std::vector<Product*> hashMapTopKRating(int k) const
    {
        std::vector<Product*> all;
        for (auto& kv : hashMap_)
            all.push_back(kv.second);
        std::sort(all.begin(), all.end(),
                  [](Product* a, Product* b){ return a->rating > b->rating; });
        if ((int)all.size() > k) all.resize(k);
        return all;
    }

    std::vector<Product*> hashMapTopKPopularity(int k) const
    {
        std::vector<Product*> all;
        for (auto& kv : hashMap_)
            all.push_back(kv.second);
        std::sort(all.begin(), all.end(),
                  [](Product* a, Product* b){ return a->popularity > b->popularity; });
        if ((int)all.size() > k) all.resize(k);
        return all;
    }

    // hash map update: O(1) lookup then direct field write 
    void hashMapUpdate(int id, double newPrice, double newRating)
    {
        auto it = hashMap_.find(id);
        if (it != hashMap_.end())
        {
            it->second->price  = newPrice;
            it->second->rating = newRating;
        }
    }


    Product* findById(int id)
    {
        for (auto& p : products_)
            if (p.id == id) return &p;
        return nullptr;
    }


private:

    std::deque<Product>              products_;
    AVLTree                          priceIndex_;
    AVLTree                          ratingIndex_;
    MaxHeap                          popularityHeap_;
    std::vector<Product*>            sortedByPrice_;
    std::unordered_map<int,Product*> hashMap_;


    void rebuildHeap()
    {
        MaxHeap fresh;
        for (auto& p : products_)
            fresh.push({p.popularity, &p});
        popularityHeap_ = std::move(fresh);
    }

};


//  helpers

static void printSeparator(const std::string& title)
{
    std::cout << "\n==================================================\n"
              << "  " << title
              << "\n==================================================\n";
}

static void printComparison(const std::string& operation,
                             double avlTime,   const std::string& avlUnit,
                             double otherTime, const std::string& otherUnit,
                             const std::string& otherName,
                             const std::string& caveat = "")
{
    double avlMs   = (avlUnit   == "ns") ? avlTime   / 1e6 : avlTime;
    double otherMs = (otherUnit == "ns") ? otherTime / 1e6 : otherTime;
    double speedup = (avlMs > 0) ? otherMs / avlMs : 0;

    std::cout << "\n" << operation << "\n";
    std::cout << std::left
              << std::setw(25) << "  AVL Tree:"
              << std::fixed << std::setprecision(3)
              << avlTime << " " << avlUnit << "\n"
              << std::setw(25) << ("  " + otherName + ":")
              << otherTime << " " << otherUnit << "\n";

    if (speedup > 1.0)
        std::cout << "  AVL is " << std::setprecision(1) << speedup << "x faster\n";
    else if (avlMs > 0)
        std::cout << "  " << otherName << " is "
                  << std::setprecision(1) << (1.0 / speedup) << "x faster\n";

    if (!caveat.empty())
        std::cout << "  NOTE: " << caveat << "\n";
}


// helper: build a fresh ProductStore with the same seed — used so the
// comparison section always runs on a clean, unmodified dataset
static ProductStore buildStore(int N)
{
    std::mt19937 rng(42);
    std::uniform_real_distribution<double> priceDist(5.0,   5000.0);
    std::uniform_real_distribution<double> ratingDist(1.0,  5.0);
    std::uniform_int_distribution<int>     popDist(1,       1000000); 

    ProductStore s;
    for (int i = 1; i <= N; ++i)
    {
        std::ostringstream ss;
        ss << "Product_" << i;
        s.insert(i, ss.str(), priceDist(rng), ratingDist(rng), popDist(rng));
    }
    return s;
}


int main()
{
    //  user input 
    int N;
    std::cout << "How many products to generate? ";
    std::cin >> N;

    double rangeLow, rangeHigh;
    std::cout << "Enter price range for query (low high): ";
    std::cin >> rangeLow >> rangeHigh;

    int K;
    std::cout << "Enter K for top-K queries: ";
    std::cin >> K;

    int updateId;
    double updatePrice, updateRating;
    std::cout << "Enter product ID to update (1 to " << N << "): ";
    std::cin >> updateId;
    std::cout << "Enter new price and rating: ";
    std::cin >> updatePrice >> updateRating;

    int algoChoice;
    std::cout << "\nSelect algorithm to compare against AVL Tree:\n"
              << "  1. Brute Force Linear Scan\n"
              << "  2. Sorted Vector + Binary Search\n"
              << "  3. Hash Map\n"
              << "Choice (1-3): ";
    std::cin >> algoChoice;

    int opChoice;
    std::cout << "\nSelect operation to compare:\n"
              << "  1. Range Query\n"
              << "  2. Top-K by Rating\n"
              << "  3. Top-K by Popularity\n"
              << "  4. Update\n"
              << "Choice (1-4): ";
    std::cin >> opChoice;

    std::string algoName;
    if      (algoChoice == 1) algoName = "Brute Force";
    else if (algoChoice == 2) algoName = "Sorted Vector";
    else                      algoName = "Hash Map";

    // print honest limitation warnings up front
    if (algoChoice == 3 && opChoice != 4)
        std::cout << "\n  [!] Hash Map is keyed on product ID only.\n"
                  << "      Range and Top-K require a full scan (O(n)) —\n"
                  << "      same cost as Brute Force. Comparison shows that limitation.\n";

    if (algoChoice == 2 && (opChoice == 2 || opChoice == 3))
        std::cout << "\n  [!] Sorted Vector is sorted by price only.\n"
                  << "      Top-K by rating/popularity still needs a full re-sort,\n"
                  << "      so it offers no advantage over Brute Force here.\n";

    if (algoChoice == 3 && opChoice == 4)
        std::cout << "\n  [!] Hash Map update is O(1) — its genuine strength.\n"
                  << "      Expect it to beat AVL here.\n";


    // 1 insertion 
    printSeparator("INSERTION  (" + std::to_string(N) + " products)");

    ProductStore store;
    auto t0 = Clock::now();

    {
        std::mt19937 rng(42);
        std::uniform_real_distribution<double> priceDist(5.0,   5000.0);
        std::uniform_real_distribution<double> ratingDist(1.0,  5.0);
        std::uniform_int_distribution<int>     popDist(1,       1000000);

        for (int i = 1; i <= N; ++i)
        {
            std::ostringstream ss;
            ss << "Product_" << i;
            store.insert(i, ss.str(), priceDist(rng), ratingDist(rng), popDist(rng));
        }
    }

    double insertMs = elapsedMs(t0);
    std::cout << "Inserted " << store.count() << " products in "
              << std::fixed << std::setprecision(3) << insertMs << " ms\n";


    // 2 range query 
    printSeparator("RANGE QUERY  price in [" + std::to_string((int)rangeLow)
                   + ", " + std::to_string((int)rangeHigh) + "]");

    t0 = Clock::now();
    auto inRange   = store.rangeByPrice(rangeLow, rangeHigh);
    double rangeMs = elapsedMs(t0);

    std::cout << "Found " << inRange.size() << " products in "
              << rangeMs << " ms\n\n";

    int show = std::min((int)inRange.size(), 5);
    for (int i = 0; i < show; ++i) inRange[i]->print();
    if ((int)inRange.size() > show)
        std::cout << "  ... (" << inRange.size() - show << " more)\n";


    // 3 top-K by rating 
    printSeparator("TOP-" + std::to_string(K) + "  by RATING");

    t0 = Clock::now();
    auto topRated   = store.topKByRating(K);
    double ratingNs = elapsedNs(t0);

    std::cout << "Query time: " << ratingNs << " ns\n\n";
    for (auto* p : topRated) p->print();


    //  4 top-K by popularity 
    printSeparator("TOP-" + std::to_string(K) + "  by POPULARITY");

    t0 = Clock::now();
    auto topPop  = store.topKByPopularity(K);
    double popMs = elapsedMs(t0);

    std::cout << "Query time: " << popMs << " ms\n\n";
    for (auto* p : topPop) p->print();


    // 5 update
    printSeparator("UPDATE  product ID=" + std::to_string(updateId));

    auto before = store.rangeByPrice(updatePrice - 1, updatePrice + 1);
    std::cout << "Before update — products near $" << updatePrice << ": "
              << before.size() << "\n";

    t0 = Clock::now();
    store.update(updateId, updatePrice, updateRating);
    double updateMs = elapsedMs(t0);

    std::cout << "Update time: " << updateMs << " ms\n";

    auto after = store.rangeByPrice(updatePrice - 1, updatePrice + 1);
    std::cout << "After  update — products near $" << updatePrice << ": "
              << after.size() << "\n";
    for (auto* p : after) p->print();


    // 6 algorithm comparison 

    printSeparator("ALGORITHM COMPARISON  AVL vs " + algoName);

    ProductStore fresh = buildStore(N);

    // use a product that was not already updated in section 5
    int dummyId = (updateId == 1) ? 2 : 1;

    double avlTime   = 0;
    double otherTime = 0;
    std::string unit = "ms";
    std::string caveat;

    if (opChoice == 1)      // Range Query 
    {
        t0 = Clock::now();
        fresh.rangeByPrice(rangeLow, rangeHigh);
        avlTime = elapsedMs(t0);

        if (algoChoice == 1)
        {
            t0 = Clock::now();
            fresh.bruteForceRange(rangeLow, rangeHigh);
            otherTime = elapsedMs(t0);
        }
        else if (algoChoice == 2)
        {
            t0 = Clock::now();
            fresh.sortedVecRange(rangeLow, rangeHigh);
            otherTime = elapsedMs(t0);
        }
        else
        {
            t0 = Clock::now();
            fresh.hashMapRange(rangeLow, rangeHigh);
            otherTime = elapsedMs(t0);
            caveat = "Hash Map has no price ordering — full scan, same as Brute Force.";
        }

        printComparison("Range Query", avlTime, unit, otherTime, unit, algoName, caveat);
    }
    else if (opChoice == 2) // Top-K Rating 
    {
        unit = "ns";

        t0 = Clock::now();
        fresh.topKByRating(K);
        avlTime = elapsedNs(t0);

        if (algoChoice == 1)
        {
            t0 = Clock::now();
            fresh.bruteForceTopKRating(K);
            otherTime = elapsedNs(t0);
        }
        else if (algoChoice == 2)
        {
            t0 = Clock::now();
            fresh.sortedVecTopKRating(K);
            otherTime = elapsedNs(t0);
            caveat = "Sorted Vector re-sorts by rating every call — no benefit over Brute Force.";
        }
        else
        {
            t0 = Clock::now();
            fresh.hashMapTopKRating(K);
            otherTime = elapsedNs(t0);
            caveat = "Hash Map scans and sorts all entries — no benefit over Brute Force.";
        }

        printComparison("Top-K by Rating", avlTime, unit, otherTime, unit, algoName, caveat);
    }
    else if (opChoice == 3) // Top-K Popularity 
    {
        t0 = Clock::now();
        fresh.topKByPopularity(K);
        avlTime = elapsedMs(t0);

        if (algoChoice == 1)
        {
            t0 = Clock::now();
            fresh.bruteForceTopKPopularity(K);
            otherTime = elapsedMs(t0);
        }
        else if (algoChoice == 2)
        {
            t0 = Clock::now();
            fresh.sortedVecTopKPopularity(K);
            otherTime = elapsedMs(t0);
            caveat = "Sorted Vector re-sorts by popularity every call — same cost as Brute Force.";
        }
        else
        {
            t0 = Clock::now();
            fresh.hashMapTopKPopularity(K);
            otherTime = elapsedMs(t0);
            caveat = "Hash Map scans and sorts all entries — same cost as Brute Force.";
        }

        printComparison("Top-K by Popularity", avlTime, unit, otherTime, unit, algoName, caveat);
    }
    else                    //  Update 
    {
        // AVL update: remove from 2 trees + re-insert + rebuildHeap (O(log n) + O(n))
        t0 = Clock::now();
        fresh.update(dummyId, 1234.0, 3.0);
        avlTime = elapsedMs(t0);

        if (algoChoice == 1)
        {
            // brute force: linear scan only, no index to maintain (O(n))
            t0 = Clock::now();
            fresh.bruteForceUpdate(dummyId, 1234.0, 3.0);
            otherTime = elapsedMs(t0);
        }
        else if (algoChoice == 2)
        {

            t0 = Clock::now();
            fresh.sortedVecUpdate(dummyId, 1234.0, 3.0);
            otherTime = elapsedMs(t0);
            caveat = "Sorted Vector update = scan + full re-sort. Both steps are timed.";
        }
        else
        {
            // hash map: O(1) lookup + direct field write 
            t0 = Clock::now();
            fresh.hashMapUpdate(dummyId, 1234.0, 3.0);
            otherTime = elapsedMs(t0);
            caveat = "Hash Map update is O(1) , expected to beat AVL here.";
        }

        printComparison("Update", avlTime, unit, otherTime, unit, algoName, caveat);
    }


    // 7 performance summary 
    printSeparator("PERFORMANCE SUMMARY");

    std::cout << std::left
              << std::setw(30) << "Insertion"          << std::fixed << std::setprecision(3) << insertMs  << " ms\n"
              << std::setw(30) << "Range query (AVL)"  << rangeMs   << " ms\n"
              << std::setw(30) << "Top-K by rating"    << ratingNs  << " ns\n"
              << std::setw(30) << "Top-K by popularity"<< popMs     << " ms\n"
              << std::setw(30) << "Update (AVL)"       << updateMs  << " ms\n"
              << "\n"
              << std::setw(30) << "Comparison op (AVL)"   << avlTime   << " " << unit << "\n"
              << std::setw(30) << ("Comparison op (" + algoName + ")") << otherTime << " " << unit << "\n";

    return 0;
}