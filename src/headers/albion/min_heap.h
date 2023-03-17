#pragma once
#include <gb/forward_def.h>
#include <albion/lib.h>
#include <albion/debug.h>

// TODO: remove undeeded generics with this and just replace the event type with an int
// we can just pass the system struct into the event method and not require all this overkill

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
    u64 start;

    // when it will trigger
    u64 end;

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
template<u32 SIZE,typename event_type>
class MinHeap
{
public:
    MinHeap();

    void save_state(std::ofstream &fp);
    void load_state(std::ifstream &fp);

    EventNode<event_type> peek() const;
    void pop();
    u32 size() const;
    void clear();
    std::optional<EventNode<event_type>> get(event_type t) const;

    std::optional<EventNode<event_type>> remove(event_type t);
    bool is_active(event_type t) const;
	void insert(EventNode<event_type> event);

    std::array<EventNode<event_type>,SIZE> buf;
    const u32 IDX_INVALID = 0xffffffff;
private:
    void heapify(u32 idx);
    void verify();
    EventNode<event_type> remove(u32 idx);

	u32 left(u32 idx) const;
	u32 right(u32 idx) const;
	u32 parent(u32 idx) const;

	void swap(u32 idx1, u32 idx2);

    std::array<EventNode<event_type>*,SIZE> heap;
    // stores pos of each event so it can be deleted fast
    std::array<u32,SIZE> type_idx;
    
    u32 len = 0;
};

template<u32 SIZE,typename event_type>
MinHeap<SIZE,event_type>::MinHeap()
{
    assert(SIZE < IDX_INVALID);
    assert(SIZE != 0);
    clear();
}


template<u32 SIZE,typename event_type>
void MinHeap<SIZE,event_type>::clear()
{
    len = 0;
    // fill with obv error value incase we attempt to use something not initialized
    for(u32 i = 0; i < SIZE; i++)
    {
        const auto event = EventNode(0xdeadbeef,0xdeadbeef,static_cast<event_type>(i));
        buf[i] = event;
        heap[i] = &buf[i];
        type_idx[i] = IDX_INVALID;
    }
}

template<u32 SIZE,typename event_type>
EventNode<event_type> MinHeap<SIZE,event_type>::peek() const
{
    return *heap[0];
}


template<u32 SIZE,typename event_type>
u32 MinHeap<SIZE,event_type>::size() const
{
    return len;
}

template<u32 SIZE,typename event_type>
void MinHeap<SIZE,event_type>::pop()
{
    remove(0);
}

template<u32 SIZE,typename event_type>
u32 MinHeap<SIZE,event_type>::left(u32 idx) const
{
	return 2 * idx + 1;
}

template<u32 SIZE,typename event_type>
u32 MinHeap<SIZE,event_type>::right(u32 idx) const
{
	return 2 * idx + 2;
}

template<u32 SIZE,typename event_type>
u32 MinHeap<SIZE,event_type>::parent(u32 idx) const
{
	return (idx - 1) / 2;
}

template<u32 SIZE,typename event_type>
bool MinHeap<SIZE,event_type>::is_active(event_type t) const
{
    return type_idx[u32(t)] != IDX_INVALID;
}

// should ideally swap pointers here...
template<u32 SIZE,typename event_type>
void MinHeap<SIZE,event_type>::swap(u32 idx1,u32 idx2)
{
	std::swap(heap[idx1],heap[idx2]);

    type_idx[u32(heap[idx1]->type)] = idx1;
    type_idx[u32(heap[idx2]->type)] = idx2;
}

template<u32 SIZE,typename event_type>
void MinHeap<SIZE,event_type>::insert(EventNode<event_type> event)
{
    // no cycles would be ticked this event does nothing
    if(event.start == event.end)
    {
        return;
    }

	// in practice in our emulator events are unique and this shouldunt happen
	// i.e inserted events will be tagged with a enum and existing ones
    // will be removed by the scheduler before this should happen
	if(len == SIZE)
	{
		printf("event heap at SIZE %x\n",u32(event.type));
		exit(1);
	}

    // ok this can only go in a fixed slot
    const u32 event_idx = u32(event.type);
    u32 idx = len;
    buf[event_idx] = event;
    heap[len++] = &buf[event_idx];

    // update idx here incase it is first insertion
    type_idx[event_idx] = idx;

	// while parent is greater swap
	while(idx != 0 && *heap[parent(idx)] > *heap[idx])
	{
		const u32 parent_idx = parent(idx);
		swap(idx,parent_idx);
		idx = parent_idx;
	}

    //verify();
}

// verify min heap property is not violated used for debugging
template<u32 SIZE,typename event_type>
void MinHeap<SIZE,event_type>::verify()
{
    for(u32 i = 0; i < len; i++)
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
template<u32 SIZE,typename event_type>
void MinHeap<SIZE,event_type>::heapify(u32 idx)
{
    // check that children arent smaller
    while(idx <= len)
    {
        u32 min = idx;
        const u32 l = left(idx);
        const u32 r = right(idx);
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

template<u32 SIZE,typename event_type>
std::optional<EventNode<event_type>> MinHeap<SIZE,event_type>::remove(event_type t)
{
    //printf("removing type... \n");
    const auto idx = u32(t);
    if(type_idx[idx] != IDX_INVALID)
    {
        return remove(type_idx[idx]);
    }

    // not there nothing was removed
    return std::nullopt;
}

template<u32 SIZE,typename event_type>
std::optional<EventNode<event_type>> MinHeap<SIZE,event_type>::get(event_type t) const
{
    const auto idx = u32(t);

    if(type_idx[idx] != IDX_INVALID)
    {
        return buf[idx];
    }

    return std::nullopt;
}


// this can probably be faster but leave for now
template<u32 SIZE,typename event_type>
EventNode<event_type> MinHeap<SIZE,event_type>::remove(u32 idx)
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
    type_idx[u32(v.type)] = IDX_INVALID;


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

    // children may need fixing
    else
    {
        heapify(idx);
    }

    //verify();

	return v;
}


template<u32 SIZE,typename event_type>
void MinHeap<SIZE,event_type>::save_state(std::ofstream &fp)
{
    file_write_arr(fp,type_idx.data(),sizeof(type_idx[0]) * type_idx.size());


    file_write_arr(fp,buf.data(),sizeof(buf[0]) * buf.size());

    // ok as our heap now has pointers we will write out indexes and re populate the pointers
    // instead 
    const auto start = size_t(&buf[0]);
    std::array<size_t,SIZE> idx_list;

    for(size_t i = 0; i < SIZE; i++)
    {
        const auto ptr = size_t(heap[i]);
        idx_list[i] = ((ptr - start) / sizeof(EventNode<event_type>));
    }

    file_write_arr(fp,idx_list.data(),sizeof(idx_list[0]) * idx_list.size());

    file_write_var(fp,len);
}

template<u32 SIZE,typename event_type>
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

    for(u32 i = 0; i < SIZE; i++)
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
        if(u32(x->type) >= SIZE)
        {
            throw std::runtime_error("minheap invalid event type");
        }
    }
}
