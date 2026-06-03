#include "../h files/Product.h"
#include <iostream>
#include <iomanip>

Product::Product(int id, std::string name, double price, double rating, int popularity)
    : id(id), name(std::move(name)), price(price), rating(rating), popularity(popularity) {}

void Product::print() const {
    std::cout << std::fixed << std::setprecision(2)
              << "[ID:" << id << "] " << name
              << " | $" << price
              << " | ★" << rating
              << " | pop:" << popularity
              << "\n";
}