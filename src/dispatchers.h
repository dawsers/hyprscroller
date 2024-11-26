#ifndef SCROLLER_DISPATCHERS_H
#define SCROLLER_DISPATCHERS_H

#include <string>

namespace dispatchers {
    void addDispatchers();
    void dispatch_movefocus(std::string arg);
    void dispatch_movewindow(std::string arg);
}

#endif // SCROLLER_DISPATCHERS_H
