#pragma once

#include "Otter/ECS/export.h"
#include "Otter/Util/Array/SparseAutoArray.h"

/** @brief The system callback function signature. */
typedef void (*SystemCallback)(uint64_t entity, void**);

/** @brief The system registry. */
typedef SparseAutoArray SystemRegistry;

struct EntityComponentMap;

/**
 * @brief Create a system registry.
 *
 * @param registry The system registry to create.
 */
OTTERECS_API void system_registry_create(SystemRegistry* registry);

/**
 * @brief Destroy a system registry.
 *
 * @param registry The system registry to destroy.
 */
OTTERECS_API void system_registry_destroy(SystemRegistry* registry);

/**
 * @brief Register a system with a call back and list of components to pass to
 * it.
 *
 * @param registry The system registry to register the system with.
 * @param system The system call back to register.
 * @param componentCount The number of components to pass to the system.
 * @param ... The list of components to pass to the system.
 *
 * @return The id of the registered system.
 */
OTTERECS_API uint64_t system_registry_register_system(
    SystemRegistry* registry, SystemCallback system, int componentCount, ...);

/**
 * @brief Deregister a system with the ID `systemId`.
 *
 * @param registry The system registry with a system registered to `systemId`.
 * @param systemId The ID of the system to deregister.
 */
OTTERECS_API void system_registry_deregister_system(
    SystemRegistry* registry, uint64_t systemId);

/**
 * @brief Run the registered systems against the component map.
 *
 * @param registry The system registry to run the systems from.
 * @param entityComponentMap The component map to run the systems against.
 */
OTTERECS_API void system_registry_run_systems(
    SystemRegistry* registry, struct EntityComponentMap* entityComponentMap);
