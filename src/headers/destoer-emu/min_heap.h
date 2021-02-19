#pragma once
#include <gb/forward_def.h>
#include <destoer-emu/lib.h>
#include <destoer-emu/debug.h>

template<typename event_type>
struct EventNode
{
    EventNode() {}

    EventNode(uint64_t s, uint64_t e, event_type t)
    {
        start = s;
        end = e;
        type = t;
    }

    // when event was added
    uint32_t start;

    // when it will trigger
    uint32_t end;

    event_type type;


    template<typename T>
	friend bool operator<(const EventNode<T> &lhs,const EventNode<T> &rhs);

    template<typename T>
	friend bool operator>(const EventNode<T> &lhs,const EventNode<T> &rhs);


};

template<typename T>
inline bool operator<(const EventNode<T> &lhs,const EventNode<T> &rhs)
{
    return lhs.end < rhs.end;
}

template<typename T>
inline bool operator>(const EventNode<T> &lhs,const EventNode<T> &rhs)
{
    return lhs.end > rhs.end;
}





// binary min heap implementation
template<size_t SIZE,typename event_type>
class MinHeap
{
public:
    MinHeap();

    void save_state(std::ofstream &fp);
    void load_state(std::ifstream &fp);

    EventNode<event_type> peek() const;
    void pop();
    size_t size() const;
    void clear();
    std::optional<EventNode<event_type>> get(event_type t) const;

    std::optional<EventNode<event_type>> remove(event_type t);
    bool is_active(event_type t) const;
	void insert(EventNode<event_type> event);

    std::array<EventNode<event_type>,SIZE> buf;
    const size_t IDX_INVALID = 0xffffffff;
private:
    void heapify(size_t idx);
    void verify();
    EventNode<event_type> remove(size_t idx);

	size_t left(size_t idx) const;
	size_t right(size_t idx) const;
	size_t parent(size_t idx) const;

	void swap(size_t idx1, size_t idx2);

    std::array<EventNode<event_type>*,SIZE> heap;
    // stores pos of each event so it can be deleted fast
    std::array<size_t,SIZE> type_idx;
    
    size_t len = 0;
};

template<size_t SIZE,typename event_type>
MinHeap<SIZE,event_type>::MinHeap()
{
    assert(SIZE < IDX_INVALID);
    assert(SIZE != 0);
    clear();
}


template<size_t SIZE,typename event_type>
void MinHeap<SIZE,event_type>::clear()
{
    len = 0;
    // fill with obv error value incase we attempt to use something not initialized
    for(size_t i = 0; i < SIZE; i++)
    {
        const auto event = EventNode(0xdeadbeef,0xdeadbeef,static_cast<event_type>(i));
        buf[i] = event;
        heap[i] = &buf[i];
        type_idx[i] = IDX_INVALID;
    }
}

template<size_t SIZE,typename event_type>
EventNode<event_type> MinHeap<SIZE,event_type>::peek() const
{
    return *heap[0];
}


template<size_t SIZE,typename event_type>
size_t MinHeap<SIZE,event_type>::size() const
{
    return len;
}

template<size_t SIZE,typename event_type>
void MinHeap<SIZE,event_type>::pop()
{
    remove(0);
}

template<size_t SIZE,typename event_type>
size_t MinHeap<SIZE,event_type>::left(size_t idx) const
{
	return 2 * idx + 1;
}

template<size_t SIZE,typename event_type>
size_t MinHeap<SIZE,event_type>::right(size_t idx) const
{
	return 2 * idx + 2;
}

template<size_t SIZE,typename event_type>
size_t MinHeap<SIZE,event_type>::parent(size_t idx) const
{
	return (idx - 1) / 2;
}

template<size_t SIZE,typename event_type>
bool MinHeap<SIZE,event_type>::is_active(event_type t) const
{
    return type_idx[static_cast<size_t>(t)] != IDX_INVALID;
}

// should ideally swap pointers here...
template<size_t SIZE,typename event_type>
void MinHeap<SIZE,event_type>::swap(size_t idx1,size_t idx2)
{
	std::swap(heap[idx1],heap[idx2]);

    type_idx[static_cast<size_t>(heap[idx1]->type)] = idx1;
    type_idx[static_cast<size_t>(heap[idx2]->type)] = idx2;
}

template<size_t SIZE,typename event_type>
void MinHeap<SIZE,event_type>::insert(EventNode<event_type> event)
{
	// in practice in our emulator events are unique and this shouldunt happen
	// i.e inserted events will be tagged with a enum and existing ones
    // will be removed by the scheduler before this should happen
	if(len == SIZE)
	{
		printf("event heap at SIZE %zx\n",static_cast<size_t>(event.type));
		exit(1);
	}

    // ok this can only go in a fixed slot
    const auto event_idx = static_cast<size_t>(event.type);
    size_t idx = len;
    buf[event_idx] = event;
    heap[len++] = &buf[event_idx];

    // update idx here incase it is first insertion
    type_idx[event_idx] = idx;

	// while parent is greater swap
	while(idx != 0 && *heap[parent(idx)] > *heap[idx])
	{
		const auto parent_idx = parent(idx);
		swap(idx,parent_idx);
		idx = parent_idx;
	}

//    verify();
}

// verify min heap property is not violated used for debugging
template<size_t SIZE,typename event_type>
void MinHeap<SIZE,event_type>::verify()
{
    for(size_t i = 0; i < len; i++)
    {
        const auto r = right(i);
        const auto l = left(i);

        if(l < len && *heap[i] > *heap[l])
        {
            printf("min heap violated\n");
            exit(1);
        }

        if(r < len && *heap[i] > *heap[r])
        {
            printf("min heap violated\n");
            exit(1);
        }
    }
}

// how to do an arbitary remove?
template<size_t SIZE,typename event_type>
void MinHeap<SIZE,event_type>::heapify(size_t idx)
{

	// while we are smaller than parent swap
	if(idx != 0 && *heap[idx] < *heap[parent(idx)])
	{
        while(idx != 0 && *heap[idx] < *heap[parent(idx)])
        {
            const auto parent_idx = parent(idx);
		    swap(idx,parent_idx);
		    idx = parent(idx);
        }
	}

    // check that children arent smaller
    else
    {
        while(idx <= len)
        {
            size_t min = idx;
            const auto l = left(idx);
            const auto r = right(idx);
            // swap idx with smallest if its not allready
            if(l < len && *heap[l] < *heap[min])
            {
                min = l;
            }
            
            if(r < len &&  *heap[r] < *heap[min])
            {
                min = r;
            }
            
            // if we aernt the smallest
            if(min != idx)
            {
                swap(min,idx);
                idx = min;
            }

            // we were the smallest so min heap is fixed
            else
            {
                break;
            }
        }
    }
}

template<size_t SIZE,typename event_type>
std::optional<EventNode<event_type>> MinHeap<SIZE,event_type>::remove(event_type t)
{
    //printf("removing type... \n");
    const auto idx = static_cast<size_t>(t);
    if(type_idx[idx] != IDX_INVALID)
    {
        return remove(type_idx[idx]);
    }

    // not there nothing was removed
    return std::nullopt;
}

template<size_t SIZE,typename event_type>
std::optional<EventNode<event_type>> MinHeap<SIZE,event_type>::get(event_type t) const
{
    const auto idx = static_cast<size_t>(t);

    if(type_idx[idx] != IDX_INVALID)
    {
        return buf[idx];
    }

    return std::nullopt;
}


// this can probably be faster but leave for now
template<size_t SIZE,typename event_type>
EventNode<event_type> MinHeap<SIZE,event_type>::remove(size_t idx)
{
    //printf("removed %zd\n",idx);
	if(len == 0)
    {
        puts("attempted to remove from empty heap");
        exit(1);
    }

	const auto v = *heap[idx];
	len--;
	// swap end with deleted
	swap(len,idx);
	
	// mark as "inactive"
    type_idx[static_cast<size_t>(v.type)] = IDX_INVALID;


	// ensure we have a valid heap after the swap
	heapify(idx);

//    verify();

	return v;
}


template<size_t SIZE,typename event_type>
void MinHeap<SIZE,event_type>::save_state(std::ofstream &fp)
{
    file_write_arr(fp,type_idx.data(),sizeof(type_idx[0]) * type_idx.size());


    file_write_arr(fp,buf.data(),sizeof(buf[0]) * buf.size());

    // ok as our heap now has pointers we will write out indexes and re populate the pointers
    // instead 
    const auto start = reinterpret_cast<size_t>(&buf[0]);
    std::array<size_t,SIZE> idx_list;

    for(size_t i = 0; i < SIZE; i++)
    {
        const auto ptr = reinterpret_cast<size_t>(heap[i]);
        idx_list[i] = ((ptr - start) / sizeof(EventNode<event_type>));
    }

    file_write_arr(fp,idx_list.data(),sizeof(idx_list[0]) * idx_list.size());

    file_write_var(fp,len);
}

template<size_t SIZE,typename event_type>
void MinHeap<SIZE,event_type>::load_state(std::ifstream &fp)
{
    file_read_arr(fp,type_idx.data(),sizeof(type_idx[0]) * type_idx.size());
  

    
    file_read_arr(fp,buf.data(),sizeof(buf[0]) * buf.size());

    // read idx back in so we can reconstruct our ptrs
    std::array<size_t,SIZE> idx_list;
    file_read_arr(fp,idx_list.data(),sizeof(idx_list[0]) * idx_list.size());

    // verify idx bounds 
    for(const auto &x: idx_list)
    {
        if(x >= SIZE)
        {
            throw std::runtime_error("minheap invalid heap idx");
        }
    }

    for(size_t i = 0; i < SIZE; i++)
    {
        heap[i] = &buf[idx_list[i]];
    }

    file_read_var(fp,len);


    if(len > SIZE)
    {
        throw std::runtime_error("minheap invalid len");
    }

    for(auto &x: type_idx)
    {
        if(x >= len && x != IDX_INVALID)
        {
            throw std::runtime_error("minheapinvalid type idx");
        }
    }

    for(auto &x: heap)
    {
        if(static_cast<size_t>(x->type) >= SIZE)
        {
            throw std::runtime_error("minheap invalid event type");
        }
    }
}