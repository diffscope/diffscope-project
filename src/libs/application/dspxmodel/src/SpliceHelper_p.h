#ifndef DIFFSCOPE_DSPX_MODEL_SPLICEHELPER_P_H
#define DIFFSCOPE_DSPX_MODEL_SPLICEHELPER_P_H

#include <algorithm>

namespace dspx {

    class SpliceHelper {
    public:

        template <typename Container, typename It2>
        static void splice(Container& container, typename Container::iterator first, 
                          typename Container::iterator last, It2 newFirst, It2 newLast) {
            auto oldSize = std::distance(first, last);
            auto newSize = std::distance(newFirst, newLast);
            
            auto replaceCount = std::min(oldSize, newSize);
            auto replaceEnd = first;
            auto newIt = newFirst;
            
            if (replaceCount > 0) {
                auto newEnd = newFirst;
                std::advance(newEnd, replaceCount);
                replaceEnd = std::copy(newFirst, newEnd, first);
                newIt = newEnd;
            }
            
            if (newIt == newLast) {
                if (replaceEnd != last) {
                    container.erase(replaceEnd, last);
                }
            } else {
                auto remainingCount = std::distance(newIt, newLast);
                auto insertPos = std::distance(container.begin(), replaceEnd);
                auto oldContainerSize = container.size();
                
                container.resize(oldContainerSize + remainingCount);
                
                auto moveFrom = container.begin() + insertPos;
                auto moveTo = container.begin() + insertPos + remainingCount;
                auto moveEnd = container.begin() + oldContainerSize;
                
                std::move_backward(moveFrom, moveEnd, container.end());
                
                std::copy(newIt, newLast, container.begin() + insertPos);
            }
        }

    };

}

#endif //DIFFSCOPE_DSPX_MODEL_SPLICEHELPER_P_H
