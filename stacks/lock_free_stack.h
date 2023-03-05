#include "../common/allocator.h"
#include "../common/utils.h"

template <class T>
class Node
{
public:
    T value;
    Node<T>* next;
};

template <class T>
class LockFreeStack
{
    Node<T>* top;
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
        newNode->next = NULL;
        top = newNode;
    }

    bool tryPush(Node<T>* node){
        Node<T>* oldTop = top;
        node->next = oldTop;
        return CAS(&top, oldTop, node);	
    }

    /**
     * Create a new node with value `value` and update it to be the top of the stack.
     * This operation must always succeed.
     */
    void push(T value)
    {
        Node<T>* node = (Node<T>*)my_allocator_.newNode();
        node->value = value;
        while(true){
            if(tryPush(node)){
                return;
            }
        }
    }

    Node<T>* tryPop()
    {
        Node<T>* oldTop = top;
        Node<T>* newTop = top->next;
        if(CAS(&top, oldTop, newTop)){
            return oldTop;
        } else {
            return NULL;
        }
    }

    /**
     * If the stack is empty: return false.
     * Otherwise: copy the value at the top of the stack into `value`, update
     * the top to point to the next element in the stack, and return true.
     */
    bool pop(T *value)
    {
        Node<T>* returnNode;
        while(true){
            if(top->next == NULL){
                // Stack is empty
                return false;
            }
            returnNode = tryPop();
            if (returnNode != NULL){
                *value = returnNode->value;
                my_allocator_.freeNode(returnNode);
                return true;
            }
        }
    }

    void cleanup()
    {
        my_allocator_.cleanup();
    }
};
