
#include "propertylabel.hpp"

#include <world/mobile.hpp>

#include <misc/log.hpp>

namespace uome {
namespace ui {
namespace components {

PropertyLabel::PropertyLabel(CL_GUIComponent* parent, const UnicodeString& link) : Label(parent), linkName_(link) {
}

void PropertyLabel::update(world::Mobile* mob) {
    setText(mob->getProperty(linkName_).asString());
}

}
}
}