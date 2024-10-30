#pragma once

#include <string>

namespace dispatchers {
    void addDispatchers();
    void dispatch_movefocus(std::string arg);
    void dispatch_movewindow(std::string arg);
}
