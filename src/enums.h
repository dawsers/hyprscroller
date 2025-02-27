#ifndef SCROLLER_ENUMS_H
#define SCROLLER_ENUMS_H

#include <hyprutils/string/VarList.hpp>

#include <string>

enum class Direction { Left, Right, Up, Down, Begin, End, Center, Middle, Invalid };
enum class FitSize { Active, Visible, All, ToEnd, ToBeg };
enum class Mode { Row, Column };

class ModeModifier {
public:
    enum {
        POSITION_UNDEFINED,
        POSITION_AFTER,
        POSITION_BEFORE,
        POSITION_END,
        POSITION_BEGINNING
    };

    enum {
        FOCUS_UNDEFINED,
        FOCUS_FOCUS,
        FOCUS_NOFOCUS
    };

    enum {
        AUTO_UNDEFINED,
        AUTO_MANUAL,
        AUTO_AUTO
    };
    ModeModifier() : position(POSITION_UNDEFINED), focus(FOCUS_UNDEFINED), auto_mode(AUTO_UNDEFINED), auto_param(2) {}
    ModeModifier(const std::string &arg) : ModeModifier() {
        const auto args = CVarList(arg);
        if (args.size() > 0) {
            if (args[0] == "after")
                set_position(ModeModifier::POSITION_AFTER);
            else if (args[0] == "before")
                set_position(ModeModifier::POSITION_BEFORE);
            else if (args[0] == "end")
                set_position(ModeModifier::POSITION_END);
            else if (args[0] == "beginning" || args[0] == "beg")
                set_position(ModeModifier::POSITION_BEGINNING);

            if (args.size() > 1) {
                if (args[1] == "focus")
                    set_focus(ModeModifier::FOCUS_FOCUS);
                else if (args[1] == "nofocus")
                    set_focus(ModeModifier::FOCUS_NOFOCUS);

                if (args.size() > 2) {
                    if (args[2] == "manual")
                        set_auto_mode(ModeModifier::AUTO_MANUAL);
                    else if (args[2] == "auto") {
                        set_auto_mode(ModeModifier::AUTO_AUTO);

                        if (args.size() > 3) {
                            set_auto_param(std::stoi(args[3]));
                        }
                    }
                }
            }
        }
    }
    void set_position(int p) { position = p; }
    int get_position(bool force_default = true) const {
        if (force_default && position == POSITION_UNDEFINED)
            return POSITION_AFTER;
        return position;
    }
    std::string get_position_string() const {
        auto pos = get_position();
        switch (pos) {
        case POSITION_AFTER:
        default:
            return "after";
        case POSITION_BEFORE:
            return "before";
        case POSITION_END:
            return "end";
        case POSITION_BEGINNING:
            return "beginning";
        }
    }
    void set_focus(int f) { focus = f; }
    int get_focus(bool force_default = true) const {
        if (force_default && focus == FOCUS_UNDEFINED)
            return FOCUS_FOCUS;
        return focus;
    }
    std::string get_focus_string() const {
        if (get_focus() == FOCUS_FOCUS)
            return "focus";
        return "nofocus";
    }
    void set_auto_mode(int mode) { auto_mode = mode; }
    void set_auto_param(int param) { auto_param = param; }
    int get_auto_mode(bool force_default = true) const {
        if (force_default && auto_mode == AUTO_UNDEFINED)
            return AUTO_MANUAL;
        return auto_mode;
    }
    std::string get_auto_mode_string() const {
        if (get_auto_mode() == AUTO_MANUAL)
            return "manual";
        return "auto";
    }
    int get_auto_param() const { return auto_param; }

private:
    int position;
    int focus;
    int auto_mode;
    int auto_param;
};

#endif  // SCROLLER_ENUMS_H
