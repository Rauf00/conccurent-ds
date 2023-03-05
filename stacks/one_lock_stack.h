#include "../common/allocator.h"
#include <mutex>

template <class T>
class Node
{
public:
    T value;
    Node<T>* next;
};

template <class T>
class OneLockStack
{
    Node<T>* top;
    std::mutex mutex;
    CustomAllocator my_allocator_;
public:
    OneLockStack() : my_allocator_()
    {
        std::cout << "Using OneLockStack\n";
    }

    void initStack(long t_my_allocator_size)
    {
        std::cout << "Using Allocator\n";
        my_allocator_.initialize(t_my_allocator_size, sizeof(Node<T>));
        // Perform any necessary initializations
        Node<T>* newNode = (Node<T>*)my_allocator_.newNode();
        newNode->next = NULL;
        top = newNode;
    }

    /**
     * Create a new node with value `value` and update it to be the top of the stack.
     * This operation must always succeed.
     */
    void push(T value)
    {
        Node<T>* node = (Node<T>*)my_allocator_.newNode();
        node->value = value;
        mutex.lock();
        node->next = top;
        top = node;
        mutex.unlock();
    }

    /**
     * If the stack is empty: return false.
     * Otherwise: copy the value at the top of the stack into `value`, update
     * the top to point to the next element in the stack, and return true.
     */
    bool pop(T *value)
    {
        mutex.lock();
        Node<T>* node = top;
        Node<T>* newTop = top->next;
        if(newTop == NULL){
            // Stack is empty
            mutex.unlock();
            return false;
        }
        *value = node->value;
        top = newTop;
        mutex.unlock();
        my_allocator_.freeNode(node);
        return true;
    }

    void cleanup()
    {
        my_allocator_.cleanup();
    }
};
