#pragma once
#include <vector>
#include <memory>
#include <cstddef>
namespace pulse {
inline constexpr int busCount=10;
inline constexpr int legacyChannelCount=32,legacyBusCount=32;
// Heap-owned objects keep stable addresses when the vector grows or shrinks.
// Structural edits are performed only while the audio callback is detached.
template<class T> class StableList {
 std::vector<std::unique_ptr<T>> data;
 struct Iterator {
  typename std::vector<std::unique_ptr<T>>::iterator it;
  T& operator*() const{return **it;}
  Iterator& operator++(){++it;return *this;}
  bool operator!=(const Iterator& other)const{return it!=other.it;}
 };
 struct ConstIterator {
  typename std::vector<std::unique_ptr<T>>::const_iterator it;
  const T& operator*()const{return **it;}
  ConstIterator& operator++(){++it;return *this;}
  bool operator!=(const ConstIterator& other)const{return it!=other.it;}
 };
public:
 int size()const{return (int)data.size();}
 T& operator[](int i){return *data[(size_t)i];}
 const T& operator[](int i)const{return *data[(size_t)i];}
 Iterator begin(){return {data.begin()};}Iterator end(){return {data.end()};}
 ConstIterator begin()const{return {data.begin()};}ConstIterator end()const{return {data.end()};}
 void reserve(int n){data.reserve((size_t)n);}
 T& append(){data.push_back(std::make_unique<T>());return *data.back();}
 void erase(int i){data.erase(data.begin()+i);}
 void swap(StableList& other)noexcept{data.swap(other.data);}
};
}
