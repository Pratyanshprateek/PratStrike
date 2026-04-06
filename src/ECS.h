#pragma once

#include <cstdint>
#include <functional>
#include <limits>
#include <memory>
#include <queue>
#include <stdexcept>
#include <tuple>
#include <typeindex>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <vector>

class EntityManager
{
public:
    EntityManager()
        : nextEntityId(1)
    {
    }

    std::uint32_t createEntity()
    {
        std::uint32_t id = 0;
        if (!freeList.empty())
        {
            id = freeList.front();
            freeList.pop();
        }
        else
        {
            id = nextEntityId++;
        }

        liveEntities.insert(id);
        return id;
    }

    void destroyEntity(std::uint32_t id)
    {
        const auto it = liveEntities.find(id);
        if (it == liveEntities.end())
        {
            return;
        }

        liveEntities.erase(it);
        freeList.push(id);
    }

    bool isAlive(std::uint32_t id) const
    {
        return liveEntities.find(id) != liveEntities.end();
    }

private:
    std::uint32_t nextEntityId;
    std::queue<std::uint32_t> freeList;
    std::unordered_set<std::uint32_t> liveEntities;
};

template<typename T>
class ComponentArray
{
public:
    void add(std::uint32_t entityId, T component)
    {
        const auto found = entityToIndex.find(entityId);
        if (found != entityToIndex.end())
        {
            components[found->second] = std::move(component);
            return;
        }

        const std::size_t denseIndex = components.size();
        components.push_back(std::move(component));
        entityToIndex[entityId] = denseIndex;
        indexToEntity.push_back(entityId);
    }

    T& get(std::uint32_t entityId)
    {
        const auto found = entityToIndex.find(entityId);
        if (found == entityToIndex.end())
        {
            throw std::out_of_range("Component not found for entity.");
        }

        return components[found->second];
    }

    const T& get(std::uint32_t entityId) const
    {
        const auto found = entityToIndex.find(entityId);
        if (found == entityToIndex.end())
        {
            throw std::out_of_range("Component not found for entity.");
        }

        return components[found->second];
    }

    bool has(std::uint32_t entityId) const
    {
        return entityToIndex.find(entityId) != entityToIndex.end();
    }

    void remove(std::uint32_t entityId)
    {
        const auto found = entityToIndex.find(entityId);
        if (found == entityToIndex.end())
        {
            return;
        }

        const std::size_t removeIndex = found->second;
        const std::size_t lastIndex = components.size() - 1;

        if (removeIndex != lastIndex)
        {
            components[removeIndex] = std::move(components[lastIndex]);
            const std::uint32_t movedEntityId = indexToEntity[lastIndex];
            indexToEntity[removeIndex] = movedEntityId;
            entityToIndex[movedEntityId] = removeIndex;
        }

        components.pop_back();
        indexToEntity.pop_back();
        entityToIndex.erase(found);
    }

    std::vector<T>& getAll()
    {
        return components;
    }

    const std::vector<T>& getAll() const
    {
        return components;
    }

    const std::vector<std::uint32_t>& getEntities() const
    {
        return indexToEntity;
    }

    std::size_t size() const
    {
        return components.size();
    }

private:
    std::vector<T> components;
    std::unordered_map<std::uint32_t, std::size_t> entityToIndex;
    std::vector<std::uint32_t> indexToEntity;
};

class Registry
{
public:
    template<typename T>
    void addComponent(std::uint32_t id, T component)
    {
        validateEntity(id);
        getArray<T>().add(id, std::move(component));
    }

    template<typename T>
    T& getComponent(std::uint32_t id)
    {
        validateEntity(id);
        return getArray<T>().get(id);
    }

    template<typename T>
    const T& getComponent(std::uint32_t id) const
    {
        validateEntity(id);
        const ComponentArray<T>* array = tryGetArray<T>();
        if (array == nullptr)
        {
            throw std::out_of_range("Component array not found for entity.");
        }

        return array->get(id);
    }

    template<typename T>
    bool hasComponent(std::uint32_t id) const
    {
        if (!entityManager.isAlive(id))
        {
            return false;
        }

        const ComponentArray<T>* array = tryGetArray<T>();
        return array != nullptr && array->has(id);
    }

    template<typename T>
    void removeComponent(std::uint32_t id)
    {
        if (!entityManager.isAlive(id))
        {
            return;
        }

        ComponentArray<T>* array = tryGetArray<T>();
        if (array != nullptr)
        {
            array->remove(id);
        }
    }

    void destroyEntity(std::uint32_t id)
    {
        if (!entityManager.isAlive(id))
        {
            return;
        }

        for (auto& entry : removers)
        {
            entry.second(id);
        }

        entityManager.destroyEntity(id);
    }

    std::uint32_t createEntity()
    {
        return entityManager.createEntity();
    }

    bool isAlive(std::uint32_t id) const
    {
        return entityManager.isAlive(id);
    }

    template<typename... Ts>
    std::vector<std::uint32_t> view() const
    {
        static_assert(sizeof...(Ts) > 0, "Registry::view requires at least one component type.");

        const std::vector<std::uint32_t>* smallestEntities = nullptr;
        std::size_t smallestSize = std::numeric_limits<std::size_t>::max();
        const bool allArraysPresent = (selectSmallestArray<Ts>(smallestEntities, smallestSize) && ...);
        if (!allArraysPresent || smallestEntities == nullptr)
        {
            return {};
        }

        std::vector<std::uint32_t> result;
        result.reserve(smallestEntities->size());

        for (const std::uint32_t entityId : *smallestEntities)
        {
            if (!entityManager.isAlive(entityId))
            {
                continue;
            }

            if ((hasComponent<Ts>(entityId) && ...))
            {
                result.push_back(entityId);
            }
        }

        return result;
    }

private:
    template<typename T>
    ComponentArray<T>& getArray()
    {
        const std::type_index type = std::type_index(typeid(T));
        const auto found = arrays.find(type);
        if (found == arrays.end())
        {
            auto array = std::make_shared<ComponentArray<T>>();
            arrays[type] = array;
            removers[type] = [array](std::uint32_t entityId)
            {
                array->remove(entityId);
            };
            return *array;
        }

        return *std::static_pointer_cast<ComponentArray<T>>(found->second);
    }

    template<typename T>
    ComponentArray<T>* tryGetArray()
    {
        const std::type_index type = std::type_index(typeid(T));
        const auto found = arrays.find(type);
        if (found == arrays.end())
        {
            return nullptr;
        }

        return static_cast<ComponentArray<T>*>(found->second.get());
    }

    template<typename T>
    const ComponentArray<T>* tryGetArray() const
    {
        const std::type_index type = std::type_index(typeid(T));
        const auto found = arrays.find(type);
        if (found == arrays.end())
        {
            return nullptr;
        }

        return static_cast<const ComponentArray<T>*>(found->second.get());
    }

    template<typename T>
    bool selectSmallestArray(const std::vector<std::uint32_t>*& currentEntities, std::size_t& currentSize) const
    {
        const ComponentArray<T>* array = tryGetArray<T>();
        if (array == nullptr)
        {
            return false;
        }

        if (array->size() < currentSize)
        {
            currentEntities = &array->getEntities();
            currentSize = array->size();
        }

        return true;
    }

    void validateEntity(std::uint32_t id) const
    {
        if (!entityManager.isAlive(id))
        {
            throw std::runtime_error("Entity is not alive in registry.");
        }
    }

    EntityManager entityManager;
    std::unordered_map<std::type_index, std::shared_ptr<void>> arrays;
    std::unordered_map<std::type_index, std::function<void(std::uint32_t)>> removers;
};
