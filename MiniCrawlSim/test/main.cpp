// MiniCrawlSim.cpp : Defines the entry point for the application.
//

// export LIBGL_ALWAYS_SOFTWARE = true

#define WINDOWS 1

#ifndef WINDOWS
#include <Arduino.h>
#endif

struct GameObjectColor
{
    GameObjectColor(int r, int g, int b)
    {
        this->r = r;
        this->g = g;
        this->b = b;
    }

    GameObjectColor()
    {
        this->r = 0;
        this->g = 0;
        this->b = 0;
    }

    char r;
    char g;
    char b;
};

#if WINDOWS
#include <vector>
#include <iostream>
// Simulator stuff

// Serial.println(); replacement hackin thing
class DebugSerial
{
private:
    char m_key_in;

public:
    void println(const char *str)
    {
        std::cout << str << std::endl;
    }

    void begin(int baud)
    {
        std::cout << "Serial.begin(" << baud << ")" << std::endl;
    }

    void take_in_key(char key)
    {
        // todo: fake a key coming in
        m_key_in = key;
    }

    void printf(const char *fmt, ...)
    {
        // Im just gonna ignore this for now
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

private:
    int m_height;
    int m_width;

    std::vector<std::vector<GameObjectColor>> *m_pixels;

public:
    Debug_Display(int width, int height)
    {
        m_height = height;
        m_width = width;
        m_pixels = new std::vector<std::vector<GameObjectColor>>(width, std::vector<GameObjectColor>(height));
    }

    void clear()
    {
        delete m_pixels;
        m_pixels = new std::vector<std::vector<GameObjectColor>>(m_width, std::vector<GameObjectColor>(m_height));
    }

    /// @brief Not using in the sim, just a placeholder to make the main thing happy
    void show() {}

    std::vector<std::vector<GameObjectColor>> *get_current_display_state()
    {
        return m_pixels;
    }

    void set_pixel(int x, int y, GameObjectColor color)
    {

        // Mirror the y axis
        y = m_height - y - 1;
        // x = m_width - x - 1;

        // Set the color in the pixels vector
        m_pixels->at(x).at(y) = color;
    }
} pixels(8, 32);

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

// Getting some objes into the game, its the way to define spesfic object,
// that could be made up from one of the base types, like normal_door and
// locked_door are the same door class. This is pretty much our item id system
enum class level_item
{
    player,
    wall,
    door_normal,
    door_locked,
    door_key,
    ITEM_TYPE_COUNT // This is a special one, it is used to count the number of items
};

#if WINDOWS
// To string methods
std::string to_string(Direction dir)
{
    switch (dir)
    {
    case Direction::North:
        return "North";
    case Direction::East:
        return "East";
    case Direction::South:
        return "South";
    case Direction::West:
        return "West";
    case Direction::NorthEast:
        return "NorthEast";
    case Direction::NorthWest:
        return "NorthWest";
    case Direction::SouthEast:
        return "SouthEast";
    case Direction::SouthWest:
        return "SouthWest";
    default:
        return "Unknown";
    }
}

std::string to_string(level_item item)
{
    switch (item)
    {
    case level_item::wall:
        return "wall";
    case level_item::door_normal:
        return "door_normal";
    case level_item::door_locked:
        return "door_locked";
    case level_item::player:
        return "player";
    case level_item::door_key:
        return "door_key";
    default:
        return "Unknown";
    }
}

#endif

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

    ~GameObject(){};

    bool CanInteract() { return can_interact; }
    bool IsPlayerPassable() { return player_passable; }
    void set_is_player_passable(bool is_player_passable) { this->player_passable = is_player_passable; }
    level_item get_type() { return m_type; }

    virtual Location *get_location() = 0;
    /// @brief
    /// @return New color object
    virtual GameObjectColor get_color() = 0;

    virtual void Interact(GameObject *interactor)
    {
        Serial.println("This GameObject did not implement Interact()");
    };
};

void remove_game_object_from_level(std::vector<GameObject *> *level, GameObject *obj)
{
    for (int i = 0; i < level->size(); i++)
    {
        if (level->at(i) == obj)
        {
            level->erase(level->begin() + i);
            return;
        }
    }
}

void remove_game_object_from_active_level(GameObject *obj);

/// @brief Base class for all items that the player can have in their inventory
class CPlayer_Item : public GameObject
{
public:
    CPlayer_Item(int x, int y, level_item item) : GameObject(true, item)
    {
        m_location = new Location(x, y);
        this->set_is_player_passable(true);
    };

    ~CPlayer_Item(){};

    // TODO: handle the get location
    Location *get_location() { return m_location; }

    virtual const char *get_name() = 0; // Im not sure how this is gonna be displayed yet

    void Interact(GameObject *interactor) override
    {
        OnPickup(interactor->get_location(), interactor);
        // Add the item to the player inventory
        Serial.println("Player item interact ran");

        remove_game_object_from_active_level(this);
        // throw "Not implemented";
    }

    virtual void OnPickup(Location *pickup_location, GameObject *interactor) = 0;
    virtual void OnDrop(Location *drop_location, GameObject *interactor) = 0;

private:
    Location *m_location;
};

class Player : public GameObject
{
private:
    Location *m_Location;

    std::vector<CPlayer_Item *> *m_inventory = new std::vector<CPlayer_Item *>();

    GameObjectColor Health_to_color(uint8_t health)
    {
        if (health > 200)
            return GameObjectColor(0, 1, 0);
        else if (health > 100)
            return GameObjectColor(1, 1, 0);
        else
            return GameObjectColor(1, 0, 0);
    }

    bool can_move(std::vector<GameObject *> *World_Game_Objects, Location new_location)
    {
        for (int i = 0; i < World_Game_Objects->size(); i++)
        {
            if (World_Game_Objects->at(i)->get_location()->x == new_location.x &&
                World_Game_Objects->at(i)->get_location()->y == new_location.y)
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

    // Handle with care
    std::vector<CPlayer_Item *> *get_inventory() { return m_inventory; }

    void pick_up_item(CPlayer_Item *item)
    {
        m_inventory->push_back(item);
    }

    uint8_t m_PlayerHealth; // 0 - 255

    void start_action_select(std::vector<GameObject *> *world_game_objects)
    {
        Serial.println("Looking for things to be actions");
        // Lets get all the possible actions for the player, to do this, lets get all the objects around the player
        GameObject *objects_around_player[8] = {};

        // TODO: Get the objects around the player
        // For now lets do the easy way, looping over every object in the world to see if its around the player
        // Foreach object in the world,
        for (GameObject *game_object : *world_game_objects)
        {
            // Get the location of the object
            Location *object_location = game_object->get_location();
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

    void move(std::vector<GameObject *> *world_objects, Direction direction)
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

    Location *get_location() { return m_Location; }
    GameObjectColor get_color() { return Health_to_color(m_PlayerHealth); }
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

    Location *get_location() { return m_Location; }
    GameObjectColor get_color() { return GameObjectColor(1, 1, 1); }

private:
    Location *m_Location;
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

    void Interact(GameObject *interactor) override
    {
        if (interactor->get_type() == level_item::player)
        {
            // cast the interactor to a player

            if (m_IsLocked)
            {
                Serial.println("The door is locked TODO: check for key");
                Player *player = static_cast<Player *>(interactor);
                // TODO: Add a check to see if the player has a key
                // Check if the player has a key in their inventory
                // If they do, unlock the door

                auto player_inventory = player->get_inventory();

                for (int i = 0; i < player_inventory->size(); i++)
                {
                    if (player_inventory->at(i)->get_type() == level_item::door_key)
                    {
                        m_IsLocked = false;
                        Serial.println("The door is unlocked");
                        // Remove the item from the inventory
                        player_inventory->erase(player_inventory->begin() + i);
                        break;
                    }
                }

                set_is_player_passable(m_IsOpen);
                return;
            }

            // Toggle the door
            m_IsOpen = !m_IsOpen;
            set_is_player_passable(m_IsOpen);
            Serial.println("The door is now " + m_IsOpen ? "open" : "closed");
        }
    }

    GameObjectColor get_color()
    {
        if (m_IsOpen)
            return GameObjectColor(0, 1, 0);
        else if (m_IsLocked)
            return GameObjectColor(1, 0, 0);
        else
            return GameObjectColor(1, 1, 0);
    }

    Location *get_location() { return m_Location; }
    bool *get_lock_state() { return &m_IsLocked; }
    bool *get_open_state() { return &m_IsOpen; }

private:
    bool m_IsOpen;
    bool m_IsLocked;
    Location *m_Location;
};

class CDoorKey final : public CPlayer_Item
{
public:
    CDoorKey(int x, int y) : CPlayer_Item(x, y, level_item::door_key){};

    // TODO: add some flavor text, a rusty key, a damp key, a slimy key, a key with a skull on it, etc.
    const char *get_name() override { return "Door Key"; }

    void OnPickup(Location *pickup_location, GameObject *interactor) override
    {
        // Put item in the player's inventory

        // Cast the interactor to a player
        Player *player = static_cast<Player *>(interactor);
        if (player != nullptr)
            player->get_inventory()->push_back(this);

        // TODO: Pipe stuff into the noise system, and log
    };
    void OnDrop(Location *drop_location, GameObject *interactor) override{};

    // TODO: handle the gameobject get color
    GameObjectColor get_color() override { return GameObjectColor(1, 0, 1); }
};

std::vector<LevelItem>
    dev_level = {

        // spawn point
        LevelItem(level_item::player, 1, 1),

        // Test key
        LevelItem(level_item::door_key, 2, 2),

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

        // hallway out of the door
        LevelItem(level_item::wall, 4, 5 + 6),
        LevelItem(level_item::wall, 4, 4 + 6),

        // Lets put a locked door at the end of this hallway
        LevelItem(level_item::door_locked, 3, 5 + 6),

};

// This feels a bit **RAW** but I do like the ide of this being the world,
// it just be a class to let us get the active
// things in it, let us doing some neat things
std::vector<GameObject *> *World_Game_Objects = new std::vector<GameObject *>();

// this is where we put important things about the game
namespace neat_world_objects
{
    Player *player_pointer;
}

void remove_game_object_from_active_level(GameObject *obj)
{
    Serial.println("Remove obj from level");
    for (int i = 0; i < World_Game_Objects->size(); i++)
    {
        if (World_Game_Objects->at(i) == obj)
        {
            World_Game_Objects->erase(World_Game_Objects->begin() + i);
            return;
        }
    }
}

GameObject *build_game_object(LevelItem *item) // This may not need to take in a levelItem ptr. Not sure yet...
{
    switch (item->type)
    {
    case level_item::wall:
        return new Wall(item->x, item->y);
    case level_item::door_normal:
        return new Door(item->x, item->y, false, false, item->type);
    case level_item::door_locked:
        return new Door(item->x, item->y, false, true, item->type);
    case level_item::player:
        neat_world_objects::player_pointer = static_cast<Player *>(new Player(item->x, item->y, 255));
        return neat_world_objects::player_pointer; // Little funny stuff here, saving the player so we know
        // what object it is. TODO: Many players support
    case level_item::door_key:
        return new CDoorKey(item->x, item->y);

    default:
        throw "I cant make what i dont know into a game object silly :P";
    }
}

void load_world_state(std::vector<LevelItem> *items, std::vector<GameObject *> *out_world)
{
    for (int i = 0; i < items->size(); i++)
    {
        out_world->push_back(build_game_object(&items->at(i)));
    }
}

#ifndef WINDOWS
// Game Rendering Code
#include <Adafruit_NeoPixel.h>

Adafruit_NeoPixel pixels(32 * 8, 16, NEO_GRB);
#endif

int rowColToIndex(int x, int y)
{
    if (y % 2 == 1)
        return (y * 8) + (7 - x);
    else
        return (y * 8) + x;
}

void render_game_object(GameObject *game_object)
{
    Location *location = game_object->get_location();
    GameObjectColor color = game_object->get_color();

#ifndef WINDOWS
    pixels.setPixelColor(rowColToIndex(location->x, location->y), pixels.Color(color->r, color->g, color->b));
#endif

#if WINDOWS
    pixels.set_pixel(location->x, location->y, color);
#endif
}

void setup()
{
#ifndef WINDOWS
    Serial.begin(9600);

    // Wait for serial to connect
    while (!Serial)
        ;

    Serial.println("Starting up...");

    pixels.begin();
#endif

    // neat_world_objects::PlayerPointer = new Player(4, 4, 255);
    // World_Game_Objects->push_back(neat_world_objects::player_pointer);

    Serial.printf("Free Heap before level load: %d\r\n", ESP.getFreeHeap());
    load_world_state(&dev_level, World_Game_Objects);
    Serial.printf("Free Heap after level load: %d\r\n", ESP.getFreeHeap());
    if (neat_world_objects::player_pointer == nullptr)
        throw; // There was no player loaded after loading the level
}

void loop()
{
    // Read the key input
    if (Serial.available() > 0)
    {
        char key = Serial.read();
        // TODO: For now we pass in all the world objects,
        // but we should only be passing in the ones around the player
        switch (key)
        {
        case '8':
            neat_world_objects::player_pointer->move(World_Game_Objects, Direction::North);
            break;
        case '2':
            neat_world_objects::player_pointer->move(World_Game_Objects, Direction::South);
            break;
        case '4':
            neat_world_objects::player_pointer->move(World_Game_Objects, Direction::West);
            break;
        case '6':
            neat_world_objects::player_pointer->move(World_Game_Objects, Direction::East);
            break;
        case '7':
            neat_world_objects::player_pointer->move(World_Game_Objects, Direction::NorthWest);
            break;
        case '9':
            neat_world_objects::player_pointer->move(World_Game_Objects, Direction::NorthEast);
            break;
        case '1':
            neat_world_objects::player_pointer->move(World_Game_Objects, Direction::SouthWest);
            break;
        case '3':
            neat_world_objects::player_pointer->move(World_Game_Objects, Direction::SouthEast);
            break;
        case '-':
            neat_world_objects::player_pointer->m_PlayerHealth--;
            break;
        case '+':
            neat_world_objects::player_pointer->m_PlayerHealth++;
            break;
        case '5':
            neat_world_objects::player_pointer->start_action_select(World_Game_Objects);
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

#include <Mahi/Gui.hpp>
#include <Mahi/Util.hpp>

using namespace mahi::gui;
using namespace mahi::util;

// Inherit from Application
class MiniCrawSim : public Application
{

private:
    bool m_show_demo_window = false;

    void draw_location_viewer(Location *location)
    {
        ImGui::InputInt("X", &location->x);
        ImGui::InputInt("Y", &location->y);
    }

    void draw_type_spesfic_thing(GameObject *game_object)
    {

        if (ImGui::Button("Take Engine Step"))
        {
            Serial.take_in_key('0'); // cycle nothing into the game engine so it will rerender
            loop();                  // take a loop for the renderer
        }

        level_item type = game_object->get_type();
        // lets try to cast this as all of the diffrent GameObject types, and see if our object is that
        draw_location_viewer(game_object->get_location());

        ImGui::Separator();

        Wall *wall = dynamic_cast<Wall *>(game_object);
        if (wall != nullptr)
        {
            ImGui::Text("Wall....");
            return;
        }

        Door *door = dynamic_cast<Door *>(game_object);
        if (door != nullptr)
        {
            ImGui::Checkbox("Is open", door->get_open_state());
            ImGui::Checkbox("Is Locked", door->get_lock_state());
            return;
        }

        Player *player = dynamic_cast<Player *>(game_object);
        if (player != nullptr)
        {
            // Player inventory
            ImGui::Text("Player Inventory");
            auto inventory = player->get_inventory();
            for (int i = 0; i < inventory->size(); i++)
            {
                ImGui::Text(inventory->at(i)->get_name());
            }
            return;
        }

        // Check if it is an CPlayer_Item
        CPlayer_Item *player_item = dynamic_cast<CPlayer_Item *>(game_object);
        if (player_item != nullptr)
        {
            ImGui::Separator();
            ImGui::Text("Player Item");
            // Lets get the name of the item
            ImGui::Text(player_item->get_name());

            // Build a trashy location to run the drop on
            static int drop_x = 0;
            static int drop_y = 0;

            ImGui::InputInt("X", &drop_x);
            ImGui::InputInt("Y", &drop_y);

            if (ImGui::Button("Trigger On Drop"))
            {
                Location drop_location = Location(drop_x, drop_y);
                player_item->OnDrop(&drop_location, nullptr);
            }

            if (ImGui::Button("Trigger On Pickup"))
            {
                Location drop_location = Location(drop_x, drop_y);
                player_item->OnPickup(&drop_location, nullptr);
            }

            return;
        }

        // if we get here, we have not found a type, so lets just say that
        ImGui::Text("Unknown");
    }

    void game_object_info_list(std::vector<GameObject *> *game_objects)
    {
        ImGui::Begin("Game Objects");

        // Left, object list
        static int selected = 0;
        {
            ImGui::BeginChild("Game Objects", ImVec2(150, 0), true);

            for (int i = 0; i < game_objects->size(); i++)
            {
                auto game_object_type = game_objects->at(i)->get_type();
                auto type_to_string = to_string(game_object_type);
                auto index_string = i < 10 ? "0" + std::to_string(i) : std::to_string(i);

                auto label = index_string + " " + type_to_string;

                if (ImGui::Selectable(label.c_str(), selected == i))
                    selected = i;
            }

            ImGui::EndChild();
        }
        ImGui::SameLine();

        // Right
        {
            ImGui::BeginGroup();
            // Leave room for 1 line below us
            ImGui::BeginChild("item view", ImVec2(0, -ImGui::GetFrameHeightWithSpacing()));
            ImGui::Text("MyGameId: %d", selected);
            ImGui::Separator();
            std::string location_to_string = "Location: " +
                                             std::to_string(game_objects->at(selected)->get_location()->x) +
                                             ", " + std::to_string(game_objects->at(selected)->get_location()->y);
            std::string color_to_string = "Color: " +
                                          std::to_string(game_objects->at(selected)->get_color().r) +
                                          ", " + std::to_string(game_objects->at(selected)->get_color().g) +
                                          ", " + std::to_string(game_objects->at(selected)->get_color().b);
            // Type: get_type() as enum string + get_type() as int
            auto game_object_type = game_objects->at(selected)->get_type();
            auto type_to_string = to_string(game_object_type);
            std::string type_to_string_with_int = "Type: " + type_to_string +
                                                  " (" + std::to_string((int)game_object_type) + ")";

            ImGui::Text(location_to_string.c_str());
            ImGui::Text(color_to_string.c_str());
            ImGui::SameLine();
            // color view rect
            ImGui::ColorButton("##color",
                               ImVec4(game_objects->at(selected)->get_color().r,
                                      game_objects->at(selected)->get_color().g,
                                      game_objects->at(selected)->get_color().b,
                                      1.0f),
                               ImGuiColorEditFlags_NoTooltip | ImGuiColorEditFlags_NoDragDrop |
                                   ImGuiColorEditFlags_NoAlpha);

            ImGui::Text(type_to_string.c_str());

            // TODO: Switch out for the spesific type of object
            draw_type_spesfic_thing(game_objects->at(selected));
            ImGui::EndChild();
            ImGui::EndGroup();
        }

        ImGui::End();
    }

    void draw_game_display(std::vector<std::vector<GameObjectColor>> game_display)
    {
        draw_game_display(&game_display);
    }

    void draw_game_display(std::vector<std::vector<GameObjectColor>> *game_display)
    {
        int width = game_display->size();
        int height = game_display->at(0).size();

        if (ImGui::BeginTable("gameObjectsTable", 8, ImGuiTableFlags_Borders))
        {
            for (int curr_height = 0; curr_height < height; curr_height++)
            {
                ImGui::TableNextRow();
                for (int x = 0; x < width; x++)
                {

                    ImGui::TableNextColumn();

                    // Draw a color button
                    ImGui::ColorButton("##color",
                                       ImVec4(game_display->at(x).at(curr_height).r,
                                              game_display->at(x).at(curr_height).g,
                                              game_display->at(x).at(curr_height).b,
                                              1.0f),
                                       ImGuiColorEditFlags_NoTooltip | ImGuiColorEditFlags_NoDragDrop |
                                           ImGuiColorEditFlags_NoAlpha);
                }
            }
            ImGui::EndTable();
        }
    }

    void draw_dev_item_menu()
    {
        static int spawnX = 0;
        static int spawnY = 0;

        ImGui::Begin("dev item menu");
        ImGui::Text("Item Spawn Location");
        ImGui::InputInt("X", &spawnX);
        ImGui::InputInt("Y", &spawnY);

        // for each item in the items enum, draw a menu label and a button to spawn it
        for (int i = 0; i < (int)level_item::ITEM_TYPE_COUNT; i++)
        {
            auto item_type = (level_item)i;
            auto item_type_string = to_string(item_type);
            if (ImGui::Button(item_type_string.c_str()))
            {
                // Spawn the item
                // 1) build the new LevelItem
                // 2) pass it into the object builder
                // 3) Put the item in the world objects list
                // 4) $$$
            }
        }

        ImGui::End();
    }

    char get_key_pressed()
    {
        if (ImGui::IsKeyPressed(ImGui::GetKeyIndex(ImGuiKey_DownArrow)))
            return '2';
        if (ImGui::IsKeyPressed(ImGui::GetKeyIndex(ImGuiKey_UpArrow)))
            return '8';
        if (ImGui::IsKeyPressed(ImGui::GetKeyIndex(ImGuiKey_LeftArrow)))
            return '4';
        if (ImGui::IsKeyPressed(ImGui::GetKeyIndex(ImGuiKey_RightArrow)))
            return '6';
        if (ImGui::IsKeyPressed(ImGui::GetKeyIndex(ImGuiKey_PageUp)))
            return '9';
        if (ImGui::IsKeyPressed(ImGui::GetKeyIndex(ImGuiKey_PageDown)))
            return '3';
        if (ImGui::IsKeyPressed(ImGui::GetKeyIndex(ImGuiKey_Home)))
            return '7';
        if (ImGui::IsKeyPressed(ImGui::GetKeyIndex(ImGuiKey_End)))
            return '1';
        if (ImGui::IsKeyPressed(ImGui::GetKeyIndex(ImGuiKey_Delete)))
            return '-';
        if (ImGui::IsKeyPressed(ImGui::GetKeyIndex(ImGuiKey_Insert)))
            return '+';
        if (ImGui::IsKeyPressed(ImGui::GetKeyIndex(ImGuiKey_Enter)))
            return '5';
        return (char)255;
    }

    std::vector<std::vector<GameObjectColor>> get_display_from_objects(std::vector<GameObject *> *game_objects)
    {
        std::vector<std::vector<GameObjectColor>> game_display;
        for (int i = 0; i < 8; i++)
        {
            std::vector<GameObjectColor> row;
            for (int j = 0; j < 32; j++)
            {
                row.push_back(GameObjectColor(0, 0, 0));
            }
            game_display.push_back(row);
        }

        for (auto game_object : *game_objects)
        {
            auto location = game_object->get_location();
            // invert y
            game_display.at(location->x).at(31 - location->y) = game_object->get_color();
            // game_display.at(location->x).at(location->y) = game_object->get_color();
        }

        return game_display;
    }

    void draw_dockspace()
    {
        ImGui::DockSpaceOverViewport(ImGui::GetMainViewport());
    }

    void draw_ui()
    {
        draw_dockspace();
        game_object_info_list(World_Game_Objects);

        draw_dev_item_menu();

        ImGui::Begin("display");
        draw_game_display(pixels.get_current_display_state());
        ImGui::End();

        ImGui::Begin("internal");
        draw_game_display(get_display_from_objects(World_Game_Objects));
        ImGui::End();

        if (ImGui::Button("Show Demo"))
            m_show_demo_window = !m_show_demo_window;

        if (m_show_demo_window)
            ImGui::ShowDemoWindow(&m_show_demo_window);
    }

    void handle_input()
    {

        char key_pressed = get_key_pressed();
        if (key_pressed != (char)255)
        {
            Serial.take_in_key(key_pressed);
            loop(); // Take a game loop step
        }
    }

public:
    // 640x480 px window
    MiniCrawSim() : Application(640, 480, "Tiny Crawl Simulator")
    {
        // disable viewports in imgui
        ImGui::GetIO().ConfigFlags &= ~ImGuiConfigFlags_ViewportsEnable;
    }
    // Override update (called once per frame)
    void update() override
    {
        handle_input();
        draw_ui();
    }
};

int main()
{
    setup();
    MiniCrawSim app;
    app.run();
    return 0;
}

#endif