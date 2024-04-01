# Hyprscroller

[Hyprscroller](https://github.com/dawsers/hyprscroller) is a
[Hyprland](https://hyprland.org) layout plugin that creates a window layout
similar to PaperWM [PaperWM](https://github.com/paperwm/PaperWM). I initialy
learned how to write a *Hyprland* plugin from [hyprslidr](https://gitlab.com/magus/hyprslidr),
which is a similar idea.

![Intro](./videos/hyprscroller.gif)

The plugin is quite feature complete and supports gaps, borders, special
workspace, full screen modes and installation through `hyprpm`.

I use *hyprscroller* on my main machine and will support it for as long as I
keep on using *Hyprland*. However, I will only add new features that I find
interesting, and support the *Hyprland* version my distribution uses. I have
found problematic compiling trunk versions of *Hyprland* on a system that
already has a system-wide version of it installed, so I will not make an extra
effort there until things improve in that front.


## Requirements

*hyprscroller* currently supports *Hyprland* tagged versions 0.35.0, 0.37.1,
and you can try your luck with the latest `git` changes, but I will be slower to keep
up with those.


## Building and installing

With Hyprland installed it should be as simple as running

``` sh
# builds a shared object hyprscroller.so
make all
# installs the shared library in ~/.config/hypr/plugins
make install
```

A more automated mode is to use `hyprpm`.

``` sh
hyprpm add https://github.com/dawsers/hyprscroller
# verify it installed correctly
hyprpm list
```

You can enable or disable it via `hyprpm enable hyprscroller` and
`hyprpm disable hyprscroller`.


## Configuration

If you are not using `hyprpm`, to make Hyprland load the plugin, add this to
your configuration.

``` conf
plugin = ~/.config/hypr/plugins/hyprscroller.so
```

Instead, if you use `hyprpm`, it should be as simple as adding this to
your `~/.config/hypr/hyprland.conf` :

``` conf
exec-once = hyprpm reload -n
```

To turn on the layout, use

``` conf
general {
    ...

    layout = scroller

    ...
}
```


## Dispatchers

The plugin adds the following dispatchers:

| Dispatcher              | Description                                                                                        |
|-------------------------|----------------------------------------------------------------------------------------------------|
| `scroller:movefocus`    | A replacement for `movefocus`, takes a direction as argument.                                      |
| `scroller:movewindow`   | A replacement for `movewindow`, takes a direction as argument.                                     |
| `scroller:cyclesize`    | Resize the focused window. Cycles through: one third, half, and two thirds of the screen size.     |
| `scroller:alignwindow`  | Align the focused window on the screen, `l/left`, `c/center`, `r/right`                            |
| `scroller:admitwindow`  | Push the current window into the bottom position of the column to its left.                        |
| `scroller:expelwindow`  | Pop the current window out of its column and place it to the right.                                |
| `scroller:resetheight`  | For a multi-window column, makes every window the same height.                                     |
| `scroller:toggleheight` | Toggle between `Auto` and `Free` height mode for the active column.                                |
| `scroller:fitwidth`     | Resize columns so they fit on the screen: `active`, `visible`, `all`, `toend`, `tobeg`             |


## Window/Column Focus and Movement

If you want to use *Hyprscroller* you will need to map your key bindings from
the default `movefocus`/`movewindow` to
`scroller:movefocus`/`scroller:movewindow`. The reason is *Hyprland* doesn't
have the concept (yet) of a workspace that spans more than the space of a
monitor, and when focusing directionally, it doesn't look for windows that are
"outside" of that region. If this changes in the future, these two dispatches
may become obsolete.

`movefocus` and `movewindow` accept the following directional arguments:
`l` or `left`, `r` or `right`, `u` or `up`, `d` or `dn` or `down`, `b` or
`begin` or `beginning`, `e` or `end`. So you can focus or move windows/columns
in a direction or to the beginning or end or the row.


## Resizing

`cyclesize` accepts an argument which is either `+1`/`1`/`next`, or
`-1`/`prev`/`previous`. It cycles forward or backward through three column
widths: one third, one half or two thirds of the available width of the
monitor. However, using `resizewindow`, you can modify the width or height of
any window freely.

When using `resizewindow`, the columns/windows can be resized in two different
modes:

- Columns: `cyclesize` will create three standard widths (1/3, 1/2, 2/3), but
  using `resizewindow` the column will be marked *free* and can be resized up
  to the width of the monitor.

- Windows: There are two modes for column windows, `Auto` and `Free`. The modes
  are toggled with the `toggleheight` dispatcher. `Auto` allows resizing, but
  all windows fit in the monitor, pushing against each other when resizing. In
  `Free` mode, you can resize at will. Each window will be limited to fit the
  monitor, but a full column of windows can be larger than the height of the
  monitor. The active window will always be visible when moving focus. You can
  use the `resetheight` dispatcher to move the column windows to `Auto` mode
  and split the total height in equal parts.


## Aligning

Columns are generally aligned in automatic mode, always making the active one
visible, and trying to make at least the previously focused one visible too.
Focus is *lazy*: if a column is visible, it doesn't move it when changing
focus or moving the window, so you can keep your eyes focused on that region of
the screen. However, you can always align any column to the *center*, *left*
or *right* of the monitor, for example center a column for easier reading,
regardless of what happens to the other columns.

`alignwindow` takes a parameter: `l` or `left`, `r` or `right`, and `c` or
`center` or `centre`.


## Admit/Expel

You can create columns of windows using `admitwindow`. It takes the active
window and moves it to the column left of its current one.

To expel any window from its current column and position it in a new column on
its right, use `expelwindow`.


## Fitting the Screen

When you have a ultra-wide monitor or the default column widths don't fit your
workflow, you can use manual resizing, but it is sometimes slow and tricky.

`scroller:fitwidth` allows you to re-fit the columns you want to the screen
extents. It accepts an argument related to the columns it will try to fit. The
new width of each column will be proportional to its previous width relative
to the other columns affected.

1. `active`: It is similar to maximize, it will fit the active column.
2. `visible`: All the currently fully or partially visible columns will be
   resized to fit the screen.
3. `all`: All the columns in the row will be resized to fit.
4. `toend`: All the columns from the focused one to the end of the row will be
   affected.
5. `tobeg` or `tobeginning`: All the columns from the focused on to the
   beginning of the row will now fit the screen.


## Key bindings

As an example, you could set some key bindings in your `hyprland.conf` like this:

``` conf
# Move focus with mainMod + arrow keys
bind = $mainMod, left, scroller:movefocus, l
bind = $mainMod, right, scroller:movefocus, r
bind = $mainMod, up, scroller:movefocus, u
bind = $mainMod, down, scroller:movefocus, d
bind = $mainMod, home, scroller:movefocus, begin
bind = $mainMod, end, scroller:movefocus, end

# Movement
bind = $mainMod CTRL, left, scroller:movewindow, l
bind = $mainMod CTRL, right, scroller:movewindow, r
bind = $mainMod CTRL, up, scroller:movewindow, u
bind = $mainMod CTRL, down, scroller:movewindow, d
bind = $mainMod CTRL, home, scroller:movewindow, begin
bind = $mainMod CTRL, end, scroller:movewindow, end

# Sizing keys
bind = $mainMod, equal, scroller:cyclesize, next
bind = $mainMod, minus, scroller:cyclesize, prev
bind = $mainMod SHIFT, equal, scroller:resetheight
bind = $mainMod SHIFT, minus, scroller:toggleheight

# Admit/Expel
bind = $mainMod, I, scroller:admitwindow,
bind = $mainMod, O, scroller:expelwindow,

# Center submap
# will switch to a submap called center
bind = $mainMod, C, submap, center
# will start a submap called "center"
submap = center
# sets repeatable binds for resizing the active window
bind = , C, scroller:alignwindow, c
bind = , C, submap, reset
bind = , right, scroller:alignwindow, r
bind = , right, submap, reset
bind = , left, scroller:alignwindow, l
bind = , left, submap, reset
# use reset to go back to the global submap
bind = , escape, submap, reset
# will reset the submap, meaning end the current one and return to the global one
submap = reset

# Resize submap
# will switch to a submap called resize
bind = $mainMod SHIFT, R, submap, resize
# will start a submap called "resize"
submap = resize
# sets repeatable binds for resizing the active window
binde = , right, resizeactive, 100 0
binde = , left, resizeactive, -100 0
binde = , up, resizeactive, 0 -100
binde = , down, resizeactive, 0 100
# use reset to go back to the global submap
bind = , escape, submap, reset
# will reset the submap, meaning end the current one and return to the global one
submap = reset

# Fitwidth submap
# will switch to a submap called fitwidth
bind = $mainMod, T, submap, fitwidth
# will start a submap called "fiwidth"
submap = fitwidth
# sets repeatable binds for resizing the active window
bind = , up, scroller:fitwidth, active
bind = , down, scroller:fitwidth, all
bind = , right, scroller:fitwidth, toend
bind = , left, scroller:fitwidth, tobeg
bind = , t, scroller:fitwidth, visible
# use reset to go back to the global submap
bind = , escape, submap, reset
# will reset the submap, meaning end the current one and return to the global one
submap = reset
```


## Specifications

This is how the plugin should work. Anything different is a bug.

### Creating a window.

The window should be created in a new column, to the right of the active one.
The window size should be: *width*: `OneHalf` (one half of the monitor's
available width), *height*: `Auto` (uses the monitor's available space).

### Moving focus, moving windows.

When moving focus in a direction, if the focus changes column, it should try
first to go to the column in that direction if there is one, or if not, to the
monitor in the same direction (if there is one). If none of the previous
considerations are true, the focus will cycle to the first/last column/window
on the monitor, depending on the direction of focus.

When entering a new column, the focus will go to the last active window in
that column (there is memory).

### Sizes/Resizing

Width and Height of a column are independent.

There are three states for `width`: `OneThird`, `OneHalf` and `TwoThirds`. You
can cycle forward or backwards among them.

`cyclesize` will resize the width of the active column from the default
`OneHalf`. When using `resizeactive`, the window will resize from its current
width, and move the column width state to `Free`.

There are two modes for `height`: `Auto` and `Free`.

`toggleheight` changes mode. The initial, default state is `Auto`. `Auto`
mode makes a column fit the monitor. Resizing any window will push it against
the others, so the column doesn't change height.

`Free` mode allows resizing windows independently, and make them at most as
tall as the monitor. No window is ever taller or wider than the monitor.
When in `Free` mode, the active window is always visible.

To reset all window sizes in a column, use `resetheight`. It will change the
`height` mode to `Auto` and split the available monitor height in equal parts
for each window.

### Standard Dispatchers

Aside from the *hyprscroller* specific ones, the layout supports *maximized*
and *fullscreen* windows, the *special* workspace, gaps, borders and most
*Hyprland* features. If something that should work, doesn't, please file an
issue report.

