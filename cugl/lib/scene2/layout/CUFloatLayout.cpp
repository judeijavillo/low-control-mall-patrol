//
//  CUFloatLayout.cpp
//  Cornell University Game Library (CUGL)
//
//  This module provides an support for a float layout.  Children in a float
//  layout are arranged in order, according to the layout orientation (horizontal
//  or vertical).  If there is not enough space in the Node for the children
//  to all be in the same row or column (depending on orientation), then the
//  later children wrap around to a new row or column.  This is the same way
//  that float layouts work in Java.
//
//  This class uses our standard shared-pointer architecture.
//
//  1. The constructor does not perform any initialization; it just sets all
//     attributes to their defaults.
//
//  2. All initialization takes place via init methods, which can fail if an
//     object is initialized more than once.
//
//  3. All allocation takes place via static constructors which return a shared
//     pointer.
//
//
//  CUGL MIT License:
//      This software is provided 'as-is', without any express or implied
//      warranty.  In no event will the authors be held liable for any damages
//      arising from the use of this software.
//
//      Permission is granted to anyone to use this software for any purpose,
//      including commercial applications, and to alter it and redistribute it
//      freely, subject to the following restrictions:
//
//      1. The origin of this software must not be misrepresented; you must not
//      claim that you wrote the original software. If you use this software
//      in a product, an acknowledgment in the product documentation would be
//      appreciated but is not required.
//
//      2. Altered source versions must be plainly marked as such, and must not
//      be misrepresented as being the original software.
//
//      3. This notice may not be removed or altered from any source distribution.
//
//  Author: Walker White and Enze Zhou
//  Version: 8/20/20
//
#include <cugl/scene2/layout/CUFloatLayout.h>
#include <algorithm>

using namespace cugl::scene2;

#define UNKNOWN_STR "<unknown>"

#pragma mark Constructors
/**
 * Creates a degenerate layout manager with no data.
 *
 * NEVER USE A CONSTRUCTOR WITH NEW. If you want to allocate an object on
 * the heap, use one of the static constructors instead.
 */
FloatLayout::FloatLayout() :
_alignment(Alignment::TOP_LEFT),
_horizontal(true) {
}

/**
 * Initializes a new layout manager with the given JSON specificaton.
 *
 * In addition to the 'type' attribute (which must be "float"), the JSON
 * specification supports the following attribute values:
 *
 *      "orientation":  One of 'horizontal' or 'vertical'
 *      "x_alignment":  One of 'left', 'center', or 'right'
 *      "y_alignment":  One of 'bottom', 'middle', or 'top'
 *
 * All attributes other than 'type' are optional.
 *
 * @param data      The JSON object specifying the node
 *
 * @return true if initialization was successful.
 */
bool FloatLayout::initWithData(const std::shared_ptr<JsonValue>& data) {
    std::string orient = data->getString("orientation",UNKNOWN_STR);
    _horizontal = !(orient == "vertical");
    
    std::string horz = data->getString("x_alignment","middle");
    std::string vert = data->getString("y_alignment","middle");
    _alignment = Alignment::TOP_LEFT;
    if (horz == "left") {
        if (vert == "top") {
            _alignment = Alignment::TOP_LEFT;
        } else if (vert == "bottom") {
            _alignment = Alignment::BOTTOM_LEFT;
        } else {
            _alignment = Alignment::MIDDLE_LEFT;
        }
    } else if (horz == "right") {
        if (vert == "top") {
            _alignment = Alignment::TOP_RIGHT;
        } else if (vert == "bottom") {
            _alignment = Alignment::BOTTOM_RIGHT;
        } else {
            _alignment = Alignment::MIDDLE_RIGHT;
        }
    } else if (horz == "center")  {
        if (vert == "top") {
            _alignment = Alignment::TOP_CENTER;
        } else if (vert == "bottom") {
            _alignment = Alignment::BOTTOM_CENTER;
        } else {
            _alignment = Alignment::CENTER;
        }
    }
    return true;
}


/**
 * Deletes the layout resources and resets all attributes.
 *
 * A disposed layout manager can be safely reinitialized.
 */
void FloatLayout::dispose() {
    _entries.clear();
    _priority.clear();
}


#pragma mark -
#pragma mark Layout
/**
 * Assigns layout information for a given key.
 *
 * The JSON object may contain any of the following attribute values:
 *
 *      "priority":     An int indicating placement priority.
 *                      Children with lower priority go first.
 *      "padding" :     A four-element float array.
 *                      It defines the padding on all sides between elements
 *
 * A child with no priority is put at the end. If there is already a child with
 * the given priority, then this method will fail.
 *
 * To look up the layout information of a scene graph node, we use the name
 * of the node.  This requires all nodes to have unique names. The
 * {@link SceneLoader} prefixes all child names by the parent name, so
 * this is the case in any well-defined JSON file. If the key is already
 * in use, this method will fail.
 *
 * @param key   The key identifying the layout information
 * @param data  A JSON object with the layout information
 *
 * @return true if the layout information was assigned to that key
 */
bool FloatLayout::add(const std::string key, const std::shared_ptr<JsonValue>& data) {
    auto search = _entries.find(key);
    if (search != _entries.end()) {
        CUAssertLog(false, "key '%s', is already in use", key.c_str());
        return false;
    }
    
    Entry entry;
    entry.priority = data->getLong("priority",-1L);
    if (data->has("padding")) {
        JsonValue* pad = data->get("padding").get();
        CUAssertLog(pad->size() >= 2,
                    "'padding' must be a four element number array");
        entry.pad_left   = pad->get(0)->asFloat(0.0f);
        entry.pad_right  = pad->get(2)->asFloat(0.0f);
        entry.pad_top    = pad->get(3)->asFloat(0.0f);
        entry.pad_bottom = pad->get(1)->asFloat(0.0f);
    } else {
        entry.pad_left   = 0;
        entry.pad_right  = 0;
        entry.pad_top    = 0;
        entry.pad_bottom = 0;
    }
    _entries[key] = entry;
    _priority.push_back(key);
    return true;
}

/**
 * Removes the layout information for a given key.
 *
 * To look up the layout information of a scene graph node, we use the name
 * of the node.  This requires all nodes to have unique names. The
 * {@link SceneLoader} prefixes all child names by the parent name, so
 * this is the case in any well-defined JSON file.
 *
 * If the key is not in use, this method will fail.
 *
 * @param key   The key identifying the layout information
 *
 * @return true if the layout information was removed for that key
 */
bool FloatLayout::remove(const std::string key) {
    auto it = _entries.find(key);
    if (it == _entries.end()) {
        return false;
    }
    _entries.erase(it);
    auto position = std::find(_priority.begin(), _priority.end(), key);
    if (position != _priority.end()) {
        _priority.erase(position);
    }
    return true;
}

/**
 * Performs a layout on the given node.
 *
 * This layout manager will searches for those children that are registered
 * with it. For those children, it repositions and/or resizes them according
 * to the layout information.
 *
 * This manager attaches a child node to one of nine "anchors" in the parent
 * (corners, sides or middle), together with a percentage (or absolute)
 * offset.  As the parent grows or shinks, the child will move according to
 * its anchor.  For example, nodes in the center will stay centered, while
 * nodes on the left side will move to keep the appropriate distance from
 * the left side. In fact, the stretching behavior is very similar to that
 * of a {@link NinePatch}.
 *
 * To look up the layout information of a scene graph node, this method uses
 * the name of the node.  This requires all nodes to have unique names. The
 * {@link SceneLoader} prefixes all child names by the parent name, so
 * this is the case in any well-defined JSON file.
 *
 * Children not registered with this layout manager are not affected.
 *
 * @param node  The scene graph node to rearrange
 */
void FloatLayout::layout(SceneNode* node) {
    if (_horizontal) {
        layoutHorizontal(node);
    } else {
        layoutVertical(node);
    }
}

#pragma mark -
#pragma mark Internal Helpers
/**
 * Performs a horizontal layout on the given node.
 *
 * This method is identical to {@link layout(Node*)} except that it overrides
 * the orientation settings of the layout manager; it always lays out the
 * children horizontally.
 *
 * @param node  The scene graph node to rearrange
 */
void FloatLayout::layoutHorizontal(SceneNode* node) {
    prioritize();
    Rect bounds = node->getLayoutBounds();
    Vec2 pos;
    Rect limit;
    
    // Running calculations of size
    std::vector<float> height;
    std::vector<float> width;
    std::vector<int> count;
    width.push_back(0.0f);
    height.push_back(0.0f);
    count.push_back(0);
    
    // Get the bounding box for the contents (ignoring alignment for now)
    bool stop = false;

    for(auto it = _priority.begin(); !stop && it != _priority.end(); ++it) {
        std::shared_ptr<SceneNode> child= node->getChildByName(*it);
        if (child) {
            Size extra = child->getSize();
            auto en = _entries.find(*it);
            if (en != _entries.end()) {
                extra.width  += en->second.pad_left + en->second.pad_right;
                extra.height += en->second.pad_top + en->second.pad_bottom;
            }
            if (extra.width > bounds.size.width) {
                stop = true;
            } else if (width.back()+extra.width > bounds.size.width) {
                if (height.back()+extra.height > bounds.size.height) {
                    stop = true;
                } else {
                    limit.size.width = std::max(limit.size.width,width.back());
                    limit.size.height += height.back();
                    width.push_back(extra.width);
                    height.push_back(extra.height);
                    count.push_back(1);
                }
            } else {
                width.back() += extra.width;
                height.back() = std::max(extra.height,height.back());
                count.back()++;
            }
        }
    }
    
    // Record the last
    limit.size.width = std::max(limit.size.width,width.back());
    limit.size.height += height.back();
    
    // Now do layout.
    switch(_alignment) {
        case Alignment::BOTTOM_LEFT:
            limit.origin = Vec2::ZERO;
            break;
        case Alignment::BOTTOM_CENTER:
            limit.origin = Vec2((bounds.size.width-limit.size.width)/2,0);
            break;
        case Alignment::BOTTOM_RIGHT:
            limit.origin = Vec2(bounds.size.width-limit.size.width/2,0);
            break;
        case Alignment::MIDDLE_LEFT:
            limit.origin = Vec2(0,(bounds.size.height-limit.size.height)/2);
            break;
        case Alignment::CENTER:
            limit.origin = Vec2((bounds.size.width-limit.size.width)/2,
                                (bounds.size.height-limit.size.height)/2);
            break;
        case Alignment::MIDDLE_RIGHT:
            limit.origin = Vec2(bounds.size.width-limit.size.width,
                                (bounds.size.height-limit.size.height)/2);
            break;
        case Alignment::TOP_LEFT:
            limit.origin = Vec2(0,bounds.size.height-limit.size.height);
            break;
        case Alignment::TOP_CENTER:
            limit.origin = Vec2((bounds.size.width-limit.size.width)/2,
                                 bounds.size.height-limit.size.height);
            break;
        case Alignment::TOP_RIGHT:
            limit.origin = Vec2(bounds.size.width-limit.size.width,
                                bounds.size.height-limit.size.height);
            break;
    }
    
    float ypos;
    switch(_alignment) {
        case Alignment::BOTTOM_LEFT:
        case Alignment::BOTTOM_CENTER:
        case Alignment::BOTTOM_RIGHT:
            ypos = bounds.origin.y+limit.size.height;
            break;
        case Alignment::MIDDLE_LEFT:
        case Alignment::CENTER:
        case Alignment::MIDDLE_RIGHT:
            ypos = bounds.origin.y+(bounds.size.height+limit.size.height)/2;
            break;
        case Alignment::TOP_LEFT:
        case Alignment::TOP_CENTER:
        case Alignment::TOP_RIGHT:
            ypos = bounds.origin.y+bounds.size.height;
            break;
    }
    
    auto jt = _priority.begin();
    for(size_t row = 0; row < count.size(); row++) {
        Vec2 pos; // Top LEFT corner of float.
        float xpos;
        float rowh = height[row];
        switch(_alignment) {
            case Alignment::BOTTOM_LEFT:
            case Alignment::MIDDLE_LEFT:
            case Alignment::TOP_LEFT:
                xpos = bounds.origin.x;
                break;
            case Alignment::BOTTOM_CENTER:
            case Alignment::CENTER:
            case Alignment::TOP_CENTER:
                xpos = bounds.origin.x+(bounds.size.width-width[row])/2;
                break;
            case Alignment::BOTTOM_RIGHT:
            case Alignment::MIDDLE_RIGHT:
            case Alignment::TOP_RIGHT:
                xpos = bounds.origin.x+bounds.size.width-width[row];
                break;
        }
        for(size_t col = 0; col < count[row]; col++) {
            std::shared_ptr<SceneNode> child = node->getChildByName(*jt);
            if (child) {
                Size tmp = child->getSize();
                float padl = 0;
                float padr = 0;
                float padb = 0;
                float padt = 0;
                auto en = _entries.find(*jt);
                if (en != _entries.end()) {
                    padl = en->second.pad_left;
                    padr = en->second.pad_right;
                    padb = en->second.pad_bottom;
                    padt = en->second.pad_top;
                }
                switch(_alignment) {
                    case Alignment::BOTTOM_LEFT:
                    case Alignment::BOTTOM_CENTER:
                    case Alignment::BOTTOM_RIGHT:
                        child->setAnchor(Vec2::ANCHOR_BOTTOM_LEFT);
                        child->setPosition(xpos+padl, ypos-rowh+padb);
                        break;
                    case Alignment::MIDDLE_LEFT:
                    case Alignment::CENTER:
                    case Alignment::MIDDLE_RIGHT:
                        child->setAnchor(Vec2::ANCHOR_MIDDLE_LEFT);
                        child->setPosition(xpos+padl,
                                           ypos-rowh/2+(padb-padt)/2);
                        break;
                    case Alignment::TOP_LEFT:
                    case Alignment::TOP_CENTER:
                    case Alignment::TOP_RIGHT:
                        child->setAnchor(Vec2::ANCHOR_TOP_LEFT);
                        child->setPosition(xpos+padl, ypos-padt);
                        break;
                }
                xpos += tmp.width+padl+padr;
            }
            ++jt;
        }
        ypos -= height[row];
    }
}

/**
 * Performs a vertical layout on the given node.
 *
 * This method is identical to {@link layout(Node*)} except that it overrides
 * the orientation settings of the layout manager; it always lays out the
 * children vertically.
 *
 * @param node  The scene graph node to rearrange
 */
void FloatLayout::layoutVertical(SceneNode* node) {
    prioritize();
    Rect bounds = node->getLayoutBounds();
    Vec2 pos;
    Rect limit;
    
    // Running calculations of size
    std::vector<float> height;
    std::vector<float> width;
    std::vector<int> count;
    width.push_back(0.0f);
    height.push_back(0.0f);
    count.push_back(0);
    
    
    // Get the bounding box for the contents (ignoring alignment for now)
    bool stop = false;
    for(auto it = _priority.begin(); !stop && it != _priority.end(); ++it) {
        std::shared_ptr<SceneNode> child = node->getChildByName(*it);
        if (child) {
            Size extra = child->getSize();
            auto en = _entries.find(*it);
            if (en != _entries.end()) {
                extra.width  += en->second.pad_left + en->second.pad_right;
                extra.height += en->second.pad_top + en->second.pad_bottom;
            }
            if (extra.height > bounds.size.height) {
                stop = true;
            } else if (height.back()+extra.height > bounds.size.height) {
                if (width.back()+extra.width > bounds.size.width) {
                    stop = true;
                } else {
                    limit.size.height = std::max(limit.size.height,height.back());
                    limit.size.width += width.back();
                    width.push_back(extra.width);
                    height.push_back(extra.height);
                    count.push_back(1);
                }
            } else {
                height.back() += extra.height;
                width.back() = std::max(extra.width,width.back());
                count.back()++;
            }
        }
    }
    
    // Record the last
    limit.size.height = std::max(limit.size.height,height.back());
    limit.size.width += width.back();
    
    // Now do layout.
    switch(_alignment) {
        case Alignment::BOTTOM_LEFT:
            limit.origin = Vec2::ZERO;
            break;
        case Alignment::BOTTOM_CENTER:
            limit.origin = Vec2((bounds.size.width-limit.size.width)/2,0);
            break;
        case Alignment::BOTTOM_RIGHT:
            limit.origin = Vec2(bounds.size.width-limit.size.width/2,0);
            break;
        case Alignment::MIDDLE_LEFT:
            limit.origin = Vec2(0,(bounds.size.height-limit.size.height)/2);
            break;
        case Alignment::CENTER:
            limit.origin = Vec2((bounds.size.width-limit.size.width)/2,
                                (bounds.size.height-limit.size.height)/2);
            break;
        case Alignment::MIDDLE_RIGHT:
            limit.origin = Vec2(bounds.size.width-limit.size.width,
                                (bounds.size.height-limit.size.height)/2);
            break;
        case Alignment::TOP_LEFT:
            limit.origin = Vec2(0,bounds.size.height-limit.size.height);
            break;
        case Alignment::TOP_CENTER:
            limit.origin = Vec2((bounds.size.width-limit.size.width)/2,
                                 bounds.size.height-limit.size.height);
            break;
        case Alignment::TOP_RIGHT:
            limit.origin = Vec2(bounds.size.width-limit.size.width,
                                bounds.size.height-limit.size.height);
        break;
    }
    
    float xpos;
    switch(_alignment) {
        case Alignment::BOTTOM_LEFT:
        case Alignment::MIDDLE_LEFT:
        case Alignment::TOP_LEFT:
            xpos = 0;
            break;
        case Alignment::BOTTOM_CENTER:
        case Alignment::CENTER:
        case Alignment::TOP_CENTER:
            xpos = bounds.origin.x+(bounds.size.width-limit.size.width)/2;
            break;
        case Alignment::BOTTOM_RIGHT:
        case Alignment::MIDDLE_RIGHT:
        case Alignment::TOP_RIGHT:
            xpos = bounds.origin.x+bounds.size.width-limit.size.width;
            break;
    }
    
    auto jt = _priority.begin();
    for(size_t col = 0; col < count.size(); col++) {
        Vec2 pos; // Top LEFT corner of float.
        float ypos;
        float colw = width[col];
        switch(_alignment) {
            case Alignment::BOTTOM_LEFT:
            case Alignment::BOTTOM_CENTER:
            case Alignment::BOTTOM_RIGHT:
                ypos = height[col];
                break;
            case Alignment::MIDDLE_LEFT:
            case Alignment::CENTER:
            case Alignment::MIDDLE_RIGHT:
                ypos = bounds.origin.y+(bounds.size.height+height[col])/2;
                break;
            case Alignment::TOP_LEFT:
            case Alignment::TOP_CENTER:
            case Alignment::TOP_RIGHT:
                ypos = bounds.origin.y+bounds.size.height;
                break;
        }
        
        for(size_t row = 0; row < count[col]; row++) {
            std::shared_ptr<SceneNode> child = node->getChildByName(*jt);
            if (child) {
                Size tmp = child->getSize();
                float padl = 0;
                float padr = 0;
                float padb = 0;
                float padt = 0;
                auto en = _entries.find(*jt);
                if (en != _entries.end()) {
                    padl = en->second.pad_left;
                    padr = en->second.pad_right;
                    padb = en->second.pad_bottom;
                    padt = en->second.pad_top;
                }
                switch(_alignment) {
                    case Alignment::BOTTOM_LEFT:
                    case Alignment::MIDDLE_LEFT:
                    case Alignment::TOP_LEFT:
                        child->setAnchor(Vec2::ANCHOR_TOP_LEFT);
                        child->setPosition(xpos+padl, ypos-padt);
                        child->setAnchor(Vec2::ANCHOR_BOTTOM_LEFT);
                        break;
                    case Alignment::BOTTOM_CENTER:
                    case Alignment::CENTER:
                    case Alignment::TOP_CENTER:
                        child->setAnchor(Vec2::ANCHOR_TOP_CENTER);
                        child->setPosition(xpos+colw/2+(padl-padr)/2,
                                           ypos-padt);
                        break;
                    case Alignment::BOTTOM_RIGHT:
                    case Alignment::MIDDLE_RIGHT:
                    case Alignment::TOP_RIGHT:
                        child->setAnchor(Vec2::ANCHOR_TOP_RIGHT);
                        child->setPosition(xpos+colw-padr, ypos-padt);
                        break;
                }

                ypos -= (tmp.height+padt+padb);
            }
            ++jt;
        }
        xpos += colw;
    }
}

/**
 * Computes the priority of the layout elements.
 *
 * This method resorts the contents of the priority
 * queue to match the current layout values.
 */
void FloatLayout::prioritize() {
    auto sortrule = [this] (const std::string& s1, const std::string& s2) -> bool {
        auto a = _entries.find(s1);
        auto b = _entries.find(s2);
        if (a == _entries.end()) {
            return (b == _entries.end() ? s1.compare(s2) < 0 : false);
        } else if (b == _entries.end()) {
            return true;
        } else if (a->second.priority < 0) {
            return (b->second.priority < 0 &&
                    b->second.priority < a->second.priority);
        } else if (b->second.priority < 0) {
            return true;
        } else {
            return (a->second.priority < b->second.priority);
        }
    };
    
    std::sort(_priority.begin(),_priority.end(),sortrule);
}
