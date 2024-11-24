# Tutorial

[hyprscroller](https://github.com/dawsers/hyprscroller) is a [Hyprland](https://github.com/hyprwm/Hyprland)
plugin that creates a window layout similar to [PaperWM](https://github.com/paperwm/PaperWM).

Consult the [README](README.md) for more details beyond this quick tutorial.

## Rows and Columns

*hyprscroller* supports multiple workspaces per monitor. Each workspace is a
*row*: a scrollable tiling of columns, where each *column* may contain one or
more windows.

Your workspace is not limited by the size of the monitor; windows will have a
standard size and will scroll in and out of the view as they gain or lose focus.
This is an important difference with respect to other tiling window managers,
you always control the size of your windows, and if they are too big and not
in focus, they will just scroll out of view. There are ways to view all
windows at the same time, through [overview mode](#overview-mode), but more on that later.

There are two working modes that can be changed at any time:

1. *row*: it is the default. For each new window you open, it creates a new
   column containing only that window.
2. *column*: a new window will be placed in the current *column*,
   right below the active window.

If your monitor is in landscape mode (the usual one), you will probably work
mostly in *row* mode. If you are using a monitor in portrait mode, you may
prefer *column* mode. You can configure each monitor independently through
available options.

[rows-columns.mp4](https://github.com/user-attachments/assets/5b90d308-85e7-46a5-b8da-54e29fdbf46c)


## Window Sizes

There are options to define the default width and height of windows. These
sizes are usually a fraction of the available monitor space so it is easier to
fit an integer number of windows fully on screen at a time. If you use a regular
monitor, you may use fractions like `onehalf` or `onethird` to fit 2 or 3
windows. If you have an ultra-wide monitor, you may decide to use `onethird`,
`onefourth` or `onesisxth` to fit 3, 4 or 6 windows in the viewport at a time.

There are also complementary fractions like `twothirds`, `threequarters` etc.,
so for example you can fit a `onethird` and a `twothirds` window for the full
size of the monitor. Depending on the size and resolution of your monitor you
can decide which *fractions* are best suited for you, and configure them
through options.

You can toggle different window widths and heights in real-time using
`cyclesize`. It will go through your list of selected fractions defined in the
configuration, and resize the active window accordingly.

Of course you can also manually define exactly the size of a window with
*Hyprland's* `resizewindow`.

If you want to quickly fit a series of windows in a column or columns in a
row, you can use `fitsize`. It will ignore fractions, and re-scale all the
affected windows to fit in the screen.

[cyclesize-fitview.mp4](https://github.com/user-attachments/assets/1f51da15-3ef7-413a-8d1d-893a7355a4f1)


## Focusing and Alignment

When you change window focus, *hyprscroller* will always show the active
(focused) window on screen, and intelligently try to fit another adjacent
window if it can. However, in some cases you may want to decide to align a
window manually; `alignwindow` does exactly that.

[alignwindow.mp4](https://github.com/user-attachments/assets/2650b480-83c5-4e12-b963-13ff9a1df910)


## Overview Mode

When you have too many windows in a workspace, it can be hard to know where
things are. `toggleoverview` helps with that by creating a bird's eye view
of the whole workspace.

[overview-mode.mp4](https://github.com/user-attachments/assets/5f43ba62-ef97-4677-aa81-cf6169168bff)


## Jump Mode

Jump mode takes advantage of overview mode to provide a very quick way to focus on
any window on your monitors. Similar to [vim-easymotion](https://github.com/easymotion/vim-easymotion),
it overlays a label on each window. Pressing all the numbers of a label,
changes focus to the selected window.

1. Assign a key binding to `scroller:jump`, for this tutorial:

```
bind = $mainMod, slash, scroller:jump,
```

2. Pressing your `mod` key and `/` will show an overview of your windows on
   each monitor.
3. Now press the numbers shown on the overlay for the window you want to change
   focus to.
4. `jump` will exit, and the focus will be on the window you selected.


## Pinning a Column

Sometimes you want to keep one window static at all times, for example an
editor with a file you are working on, while letting other windows scroll in
the rest of the available space. These windows may include terminals or
documentation browser windows. You can `pin` a column, and it will stay at its
current location until you call `pin` again. To change the pinned column,
call `pin` twice; once to unpin the currently pinned column, and the second to
pin the active one.

[pin.mp4](https://github.com/user-attachments/assets/13f26203-3ece-47ea-91eb-779c9b2eab73)


## Copying/Pasting (Moving) Windows

To be able to re-organize windows and columns more easily, you can use
`selectiontoggle` to select windows. Those windows can be in any workspace,
and can also be selected from *overview* mode. After you are done with your
selection, go to the place where you want to move them (same or different
workspace), and call `selectionmove`. `selectionmove` needs an argument,
`right` is usually the most intuitive choice, because it will locate the
selection right of the active window of the current workspace.


## Marks

*hyprscroller* supports vim-like marks. You can use marks to quickly navigate
to frequently used windows, regardless of which workspace they are in. For
example, you can assign a key binding to a mark that focuses on your email
client. When you invoke `marksvisit`, *hyprscroller* will take you to that window,
even warping to a different workspace if necessary.

``` conf
# Create a window rule that assigns mark m to the main Thunderbird window when
# opened
windowrulev2 = plugin:scroller:marksadd m, class:(thunderbird), title:(Mozilla Thunderbird)$

# Create a submap to visit Thunderbird using 'm. You can add more.
bind = $mainMod, apostrophe, submap, marksvisit
submap = marksvisit
bind = , m, scroller:marksvisit, m
bind = , m, submap, reset
bind = , escape, submap, reset
submap = reset
```


## Trails and Trailmarks

Marks are good for quickly moving to some window, but you need to assign a
name to each one. Trailmarks are like anonymous marks that live in a list
called a trail. You can have as many trailmarks as you want in each trail, and
as many trails as you want. To navigate these trailmarks, you navigate the
list with previous/next dispatchers.

Read the section of the README for a more detailed explanation, possible key
bindings, and how to set IPC events to show trail and trailmark information on
your desktop bar.


## Touchpad Gestures

By default, *hyprscroller* supports three and four finger touchpad swipe
gestures to scroll windows, call *overview* and switch workspaces.


## Working in Full Screen Mode

Overview can be very helpful when you work in `fullscreen` mode. You can keep all your windows
maximized or in full screen mode, and toggle `overview` on/off to see your full desktop, and quickly
focus on a different application.

[fullscreen.mp4](https://github.com/user-attachments/assets/1afa3c37-905e-408d-8d44-6230c7e1edec)


## Example Configuration

The README includes a simple set of [key bindings](./README.md#key-bindings) to
minimally configure *hyprscroller*.
