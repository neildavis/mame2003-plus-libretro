#include <wiringPi.h>
#include <cstddef>

struct wiringPiNodeStruct *wiringPiRemoveNode (int pin) {
    struct wiringPiNodeStruct *node = wiringPiNodes, *prevNode = NULL;
    while (node != NULL) {
        if ((pin >= node->pinBase) && (pin <= node->pinMax)) {
            // This is the node we want to remove
            if (NULL == prevNode) {
                // Node to be removed is HEAD node. Replace HEAD with next
                wiringPiNodes = node->next;
            } else {
                // Node to be removed is NOT HEAD node. Fix up linked list
                prevNode->next = node->next;
            }
            break;
        } else {
            // This is not the node you're looking for. Move along ...
            prevNode = node;
            node = node->next;
        }
    }
    return node;
}
