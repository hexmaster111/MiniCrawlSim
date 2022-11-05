// MiniCrawlSim.cpp : Defines the entry point for the application.
//

#define WINDOWS 1


#ifndef WINDOWS
#include <Arduino.h>
#endif


#if WINDOWS
#include <vector>
#include <iostream>
//Simulator stuff


// Serial.println(); replacement hackin thing
class DebugSerial
{
private:
	char m_key_in;

public:
	void println(const char* str)
	{
		std::cout << str << std::endl;
	}

	void begin(int baud)
	{
		std::cout << "Serial.begin(" << baud << ")" << std::endl;
	}

	void take_in_key(char key)
	{
		//todo: fake a key coming in
		m_key_in = key;
	}

	void printf(const char* fmt, ...)
	{
		//Im just gonna ignore this for now
	}


	// Two step process to get data into the amening, first mark the data as available, then provide it to read
	int available()
	{
		return m_key_in != 0;
	}

	char read()
	{
		char ret = m_key_in;
		m_key_in = 0;
		return ret;
	}
} Serial;


class Debug_Display
{
public:
	struct Color
	{
		Color(int r, int g, int b)
		{
			this->r = r;
			this->g = g;
			this->b = b;
		}

		uint8_t r;
		uint8_t g;
		uint8_t b;
	};

	// TODO: Draw this to a window
	void begin()
	{
	}

	void clear()
	{
	}

	void show()
	{
	};

	void setPixelColor(int row_col_to_index, Color color)
	{
	};
} pixels;

static class ESP
{
public:
	static long getFreeHeap()
	{
		return 100000;
	}
} ESP;

#endif

struct Location
{
	Location(int x, int y)
	{
		this->x = x;
		this->y = y;
	}

	int x;
	int y;
};

struct GameObjectColor
{
	GameObjectColor(int r, int g, int b)
	{
		this->r = r;
		this->g = g;
		this->b = b;
	}

	char r;
	char g;
	char b;
};

enum class Direction
{
	North,
	East,
	South,
	West,
	NorthEast,
	NorthWest,
	SouthEast,
	SouthWest
};

// Getting some objes into the game, its the way to define spesfic object, that could be made up from one of the base types,
// like normal_door and locked_door are the same door class.
enum class level_item
{
	wall,
	door_normal,
	door_locked,
	player
};

enum class player_item
{
	key,
};

struct LevelItem
{
	LevelItem(level_item type, int x, int y)
	{
		this->type = type;
		this->x = x;
		this->y = y;
	}

	level_item type;
	int x;
	int y;
};

class GameObject
{
private:
	bool can_interact = false;
	bool player_passable = false;
	level_item m_type;

public:
	GameObject(bool interactive, level_item levelItemType)
	{
		can_interact = interactive;
		m_type = levelItemType;
	}

	~GameObject()
	{
	};

	bool CanInteract() { return can_interact; }
	bool IsPlayerPassable() { return player_passable; }
	void set_is_player_passable(bool is_player_passable) { this->player_passable = is_player_passable; }
	level_item get_type() { return m_type; }

	virtual Location* GetLocation() = 0;
	/// @brief
	/// @return New color object
	virtual GameObjectColor* GetColor() = 0;

	virtual void Interact(GameObject* interactor)
	{
		Serial.println("This GameObject did not implement Interact()");
	};
};

class Player : public GameObject
{
private:
	Location* m_Location;

	std::vector<player_item>* m_inventory = new std::vector<player_item>();

	// IM A MEMORY LEAK (I think, delete what this returns)
	GameObjectColor* Health_to_color(uint8_t health)
	{
		if (health > 200)
			return new GameObjectColor(0, 1, 0);
		else if (health > 100)
			return new GameObjectColor(1, 1, 0);
		else
			return new GameObjectColor(1, 0, 0);
	}

	bool can_move(std::vector<GameObject*>* World_Game_Objects, Location new_location)
	{
		for (int i = 0; i < World_Game_Objects->size(); i++)
		{
			if (World_Game_Objects->at(i)->GetLocation()->x == new_location.x &&
				World_Game_Objects->at(i)->GetLocation()->y == new_location.y)
			{
				if (!World_Game_Objects->at(i)->IsPlayerPassable())
				{
					return false;
				}
			}
		}
		return true;
	}

public:
	Player(const int starting_x, const int starting_y, const uint8_t starting_health) : GameObject(
		false, level_item::player)
	{
		m_Location = new Location(starting_x, starting_y);
		m_PlayerHealth = starting_health;
	};

	void pick_up_item(player_item item)
	{
		m_inventory->push_back(item);
	}

	uint8_t m_PlayerHealth; // 0 - 255

	void start_action_select(std::vector<GameObject*>* world_game_objects)
	{
		Serial.println("Looking for things to be actions");
		// Lets get all the possible actions for the player, to do this, lets get all the objects around the player
		GameObject* objects_around_player[8] = {};

		// TODO: Get the objects around the player
		// For now lets do the easy way, looping over every object in the world to see if its around the player
		// Foreach object in the world,
		for (GameObject* game_object : *world_game_objects)
		{
			// Get the location of the object
			Location* object_location = game_object->GetLocation();
			// Check if the object is around the player
			if (object_location->x == m_Location->x + 1 && object_location->y == m_Location->y)
			{
				// If it is, add it to the array
				objects_around_player[0] = game_object;
			}
			else if (object_location->x == m_Location->x + 1 && object_location->y == m_Location->y + 1)
			{
				objects_around_player[1] = game_object;
			}
			else if (object_location->x == m_Location->x && object_location->y == m_Location->y + 1)
			{
				objects_around_player[2] = game_object;
			}
			else if (object_location->x == m_Location->x - 1 && object_location->y == m_Location->y + 1)
			{
				objects_around_player[3] = game_object;
			}
			else if (object_location->x == m_Location->x - 1 && object_location->y == m_Location->y)
			{
				objects_around_player[4] = game_object;
			}
			else if (object_location->x == m_Location->x - 1 && object_location->y == m_Location->y - 1)
			{
				objects_around_player[5] = game_object;
			}
			else if (object_location->x == m_Location->x && object_location->y == m_Location->y - 1)
			{
				objects_around_player[6] = game_object;
			}
			else if (object_location->x == m_Location->x + 1 && object_location->y == m_Location->y - 1)
			{
				objects_around_player[7] = game_object;
			}
		}

		// Once we have all the objects, we can check if they are interactable.
		for (int i = 0; i < 8; i++)
		{
			if (objects_around_player[i] != nullptr && objects_around_player[i]->CanInteract())
			{
				// Do something with the interactable object
				objects_around_player[i]->Interact(this);
				Serial.println("Interacted with object");
			}
		}
	}

	void move(std::vector<GameObject*>* world_objects, Direction direction)
	{
		switch (direction)
		{
		case Direction::North:
			if (can_move(world_objects, Location(m_Location->x, m_Location->y + 1)))
			{
				m_Location->y++;
			}
			break;
		case Direction::East:
			if (can_move(world_objects, Location(m_Location->x + 1, m_Location->y)))
			{
				m_Location->x++;
			}
			break;
		case Direction::South:
			if (can_move(world_objects, Location(m_Location->x, m_Location->y - 1)))
			{
				m_Location->y--;
			}
			break;
		case Direction::West:
			if (can_move(world_objects, Location(m_Location->x - 1, m_Location->y)))
			{
				m_Location->x--;
			}
			break;
		case Direction::NorthEast:
			if (can_move(world_objects, Location(m_Location->x + 1, m_Location->y + 1)))
			{
				m_Location->x++;
				m_Location->y++;
			}
			break;
		case Direction::NorthWest:
			if (can_move(world_objects, Location(m_Location->x - 1, m_Location->y + 1)))
			{
				m_Location->x--;
				m_Location->y++;
			}
			break;
		case Direction::SouthEast:
			if (can_move(world_objects, Location(m_Location->x + 1, m_Location->y - 1)))
			{
				m_Location->x++;
				m_Location->y--;
			}
			break;
		case Direction::SouthWest:
			if (can_move(world_objects, Location(m_Location->x - 1, m_Location->y - 1)))
			{
				m_Location->x--;
				m_Location->y--;
			}
			break;
		default:
			break;
		}
	}

	~Player()
	{
		delete m_Location;
		delete m_inventory;
	};

	Location* GetLocation() { return m_Location; }
	GameObjectColor* GetColor() { return Health_to_color(m_PlayerHealth); }
};

class Wall : public GameObject
{
public:
	Wall(const int x, const int y) : GameObject(false, level_item::wall)
	{
		m_Location = new Location(x, y);
	};

	~Wall()
	{
		delete m_Location;
	};

	Location* GetLocation() { return m_Location; }
	GameObjectColor* GetColor() { return new GameObjectColor(1, 1, 1); }

private:
	Location* m_Location;
};

class Door : public GameObject
{
public:
	Door(const int x, const int y, bool is_open, bool is_locked, level_item my_type) : GameObject(true, my_type)
	{
		m_Location = new Location(x, y);
		m_IsOpen = is_open;
		m_IsLocked = is_locked;
	};

	~Door()
	{
		delete m_Location;
	};

	void Interact(GameObject* interactor) override
	{
		// If its a player, check if the door is locked
		// If its locked, display a message saying its locked
		// If its not locked, check if its open
		// If its open, close it
		// If its closed, open it
		// If its not a player, do nothing

		if (interactor->get_type() == level_item::player)
		{
			if (m_IsLocked)
			{
				Serial.println("The door is locked");
				return;
			}

			// cast the interactor to a player
			Player* player = static_cast<Player*>(interactor);

			// Toggle the door
			m_IsOpen = !m_IsOpen;
			Serial.println("The door is now " + m_IsOpen ? "open" : "closed");
		}

		set_is_player_passable(m_IsOpen);
	}

	GameObjectColor* GetColor()
	{
		if (m_IsOpen)
			return new GameObjectColor(0, 1, 0);
		else if (m_IsLocked)
			return new GameObjectColor(1, 0, 0);
		else
			return new GameObjectColor(1, 1, 0);
	}

	Location* GetLocation() { return m_Location; }

private:
	bool m_IsOpen;
	bool m_IsLocked;
	Location* m_Location;
};

std::vector<LevelItem> dev_level = {
	// Left Wall
	LevelItem(level_item::wall, 0, 0),
	LevelItem(level_item::wall, 0, 1),
	LevelItem(level_item::wall, 0, 2),
	LevelItem(level_item::wall, 0, 3),
	LevelItem(level_item::wall, 0, 4),
	LevelItem(level_item::wall, 0, 5),
	LevelItem(level_item::wall, 0, 6),

	// Right Wall
	LevelItem(level_item::wall, 7, 0),
	LevelItem(level_item::wall, 7, 1),
	LevelItem(level_item::wall, 7, 2),
	LevelItem(level_item::wall, 7, 3),
	LevelItem(level_item::wall, 7, 4),
	LevelItem(level_item::wall, 7, 5),
	LevelItem(level_item::wall, 7, 6),

	// Bottom Wall
	LevelItem(level_item::wall, 1, 0),
	LevelItem(level_item::wall, 2, 0),
	LevelItem(level_item::wall, 3, 0),
	LevelItem(level_item::wall, 4, 0),
	LevelItem(level_item::wall, 5, 0),
	LevelItem(level_item::wall, 6, 0),

	// Two stagered boxes in the middle of the room
	LevelItem(level_item::wall, 2, 2),
	LevelItem(level_item::wall, 3, 3),

	// Top Wall, with door
	LevelItem(level_item::wall, 1, 6),
	LevelItem(level_item::wall, 2, 6),
	LevelItem(level_item::door_normal, 3, 6),
	LevelItem(level_item::wall, 4, 6),
	LevelItem(level_item::wall, 5, 6),
	LevelItem(level_item::wall, 6, 6),

	// hallway out of the door
	LevelItem(level_item::wall, 2, 5 + 6),
	LevelItem(level_item::wall, 2, 4 + 6),
	LevelItem(level_item::wall, 2, 3 + 6),
	LevelItem(level_item::wall, 2, 2 + 6),
	LevelItem(level_item::wall, 2, 1 + 6),

	// hallway out of the door
	LevelItem(level_item::wall, 4, 5 + 6),
	LevelItem(level_item::wall, 4, 4 + 6),
	LevelItem(level_item::wall, 4, 3 + 6),
	LevelItem(level_item::wall, 4, 2 + 6),
	LevelItem(level_item::wall, 4, 1 + 6),

	// Lets put a locked door at the end of this hallway
	LevelItem(level_item::door_locked, 3, 5 + 6),

};

// This feels a bit **RAW** but I do like the ide of this being the world, it just be a class to let us get the active
// things in it, let us doing some neat things
std::vector<GameObject*>* World_Game_Objects = new std::vector<GameObject*>();

void loadWorldState(std::vector<LevelItem>* level, std::vector<GameObject*>* out_world)
{
	for (int i = 0; i < level->size(); i++)
	{
		switch (level->at(i).type)
		{
		case level_item::wall:
			out_world->push_back(new Wall(level->at(i).x, level->at(i).y));
			break;
		case level_item::door_normal:
			out_world->push_back(new Door(level->at(i).x, level->at(i).y, false, false, level->at(i).type));
			break;
		case level_item::door_locked:
			out_world->push_back(new Door(level->at(i).x, level->at(i).y, false, true, level->at(i).type));
			break;
		}
	}
}

// this is where we put important things about the game
namespace World_object_helpers
{
	Player* PlayerPointer;
}

#ifndef WINDOWS
// Game Rendering Code
#include <Adafruit_NeoPixel.h>

Adafruit_NeoPixel pixels(32 * 8, 16, NEO_GRB);
#endif

int rowColToIndex(int x, int y)
{
	// if y is odd
	if (y % 2 == 1)
	{
		return (y * 8) + (7 - x);
	}
	else
	{
		return (y * 8) + x;
	}
}

void render_game_object(GameObject* game_object)
{
	Location* location = game_object->GetLocation();
	GameObjectColor* color = game_object->GetColor();

#ifndef WINDOWS
	pixels.setPixelColor(rowColToIndex(location->x, location->y), pixels.Color(color->r, color->g, color->b));
	delete color;
#endif

#if WINDOWS
	std::cout << "X: " << location->x << " Y: " << location->y << " R: " << color->r << " G: " << color->g << " B: " <<
		color->b << std::endl;
	delete color;
#endif
}

void setup()
{
	Serial.begin(9600);

#ifndef WINDOWS
	// Wait for serial to connect
	while (!Serial)
		;
#endif

	Serial.println("Starting up...");

	pixels.begin();

	World_object_helpers::PlayerPointer = new Player(4, 4, 255);
	World_Game_Objects->push_back(World_object_helpers::PlayerPointer);

	Serial.printf("Free Heap before level load: %d\r\n", ESP.getFreeHeap());
	loadWorldState(&dev_level, World_Game_Objects);
	Serial.printf("Free Heap after level load: %d\r\n", ESP.getFreeHeap());
}

void loop()
{
	// Read the key input
	if (Serial.available() > 0)
	{
		char key = Serial.read();
		// TODO: For now we pass in all the world objects, but we should only be passing in the ones around the player
		switch (key)
		{
		case '8':
			World_object_helpers::PlayerPointer->move(World_Game_Objects, Direction::North);
			break;
		case '2':
			World_object_helpers::PlayerPointer->move(World_Game_Objects, Direction::South);
			break;
		case '4':
			World_object_helpers::PlayerPointer->move(World_Game_Objects, Direction::West);
			break;
		case '6':
			World_object_helpers::PlayerPointer->move(World_Game_Objects, Direction::East);
			break;
		case '7':
			World_object_helpers::PlayerPointer->move(World_Game_Objects, Direction::NorthWest);
			break;
		case '9':
			World_object_helpers::PlayerPointer->move(World_Game_Objects, Direction::NorthEast);
			break;
		case '1':
			World_object_helpers::PlayerPointer->move(World_Game_Objects, Direction::SouthWest);
			break;
		case '3':
			World_object_helpers::PlayerPointer->move(World_Game_Objects, Direction::SouthEast);
			break;
		case '-':
			World_object_helpers::PlayerPointer->m_PlayerHealth--;
			break;
		case '+':
			World_object_helpers::PlayerPointer->m_PlayerHealth++;
			break;
		case '5':
			World_object_helpers::PlayerPointer->start_action_select(World_Game_Objects);
			break;
		}
		pixels.clear();
		for (int i = 0; i < World_Game_Objects->size(); i++)
		{
			render_game_object(World_Game_Objects->at(i));
		}
		pixels.show();
		Serial.printf("Free Heap: %d\r\n", ESP.getFreeHeap());
		// Im not worried, but this is 200k <- thats what copilot says
	}
}

#if WINDOWS

#include "MiniCrawlSim.h"
#include <Mahi/Gui.hpp>
#include <Mahi/Util.hpp>

using namespace mahi::gui;
using namespace mahi::util;

// Inherit from Application
class MiniCrawSim : public Application {

private:
	bool m_show_demo_window = false;

public:
	// 640x480 px window
	MiniCrawSim() : Application(640, 480, "My App") { }
	// Override update (called once per frame)
	void update() override {
		// App logic and/or ImGui code goes here

		{ //Demo window
			ImGui::Begin("Example");
			if (ImGui::Button("Press Me!"))
				print("Hello, World!");

			if (ImGui::Button("Show Demo"))
				m_show_demo_window = !m_show_demo_window;

			if (m_show_demo_window)
				ImGui::ShowDemoWindow(&m_show_demo_window);
			ImGui::End();
		}
	}
};



int main()
{
	MiniCrawSim app;
	app.run();
	return 0;
}

#endif