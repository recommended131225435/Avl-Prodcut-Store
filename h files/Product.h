#pragma once
#include <string>


//  Every index stores a RAW POINTER to one of these bkz we dont want copies 
// There is exactly one Product instance per product in the system.

struct Product {
    int         id;
    std::string name;
    double      price;
    double      rating;
    int         popularity;

    Product(int id, std::string name, double price, double rating, int popularity);
    void print() const;
};