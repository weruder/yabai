#include "display.h"

extern struct eventloop g_eventloop;
extern int g_connection;

static DISPLAY_EVENT_HANDLER(display_handler)
{
    struct event *event;

    if (flags & kCGDisplayAddFlag) {
        event_create(event, DISPLAY_ADDED, (void *)(intptr_t) display_id);
    } else if (flags & kCGDisplayRemoveFlag) {
        event_create(event, DISPLAY_REMOVED, (void *)(intptr_t) display_id);
    } else if (flags & kCGDisplayMovedFlag) {
        event_create(event, DISPLAY_MOVED, (void *)(intptr_t) display_id);
    } else if (flags & kCGDisplayDesktopShapeChangedFlag) {
        event_create(event, DISPLAY_RESIZED, (void *)(intptr_t) display_id);
    } else {
        event = NULL;
    }

    if (event) eventloop_post(&g_eventloop, event);
}

CFStringRef display_uuid(uint32_t display_id)
{
    CFUUIDRef uuid_ref = CGDisplayCreateUUIDFromDisplayID(display_id);
    CFStringRef uuid_str = CFUUIDCreateString(NULL, uuid_ref);
    CFRelease(uuid_ref);
    return uuid_str;
}

CGRect display_bounds(uint32_t display_id)
{
    return CGDisplayBounds(display_id);
}

CGRect display_bounds_constrained(uint32_t display_id)
{
    CGRect frame = display_bounds(display_id);
    CGRect menu  = space_manager_menu_bar_rect();
    CGRect dock  = space_manager_dock_rect();

    if (!space_manager_menu_bar_hidden()) {
        frame.origin.y    += menu.size.height;
        frame.size.height -= menu.size.height;
    }

    if (!space_manager_dock_hidden()) {
        switch (space_manager_dock_orientation()) {
        case DOCK_ORIENTATION_LEFT: {
        if (display_id == display_manager_left_display_id()) {
            frame.origin.x   += dock.size.width;
            frame.size.width -= dock.size.width;
        }
        } break;
        case DOCK_ORIENTATION_RIGHT: {
        if (display_id == display_manager_right_display_id()) {
            frame.size.width -= dock.size.width;
        }
        } break;
        case DOCK_ORIENTATION_BOTTOM: {
        if (display_id == display_manager_bottom_display_id()) {
            frame.size.height -= dock.size.height;
        }
        } break;
        }
    }

    return frame;
}

uint64_t display_space_id(uint32_t display_id)
{
    CFStringRef uuid = display_uuid(display_id);
    uint64_t sid = CGSManagedDisplayGetCurrentSpace(g_connection, uuid);
    CFRelease(uuid);
    return sid;
}

uint64_t *display_space_list(uint32_t display_id, int *count)
{
    uint64_t *space_list = NULL;

    CFStringRef uuid = display_uuid(display_id);
    CFArrayRef display_spaces_ref = SLSCopyManagedDisplaySpaces(g_connection);
    int display_spaces_count = CFArrayGetCount(display_spaces_ref);

    for (int i = 0; i < display_spaces_count; ++i) {
        CFDictionaryRef display_ref = CFArrayGetValueAtIndex(display_spaces_ref, i);
        CFStringRef identifier = CFDictionaryGetValue(display_ref, CFSTR("Display Identifier"));
        if (!CFEqual(uuid, identifier)) continue;

        CFArrayRef spaces_ref = CFDictionaryGetValue(display_ref, CFSTR("Spaces"));
        int spaces_count = CFArrayGetCount(spaces_ref);

        space_list = malloc(sizeof(uint64_t) * spaces_count);
        *count = spaces_count;

        for (int j = 0; j < spaces_count; ++j) {
            CFDictionaryRef space_ref = CFArrayGetValueAtIndex(spaces_ref, j);
            CFNumberRef sid_ref = CFDictionaryGetValue(space_ref, CFSTR("id64"));
            CFNumberGetValue(sid_ref, CFNumberGetType(sid_ref), &space_list[j]);
        }
    }

    CFRelease(display_spaces_ref);
    CFRelease(uuid);

    return space_list;
}

int display_arrangement(uint32_t display_id)
{
    int result = 0;
    CFStringRef uuid = display_uuid(display_id);
    CFArrayRef displays = SLSCopyManagedDisplays(g_connection);

    int displays_count = CFArrayGetCount(displays);
    for (int i = 0; i < displays_count; ++i) {
        if (CFEqual(CFArrayGetValueAtIndex(displays, i), uuid)) {
            result = i;
            break;
        }
    }

    CFRelease(displays);
    CFRelease(uuid);
    return result;
}