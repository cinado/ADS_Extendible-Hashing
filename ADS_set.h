#ifndef ADS_SET_H
#define ADS_SET_H

#include <functional>
#include <algorithm>
#include <iostream>
#include <stdexcept>

template <typename Key, size_t N = 9>
class ADS_set {
public:
  class Iterator;
  using value_type = Key;
  using key_type = Key;
  using reference = value_type &;
  using const_reference = const value_type &;
  using size_type = size_t;
  using difference_type = std::ptrdiff_t;
  using const_iterator = Iterator;
  using iterator = const_iterator;
  //using key_compare = std::less<key_type>;                         // B+-Tree
  using key_equal = std::equal_to<key_type>;                       // Hashing
  using hasher = std::hash<key_type>;                              // Hashing

private:	
	struct Bucket{
		size_type localDepth{0};
		size_type currentSize {0};				//Amount of elements in this bucket	
		key_type elem[N];									//Array with the size of N. Contains the elements
		Bucket* next{nullptr};						//Link - points to the next bucket
		Bucket(size_type localDepthIn, Bucket* next = nullptr):localDepth{localDepthIn}, next{next}{}
	};
	
	size_type setSize{0};								//Size of the set - total number of elements
	size_type tableSize{1};							//Size of the table
	size_type globalDepth{0};
	Bucket** table{nullptr}; 						//Pointer to a pointer; Table of pointers which point to the buckets
	Bucket* head{nullptr};							//Beginning of the "linked list"

	//Function hash: Flip 0 bitmask to 1, left shift by #<globalDepth> positions thus, you get 0 at LSB. Flip Bitmask again, so you get 1 at LSB before you make &-bitoperation on hashed key
	//Summary: Get #<globalDepth> last bits of the hashed key.
	size_type hash(const key_type &key) const {return hasher{}(key) & ~(~0U << globalDepth);}
	void insertHelper(const key_type &key);
	void splitBucket(const key_type &key);
	void expandDirectory();
	void add(const key_type &key);
	
public:
  ADS_set():table{new Bucket*[tableSize]{new Bucket{0}}}, head{*table} {}                             	// PH1
  ADS_set(std::initializer_list<key_type> ilist): ADS_set{} {insert(ilist);}                      			// PH1
  template<typename InputIt> ADS_set(InputIt first, InputIt last): ADS_set{} {insert(first,last);}     // PH1
  ADS_set(const ADS_set &other);

  ~ADS_set();

  ADS_set &operator=(const ADS_set &other);
  ADS_set &operator=(std::initializer_list<key_type> ilist);

  size_type size() const {return setSize;}                                              								// PH1
  bool empty() const;                                                  																	// PH1

  void insert(std::initializer_list<key_type> ilist) {insert(ilist.begin(), ilist.end());}              // PH1
  std::pair<iterator,bool> insert(const key_type &key);
  template<typename InputIt> void insert(InputIt first, InputIt last); 																	// PH1

  void clear();
  size_type erase(const key_type &key);

  size_type count(const key_type &key) const;                          																	// PH1
  iterator find(const key_type &key) const;

  void swap(ADS_set &other);

  const_iterator begin() const{
  	return const_iterator{0, head};
  }
  
  const_iterator end() const{
  	return const_iterator{(*table)->currentSize, *table};
  }

  void dump(std::ostream &o = std::cerr) const;

  friend bool operator==(const ADS_set &lhs, const ADS_set &rhs){
  	if(lhs.setSize != rhs.setSize){
  		return false;
  	}
  	
  	for(const auto& x:lhs){
  		if(!rhs.count(x)){
  			return false;
  		}
  	}
  	return true;
  }
  
  friend bool operator!=(const ADS_set &lhs, const ADS_set &rhs){
  	return !(lhs==rhs);
  }
};

template <typename Key, size_t N>
ADS_set<Key,N> &ADS_set<Key,N>::operator=(const ADS_set &other){
	ADS_set temp{other};
	swap(temp);
	return *this;
}

template <typename Key, size_t N>
ADS_set<Key,N> &ADS_set<Key,N>::operator=(std::initializer_list<key_type> ilist){
	ADS_set temp{ilist};
	swap(temp);
	return *this;
}

template <typename Key, size_t N>
ADS_set<Key,N>::~ADS_set(){
	Bucket* bucket{head};
	while(bucket){
		Bucket* nextBucket{bucket->next};
		delete bucket;
		bucket = nextBucket;
	}
	delete[] table;
}

template <typename Key, size_t N>
std::pair<typename ADS_set<Key,N>::iterator,bool> ADS_set<Key,N>::insert(const key_type &key){
	auto it{find(key)};
	if(it != end()){
		return {it, false};
	}
	insertHelper(key);
	return {find(key), true};
}

template <typename Key, size_t N>
void ADS_set<Key,N>::swap(ADS_set &other){
	using std::swap;
	swap(setSize, other.setSize);
	swap(tableSize, other.tableSize);
	swap(globalDepth, other.globalDepth);
	swap(table, other.table);
	swap(head, other.head);
}

template <typename Key, size_t N>
typename ADS_set<Key,N>::iterator ADS_set<Key,N>::find(const key_type &key) const{
	size_type adr{hash(key)};
	for(size_type i{0}; i < (*(table+adr))->currentSize; i++){
		if(key_equal{}((*(table+adr))->elem[i], key)){
			return iterator(i, *(table+adr));
		}
	}
	return end();
}

template <typename Key, size_t N>
typename ADS_set<Key,N>::size_type ADS_set<Key,N>::erase(const key_type &key){
	size_type adr{hash(key)};
	for(size_type i{0}; i < (*(table+adr))->currentSize; i++){
		if(key_equal{}((*(table+adr))->elem[i], key)){
			if((*(table+adr))->currentSize != 1 && i != (*(table+adr))->currentSize-1){
				(*(table+adr))->elem[i] = (*(table+adr))->elem[(*(table+adr))->currentSize-1];
			}
			(*(table+adr))->currentSize--;
			setSize--;
			return 1;
		}
	}
	return 0;
}

template <typename Key, size_t N>
void ADS_set<Key,N>::clear(){
	ADS_set tmp;
	swap(tmp);
}

//Insert every element from other into the new ADS_set
template <typename Key, size_t N>
ADS_set<Key,N>::ADS_set(const ADS_set &other):ADS_set{} {
	for(const auto& x:other){
		insertHelper(x);
	}
}

//Check if every bucket is empty
template <typename Key, size_t N>
bool ADS_set<Key,N>::empty() const{
	for(size_type i{0}; i < tableSize; i++){
		if(table[i]->currentSize != 0){
			return false;
		}
	}
	return true;
}

//Get the right bucket by hashing the key and check if the key exists in the bucket
template <typename Key, size_t N>
typename ADS_set<Key,N>::size_type ADS_set<Key,N>::count(const key_type &key) const{
	size_type adr{hash(key)};
	for(size_type i{0}; i < (*(table+adr))->currentSize; i++){
		if(key_equal{}((*(table+adr))->elem[i], key)){
			return 1;
		}
	}
	return 0;
}

template <typename Key, size_t N>
template<typename InputIt> void ADS_set<Key,N>::insert(InputIt first, InputIt last){
	for(auto it{first}; it != last; it++){
		//Check if element already exists
		if(!count(*it)){
			insertHelper(*it);
		}
	}
}

template <typename Key, size_t N>
void ADS_set<Key,N>::insertHelper(const key_type &key){
	add(key);
	setSize++;
}

template <typename Key, size_t N>
void ADS_set<Key,N>::splitBucket(const key_type &key){
	//Increase local depth. If local depth equals global depth --> expand directory, Else: Just split the bucket
	size_type adr{hash(key)};
	if((*(table+adr))->localDepth >= globalDepth){
		expandDirectory();
		//If the directory has been expanded, it is necessary to get the new position 
		adr = hash(key);
	}
	
	(*(table+adr))->localDepth++;
	//Copy bucket before rehashing
	Bucket copiedBucket = **(table+adr);
	(*(table+adr))->currentSize = 0;
	//Distance between the pointers of the old bucket euqals to 2^localDepth
	size_type distance{1U << (*(table+adr))->localDepth};
	//Get first index/pointer which will point to the new bucket	
	size_type pos{(adr & ~(~0U << ((*(table+adr))->localDepth-1))) + distance/2};
	//link to the old bucket and make the new bucket as head
	table[pos] = new Bucket{(*(table+adr))->localDepth, head};
	head = table[pos];
	
	//Assign the right pointers to the new bucket
	for(size_type i{pos}; i < tableSize; i+=distance){
		table[i] = table[pos];
	}
	//Rehash every Element
	for(size_type i{0}; i < copiedBucket.currentSize; i++){
		add(copiedBucket.elem[i]);
	}
}

template <typename Key, size_t N>
void ADS_set<Key,N>::expandDirectory(){
	//Increase global depth, create new table with twice as much indices
	globalDepth++;
	Bucket** newTable = new Bucket*[tableSize*=2];
	//Take over old values and rearrange new table pointers
	for(size_type i{0}; i < tableSize/2; i++){
		newTable[i] = table[i];
		//Rearrange the pointers of new indices thus they point to existing buckets
		newTable[i+tableSize/2] = table[i];
	}

	//Delete the old table and replace it with the new one
	delete[] table;
	table = newTable;
}

template <typename Key, size_t N>
void ADS_set<Key,N>::add(const key_type &key){
	//Check if bucket has enough space. If not split bucket if possible or duplicate directory
	while((*(table+hash(key)))->currentSize == N){
		splitBucket(key);
	}
	//Insert new key and increment size
	(*(table+hash(key)))->elem[(*(table+hash(key)))->currentSize] = key;
	(*(table+hash(key)))->currentSize++;
}

template <typename Key, size_t N>
void ADS_set<Key,N>::dump(std::ostream &o) const{
	std::string res = "";
	o << "-----------------------------------------\n";
	o << "Setsize: " << setSize << " Global depth: " << globalDepth << " Tablesize: " << tableSize << "\n";
	for(size_type i{0}; i < tableSize; i++){
		res.clear();
		o << "+++++++++++++++++++++++++++++++++++++++\n";
		o << "\n";
		o << "Nummer: " << i << " Bucket-Adress: " << table[i] << " Local depth: " << table[i]->localDepth << " Elements: " << table[i]->currentSize << "\n";
		for(size_type j{0}; j < table[i]->currentSize; j++){
			o << table[i]->elem[j] << " ";
		}
		o << "\n";
		o << "\n";
	}
	o << "+++++++++++++++++++++++++++++++++++++++\n";
	o << "-----------------------------------------\n";
}



template <typename Key, size_t N>
class ADS_set<Key,N>::Iterator{
	//We need to know in which bucket the iterator is and the position the iterator points to.
	size_type position;
	Bucket* bucket;
	//Überspringen von leeren Buckets und springen zum nächsten Bucket 
	void skip(){
		while(bucket != nullptr && bucket->next != nullptr && position >= bucket->currentSize){
			position = 0;
			bucket = bucket->next;
		}
	}
public:
  using value_type = Key;
  using difference_type = std::ptrdiff_t;
  using reference = const value_type &;
  using pointer = const value_type *;
  using iterator_category = std::forward_iterator_tag;

  explicit Iterator(size_type position = 0, Bucket* bucket = nullptr):position{position}, bucket{bucket} {
  	skip();
  };
  reference operator*() const{
  	return bucket->elem[position];
  }
  
  pointer operator->() const{
  	return &bucket->elem[position];
  }
  
  Iterator &operator++(){
  	++position;
  	skip();
  	return *this;
  }
  
  Iterator operator++(int){
  	auto rc{*this};
  	++*this;
  	return rc;
  }
  
  friend bool operator==(const Iterator &lhs, const Iterator &rhs){
  	return lhs.position == rhs.position && lhs.bucket == rhs.bucket;
  }
  
  friend bool operator!=(const Iterator &lhs, const Iterator &rhs){
  	return !(lhs == rhs);
  }
};

template <typename Key, size_t N>
void swap(ADS_set<Key,N> &lhs, ADS_set<Key,N> &rhs) { lhs.swap(rhs); }


#endif // ADS_SET_H
