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
class LockFreeStack
{
    pointer_t<Node<T>> q_top;
    CustomAllocator my_allocator_;
public:
    LockFreeStack() : my_allocator_()
    {
        std::cout << "Using LockFreeStack\n";
    }

    void initStack(long t_my_allocator_size)
    {
        std::cout << "Using Allocator\n";
        my_allocator_.initialize(t_my_allocator_size, sizeof(Node<T>));
        // Perform any necessary initializations
        Node<T>* newNode = (Node<T>*)my_allocator_.newNode();
        newNode->next.ptr = NULL;
        q_top.ptr = newNode;
    }

    pointer_t<Node<T>> incrementAndCombine(Node<T>* ptr, uint count){
        uintptr_t combined_ptr_int = (static_cast<uintptr_t>(count) << 48) | reinterpret_cast<uintptr_t>(ptr);
        pointer_t<Node<T>> res;
        Node<T>* node_ptr = reinterpret_cast<Node<T>*>(combined_ptr_int);
        res.ptr = node_ptr;
        return res;
    }

    bool tryPush(Node<T>* node){
        pointer_t<Node<T>> top = q_top;
        LFENCE;
        if (top.ptr == q_top.ptr ) {
            node->next = top;
        }
        return CAS(&q_top, top, incrementAndCombine(node, top.count() + 1));	
    }

    /**
     * Create a new node with value `value` and update it to be the q_top of the stack.
     * This operation must always succeed.
     */
    void push(T value)
    {
        Node<T>* node = (Node<T>*)my_allocator_.newNode();
        node->value = value;
        SFENCE;
        while(true){
            if(tryPush(node)){
                return;
            }
        }
        // SFENCE;
    }

    /**
     * If the stack is empty: return false.
     * Otherwise: copy the value at the q_top of the stack into `value`, update
     * the q_top to point to the next element in the stack, and return true.
     */
    bool pop(T *value)
    {
        pointer_t<Node<T>> top;
        pointer_t<Node<T>> next;
        while(true){
            LFENCE;
            top = q_top;
            LFENCE;
            next = top.address()->next;
            LFENCE;
            if (top.ptr == q_top.ptr) {
                if(next.address() == NULL){
                    // Stack is empty
                    return false;
                }
            }
            *value = top.address()->value;
            if(CAS(&q_top, top, incrementAndCombine(next.address(), top.count() + 1))){
                break;
            } 
        }
        my_allocator_.freeNode(top.address());
        return true;
    }

    void cleanup()
    {
        my_allocator_.cleanup();
    }
};
