//#define DEBUG_INSERT
//#define DEBUG_REMOVE

#include <iostream>
#include <math.h>
#include <string.h>
#include "support.h"
#include "ChHashTable.h"

using std::cout;
using std::endl;

template <typename T>
ChHashTable<T>::ChHashTable(const ChHashTable<T>::HTConfig& Config, ObjectAllocator* allocator): config_{Config}, oa_{allocator} {
    // init stats
    stats_.TableSize_ = config_.InitialTableSize_;
    stats_.HashFunc_ = config_.HashFunc_;
    
    // init table array
    table_ = new ChHTHeadNode[stats_.TableSize_];
}

template <typename T>
ChHashTable<T>::~ChHashTable() {
    clear();
    delete[] table_;
}

template <typename T>
void ChHashTable<T>::insert(const char *Key, const T& Data) throw(HashTableException) {
    // find the node to check for dupes (will throw if dupe)
    // - if this is the first node then no need to waste time checking
    if (stats_.Count_!=0 && find_node(Key) != nullptr)
        throw HashTableException(HashTableException::E_DUPLICATE, "Key to be inserted is already in hashtable.");

    insert_node(make_node(Key, Data));
    ++stats_.Count_;
    
    // calc load factor
    double load_factor = stats_.Count_ / static_cast<double>(stats_.TableSize_);

#ifdef DEBUG_INSERT
    cout << "insert: {" << Key << "} load_factor=" << load_factor << endl;     
#endif

    // expand table if > max load
    if ( load_factor > config_.MaxLoadFactor_ ) {

#ifdef DEBUG_INSERT
        cout << "insert: {" << Key << "} EXPANDing table size="  << stats_.TableSize_ << endl;     
#endif

        // get new table size
        auto old_tablesize = stats_.TableSize_;
        auto factor = ceil(stats_.TableSize_ * config_.GrowthFactor_);
        stats_.TableSize_ = GetClosestPrime(static_cast<unsigned>(factor)); 

        // re-insert all items into new table
        auto old_table = table_;
        table_ = new ChHTHeadNode[stats_.TableSize_]; 
        for (auto i=0; i<old_tablesize; ++i) {
            auto hnode = old_table[i];
            auto node = hnode.Nodes;
            while (node) {
                ++stats_.Probes_;
                auto tmp = node;
                node = node->Next;
                insert_node(tmp);
            }
            old_table[i].Nodes = nullptr;
        }
        delete[] old_table;
        ++stats_.Expansions_;

#ifdef DEBUG_INSERT
        cout << "insert: {" << Key << "} EXPANDED table new size="  << stats_.TableSize_ << endl;     
#endif

    }
      
}

template <typename T>
void ChHashTable<T>::insert_node(ChHTNode* node) {
    // get the correct i into table
    auto i = stats_.HashFunc_(node->Key, stats_.TableSize_);
    ++stats_.Probes_;

#ifdef DEBUG_INSERT
    cout << "insert_node: {" << node->Key << "," << node->Data << "}, hashed i=" << i << endl;     
#endif
   
    // insert into table
    // - set new node as the head
    // - update stats
    node->Next = table_[i].Nodes;
    table_[i].Nodes = node;
    ++table_[i].Count;
}

//TODO try to use the shared find_node method
template <typename T>
void ChHashTable<T>::remove(const char *Key) throw(HashTableException) {
    // get i into the table 
    auto i = stats_.HashFunc_(Key, stats_.TableSize_);
    ++stats_.Probes_;

#ifdef DEBUG_REMOVE
        cout << "remove: {" << Key << ", DATA}, hashed i=" << i << endl;     
#endif

    // iterate through linked list to find key 
    if (table_[i].Count == 0)
        throw HashTableException(HashTableException::E_ITEM_NOT_FOUND, "Key to be deleted not found in hashtable.");
    else {
        auto node = table_[i].Nodes;
        auto prev_node = node;
        while (node) {

#ifdef DEBUG_REMOVE
        cout << "remove: looping LL of table[" << i << "] node {" << node->Key << ", DATA}" << endl;     
#endif

            if (strcmp(node->Key, Key) == 0)
                break;
            prev_node = node;
            node = node->Next;

            ++stats_.Probes_;
        }
        if (node == nullptr)
            throw HashTableException(HashTableException::E_ITEM_NOT_FOUND, "Key to be deleted not found in hashtable.");
       
#ifdef DEBUG_REMOVE
        cout << "remove: end looping LL of table[" << i << "] node {" << node->Key << ", DATA}" << endl;     
#endif

        // remove item
        auto tmp = node;
        if (prev_node == node)
            table_[i].Nodes = node->Next;
        else
            prev_node->Next = node->Next;
        delete_node(tmp);
        --table_[i].Count;
        --stats_.Count_;
    }
}

template <typename T>
const T& ChHashTable<T>::find(const char *Key) const throw(HashTableException) {
    auto node = find_node(Key);
    if (node == nullptr)
        throw HashTableException(HashTableException::E_ITEM_NOT_FOUND, "Key to find does not exist in table.");
    return node->Data;
}

template <typename T>
void ChHashTable<T>::clear(void) {
    for (auto i=0; i<stats_.TableSize_; ++i) {
        auto hnode = table_[i];
        auto node = hnode.Nodes;
        while (node) {
            auto tmp = node;
            node = node->Next;
            delete_node(tmp);
            --table_[i].Count;
            --stats_.Count_;
        }
        table_[i].Nodes = nullptr;
    }
}

template <typename T>
HTStats ChHashTable<T>::GetStats(void) const {
    return stats_;
}

template <typename T>
const typename ChHashTable<T>::ChHTHeadNode* ChHashTable<T>::GetTable(void) const {
    return table_;
}

template <typename T>
typename ChHashTable<T>::ChHTNode* ChHashTable<T>::make_node(const char* Key, const T& Data) {
    ChHTNode* node;
    if (oa_)
        node = new (oa_->Allocate()) ChHTNode(Data);
    else
        node = new ChHTNode(Data);
    std::strcpy(node->Key, Key);
    return node;
}

template <typename T>
void ChHashTable<T>::delete_node(ChHTNode* node) {
    if (oa_)
        node->~ChHTNode();
    delete node;
}

template <typename T>
typename ChHashTable<T>::ChHTNode* ChHashTable<T>::find_node(const char* key) const{
    // get i into the table 
    auto i = stats_.HashFunc_(key, stats_.TableSize_);
    ++stats_.Probes_;

    // iterate through linked list to find key 
    if (table_[i].Count == 0)
        return nullptr;
    else {
        auto node = table_[i].Nodes;
        while (node) {
            if (strcmp(node->Key, key) == 0)
                break;
            node = node->Next;
        }
        return node;
    }
}
