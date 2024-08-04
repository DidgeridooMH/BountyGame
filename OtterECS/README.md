# OtterECS

OtterECS is a simple, fast, and flexible Entity-Component-System library for C.

## Design

- ComponentMap [Array(u64, u64)] Bit map of components where each bit represents a component type and each entry is an entity. Multiple component maps can be used to extend functionality of the ECS. This similarly has a masked usage bit to help with stability but memory compaction.
- ComponentList [Array(Component, u64)] List of components where each entry is component data with a usage mask.
- ComponentPool [HashMap(ComponentType, ComponentList)] List of component lists where each entry is a component type and a list of components.
- Entity [u64] Unique identifier for an entity.
- SystemMap [Array(Function, u64)] List of system functions where each entry is a function pointer and a mask that says which components it applies to.

## Usage

### Component registry

```c
enum ComponentType {
    COMPONENT_POSITION,
    COMPONENT_VELOCITY,
    COMPONENT_HEALTH,
    COMPONENT_COUNT
};

#assert COMPONENT_COUNT < 64

void register_components() {
    component_pool_register(COMPONENT_POSITION, sizeof(Position));
    component_pool_register(COMPONENT_VELOCITY, sizeof(Velocity));
    component_pool_register(COMPONENT_HEALTH, sizeof(Health));
}

void unregister_components() {
    component_pool_unregister(COMPONENT_POSITION);
    component_pool_unregister(COMPONENT_VELOCITY);
    component_pool_unregister(COMPONENT_HEALTH);
}

void main()
{
    register_components();
    // ...

    uint64_t myEntity = ...;
    component_map_add(COMPONENT_POSITION, myEntity, &(Position){0, 0}, componentPool);
    system_map_add(update_position, system_map, COMPONENT_POSITION);

    // ...
    unregister_components();
}
```
