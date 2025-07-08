# Advanced GDScript example for the GDScript Compiler
# This file demonstrates more complex language features

extends Object
class_name AdvancedExample

# Class variables with types
var health: int = 100
var speed: float = 5.5
var name: String = "Player"
var is_active: bool = true
var items: Array = ["sword", "shield", "potion"]
var stats: Dictionary = {"strength": 10, "intelligence": 8, "dexterity": 12}

# Constants
const MAX_HEALTH: int = 200
const GRAVITY: float = 9.8

# Signals
signal health_changed(new_health: int)
signal player_died()

# Constructor
func _init(player_name: String = "Unknown"):
    name = player_name
    print("Player initialized: ", name)

# Built-in callback
func _ready():
    print("Player ready with ", health, " health")
    
# Method with parameters and return type
func take_damage(amount: int) -> bool:
    health -= amount
    emit_signal("health_changed", health)
    
    if health <= 0:
        health = 0
        emit_signal("player_died")
        return true  # Player died
    
    return false  # Player still alive

# Method with control flow
func process_items():
    print("Processing ", len(items), " items")
    
    for item in items:
        print("Item: ", item)
        
        match item:
            "sword":
                print("A sharp weapon")
            "shield":
                print("Defensive equipment")
            "potion":
                print("Restores health")
            _:
                print("Unknown item")
    
    var i: int = 0
    while i < len(items):
        print("Item ", i, ": ", items[i])
        i += 1

# Method with lambda/anonymous function
func process_with_callback(callback):
    var result = callback.call("Processing complete")
    return result

# Static method
static func calculate_damage(base_damage: int, multiplier: float) -> int:
    return int(base_damage * multiplier)