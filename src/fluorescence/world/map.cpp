
#include "map.hpp"

#include <data/manager.hpp>
#include <data/artloader.hpp>
#include <data/maptexloader.hpp>
#include <data/tiledataloader.hpp>

#include <ui/texture.hpp>
#include <ui/manager.hpp>

namespace fluo {
namespace world {

MapTile::MapTile() : IngameObject(IngameObject::TYPE_MAP), artId_(0), isFlat_(false) {
}

boost::shared_ptr<ui::Texture> MapTile::getIngameTexture() const {
    return texture_;
}

void MapTile::set(int locX, int locY, int locZ, unsigned int artId) {
    artId_ = artId;
    tileDataInfo_ = data::Manager::getTileDataLoader()->getLandTileInfo(artId_);

    // set surrounding z to safety values until they arrive
    zLeft_ = locZ;
    zRight_ = locZ;
    zBottom_ = locZ;

    if (artId_ <= 2) {
        setVisible(false);
    }

    // texture is not set here, but in updateTexture

    // do not print nodraw tiles
    if (artId_ <= 2) {
        setVisible(false);
    }

    calculateIsFlat();

    setLocation(locX, locY, locZ);
}

void MapTile::updateVertexCoordinates() {
    // top left corner of a rectangle spanning the whole texture
    int px = (getLocX() - getLocY()) * 22;
    int py = (getLocX() + getLocY()) * 22;

    if (isFlat_) {
        // flat tile or tile without texture
        py -= getLocZ() * 4;
        CL_Rectf rect(px, py, px + 44, py + 44);

        worldRenderData_.setVertexCoordinates(rect);

        //LOG_DEBUG(LOGTYPE_UI, "Pixel coords:");
        //for (unsigned int i = 0; i < 6; ++i) {
            //LOGARG_DEBUG(LOGTYPE_UI, "%u [ %.0f %.0f ]", i, vertexCoordinates_[i].x, vertexCoordinates_[i].y);
        //}
    } else {
        // stretched texture
        worldRenderData_.setVertexCoordinates(0, px + 22,   py - getLocZ() * 4);
        worldRenderData_.setVertexCoordinates(1, px,        py + 22 - zLeft_ * 4);
        worldRenderData_.setVertexCoordinates(2, px + 44,   py + 22 - zRight_ * 4);
        worldRenderData_.setVertexCoordinates(3, px,        py + 22 - zLeft_ * 4);
        worldRenderData_.setVertexCoordinates(4, px + 44,   py + 22 - zRight_ * 4);
        worldRenderData_.setVertexCoordinates(5, px + 22,   py + 44 - zBottom_ * 4);
    }
}

void MapTile::calculateIsFlat() {
    isFlat_ = (zLeft_ == zRight_ && zLeft_ == zBottom_ && zLeft_ == getLocZ()) || tileDataInfo_->textureId_ <= 0;
}

void MapTile::updateRenderDepth() {
    worldRenderData_.setRenderDepth(getLocX() + getLocY(), getLocZ(), 0, 0, 0);
}

void MapTile::updateTextureProvider() {
    calculateIsFlat();

    bool hasTexture = (bool)texture_;

    if (isFlat_) {
        texture_ = data::Manager::getArtLoader()->getMapTexture(artId_);
    } else {
        texture_ = data::Manager::getMapTexLoader()->get(tileDataInfo_->textureId_);
    }

    if (!hasTexture && texture_) {
        // was assigned a texture for the first time
        addToRenderQueue(ui::Manager::getWorldRenderQueue());
    }
}

bool MapTile::updateAnimation(unsigned int elapsedMillis) {
    // no animation on maptile
    return false;
}

void MapTile::setSurroundingZ(int left, int right, int bottom) {
    zLeft_ = left;
    zRight_ = right;
    zBottom_ = bottom;

    calculateIsFlat();

    invalidateVertexCoordinates();
}

unsigned int MapTile::getArtId() {
    return artId_;
}

const data::LandTileInfo* MapTile::getTileDataInfo() {
    return tileDataInfo_;
}

bool MapTile::isInDrawArea(int leftPixelCoord, int rightPixelCoord, int topPixelCoord, int bottomPixelCoord) const {
    //LOGARG_DEBUG(LOGTYPE_WORLD, "isInDrawArea (%u %u %u %u) => x=%u y=%u\n", leftPixelCoord, rightPixelCoord, topPixelCoord, bottomPixelCoord, vertexCoordinates_[0u].x, vertexCoordinates_[0u].y);

    if (isFlat_) {
        return IngameObject::isInDrawArea(leftPixelCoord, rightPixelCoord, topPixelCoord, bottomPixelCoord);
    } else {
        const CL_Vec3f* vc = worldRenderData_.getVertexCoordinates();
        return vc[1].x <= rightPixelCoord &&
                (vc[0].y <= bottomPixelCoord || vc[1].y <= bottomPixelCoord || vc[2].y <= bottomPixelCoord || vc[5].y <= bottomPixelCoord) &&
                vc[4].x >= leftPixelCoord &&
                (vc[0].y >= topPixelCoord || vc[1].y >= topPixelCoord || vc[2].y >= topPixelCoord || vc[5].y >= topPixelCoord);
    }
}

bool MapTile::hasPixel(int pixelX, int pixelY) const {
    if (isFlat_) {
        return IngameObject::hasPixel(pixelX, pixelY);
    } else {
        const CL_Vec3f* vc = worldRenderData_.getVertexCoordinates();
        return isPixelInside(pixelX, pixelY, vc[0], vc[2]) &&
                isPixelInside(pixelX, pixelY, vc[2], vc[5]) &&
                isPixelInside(pixelX, pixelY, vc[5], vc[3]) &&
                isPixelInside(pixelX, pixelY, vc[3], vc[0]);
    }

}

bool MapTile::isPixelInside(int pixelX, int pixelY, const CL_Vec2f& b, const CL_Vec2f& c) const {
    return (b.x * c.y + c.x * pixelY + pixelX * b.y - c.x*b.y - pixelX * c.y - b.x * pixelY) >= 0;
}

void MapTile::setVertexNormals(const CL_Vec3f& top, const CL_Vec3f& right, const CL_Vec3f& bottom, const CL_Vec3f& left) {
    if (isFlat_) {
        worldRenderData_.vertexNormals_[0] = CL_Vec3f(0, 0, 1);
        worldRenderData_.vertexNormals_[1] = CL_Vec3f(0, 0, 1);
        worldRenderData_.vertexNormals_[2] = CL_Vec3f(0, 0, 1);
        worldRenderData_.vertexNormals_[3] = CL_Vec3f(0, 0, 1);
        worldRenderData_.vertexNormals_[4] = CL_Vec3f(0, 0, 1);
        worldRenderData_.vertexNormals_[5] = CL_Vec3f(0, 0, 1);
    } else {
        worldRenderData_.vertexNormals_[0] = top;
        worldRenderData_.vertexNormals_[1] = left;
        worldRenderData_.vertexNormals_[2] = right;
        worldRenderData_.vertexNormals_[3] = left;
        worldRenderData_.vertexNormals_[4] = right;
        worldRenderData_.vertexNormals_[5] = bottom;
    }
}

void MapTile::onClick() {
    LOG_INFO << "Clicked map, id=" << std::hex << getArtId() << std::dec << " loc=(" << getLocX() << "/" << getLocY() << "/" <<
            getLocZ() << ") name=" << tileDataInfo_->name_ << " flat=" << isFlat_ << std::endl;

    //LOG_INFO << "z value: self=" << getLocZ() << " right=" << zRight_ << " bottom=" << zBottom_ << " left=" << zLeft_ << std::endl;
    printRenderDepth();
}


MapBlock::MapBlock() {
    for (unsigned int i = 0; i < 64; ++i) {
        tiles_[i].reset(new MapTile());
    }
}

boost::shared_ptr<MapTile> MapBlock::get(unsigned int x, unsigned int y) {
    if (x > 7) {
        x = 0;
    }

    if (y > 7) {
        y = 0;
    }

    return tiles_[(y*8) + x];
}

}
}
