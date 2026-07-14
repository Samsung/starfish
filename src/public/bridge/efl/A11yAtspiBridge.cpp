/*
 * Copyright (c) 2026-present Samsung Electronics Co., Ltd
 *
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Lesser General Public
 *  License as published by the Free Software Foundation; either
 *  version 2.1 of the License, or (at your option) any later version.
 *
 *  This library is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  Lesser General Public License for more details.
 *
 *  You should have received a copy of the GNU Lesser General Public
 *  License along with this library; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301
 *  USA
 */

#include "StarfishConfig.h"

#if defined(STARFISH_SHELL_EFL) && defined(STARFISH_ENABLE_A11Y_ATSPI)

#include "public/bridge/efl/A11yAtspiBridge.h"

#include "Starfish.h"
#include "core/page/A11yAtspiTreeSource.h"

#include <Ecore_Evas.h>
#include <atk/atk.h>
#include <atk-bridge.h>
#include <dbus/dbus.h>
#include <vconf/vconf.h>

// From libatspi (at-spi2-core): returns the process-wide singleton a11y bus
// connection - the same one atk-bridge serves our tree on. Declared here to
// avoid dragging in the full atspi headers.
extern "C" DBusConnection* atspi_get_a11y_bus(void);

#include <dlfcn.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <algorithm>
#include <map>
#include <set>
#include <vector>

// The accessibility TTS key lives in vconf-internal-setting-keys.h, which is
// not available on every profile. Fall back to the literal key.
#ifndef VCONFKEY_SETAPPL_ACCESSIBILITY_TTS
#define VCONFKEY_SETAPPL_ACCESSIBILITY_TTS "db/setting/accessibility/tts"
#endif
// Some settings apps (e.g. Family Hub) toggle only this preview twin of the
// key and never touch (or even delete) the base key, so both must be
// watched; accessibility is on when either reads true.
#define VCONFKEY_SETAPPL_ACCESSIBILITY_TTS_TEMPORARY \
    "db/setting/accessibility/tts/temporary"

namespace LWEDelegate {

// The convention shared with elm_atspi_bridge: an embedder looks up this evas
// object data key to find the AtkPlug id to embed into its AtkSocket.
static const char kPlugIdKey[] = "__PlugID";

static Evas_Object* g_window = nullptr;
// The elm widget standing in for the webview inside the host's widget tree;
// the host-side elm_atspi_bridge reads "__PlugID" off it and embeds our
// plug as that widget's accessibility child (socket).
static Evas_Object* g_accessWidget = nullptr;
// The evas object covering the web content area; the elm accessibility
// wrapper (elm_atspi_ewk_wrapper) tracks its geometry and reads "__PlugID"
// off it.
static Evas_Object* g_webviewObject = nullptr;
// Whether the elm-side wrapper embedding succeeded; when it did, the host's
// elm window owns window-activation and we must not claim it ourselves.
static bool g_elmEmbedded = false;
static AtkObject* g_plug = nullptr;
static bool g_bridgeInitialized = false;
static bool g_enabled = false;
// Element handle -> ATK wrapper. One cache-owned ref per wrapper; the same
// handle always maps to the same AtkObject so the daemon sees stable
// identities.
static std::map<void*, AtkObject*>* g_nodeCache = nullptr;
// Currently highlighted target. Must be reflected in ref_state_set:
// at-spi2-atk's GetNeighbor locates its DFS start point by looking for the
// node with ATK_STATE_HIGHLIGHTED; without it every swipe restarts from the
// root and returns the first element again.
static void* g_highlightedHandle = nullptr;

// Visible focus ring for the highlighted target. The device UA stylesheet
// compiles the generic *:focus outline out (product policy), so the web
// engine paints no ring; draw one as Evas rectangles over the webview
// instead, the same way elm/dali toolkits render the screen-reader
// highlight themselves. Four edge bars forming a frame.
// DA LCD Design Principle "Highlight box": 1-unit (4px) inner line within
// the focused component's area (same as touch target), #0381FE at 90%.
static Evas_Object* g_focusRing[4] = { nullptr, nullptr, nullptr, nullptr };
static const int kFocusRingThickness = 4;

static void focusRingHide()
{
    for (int i = 0; i < 4; i++) {
        if (g_focusRing[i]) {
            evas_object_hide(g_focusRing[i]);
        }
    }
}

static void focusRingShowAt(double x, double y, double width, double height)
{
    if (!g_window) {
        return;
    }
    Evas* evas = evas_object_evas_get(g_window);
    if (!evas) {
        return;
    }
    for (int i = 0; i < 4; i++) {
        if (!g_focusRing[i]) {
            g_focusRing[i] = evas_object_rectangle_add(evas);
            // #0381FE at 90% opacity; Evas takes premultiplied RGBA.
            evas_object_color_set(g_focusRing[i], 3, 116, 229, 230);
            evas_object_pass_events_set(g_focusRing[i], EINA_TRUE);
        }
    }
    int t = kFocusRingThickness;
    int ix = (int)x, iy = (int)y, iw = (int)width, ih = (int)height;
    // Inner line: bars sit inside the target bounds, side bars shortened so
    // the translucent corners don't double-blend.
    int vh = ih - 2 * t;
    if (vh < 0) {
        vh = 0;
    }
    evas_object_geometry_set(g_focusRing[0], ix, iy, iw, t);
    evas_object_geometry_set(g_focusRing[1], ix, iy + ih - t, iw, t);
    evas_object_geometry_set(g_focusRing[2], ix, iy + t, t, vh);
    evas_object_geometry_set(g_focusRing[3], ix + iw - t, iy + t, t, vh);
    for (int i = 0; i < 4; i++) {
        evas_object_raise(g_focusRing[i]);
        evas_object_show(g_focusRing[i]);
    }
}

static void focusRingDestroy()
{
    for (int i = 0; i < 4; i++) {
        if (g_focusRing[i]) {
            evas_object_del(g_focusRing[i]);
            g_focusRing[i] = nullptr;
        }
    }
}

static Starfish::A11yAtspiTreeSource* treeSource()
{
    if (!g_enabled) {
        return nullptr;
    }
    return Starfish::A11yAtspiTreeSource::current();
}

/////////////////////////////////////////////////////////////////////////////
// StarfishAtkPlug: AtkPlug subclass acting as the (childless, for now) root
// of this app's accessibility tree. Every vfunc logs so that D-Bus traffic
// from the screen-reader daemon is observable in dlog.
/////////////////////////////////////////////////////////////////////////////

extern "C" {

static AtkObject* wrapNode(void* handle);
static AtkObject* lookupNode(void* handle);

typedef struct _StarfishAtkPlug {
    AtkPlug parent;
} StarfishAtkPlug;

typedef struct _StarfishAtkPlugClass {
    AtkPlugClass parentClass;
} StarfishAtkPlugClass;

#define STARFISH_ATK_PLUG_TYPE (starfish_atk_plug_get_type())

static void starfish_atk_plug_component_iface_init(AtkComponentIface* iface);
static void starfish_atk_plug_window_iface_init(AtkWindowIface*);

// AtkWindow marker: the screen-reader daemon's app tracker follows
// window:activate events to decide which window subtree to build its
// navigation context from. Without it our app never becomes the "active
// window" and swipe navigation keeps running over the previous app's tree.
G_DEFINE_TYPE_WITH_CODE(
    StarfishAtkPlug, starfish_atk_plug, ATK_TYPE_PLUG,
    G_IMPLEMENT_INTERFACE(ATK_TYPE_COMPONENT,
                          starfish_atk_plug_component_iface_init)
        G_IMPLEMENT_INTERFACE(ATK_TYPE_WINDOW,
                              starfish_atk_plug_window_iface_init))

static void starfish_atk_plug_window_iface_init(AtkWindowIface*)
{
    // Signal-only interface; nothing to fill in.
}

static void starfish_atk_plug_init(StarfishAtkPlug*)
{
}

static const gchar* starfish_atk_plug_get_name(AtkObject*)
{
    return "Starfish";
}

static AtkRole starfish_atk_plug_get_role(AtkObject*)
{
    return ATK_ROLE_WINDOW;
}

static gint starfish_atk_plug_get_n_children(AtkObject*)
{
    Starfish::A11yAtspiTreeSource* source = treeSource();
    gint count = source ? (gint)source->rootChildCount() : 0;
    STARFISH_LOG_INFO("A11yAtspiBridge: get_n_children -> %d\n", (int)count);
    return count;
}

static AtkObject* starfish_atk_plug_ref_child(AtkObject*, gint index)
{
    Starfish::A11yAtspiTreeSource* source = treeSource();
    if (!source) {
        return nullptr;
    }
    void* handle = source->rootChildAt((size_t)index);
    STARFISH_LOG_INFO("A11yAtspiBridge: plug ref_child %d -> %p\n", (int)index,
                      handle);
    return wrapNode(handle);
}

static gint starfish_atk_plug_get_index_in_parent(AtkObject*)
{
    // AtkPlug is the only child of the embedding AtkSocket.
    return 0;
}

static AtkStateSet* starfish_atk_plug_ref_state_set(AtkObject* atkObject)
{
    AtkStateSet* stateSet = nullptr;
    if (ATK_OBJECT_CLASS(starfish_atk_plug_parent_class)->ref_state_set) {
        stateSet = ATK_OBJECT_CLASS(starfish_atk_plug_parent_class)
                       ->ref_state_set(atkObject);
    }
    if (!stateSet) {
        stateSet = atk_state_set_new();
    }
    atk_state_set_add_state(stateSet, ATK_STATE_ENABLED);
    atk_state_set_add_state(stateSet, ATK_STATE_SENSITIVE);
    atk_state_set_add_state(stateSet, ATK_STATE_SHOWING);
    atk_state_set_add_state(stateSet, ATK_STATE_VISIBLE);
    atk_state_set_add_state(stateSet, ATK_STATE_ACTIVE);
#if defined(STARFISH_ATK_HAS_GRAB_HIGHLIGHT)
    atk_state_set_add_state(stateSet, ATK_STATE_HIGHLIGHTABLE);
#endif
    return stateSet;
}

static void starfish_atk_plug_class_init(StarfishAtkPlugClass* klass)
{
    AtkObjectClass* atkObjectClass = ATK_OBJECT_CLASS(klass);
    atkObjectClass->get_name = starfish_atk_plug_get_name;
    atkObjectClass->get_role = starfish_atk_plug_get_role;
    atkObjectClass->get_n_children = starfish_atk_plug_get_n_children;
    atkObjectClass->ref_child = starfish_atk_plug_ref_child;
    atkObjectClass->get_index_in_parent = starfish_atk_plug_get_index_in_parent;
    atkObjectClass->ref_state_set = starfish_atk_plug_ref_state_set;
}

static void windowGeometry(gint* x, gint* y, gint* width, gint* height)
{
    int wx = 0, wy = 0, ww = 0, wh = 0;
    if (g_window) {
        evas_object_geometry_get(g_window, &wx, &wy, &ww, &wh);
    }
    if (x) {
        *x = wx;
    }
    if (y) {
        *y = wy;
    }
    if (width) {
        *width = ww;
    }
    if (height) {
        *height = wh;
    }
}

// Daemon coordinates (screen/window px) -> viewport-relative CSS px.
static void toClientCss(gint x, gint y, AtkCoordType coordType, double& clientX,
                        double& clientY)
{
    double px = x;
    double py = y;
    if (coordType == ATK_XY_SCREEN) {
        gint winX = 0, winY = 0;
        windowGeometry(&winX, &winY, nullptr, nullptr);
        px -= winX;
        py -= winY;
    }
    Starfish::A11yAtspiTreeSource* source = treeSource();
    double dpr = source ? source->devicePixelRatio() : 1.0;
    if (dpr <= 0) {
        dpr = 1.0;
    }
    clientX = px / dpr;
    clientY = py / dpr;
}

static AtkObject* starfish_atk_plug_ref_accessible_at_point(
    AtkComponent* component, gint x, gint y, AtkCoordType coordType)
{
    Starfish::A11yAtspiTreeSource* source = treeSource();
    if (source) {
        double clientX = 0, clientY = 0;
        toClientCss(x, y, coordType, clientX, clientY);
        void* handle = source->hitTest(clientX, clientY);
        STARFISH_LOG_INFO(
            "A11yAtspiBridge: ref_accessible_at_point (%d, %d) coordType %d "
            "-> client (%d, %d) handle %p\n",
            (int)x, (int)y, (int)coordType, (int)clientX, (int)clientY, handle);
        if (handle) {
            return wrapNode(handle);
        }
    }
    g_object_ref(component);
    return ATK_OBJECT(component);
}

static void starfish_atk_plug_get_extents(AtkComponent*, gint* x, gint* y,
                                          gint* width, gint* height,
                                          AtkCoordType)
{
    windowGeometry(x, y, width, height);
}

static void starfish_atk_plug_get_position(AtkComponent*, gint* x, gint* y,
                                           AtkCoordType)
{
    windowGeometry(x, y, nullptr, nullptr);
}

static void starfish_atk_plug_get_size(AtkComponent*, gint* width, gint* height)
{
    windowGeometry(nullptr, nullptr, width, height);
}

#if defined(STARFISH_ATK_HAS_GRAB_HIGHLIGHT)
static gboolean starfish_atk_plug_grab_highlight(AtkComponent* component)
{
    STARFISH_LOG_INFO("A11yAtspiBridge: grab_highlight\n");
    atk_object_notify_state_change(ATK_OBJECT(component), ATK_STATE_HIGHLIGHTED,
                                   TRUE);
    return TRUE;
}

static gboolean starfish_atk_plug_clear_highlight(AtkComponent* component)
{
    STARFISH_LOG_INFO("A11yAtspiBridge: clear_highlight\n");
    atk_object_notify_state_change(ATK_OBJECT(component), ATK_STATE_HIGHLIGHTED,
                                   FALSE);
    return TRUE;
}
#endif

static void starfish_atk_plug_component_iface_init(AtkComponentIface* iface)
{
    iface->ref_accessible_at_point = starfish_atk_plug_ref_accessible_at_point;
    iface->get_extents = starfish_atk_plug_get_extents;
    iface->get_position = starfish_atk_plug_get_position;
    iface->get_size = starfish_atk_plug_get_size;
#if defined(STARFISH_ATK_HAS_GRAB_HIGHLIGHT)
    iface->grab_highlight = starfish_atk_plug_grab_highlight;
    iface->clear_highlight = starfish_atk_plug_clear_highlight;
#endif
}

/////////////////////////////////////////////////////////////////////////////
// StarfishAtkNode: ATK wrapper for one engine-side accessibility target
// (opaque Element handle). Handles are revalidated by the tree source on
// every call, so a wrapper the daemon kept across a DOM change degrades to
// empty/no-op instead of touching a stale element.
/////////////////////////////////////////////////////////////////////////////

typedef struct _StarfishAtkNode {
    AtkObject parent;
    void* handle;
    gchar* name;
} StarfishAtkNode;

typedef struct _StarfishAtkNodeClass {
    AtkObjectClass parentClass;
} StarfishAtkNodeClass;

#define STARFISH_ATK_NODE_TYPE (starfish_atk_node_get_type())
#define STARFISH_ATK_NODE(obj) \
    (G_TYPE_CHECK_INSTANCE_CAST((obj), STARFISH_ATK_NODE_TYPE, StarfishAtkNode))

static void starfish_atk_node_component_iface_init(AtkComponentIface* iface);
static void starfish_atk_node_action_iface_init(AtkActionIface* iface);
static void starfish_atk_node_text_iface_init(AtkTextIface* iface);

G_DEFINE_TYPE_WITH_CODE(
    StarfishAtkNode, starfish_atk_node, ATK_TYPE_OBJECT,
    G_IMPLEMENT_INTERFACE(ATK_TYPE_COMPONENT,
                          starfish_atk_node_component_iface_init)
        G_IMPLEMENT_INTERFACE(ATK_TYPE_ACTION,
                              starfish_atk_node_action_iface_init)
            G_IMPLEMENT_INTERFACE(ATK_TYPE_TEXT,
                                  starfish_atk_node_text_iface_init))

static void starfish_atk_node_init(StarfishAtkNode* node)
{
    node->handle = nullptr;
    node->name = nullptr;
}

static void starfish_atk_node_finalize(GObject* object)
{
    StarfishAtkNode* node = STARFISH_ATK_NODE(object);
    g_free(node->name);
    node->name = nullptr;
    G_OBJECT_CLASS(starfish_atk_node_parent_class)->finalize(object);
}

static const gchar* starfish_atk_node_get_name(AtkObject* atkObject)
{
    StarfishAtkNode* node = STARFISH_ATK_NODE(atkObject);
    Starfish::A11yAtspiTreeSource* source = treeSource();
    if (!source) {
        return "";
    }
    auto name = source->nameOf(node->handle);
    g_free(node->name);
    node->name = g_strdup(name.data());
    STARFISH_LOG_INFO("A11yAtspiBridge: node get_name %p role %d -> \"%s\"\n",
                      node->handle, (int)source->roleOf(node->handle),
                      node->name);
    return node->name;
}

static AtkRole starfish_atk_node_get_role(AtkObject* atkObject)
{
    StarfishAtkNode* node = STARFISH_ATK_NODE(atkObject);
    Starfish::A11yAtspiTreeSource* source = treeSource();
    if (!source) {
        return ATK_ROLE_LABEL;
    }
    using Role = Starfish::A11yAtspiTreeSource::Role;
    switch (source->roleOf(node->handle)) {
    case Role::Button:
        return ATK_ROLE_PUSH_BUTTON;
    case Role::Link:
        return ATK_ROLE_LINK;
    case Role::Entry:
        return ATK_ROLE_ENTRY;
    case Role::CheckBox:
        return ATK_ROLE_CHECK_BOX;
    case Role::RadioButton:
        return ATK_ROLE_RADIO_BUTTON;
    case Role::ComboBox:
        return ATK_ROLE_COMBO_BOX;
    case Role::Image:
        return ATK_ROLE_IMAGE;
    case Role::Heading:
        return ATK_ROLE_HEADING;
    case Role::Section:
        return ATK_ROLE_SECTION;
    case Role::Document:
        return ATK_ROLE_DOCUMENT_WEB;
    case Role::Label:
    default:
        return ATK_ROLE_LABEL;
    }
}

// Hierarchy vfuncs mirror the engine-side tree (chromium-style): parents
// are the containers on the DOM ancestor chain, not the plug directly.
static AtkObject* starfish_atk_node_get_parent(AtkObject* atkObject)
{
    StarfishAtkNode* node = STARFISH_ATK_NODE(atkObject);
    Starfish::A11yAtspiTreeSource* source = treeSource();
    if (!source) {
        return g_plug;
    }
    void* parent = source->parentOf(node->handle);
    return parent ? lookupNode(parent) : g_plug;
}

static gint starfish_atk_node_get_n_children(AtkObject* atkObject)
{
    StarfishAtkNode* node = STARFISH_ATK_NODE(atkObject);
    Starfish::A11yAtspiTreeSource* source = treeSource();
    return source ? (gint)source->childCountOf(node->handle) : 0;
}

static AtkObject* starfish_atk_node_ref_child(AtkObject* atkObject, gint index)
{
    StarfishAtkNode* node = STARFISH_ATK_NODE(atkObject);
    Starfish::A11yAtspiTreeSource* source = treeSource();
    if (!source) {
        return nullptr;
    }
    return wrapNode(source->childAt(node->handle, (size_t)index));
}

static gint starfish_atk_node_get_index_in_parent(AtkObject* atkObject)
{
    StarfishAtkNode* node = STARFISH_ATK_NODE(atkObject);
    Starfish::A11yAtspiTreeSource* source = treeSource();
    if (!source) {
        return -1;
    }
    size_t index = source->indexInParentOf(node->handle);
    STARFISH_LOG_INFO("A11yAtspiBridge: node get_index_in_parent %p -> %d\n",
                      node->handle, index == SIZE_MAX ? -1 : (int)index);
    return index == SIZE_MAX ? -1 : (gint)index;
}

static AtkStateSet* starfish_atk_node_ref_state_set(AtkObject* atkObject)
{
    StarfishAtkNode* node = STARFISH_ATK_NODE(atkObject);
    AtkStateSet* stateSet = atk_state_set_new();
    Starfish::A11yAtspiTreeSource* source = treeSource();
    if (!source || !source->isValid(node->handle)) {
        STARFISH_LOG_INFO("A11yAtspiBridge: node ref_state_set %p -> DEFUNCT\n",
                          node->handle);
        atk_state_set_add_state(stateSet, ATK_STATE_DEFUNCT);
        return stateSet;
    }
    Starfish::A11yAtspiTreeSource::States states =
        source->statesOf(node->handle);
    if (!states.disabled) {
        atk_state_set_add_state(stateSet, ATK_STATE_ENABLED);
        atk_state_set_add_state(stateSet, ATK_STATE_SENSITIVE);
    }
    // SHOWING means "in view": scrolled/clipped-out targets keep VISIBLE
    // (not display:none) but drop SHOWING, as chromium reports offscreen.
    if (!states.offscreen) {
        atk_state_set_add_state(stateSet, ATK_STATE_SHOWING);
    }
    atk_state_set_add_state(stateSet, ATK_STATE_VISIBLE);
    // Only targets are focusable/highlightable; the daemon's navigation
    // uses these states to skip the structural Section/Document containers
    // (same effect as chromium's ignored-but-present nodes).
    if (source->isTarget(node->handle)) {
        atk_state_set_add_state(stateSet, ATK_STATE_FOCUSABLE);
#if defined(STARFISH_ATK_HAS_GRAB_HIGHLIGHT)
        atk_state_set_add_state(stateSet, ATK_STATE_HIGHLIGHTABLE);
#endif
    }
    if (node->handle == g_highlightedHandle) {
        atk_state_set_add_state(stateSet, ATK_STATE_HIGHLIGHTED);
    }
    if (states.checkable) {
        if (states.mixed) {
            atk_state_set_add_state(stateSet, ATK_STATE_INDETERMINATE);
        } else if (states.checked) {
            atk_state_set_add_state(stateSet, ATK_STATE_CHECKED);
        }
    }
    if (states.expandable) {
        atk_state_set_add_state(stateSet, ATK_STATE_EXPANDABLE);
        if (states.expanded) {
            atk_state_set_add_state(stateSet, ATK_STATE_EXPANDED);
        }
    }
    if (states.selectable) {
        atk_state_set_add_state(stateSet, ATK_STATE_SELECTABLE);
        if (states.selected) {
            atk_state_set_add_state(stateSet, ATK_STATE_SELECTED);
        }
    }
    return stateSet;
}

// aria-labelledby / aria-describedby as ATK relations (chromium exposes
// the same pairs). Only references to exposed nodes are handed out.
static void addRelations(AtkRelationSet* set,
                         Starfish::A11yAtspiTreeSource* source, void* handle,
                         bool describedBy, AtkRelationType type)
{
    std::vector<void*> handles = source->relationTargetsOf(handle, describedBy);
    if (handles.empty()) {
        return;
    }
    std::vector<AtkObject*> objects;
    for (void* target : handles) {
        AtkObject* obj = lookupNode(target);
        if (obj) {
            objects.push_back(obj);
        }
    }
    if (objects.empty()) {
        return;
    }
    AtkRelation* relation =
        atk_relation_new(objects.data(), (gint)objects.size(), type);
    atk_relation_set_add(set, relation);
    g_object_unref(relation);
}

static AtkRelationSet* starfish_atk_node_ref_relation_set(AtkObject* atkObject)
{
    StarfishAtkNode* node = STARFISH_ATK_NODE(atkObject);
    AtkRelationSet* set = atk_relation_set_new();
    Starfish::A11yAtspiTreeSource* source = treeSource();
    if (source && source->isValid(node->handle)) {
        addRelations(set, source, node->handle, false,
                     ATK_RELATION_LABELLED_BY);
        addRelations(set, source, node->handle, true,
                     ATK_RELATION_DESCRIBED_BY);
    }
    return set;
}

static void starfish_atk_node_class_init(StarfishAtkNodeClass* klass)
{
    GObjectClass* gobjectClass = G_OBJECT_CLASS(klass);
    AtkObjectClass* atkObjectClass = ATK_OBJECT_CLASS(klass);
    gobjectClass->finalize = starfish_atk_node_finalize;
    atkObjectClass->get_name = starfish_atk_node_get_name;
    atkObjectClass->get_role = starfish_atk_node_get_role;
    atkObjectClass->get_parent = starfish_atk_node_get_parent;
    atkObjectClass->get_n_children = starfish_atk_node_get_n_children;
    atkObjectClass->ref_child = starfish_atk_node_ref_child;
    atkObjectClass->get_index_in_parent = starfish_atk_node_get_index_in_parent;
    atkObjectClass->ref_state_set = starfish_atk_node_ref_state_set;
    atkObjectClass->ref_relation_set = starfish_atk_node_ref_relation_set;
}

// Border box in window px (window-relative device px); false if stale.
static bool handleWindowRect(void* handle, double& x, double& y, double& width,
                             double& height)
{
    Starfish::A11yAtspiTreeSource* source = treeSource();
    if (!source) {
        return false;
    }
    double cx = 0, cy = 0, cw = 0, ch = 0;
    if (!source->rectOf(handle, cx, cy, cw, ch)) {
        return false;
    }
    double dpr = source->devicePixelRatio();
    if (dpr <= 0) {
        dpr = 1.0;
    }
    x = cx * dpr;
    y = cy * dpr;
    width = cw * dpr;
    height = ch * dpr;
    return true;
}

static bool nodeWindowRect(StarfishAtkNode* node, double& x, double& y,
                           double& width, double& height)
{
    return handleWindowRect(node->handle, x, y, width, height);
}

// (Re)position the focus ring on the currently highlighted target - called
// on highlight moves, after scrolling, and from the change flush so the ring
// follows layout/scroll changes.
static void updateFocusRing()
{
    if (!g_highlightedHandle) {
        focusRingHide();
        return;
    }
    double x = 0, y = 0, width = 0, height = 0;
    if (handleWindowRect(g_highlightedHandle, x, y, width, height) &&
        width > 0 && height > 0) {
        focusRingShowAt(x, y, width, height);
    } else {
        focusRingHide();
    }
}

static void starfish_atk_node_get_extents(AtkComponent* component, gint* x,
                                          gint* y, gint* width, gint* height,
                                          AtkCoordType coordType)
{
    double wx = 0, wy = 0, ww = 0, wh = 0;
    if (!nodeWindowRect(STARFISH_ATK_NODE(component), wx, wy, ww, wh)) {
        if (x)
            *x = 0;
        if (y)
            *y = 0;
        if (width)
            *width = 0;
        if (height)
            *height = 0;
        return;
    }
    if (coordType == ATK_XY_SCREEN) {
        gint winX = 0, winY = 0;
        windowGeometry(&winX, &winY, nullptr, nullptr);
        wx += winX;
        wy += winY;
    }
    if (x)
        *x = (gint)wx;
    if (y)
        *y = (gint)wy;
    if (width)
        *width = (gint)ww;
    if (height)
        *height = (gint)wh;
    STARFISH_LOG_INFO(
        "A11yAtspiBridge: node get_extents %p coordType %d -> (%d, %d, %d, "
        "%d)\n",
        (void*)STARFISH_ATK_NODE(component)->handle, (int)coordType, (int)wx,
        (int)wy, (int)ww, (int)wh);
}

static void starfish_atk_node_get_position(AtkComponent* component, gint* x,
                                           gint* y, AtkCoordType coordType)
{
    starfish_atk_node_get_extents(component, x, y, nullptr, nullptr, coordType);
}

static void starfish_atk_node_get_size(AtkComponent* component, gint* width,
                                       gint* height)
{
    starfish_atk_node_get_extents(component, nullptr, nullptr, width, height,
                                  ATK_XY_WINDOW);
}

#if defined(STARFISH_ATK_HAS_GRAB_HIGHLIGHT)
static gboolean starfish_atk_node_grab_highlight(AtkComponent* component)
{
    StarfishAtkNode* node = STARFISH_ATK_NODE(component);
    STARFISH_LOG_INFO("A11yAtspiBridge: node grab_highlight %p\n",
                      node->handle);
    Starfish::A11yAtspiTreeSource* source = treeSource();
    if (!source || !source->isValid(node->handle)) {
        return FALSE;
    }
    source->highlight(node->handle);
    g_highlightedHandle = node->handle;
    updateFocusRing();
    atk_object_notify_state_change(ATK_OBJECT(component), ATK_STATE_HIGHLIGHTED,
                                   TRUE);
    return TRUE;
}

static gboolean starfish_atk_node_clear_highlight(AtkComponent* component)
{
    if (STARFISH_ATK_NODE(component)->handle == g_highlightedHandle) {
        g_highlightedHandle = nullptr;
        focusRingHide();
    }
    atk_object_notify_state_change(ATK_OBJECT(component), ATK_STATE_HIGHLIGHTED,
                                   FALSE);
    return TRUE;
}
#endif

static void starfish_atk_node_component_iface_init(AtkComponentIface* iface)
{
    iface->get_extents = starfish_atk_node_get_extents;
    iface->get_position = starfish_atk_node_get_position;
    iface->get_size = starfish_atk_node_get_size;
#if defined(STARFISH_ATK_HAS_GRAB_HIGHLIGHT)
    iface->grab_highlight = starfish_atk_node_grab_highlight;
    iface->clear_highlight = starfish_atk_node_clear_highlight;
#endif
}

// Activation is deferred to the next main-loop iteration so DOM event
// dispatch (which can run script) does not reenter from inside the D-Bus
// callback. The handle is revalidated at fire time.
static gboolean activateIdle(gpointer data)
{
    Starfish::A11yAtspiTreeSource* source = treeSource();
    if (source) {
        source->activate(data);
    }
    return G_SOURCE_REMOVE;
}

static gboolean starfish_atk_node_do_action(AtkAction* action, gint index)
{
    if (index != 0) {
        return FALSE;
    }
    StarfishAtkNode* node = STARFISH_ATK_NODE(action);
    STARFISH_LOG_INFO("A11yAtspiBridge: node do_action %p\n", node->handle);
    Starfish::A11yAtspiTreeSource* source = treeSource();
    // Structural containers expose no default action (as in chromium).
    if (!source || !source->isTarget(node->handle)) {
        return FALSE;
    }
    g_idle_add(activateIdle, node->handle);
    return TRUE;
}

static gint starfish_atk_node_get_n_actions(AtkAction*)
{
    return 1;
}

static const gchar* starfish_atk_node_action_get_name(AtkAction*, gint index)
{
    return index == 0 ? "activate" : nullptr;
}

static void starfish_atk_node_action_iface_init(AtkActionIface* iface)
{
    iface->do_action = starfish_atk_node_do_action;
    iface->get_n_actions = starfish_atk_node_get_n_actions;
    iface->get_name = starfish_atk_node_action_get_name;
}

// Minimal AtkText over the target's editable value (input/textarea), so the
// daemon can read entry contents. Offsets are character-based per the ATK
// contract; convert via glib's UTF-8 helpers.
static gchar* starfish_atk_node_text_get_text(AtkText* text, gint startOffset,
                                              gint endOffset)
{
    StarfishAtkNode* node = STARFISH_ATK_NODE(text);
    Starfish::A11yAtspiTreeSource* source = treeSource();
    if (!source) {
        return g_strdup("");
    }
    auto value = source->textOf(node->handle);
    const char* data = value.data();
    glong charCount = g_utf8_strlen(data, -1);
    if (startOffset < 0) {
        startOffset = 0;
    }
    if (endOffset < 0 || endOffset > charCount) {
        endOffset = (gint)charCount;
    }
    if (startOffset >= endOffset) {
        return g_strdup("");
    }
    const char* begin = g_utf8_offset_to_pointer(data, startOffset);
    const char* end = g_utf8_offset_to_pointer(data, endOffset);
    return g_strndup(begin, end - begin);
}

static gint starfish_atk_node_text_get_character_count(AtkText* text)
{
    StarfishAtkNode* node = STARFISH_ATK_NODE(text);
    Starfish::A11yAtspiTreeSource* source = treeSource();
    if (!source) {
        return 0;
    }
    auto value = source->textOf(node->handle);
    return (gint)g_utf8_strlen(value.data(), -1);
}

static gint starfish_atk_node_text_get_caret_offset(AtkText*)
{
    return 0;
}

static void starfish_atk_node_text_iface_init(AtkTextIface* iface)
{
    iface->get_text = starfish_atk_node_text_get_text;
    iface->get_character_count = starfish_atk_node_text_get_character_count;
    iface->get_caret_offset = starfish_atk_node_text_get_caret_offset;
}

// Cache-owned (borrowed) wrapper; the same handle always maps to the same
// AtkObject so the daemon sees stable identities across tree updates.
static AtkObject* lookupNode(void* handle)
{
    if (!handle) {
        return nullptr;
    }
    if (!g_nodeCache) {
        g_nodeCache = new std::map<void*, AtkObject*>();
    }
    auto it = g_nodeCache->find(handle);
    if (it != g_nodeCache->end()) {
        return it->second;
    }
    AtkObject* obj = ATK_OBJECT(g_object_new(STARFISH_ATK_NODE_TYPE, nullptr));
    STARFISH_ATK_NODE(obj)->handle = handle;
    (*g_nodeCache)[handle] = obj; // cache owns the initial ref
    return obj;
}

// Like lookupNode, but returns a new reference (for ref_child etc. whose
// ATK contract is transfer-full).
static AtkObject* wrapNode(void* handle)
{
    AtkObject* obj = lookupNode(handle);
    if (obj) {
        g_object_ref(obj);
    }
    return obj;
}

static void clearNodeCache()
{
    if (!g_nodeCache) {
        return;
    }
    for (auto& entry : *g_nodeCache) {
        g_object_unref(entry.second);
    }
    delete g_nodeCache;
    g_nodeCache = nullptr;
}

// Event-driven tree updates (chromium-style): engine hooks (DOM mutation,
// attribute change, control state change, scroll) ping onTreeSourceChanged
// via A11yAtspiTreeSource::setChangeListener; pings are coalesced into one
// flush that re-walks the tree and emits the difference as TRUTHFUL
// per-child add/remove events. at-spi2-atk keeps a cache of our children
// keyed by these signals, and a made-up "add at index 0" corrupts it (the
// daemon's neighbor search then runs over a broken sibling list and swipe
// navigation sticks).
static guint g_flushTimer = 0;

// Bridge-side copy of the exposed hierarchy from the previous flush.
// children is keyed by parent handle (nullptr = plug level) and has an
// entry for EVERY exposed node, so key presence doubles as a node
// existence check. states tracks the live widget states for
// state-change emissions.
struct TreeSnapshotData {
    std::map<void*, std::vector<void*>> children;
    std::map<void*, Starfish::A11yAtspiTreeSource::States> states;
};
static TreeSnapshotData* g_lastTree = nullptr;

static void captureChildren(Starfish::A11yAtspiTreeSource* source, void* handle,
                            TreeSnapshotData& out)
{
    std::vector<void*>& list = out.children[handle];
    size_t count =
        handle ? source->childCountOf(handle) : source->rootChildCount();
    for (size_t i = 0; i < count; i++) {
        void* child =
            handle ? source->childAt(handle, i) : source->rootChildAt(i);
        if (!child) {
            continue;
        }
        list.push_back(child);
        out.states[child] = source->statesOf(child);
        captureChildren(source, child, out);
    }
}

static gboolean flushTreeEvents(gpointer)
{
    g_flushTimer = 0;
    Starfish::A11yAtspiTreeSource* source = treeSource();
    if (!source || !g_plug) {
        return G_SOURCE_REMOVE;
    }
    TreeSnapshotData current;
    captureChildren(source, nullptr, current);

    // Keep the focus ring glued to the highlighted target across layout and
    // scroll changes.
    updateFocusRing();

    if (!g_lastTree) {
        // First flush after enabling: seed only, nothing to diff against.
        g_lastTree = new TreeSnapshotData(std::move(current));
        return G_SOURCE_REMOVE;
    }
    TreeSnapshotData& previous = *g_lastTree;

    // A pure reorder (same membership, different sibling order) would fall
    // through the membership diff below and leave the daemon's cached
    // sibling order stale; detect it by comparing the surviving items'
    // relative order and resync that parent wholesale (remove all old
    // children, re-add all new ones).
    auto survivorsReordered = [](const std::vector<void*>& oldList,
                                 const std::vector<void*>& newList) {
        std::vector<void*> oldSurvivors;
        std::vector<void*> newSurvivors;
        for (void* handle : oldList) {
            if (std::find(newList.begin(), newList.end(), handle) !=
                newList.end()) {
                oldSurvivors.push_back(handle);
            }
        }
        for (void* handle : newList) {
            if (std::find(oldList.begin(), oldList.end(), handle) !=
                oldList.end()) {
                newSurvivors.push_back(handle);
            }
        }
        return oldSurvivors != newSurvivors;
    };
    std::set<void*> resyncParents;

    // Removals first (old indices), per surviving parent. A parent that
    // vanished entirely is reported as a single remove under ITS parent;
    // its subtree needs no events of its own.
    for (const auto& entry : previous.children) {
        void* parent = entry.first;
        if (parent && current.children.find(parent) == current.children.end()) {
            continue;
        }
        const std::vector<void*>& oldList = entry.second;
        auto newIt = current.children.find(parent);
        const std::vector<void*>* newList =
            newIt != current.children.end() ? &newIt->second : nullptr;
        bool resync =
            newList && survivorsReordered(oldList, *newList);
        if (resync) {
            resyncParents.insert(parent);
        }
        // Descending order so each emitted index is still accurate after
        // the preceding (higher-index) siblings were removed.
        for (size_t j = oldList.size(); j > 0; j--) {
            size_t i = j - 1;
            void* handle = oldList[i];
            if (!resync && newList &&
                std::find(newList->begin(), newList->end(), handle) !=
                    newList->end()) {
                continue;
            }
            AtkObject* parentObj = parent ? lookupNode(parent) : g_plug;
            STARFISH_LOG_INFO(
                "A11yAtspiBridge: children-changed::remove parent %p index "
                "%u handle %p\n",
                parent, (unsigned)i, handle);
            g_signal_emit_by_name(parentObj, "children-changed::remove",
                                  (gint)i, lookupNode(handle));
        }
    }
    // Additions (new indices). A parent that is itself new was announced by
    // its own add; the daemon discovers its children lazily.
    for (const auto& entry : current.children) {
        void* parent = entry.first;
        if (parent &&
            previous.children.find(parent) == previous.children.end()) {
            continue;
        }
        const std::vector<void*>& newList = entry.second;
        auto oldIt = previous.children.find(parent);
        const std::vector<void*>* oldList =
            oldIt != previous.children.end() ? &oldIt->second : nullptr;
        bool resync = resyncParents.count(parent) != 0;
        for (size_t i = 0; i < newList.size(); i++) {
            void* handle = newList[i];
            if (!resync && oldList &&
                std::find(oldList->begin(), oldList->end(), handle) !=
                    oldList->end()) {
                continue;
            }
            AtkObject* parentObj = parent ? lookupNode(parent) : g_plug;
            STARFISH_LOG_INFO(
                "A11yAtspiBridge: children-changed::add parent %p index %u "
                "handle %p\n",
                parent, (unsigned)i, handle);
            g_signal_emit_by_name(parentObj, "children-changed::add", (gint)i,
                                  lookupNode(handle));
        }
    }
    // State transitions on surviving nodes (checked/indeterminate/
    // expanded/selected/enabled), mirroring chromium's state-change
    // notifications.
    for (const auto& entry : current.states) {
        auto oldIt = previous.states.find(entry.first);
        if (oldIt == previous.states.end()) {
            continue;
        }
        const auto& was = oldIt->second;
        const auto& now = entry.second;
        AtkObject* obj = nullptr;
        struct Transition {
            bool before;
            bool after;
            AtkStateType state;
        } transitions[] = {
            { was.checked, now.checked, ATK_STATE_CHECKED },
            { was.mixed, now.mixed, ATK_STATE_INDETERMINATE },
            { was.expanded && was.expandable, now.expanded && now.expandable,
              ATK_STATE_EXPANDED },
            { was.selected && was.selectable, now.selected && now.selectable,
              ATK_STATE_SELECTED },
            { !was.disabled, !now.disabled, ATK_STATE_ENABLED },
            { !was.disabled, !now.disabled, ATK_STATE_SENSITIVE },
            { !was.offscreen, !now.offscreen, ATK_STATE_SHOWING },
        };
        for (const Transition& t : transitions) {
            if (t.before == t.after) {
                continue;
            }
            if (!obj) {
                obj = lookupNode(entry.first);
            }
            STARFISH_LOG_INFO(
                "A11yAtspiBridge: state-change %d on %p -> %d\n",
                (int)t.state, entry.first, t.after ? 1 : 0);
            atk_object_notify_state_change(obj, t.state,
                                           t.after ? TRUE : FALSE);
        }
    }

    *g_lastTree = std::move(current);
    return G_SOURCE_REMOVE;
}

// Engine change listener. Runs synchronously inside DOM mutation paths, so
// only coalesce here: one pending 100ms one-shot batches event storms
// (page load, animations) into a single diff.
static void onTreeSourceChanged(void*)
{
    if (!g_enabled || g_flushTimer) {
        return;
    }
    g_flushTimer = g_timeout_add(100, flushTreeEvents, nullptr);
}

// The Tizen screen reader probes gestures with the Tizen-only D-Bus method
// org.a11y.atspi.Accessible.DoGesture ("should the app consume this
// gesture?"). elm apps answer it via elm_atspi_bridge, but at-spi2-atk has
// no handler, so the daemon got UnknownMethod and aborted its swipe
// navigation (navigator.c _gesture_is_consumed). Answer "not consumed" from
// a connection filter on the shared a11y bus so the daemon proceeds with its
// own flat navigation.
static bool g_dbusFilterAdded = false;

// GestureInfo wire format (iiiiiiu): type, startX, endX, startY, endY,
// state, eventTime. Types/states follow the Tizen accessibility gesture
// enums (dali accessibility-actions.h): TWO_FINGER_HOVER=1 is the
// two-finger pan; states BEGIN=0 ONGOING=1 ENDED=2 ABORTED=3.
static const dbus_int32_t kGestureTwoFingerHover = 1;
static bool g_panActive = false;
static dbus_int32_t g_panLastX = 0;
static dbus_int32_t g_panLastY = 0;

static dbus_bool_t handleA11yGesture(dbus_int32_t type, dbus_int32_t startX,
                                     dbus_int32_t endX, dbus_int32_t startY,
                                     dbus_int32_t endY, dbus_int32_t state)
{
    if (type != kGestureTwoFingerHover) {
        return FALSE;
    }
    // Two-finger pan scrolls the content under the screen reader. Events
    // carry the gesture-start and the current finger position; scroll by the
    // increment since the previous event, content following the finger.
    if (!g_panActive || state == 0 /* BEGIN */) {
        g_panActive = true;
        g_panLastX = startX;
        g_panLastY = startY;
    }
    double deltaX = (double)(endX - g_panLastX);
    double deltaY = (double)(endY - g_panLastY);
    g_panLastX = endX;
    g_panLastY = endY;
    if (state >= 2 /* ENDED / ABORTED */) {
        g_panActive = false;
    }
    Starfish::A11yAtspiTreeSource* source = treeSource();
    if (source && (deltaX != 0 || deltaY != 0)) {
        double dpr = source->devicePixelRatio();
        if (dpr <= 0) {
            dpr = 1.0;
        }
        source->scrollBy(-deltaX / dpr, -deltaY / dpr);
        updateFocusRing();
    }
    return TRUE;
}

static DBusHandlerResult a11yDbusFilter(DBusConnection* connection,
                                        DBusMessage* message, void*)
{
    if (!dbus_message_is_method_call(message, "org.a11y.atspi.Accessible",
                                     "DoGesture")) {
        return DBUS_HANDLER_RESULT_NOT_YET_HANDLED;
    }
    dbus_bool_t consumed = FALSE;
    dbus_int32_t type = 0, startX = 0, endX = 0, startY = 0, endY = 0,
                 state = 0;
    dbus_uint32_t eventTime = 0;
    if (dbus_message_get_args(message, nullptr, DBUS_TYPE_INT32, &type,
                              DBUS_TYPE_INT32, &startX, DBUS_TYPE_INT32, &endX,
                              DBUS_TYPE_INT32, &startY, DBUS_TYPE_INT32, &endY,
                              DBUS_TYPE_INT32, &state, DBUS_TYPE_UINT32,
                              &eventTime, DBUS_TYPE_INVALID)) {
        consumed = handleA11yGesture(type, startX, endX, startY, endY, state);
    }
    STARFISH_LOG_INFO(
        "A11yAtspiBridge: DoGesture type %d state %d (%d,%d)->(%d,%d) -> "
        "%s\n",
        (int)type, (int)state, (int)startX, (int)startY, (int)endX, (int)endY,
        consumed ? "consumed" : "not consumed");
    DBusMessage* reply = dbus_message_new_method_return(message);
    if (reply) {
        dbus_message_append_args(reply, DBUS_TYPE_BOOLEAN, &consumed,
                                 DBUS_TYPE_INVALID);
        dbus_connection_send(connection, reply, nullptr);
        dbus_message_unref(reply);
    }
    return DBUS_HANDLER_RESULT_HANDLED;
}

static AtkObject* starfishUtilGetRoot(void)
{
    return g_plug;
}

static const gchar* starfishUtilGetToolkitName(void)
{
    return "Starfish";
}

static const gchar* starfishUtilGetToolkitVersion(void)
{
    return "1.0";
}

} // extern "C"

/////////////////////////////////////////////////////////////////////////////
// Bridge lifecycle
/////////////////////////////////////////////////////////////////////////////

static void enableBridge()
{
    if (g_enabled || !g_window) {
        return;
    }
    STARFISH_LOG_INFO("A11yAtspiBridge: enabling\n");

    if (!g_plug) {
        g_plug = ATK_OBJECT(g_object_new(STARFISH_ATK_PLUG_TYPE, nullptr));
    }

    if (!g_bridgeInitialized) {
        if (atk_bridge_adaptor_init(nullptr, nullptr) == 0) {
            g_bridgeInitialized = true;
        } else {
            // The usual cause: running as root, whose access to the user
            // session bus (and thus the a11y bus) is rejected by policy.
            STARFISH_LOG_ERROR(
                "A11yAtspiBridge: atk_bridge_adaptor_init failed (uid %d)%s\n",
                (int)getuid(),
                getuid() == 0
                    ? " - the session bus rejects root; run the app as the "
                      "login user (e.g. su - owner) with "
                      "DBUS_SESSION_BUS_ADDRESS set"
                    : "");
            return;
        }
    }

    // Expose the plug id so an embedding toolkit's AtkSocket can pick it up
    // (same convention as chromium-efl / elm_atspi_bridge). It must sit on
    // the elm widget representing the webview: the host-side
    // elm_atspi_bridge reads "__PlugID" off widgets while building its
    // accessibility tree (plug_type_proxy_get) and embeds our tree there.
    gchar* plugId = atk_plug_get_id(ATK_PLUG(g_plug));
    if (plugId && *plugId) {
        Evas_Object* carriers[3] = { g_webviewObject, g_accessWidget,
                                     g_window };
        for (Evas_Object* carrier : carriers) {
            if (!carrier) {
                continue;
            }
            char* previous =
                static_cast<char*>(evas_object_data_get(carrier, kPlugIdKey));
            if (!previous || strcmp(previous, plugId) != 0) {
                if (previous) {
                    free(previous);
                }
                evas_object_data_set(carrier, kPlugIdKey, strdup(plugId));
            }
        }
        STARFISH_LOG_INFO("A11yAtspiBridge: plug id %s\n", plugId);
    } else {
        STARFISH_LOG_ERROR("A11yAtspiBridge: atk_plug_get_id failed\n");
    }

    if (!g_dbusFilterAdded) {
        DBusConnection* bus = atspi_get_a11y_bus();
        if (bus &&
            dbus_connection_add_filter(bus, a11yDbusFilter, nullptr, nullptr)) {
            g_dbusFilterAdded = true;
        } else {
            STARFISH_LOG_ERROR(
                "A11yAtspiBridge: could not install a11y bus filter\n");
        }
    }

    g_enabled = true;

    // Event-driven updates: the engine pings on every page change; seed the
    // bridge-side tree copy with an initial flush.
    Starfish::A11yAtspiTreeSource::setChangeListener(onTreeSourceChanged,
                                                     nullptr);
    onTreeSourceChanged(nullptr);

    // Preferred integration: hand the webview object to elementary's ewk
    // accessibility wrapper (the same Tizen hack chromium-efl rides on, see
    // efl_ui_widget.c / elm_atspi_ewk_wrapper.c). The wrapper reads
    // "__PlugID" off the object, creates a PLUG proxy and embeds our tree
    // as an accessibility child of the host widget - daemon navigation then
    // flows from the host's elm tree into the web content. Resolved via
    // dlsym: the symbol is a Tizen extension with no public header.
    if (!g_elmEmbedded && g_accessWidget && g_webviewObject) {
        typedef void (*A11yInitFn)(Evas_Object*, Evas_Object*);
        A11yInitFn a11yInit = reinterpret_cast<A11yInitFn>(
            dlsym(RTLD_DEFAULT, "elm_atspi_ewk_wrapper_a11y_init"));
        if (a11yInit) {
            a11yInit(g_accessWidget, g_webviewObject);
            g_elmEmbedded = true;
            STARFISH_LOG_INFO(
                "A11yAtspiBridge: embedded into elm accessibility tree\n");
        } else {
            STARFISH_LOG_INFO(
                "A11yAtspiBridge: elm_atspi_ewk_wrapper_a11y_init "
                "unavailable, falling back to standalone window\n");
        }
    }

    if (!g_elmEmbedded) {
        // Standalone fallback: announce ourselves as the active window
        // (deferred so the bridge has finished registering the plug on the
        // bus first), and re-announce once in case a native host app emits
        // its own window:activate after ours.
        g_idle_add(
            [](gpointer) -> gboolean {
                if (g_enabled && g_plug && !g_elmEmbedded) {
                    STARFISH_LOG_INFO(
                        "A11yAtspiBridge: emitting window create+activate\n");
                    g_signal_emit_by_name(g_plug, "create");
                    g_signal_emit_by_name(g_plug, "activate");
                }
                return G_SOURCE_REMOVE;
            },
            nullptr);
        g_timeout_add(
            3000,
            [](gpointer) -> gboolean {
                if (g_enabled && g_plug && !g_elmEmbedded) {
                    STARFISH_LOG_INFO(
                        "A11yAtspiBridge: re-emitting window activate\n");
                    g_signal_emit_by_name(g_plug, "activate");
                }
                return G_SOURCE_REMOVE;
            },
            nullptr);
    }
}

static void disableBridge()
{
    if (!g_enabled) {
        return;
    }
    STARFISH_LOG_INFO("A11yAtspiBridge: disabling\n");

    Evas_Object* carriers[3] = { g_webviewObject, g_accessWidget, g_window };
    for (Evas_Object* carrier : carriers) {
        if (!carrier) {
            continue;
        }
        char* plugId =
            static_cast<char*>(evas_object_data_get(carrier, kPlugIdKey));
        if (plugId) {
            evas_object_data_set(carrier, kPlugIdKey, nullptr);
            free(plugId);
        }
    }

    g_enabled = false;
    if (g_dbusFilterAdded) {
        DBusConnection* bus = atspi_get_a11y_bus();
        if (bus) {
            dbus_connection_remove_filter(bus, a11yDbusFilter, nullptr);
        }
        g_dbusFilterAdded = false;
    }
    Starfish::A11yAtspiTreeSource::setChangeListener(nullptr, nullptr);
    if (g_flushTimer) {
        g_source_remove(g_flushTimer);
        g_flushTimer = 0;
    }
    delete g_lastTree;
    g_lastTree = nullptr;
    g_highlightedHandle = nullptr;
    focusRingDestroy();
    clearNodeCache();

    if (g_plug) {
        g_object_unref(g_plug);
        g_plug = nullptr;
    }

    if (g_bridgeInitialized) {
        atk_bridge_adaptor_cleanup();
        g_bridgeInitialized = false;
    }
}

// Enabled is the OR of the base key and its settings-app "temporary" twin;
// false return means neither key was readable.
static bool readAccessibilityVconf(bool& enabled)
{
    int base = 0, temporary = 0;
    int baseOk = vconf_get_bool(VCONFKEY_SETAPPL_ACCESSIBILITY_TTS, &base);
    int temporaryOk = vconf_get_bool(
        VCONFKEY_SETAPPL_ACCESSIBILITY_TTS_TEMPORARY, &temporary);
    if (baseOk != 0 && temporaryOk != 0) {
        return false;
    }
    enabled =
        (baseOk == 0 && base == 1) || (temporaryOk == 0 && temporary == 1);
    return true;
}

static void onAccessibilityVconfChanged(keynode_t*, void*)
{
    bool enabled = false;
    if (!readAccessibilityVconf(enabled)) {
        STARFISH_LOG_ERROR(
            "A11yAtspiBridge: could not read accessibility vconf keys\n");
        return;
    }
    STARFISH_LOG_INFO("A11yAtspiBridge: accessibility vconf -> %d\n",
                      enabled ? 1 : 0);
    if (enabled) {
        enableBridge();
    } else {
        disableBridge();
    }
}

void A11yAtspiBridge::registerWindow(Evas_Object* window,
                                     Evas_Object* accessWidget,
                                     Evas_Object* webviewObject)
{
    if (g_window) {
        // Prototype supports a single window; keep the first registration.
        return;
    }
    g_window = window;
    g_accessWidget = accessWidget;
    g_webviewObject = webviewObject;
    STARFISH_LOG_INFO("A11yAtspiBridge: registerWindow\n");

    static bool atkUtilInitialized = false;
    if (!atkUtilInitialized) {
        atkUtilInitialized = true;
        AtkUtilClass* utilClass =
            ATK_UTIL_CLASS(g_type_class_ref(ATK_TYPE_UTIL));
        utilClass->get_root = starfishUtilGetRoot;
        utilClass->get_toolkit_name = starfishUtilGetToolkitName;
        utilClass->get_toolkit_version = starfishUtilGetToolkitVersion;
    }

    vconf_notify_key_changed(VCONFKEY_SETAPPL_ACCESSIBILITY_TTS,
                             onAccessibilityVconfChanged, nullptr);
    vconf_notify_key_changed(VCONFKEY_SETAPPL_ACCESSIBILITY_TTS_TEMPORARY,
                             onAccessibilityVconfChanged, nullptr);

    bool enabled = false;
    if (readAccessibilityVconf(enabled) && enabled) {
        enableBridge();
    }
}

void A11yAtspiBridge::unregisterWindow(Evas_Object* window)
{
    if (g_window != window) {
        return;
    }
    STARFISH_LOG_INFO("A11yAtspiBridge: unregisterWindow\n");
    vconf_ignore_key_changed(VCONFKEY_SETAPPL_ACCESSIBILITY_TTS,
                             onAccessibilityVconfChanged);
    vconf_ignore_key_changed(VCONFKEY_SETAPPL_ACCESSIBILITY_TTS_TEMPORARY,
                             onAccessibilityVconfChanged);
    disableBridge();
    g_window = nullptr;
    g_accessWidget = nullptr;
    g_webviewObject = nullptr;
    g_elmEmbedded = false;
}

} // namespace LWEDelegate

#endif
