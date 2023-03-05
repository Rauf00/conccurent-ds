#include "../common/allocator.h"
#include "../common/utils.h"

#define LFENCE asm volatile("lfence" : : : "memory")
#define SFENCE asm volatile("sfence" : : : "memory")

template<class P>
struct pointer_t {
    P* ptr;

    P* address(){
        //Get the address by getting the 48 least significant bits of ptr
        uintptr_t mask = (1ULL << 48) - 1;
        return reinterpret_cast<P*>(reinterpret_cast<uintptr_t>(ptr) & mask);
    }
    uint count(){
        //Get the count from the 16 most significant bits of ptr
        uintptr_t mask = (1ULL << 16) - 1;
        return static_cast<uint>((reinterpret_cast<uintptr_t>(ptr) >> 48) & mask);
    }
};

template <class T>
class Node
{
public:
    T value;
    pointer_t<Node<T>> next;
};

template <class T>
class NonBlockingQueue
{
    pointer_t<Node<T>> q_head;
    pointer_t<Node<T>> q_tail;
    CustomAllocator my_allocator_;
public:
    
    NonBlockingQueue() : my_allocator_()
    {
        std::cout << "Using NonBlockingQueue\n";
    }

    void initQueue(long t_my_allocator_size){
        std::cout << "Using Allocator\n";
        my_allocator_.initialize(t_my_allocator_size, sizeof(Node<T>));
        // Initialize the queue head or tail here
        Node<T>* newNode = (Node<T>*)my_allocator_.newNode();
        newNode->next.ptr = NULL;
        q_head.ptr = q_tail.ptr = newNode;
        my_allocator_.freeNode(newNode);
    }

    pointer_t<Node<T>> incrementAndCombine(Node<T>* ptr, uint count){
        uintptr_t combined_ptr_int = (static_cast<uintptr_t>(count) << 48) | reinterpret_cast<uintptr_t>(ptr);
        pointer_t<Node<T>> res;
        Node<T>* node_ptr = reinterpret_cast<Node<T>*>(combined_ptr_int);
        res.ptr = node_ptr;
        return res;
    }

    void enqueue(T value)
    {
        Node<T>* node = (Node<T>* )my_allocator_.newNode();
        node->value = value;
        node->next.ptr = NULL;
        pointer_t<Node<T>> tail;
        SFENCE;
        while(true) {
            tail = q_tail;
            LFENCE;
            pointer_t<Node<T>> next = tail.address()->next;
            LFENCE;
            if (tail.ptr == q_tail.ptr ) {
                if (next.address() == NULL) {
                    if(CAS(&tail.address()->next, next, incrementAndCombine(node, next.count() + 1)))
                        break;
                }
                else
                    CAS(&q_tail, tail, incrementAndCombine(next.address(), tail.count() + 1));	// ELABEL
            }
        }
        SFENCE;
        CAS(&q_tail, tail, incrementAndCombine(node, tail.count() + 1));
    }

    bool dequeue(T *value)
    {
        // Use LFENCE and SFENCE as mentioned in pseudocode
        pointer_t<Node<T>> head;
        pointer_t<Node<T>> tail;
        while(true){
            head = q_head;
            LFENCE;
            tail = q_tail;
            LFENCE;
            pointer_t<Node<T>> next = head.address()->next;
            LFENCE;
            if (head.ptr == q_head.ptr) {
                if(head.address() == tail.address()) {
                    if(next.address() == NULL)
                        return false;
                    CAS(&q_tail, tail, incrementAndCombine(next.address(), tail.count() + 1));	//DLABEL
                }
                else {
                    *value = next.address()->value;
                    if(CAS(&q_head, head, incrementAndCombine(next.address(), head.count() + 1)))
                        break;
                }
            }
        }
        my_allocator_.freeNode(head.address());
        return true;
    }

    void cleanup()
    {
        my_allocator_.cleanup();
    }

};

