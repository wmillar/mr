#include "room.h"
#include "canvas.h"
#include "constants.h"
#include "entity.h"
#include "game_data.h"
#include "logger.h"
#include "resource_manager.h"
#include "sprite.h"
#include "sprite_sheet.h"
#include <cassert>


namespace RoomConnHelper {
	constexpr char sprNN[] = "nr_n";
	constexpr char sprNS[] = "nr_s";
	constexpr char sprNW[] = "nr_w";
	constexpr char sprNE[] = "nr_e";

	template<class T>	// T is array
	void procRoomConnecting(const T& array, std::vector<RoomConnection>& vec) {
		namespace rj = rapidjson;
		vec.reserve(array.Size() / 2);
		// process one pair at a time
		RoomConnection tmpRC;
		for (rj::Value::ConstValueIterator it = array.Begin(); it != array.End();) {
			tmpRC.pos.first = JSONHelper::getIntAndInc(it);
			tmpRC.pos.second = JSONHelper::getIntAndInc(it);
			vec.push_back(tmpRC);
		}
	}
}


namespace RoomHelper {
	// default SpriteSheet name
	constexpr char defSSName[] = "default";
}


RoomConnections::~RoomConnections() {
	// don't delete sprData
	for (std::size_t i = 0; i < 4; ++i) {
		for (auto& p : conn[i]) {
			SDL::freeNull(p.tex);
		}
	}
}


// Note: north/south grow horiz, west/east grow vert
void RoomConnections::draw(Canvas& can) {
	for (std::size_t i = 0; i < 4; ++i) {
		switch (static_cast<Side>(i)) {
		case Side::NORTH:
			drawNS(can, conn[i], 0, 0);
			break;
		case Side::SOUTH:
			drawNS(can, conn[i], 0, Constants::roomHeight - sprData->szNS.second);
			break;
		case Side::WEST:
			drawWE(can, conn[i], 0, 0);
			break;
		case Side::EAST:
			drawWE(can, conn[i], (Constants::roomWidth - sprData->szWE.first), 0);
			break;
		}
	}
}


void RoomConnections::set(rapidjson::Document& data, RoomConnSpriteData* sd) {
	sprData = sd;
	const rapidjson::Value& connections = data["conn"];
	RoomConnHelper::procRoomConnecting(connections["n"], conn[SideToIndex(Side::NORTH)]);
	RoomConnHelper::procRoomConnecting(connections["e"], conn[SideToIndex(Side::EAST)]);
	RoomConnHelper::procRoomConnecting(connections["s"], conn[SideToIndex(Side::SOUTH)]);
	RoomConnHelper::procRoomConnecting(connections["w"], conn[SideToIndex(Side::WEST)]);
}


// Generate textures
void RoomConnections::render() {
	//! TODO draw cleared when needed
	SDL_Rect dst;
	SDL_Surface* surf;
	Sprite spr;
	bool isNS;
	for (std::size_t i = 0; i < 4; ++i) {
		switch (static_cast<Side>(i)) {
		case Side::NORTH:
			spr = sprData->ss->get(RoomConnHelper::sprNN);
			isNS = true;
			break;
		case Side::SOUTH:
			spr = sprData->ss->get(RoomConnHelper::sprNS);
			isNS = true;
			break;
		case Side::WEST:
			spr = sprData->ss->get(RoomConnHelper::sprNW);
			isNS = false;
			break;
		case Side::EAST:
			spr = sprData->ss->get(RoomConnHelper::sprNE);
			isNS = false;
			break;
		}
		dst.x = 0;
		dst.y = 0;
		dst.w = spr.getDrawWidth();
		dst.h = spr.getDrawHeight();
		for (auto& p : conn[i]) {
			unsigned int count;
			if (isNS) {
				surf = SDL::newSurface32(p.pos.second - p.pos.first + 1, dst.h);
				count = static_cast<decltype(count)>(surf->w / dst.w);
				float cur = 0;
				float delta = (static_cast<float>(surf->w) / count);
				for (unsigned int j = 0; j < count; ++j) {
					if (j == (count-1)) {
						dst.x = (surf->w - dst.w);
						spr.blit(surf, &dst);
					}
					else {
						spr.blit(surf, &dst);
						cur += delta;
						dst.x = static_cast<int>(cur);
					}
				}
			}
			else {
				surf = SDL::newSurface32(dst.w, p.pos.second - p.pos.first + 1);
				count = static_cast<decltype(count)>(surf->h / dst.h);
				float cur = 0;
				float delta = (static_cast<float>(surf->h) / count);
				for (unsigned int j = 0; j < count; ++j) {
					if (j == (count-1)) {
						dst.y = (surf->h - dst.h);
						spr.blit(surf, &dst);
					}
					else {
						spr.blit(surf, &dst);
						cur += delta;
						dst.y = static_cast<int>(cur);
					}
				}
			}
			p.tex = SDL::toTexture(surf);
		}
	}
}


void RoomConnections::drawNS(Canvas& can, std::vector<RoomConnection>& vec, const int x, const int y) {
	SDL_Rect r;
	r.y = y;
	r.h = sprData->szNS.second;
	for (auto& p : vec) {
		r.x = (x + p.pos.first);
		r.w = (p.pos.second - p.pos.first + 1);
		can.draw(p.tex, &r);
	}
}


void RoomConnections::drawWE(Canvas& can, std::vector<RoomConnection>& vec, const int x, const int y) {
	SDL_Rect r;
	r.x = x;
	r.w = sprData->szWE.first;
	for (auto& p : vec) {
		r.y = (y + p.pos.first);
		r.h = (p.pos.second - p.pos.first + 1);
		can.draw(p.tex, &r);
	}
}


RoomStruct::~RoomStruct() {
	SDL::freeNull(bgSurf);
	SDL::freeNull(bgTex);
}


Room::Room() {
	sprData.ss = GameData::instance().resources->getSpriteSheet(RoomHelper::defSSName, true, false);
	Sprite sprConn = sprData.ss->get(RoomConnHelper::sprNN);
	sprData.szNS.first = sprConn.getDrawWidth();
	sprData.szNS.second = sprConn.getDrawHeight();
	sprConn = sprData.ss->get(RoomConnHelper::sprNW);
	sprData.szWE.first = sprConn.getDrawWidth();
	sprData.szWE.second = sprConn.getDrawHeight();
	drawRect.x = Constants::RoomX;
	drawRect.y = Constants::RoomY;
	drawRect.w = Constants::roomWidth;
	drawRect.h = Constants::roomHeight;
}


Room::~Room() {
	if (room != nullptr)
		delete room;
	GameData::instance().resources->freeSpriteSheet(RoomHelper::defSSName, true, false);
}


void Room::draw(Canvas& can) {
	assert(room != nullptr);
	can.draw(room->bgTex, &drawRect);
#if defined(DEBUG_ROOM_BLOCK) && DEBUG_ROOM_BLOCK
	room->block.draw(can);
#endif
	if (cleared) {
		can.setViewport(drawRect);
		room->connections.draw(can);
		can.clearViewport();
	}
}


void Room::set(rapidjson::Document& data) {
	namespace rj = rapidjson;
	assert(room == nullptr);
	// setup RoomStruct defaults
	room = new RoomStruct;
	room->block.setBounds(Rectangle{
		// block is 1 pixel longer on each side to add 4 block rects
		// to surround drawRect
		Constants::RoomX - 1,
		Constants::RoomY - 1,
		Constants::roomWidth + 2,
		Constants::roomHeight + 2
	});
	// add rectangles around edges of block
	const auto& bounds = room->block.getBounds();
	test.resize(bounds.width(), 1);
	test.move(bounds.getX(), bounds.getY());
	room->block.insert(test);	// top
	test.move(test.getX(), test.getY() + bounds.height() - 1);
	room->block.insert(test);	// bottom
	test.resize(1, bounds.height());
	test.move(bounds.getX(), bounds.getY());
	room->block.insert(test);	// left
	test.move(test.getX() + bounds.width() - 1, test.getY());
	room->block.insert(test);	// right
	{	// process block data
		SDL_Rect tmpRect;
		const rj::Value& block = data["block"];
		for (rj::Value::ConstValueIterator it = block.Begin(); it != block.End(); ++it) {
			rj::Value::ConstValueIterator it2 = it->Begin();
			JSONHelper::readRect(tmpRect, it2);
			test.resize(tmpRect.w, tmpRect.h);
			test.move(Constants::RoomX + tmpRect.x, Constants::RoomY + tmpRect.y);
			room->block.insert(test);
		}
	}
	// set background image
	room->bgSurf = renderBg(data["background"]);
	room->bgTex = SDL::newTexture(room->bgSurf);
	room->connections.set(data, &sprData);
}


bool Room::space(const int x, const int y, const int w, const int h) const {
	test.set(x, y, w, h);
	return !room->block.collides(test);
}


// attempt to move entity to (x, y)
void Room::updateEntity(GameEntity& entity, const int x, const int y) const {
	SDL_Rect rect = entity.getBounds();
	int currentX = rect.x;
	int currentY = rect.y;
	int deltaX = x - rect.x;
	int deltaY = y - rect.y;
	int increment;
	if (deltaX != 0) {
		// attempt to move along x axis
		increment = deltaX > 0 ? 1 : -1;
		while ((deltaX != 0) && space(currentX + increment, currentY, rect.w, rect.h)) {
			currentX += increment;
			deltaX -= increment;
		}
	}
	if (deltaY != 0) {
		// move along y axis
		increment = deltaY > 0 ? 1 : -1;
		while ((deltaY != 0) && space(currentX, currentY + increment, rect.w, rect.h)) {
			currentY += increment;
			deltaY -= increment;
		}
	}
	entity.setPos(currentX, currentY);
}


void Room::notifyClear() {
	room->connections.render();
	cleared = true;
}


// Render background image
SDL_Surface* Room::renderBg(const rapidjson::Value& data) {
	namespace rj = rapidjson;
	SDL_Rect dstRect;
	SDL_Surface* surf = SDL::newSurface24(Constants::roomWidth, Constants::roomHeight);
	SDL_FillRect(surf, nullptr, SDL::mapRGB(surf->format, COLOR_BLACK));
	Sprite spr;
	for (rj::Value::ConstValueIterator it = data.Begin(); it != data.End(); ++it) {
		spr = sprData.ss->get((*it)["name"].GetString());
		dstRect.w = spr.getDrawWidth();
		dstRect.h = spr.getDrawHeight();
		dstRect.x = (*it)["x"].GetInt();
		dstRect.y = (*it)["y"].GetInt();
		if (spr.blit(surf, &dstRect)) {
			// error drawing sprite, log error and continue
			SDL::logError("Room::renderBg blit");
		}
	}
	return surf;
}
