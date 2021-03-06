#pragma once

/* Unstable API */

#include <string>
#include <sstream>
#include <array>
#include <functional>

namespace flecs {

////////////////////////////////////////////////////////////////////////////////
//// Forward declarations and types
////////////////////////////////////////////////////////////////////////////////

using world_t = ecs_world_t;
using entity_t = ecs_entity_t;
using type_t = ecs_type_t;
using snapshot_t = ecs_snapshot_t;
using filter_t = ecs_filter_t;
using query_t = ecs_query_t;

class world;
class snapshot;
class entity;
class type;
class iter;
class filter;
class filter_iterator;
class child_iterator;
class world_filter;
class snapshot_filter;

template<typename ... Components>
class query_iterator;

template<typename ... Components>
class query;

template <typename T>
class component_base;

enum match_kind {
    MatchAll = EcsMatchAll,
    MatchAny = EcsMatchAny,
    MatchExact = EcsMatchExact
};


////////////////////////////////////////////////////////////////////////////////
//// Builtin components and tags 
////////////////////////////////////////////////////////////////////////////////

/* Builtin components */
using Component = EcsComponent;
using ComponentLifecycle = EcsComponentLifecycle;
using Trigger = EcsTrigger;
using Type = EcsType;
using Name = EcsName;
using Timer = EcsTimer;
using RateFilter = EcsRateFilter;
using TickSource = EcsTickSource;
using SignatureExpr = EcsSignatureExpr;
using Signature = EcsSignature;
using Query = EcsQuery;
using ViewAction = EcsIterAction;
using Context = EcsContext;

/* Builtin tag ids */
static const ecs_entity_t Module = EcsModule;
static const ecs_entity_t Prefab = EcsPrefab;
static const ecs_entity_t Hidden = EcsHidden;
static const ecs_entity_t Disabled = EcsDisabled;
static const ecs_entity_t DisabledIntern = EcsDisabledIntern;
static const ecs_entity_t Inactive = EcsInactive;
static const ecs_entity_t OnDemand = EcsOnDemand;
static const ecs_entity_t Monitor = EcsMonitor;
static const ecs_entity_t Pipeline = EcsPipeline;

/* Trigger tags */
static const ecs_entity_t OnAdd = EcsOnAdd;
static const ecs_entity_t OnRemove = EcsOnRemove;
static const ecs_entity_t OnSet = EcsOnSet;

/* Builtin pipeline tags */
static const ecs_entity_t PreFrame = EcsPreFrame;
static const ecs_entity_t OnLoad = EcsOnLoad;
static const ecs_entity_t PostLoad = EcsPostLoad;
static const ecs_entity_t PreUpdate = EcsPreUpdate;
static const ecs_entity_t OnUpdate = EcsOnUpdate;
static const ecs_entity_t OnValidate = EcsOnValidate;
static const ecs_entity_t PostUpdate = EcsPostUpdate;
static const ecs_entity_t PreStore = EcsPreStore;
static const ecs_entity_t OnStore = EcsOnStore;
static const ecs_entity_t PostFrame = EcsPostFrame;

/** Builtin entity ids */
static const ecs_entity_t World = EcsWorld;
static const ecs_entity_t Singleton = EcsSingleton;


////////////////////////////////////////////////////////////////////////////////
//// Wrapper class around a table column
////////////////////////////////////////////////////////////////////////////////

template <typename T>
class column {
public:
    column(T* array, std::size_t count, bool is_shared = false)
        : m_array(array)
        , m_count(count) 
        , m_is_shared(is_shared) {}

    column(iter &iter, int column);

    T& operator[](size_t index) {
        ecs_assert(index < m_count, ECS_COLUMN_INDEX_OUT_OF_RANGE, NULL);
        ecs_assert(!m_is_shared, ECS_INVALID_PARAMETER, NULL);
        return m_array[index];
    }

    T* operator->() {
        ecs_assert(m_array != nullptr, ECS_COLUMN_INDEX_OUT_OF_RANGE, NULL);
        return m_array;
    }

    bool is_set() const {
        return m_array != nullptr;
    }

    bool is_shared() const {
        return m_is_shared;
    }

protected:
    T* m_array;
    size_t m_count;
    bool m_is_shared;
};


////////////////////////////////////////////////////////////////////////////////
//// Like flecs::column, but abstracts away from shared vs. owned columns
////////////////////////////////////////////////////////////////////////////////

template <typename T>
class auto_column final : public column<T> {
public:
    auto_column(T* array, std::size_t count, bool is_shared = false)
        : column<T>(array, count, is_shared) { }

    T& operator[](size_t index) {
        if (!this->m_is_shared) {
            ecs_assert(index < this->m_count, ECS_COLUMN_INDEX_OUT_OF_RANGE, NULL);
            return this->m_array[index];
        } else {
            return this->m_array[0];
        }
    }   
};


////////////////////////////////////////////////////////////////////////////////
//// Iterate over an integer range (used to iterate over entities in systems)
////////////////////////////////////////////////////////////////////////////////

template <typename T>
class range_iterator
{
public:
    explicit range_iterator(T value)
        : m_value(value){}

    bool operator!=(range_iterator const& other) const
    {
        return m_value != other.m_value;
    }

    T const& operator*() const
    {
        return m_value;
    }

    range_iterator& operator++()
    {
        ++m_value;
        return *this;
    }

private:
    T m_value;
};


////////////////////////////////////////////////////////////////////////////////
//// Type that enables iterating over table columns
////////////////////////////////////////////////////////////////////////////////

class iter final {
    using row_iterator = range_iterator<int>;
public:    
    iter(const ecs_iter_t *it) : m_iter(it) { 
        m_begin = 0;
        m_end = it->count;
    }

    row_iterator begin() const {
        return row_iterator(m_begin);
    }

    row_iterator end() const {
        return row_iterator(m_end);
    }

    /* Obtain handle to current system */
    flecs::entity system() const;

    flecs::world world() const;

    /* Number of entities to iterate over */
    int32_t count() const {
        return m_iter->count;
    }

    /* Return delta_time of current frame */
    float delta_time() const {
        return m_iter->delta_time;
    }

    /* Return time elapsed since last time system was invoked */
    float delta_system_time() const {
        return m_iter->delta_system_time;
    }

    /* Is column shared */
    bool is_shared(int32_t col) const {
        return !ecs_is_owned(m_iter, col);
    }
    
    /* Access param field */
    void *param() {
        return m_iter->param;
    }

    /* Is column readonly */
    bool is_readonly(int32_t col) const {
        return ecs_is_readonly(m_iter, col);
    }

    /* Obtain entity being iterated over for row */
    flecs::entity entity(int32_t row) const;

    /* Obtain column source (0 if self) */
    flecs::entity column_source(int32_t col) const;

    /* Obtain component/tag entity of column */
    flecs::entity column_entity(int32_t col) const;

    /* Obtain type of column */
    type column_type(int32_t col) const;

    /* Obtain type of table being iterated over */
    type table_type() const;

    int32_t table_count() const {
        return m_iter->table_count;
    }

    int32_t inactive_table_count() const {
        return m_iter->inactive_table_count;
    }

    /* Obtain untyped pointer to table column */
    void* table_column(int32_t col) const {
        return ecs_table_column(m_iter, col);
    }

    /* Obtain typed pointer to table column */
    template <typename T>
    flecs::column<T> table_column() const {
        auto type = ecs_iter_type(m_iter);
        auto col = ecs_type_index_of(type, component_base<T>::s_entity);
        ecs_assert(col != -1, ECS_INVALID_PARAMETER, NULL);
        return flecs::column<T>(static_cast<T*>(ecs_table_column(m_iter, col)), m_iter->count, false);
    }

    /* Obtain column with a const type */
    template <typename T,
        typename std::enable_if<std::is_const<T>::value, void>::type* = nullptr>
    flecs::column<T> column(unsigned int col) const {
        return get_column<T>(col);
    }

    /* Obtain column with non-const type. Ensure that column is not readonly */
    template <typename T,
        typename std::enable_if<std::is_const<T>::value == false, void>::type* = nullptr>
    flecs::column<T> column(int32_t col) const {
        ecs_assert(!ecs_is_readonly(m_iter, col), ECS_COLUMN_ACCESS_VIOLATION, NULL);
        return get_column<T>(col);
    }

    /* Get owned */
    template <typename T>
    flecs::column<T> owned(int32_t col) const {
        ecs_assert(!!ecs_is_owned(m_iter, col), ECS_COLUMN_IS_SHARED, NULL);
        return this->column<T>(col);
    }

    /* Get shared */
    template <typename T>
    const T& shared(int32_t col) const {
        ecs_assert(ecs_column_entity(m_iter, col) == component_base<T>::s_entity, ECS_COLUMN_TYPE_MISMATCH, NULL);
        ecs_assert(!ecs_is_owned(m_iter, col), ECS_COLUMN_IS_NOT_SHARED, NULL);
        return *static_cast<T*>(ecs_column_w_size(m_iter, sizeof(T), col));
    }

    /* Get single field of a const type */
    template <typename T,
        typename std::enable_if<std::is_const<T>::value, void>::type* = nullptr>    
    T& field(int32_t col, int32_t row) const {
        return get_field<T>(col, row);
    }

    /* Get single field of a non-const type. Ensure that column is not readonly */
    template <typename T,
        typename std::enable_if<std::is_const<T>::value == false, void>::type* = nullptr>
    T& field(int32_t col, int32_t row) const {
        ecs_assert(!ecs_is_readonly(m_iter, col), ECS_COLUMN_ACCESS_VIOLATION, NULL);
        return get_field<T>(col, row);
    }

private:
    /* Get column, check if correct type is used */
    template <typename T>
    flecs::column<T> get_column(int32_t column_id) const {
        ecs_assert(ecs_column_entity(m_iter, column_id) == component_base<T>::s_entity, ECS_COLUMN_TYPE_MISMATCH, NULL);
        int32_t count;
        bool is_shared = !ecs_is_owned(m_iter, column_id);

        /* If a shared column is retrieved with 'column', there will only be a
         * single value. Ensure that the application does not accidentally read
         * out of bounds. */
        if (is_shared) {
            count = 1;
        } else {
            /* If column is owned, there will be as many values as there are
             * entities. */
            count = m_iter->count;
        }

        return flecs::column<T>(static_cast<T*>(ecs_column_w_size(m_iter, sizeof(T), column_id)), count, is_shared);
    }   

    /* Get single field, check if correct type is used */
    template <typename T>
    T& get_field(int32_t col, int32_t row) const {
        ecs_assert(ecs_column_entity(m_iter, col) == component_base<T>::s_entity, ECS_COLUMN_TYPE_MISMATCH, NULL);
        return *static_cast<T*>(ecs_element_w_size(m_iter, sizeof(T), col, row));
    }       

    const ecs_iter_t *m_iter;
    int32_t m_begin;
    int32_t m_end;
};

template <typename T>
inline column<T>::column(iter &iter, int col) {
    *this = iter.column<T>(col);
}


////////////////////////////////////////////////////////////////////////////////
//// The ECS world
////////////////////////////////////////////////////////////////////////////////

class world final {
public:
    world() 
        : m_world( ecs_init() )
        , m_owned( true ) { init_builtin_components(); }

    world(int argc, char *argv[])
        : m_world( ecs_init_w_args(argc, argv) )
        , m_owned( true ) { init_builtin_components(); }

    explicit world(world_t *w) 
        : m_world( w ) 
        , m_owned( false ) { }

    /* Not allowed to copy a world. May only take a reference */
    world(const world& obj) = delete;

    world(world&& obj) {
        m_world = obj.m_world;
    }

    /* Not allowed to copy a world. May only take a reference */
    world& operator=(const world& obj) = delete;

    world& operator=(world&& obj) {
        m_world = obj.m_world;
        return *this;
    }
    
    ~world() { 
        if (m_owned) {
            ecs_fini(m_world); 
        }
    }

    world_t* c_ptr() const {
        return m_world;
    }

    bool progress(float delta_time = 0.0) const {
        return ecs_progress(m_world, delta_time);
    }

    /* Threading */
    void set_threads(std::int32_t threads) const {
        ecs_set_threads(m_world, threads);
    }

    std::int32_t get_threads() const {
        return ecs_get_threads(m_world);
    }

    std::int32_t get_thread_index() const {
        return ecs_get_thread_index(m_world);
    }

    /* Time management */
    void set_target_fps(float target_fps) const {
        ecs_set_target_fps(m_world, target_fps);
    }

    float get_target_fps() const {
        const ecs_world_info_t *stats = ecs_get_world_info(m_world);
        return stats->target_fps;
    }

    std::int32_t get_tick() const {
        const ecs_world_info_t *stats = ecs_get_world_info(m_world);
        return stats->frame_count_total;

    }

    /* Get/set user-context */
    void set_context(void* ctx) const {
        ecs_set_context(m_world, ctx);
    }

    void* get_context() const {
        return ecs_get_context(m_world);
    }

    /* Preallocating memory */
    void dim(std::int32_t entity_count) const {
        ecs_dim(m_world, entity_count);
    }

    void dim_type(type_t type, std::int32_t entity_count) const {
        ecs_dim_type(m_world, type, entity_count);
    }

    /* Entity ranges */
    void set_entity_range(entity_t min, entity_t max) const {
        ecs_set_entity_range(m_world, min, max);
    }

    void enable_range_check(bool enabled) const {
        ecs_enable_range_check(m_world, enabled);
    }

    /* Lookup by name */
    entity lookup(const char *name) const;

    /* Bulk operations */
    void delete_entities(flecs::filter filter) const;

    template <typename T>
    void add() const;
    template <typename T>
    void add(flecs::filter filter) const;

    void add(type type) const;
    void add(type type, flecs::filter filter) const;

    void add(entity entity) const;
    void add(entity entity, flecs::filter filter) const;

    template <typename T>
    void remove() const;
    template <typename T>
    void remove(flecs::filter filter) const;

    void remove(type type) const;
    void remove(type type, flecs::filter filter) const;

    void remove(entity entity) const;
    void remove(entity entity, flecs::filter filter) const;

    /* Iterate world tables */
    world_filter filter(const flecs::filter& filter) const;
    filter_iterator begin() const;
    filter_iterator end() const;

    /* Count entities */
    template <typename T>
    int count() const {
        return ecs_count_type(m_world, component_base<T>::s_type);
    }

    int count(flecs::filter filter) const;

    /* Enable locking */
    bool enable_locking(bool enabled) {
        return ecs_enable_locking(m_world, enabled);
    }

    /* Lock world */
    void lock() {
        ecs_lock(m_world);
    }

    /* Unlock world */
    void unlock() {
        ecs_unlock(m_world);
    }
    
private:
    void init_builtin_components();

    world_t *m_world;
    bool m_owned;
};


////////////////////////////////////////////////////////////////////////////////
//// Fluent API for chaining entity operations
////////////////////////////////////////////////////////////////////////////////

template <typename base>
class entity_fluent {
    using base_type = const base;
public:

    /* -- adopt -- */

    base_type& add(entity_t entity) const {
        static_cast<base_type*>(this)->invoke(
        [entity](world_t *world, entity_t id) {
            ecs_add_entity(world, id, entity);
        });
        return *static_cast<base_type*>(this);         
    }

    template <typename T>
    base_type& add() const {
        return add(component_base<T>::s_entity);
    }

    base_type& add(const entity& entity) const;

    base_type& add(type_t type) const {
        static_cast<base_type*>(this)->invoke(
        [type](world_t *world, entity_t id) {
            ecs_add_type(world, id, type);
        });
        return *static_cast<base_type*>(this); 
    }

    base_type& add(type type) const;

    /* -- remove -- */

    base_type& remove(entity_t entity) const {
        static_cast<base_type*>(this)->invoke(
        [entity](world_t *world, entity_t id) {
            ecs_remove_entity(world, id, entity);
        });
        return *static_cast<base_type*>(this);         
    }    

    template <typename T>
    base_type& remove() const {
        return remove(component_base<T>::s_entity);
    }

    base_type& remove(const entity& entity) const;

    base_type& remove(type_t type) const {
        static_cast<base_type*>(this)->invoke(
        [type](world_t *world, entity_t id) {
            ecs_remove_type(world, id, type);
        });
        return *static_cast<base_type*>(this);         
    }

    base_type& remove(type type) const;

    /* -- add_childof -- */

    base_type& add_childof(entity_t parent) const {
        static_cast<base_type*>(this)->invoke(
        [parent](world_t *world, entity_t id) {
            ecs_add_entity(world, id, ECS_CHILDOF | parent);
        });
        return *static_cast<base_type*>(this);  
    }

    base_type& add_childof(const entity& parent) const;

    /* -- remove_childof -- */

    base_type& remove_childof(entity_t parent) const {
        static_cast<base_type*>(this)->invoke(
        [parent](world_t *world, entity_t id) {
            ecs_remove_entity(world, id, ECS_CHILDOF | parent);
        });
        return *static_cast<base_type*>(this);  
    }

    base_type& remove_childof(const entity& parent) const;

    /* -- add_instanceof -- */

    base_type& add_instanceof(entity_t base_entity) const {
        static_cast<base_type*>(this)->invoke(
        [base_entity](world_t *world, entity_t id) {
            ecs_add_entity(world, id, ECS_INSTANCEOF | base_entity);
        });
        return *static_cast<base_type*>(this);  
    }

    base_type& add_instanceof(const entity& base_entity) const;  

    /* -- remove_instanceof -- */

    base_type& remove_instanceof(entity_t base_entity) const {
        static_cast<base_type*>(this)->invoke(
        [base_entity](world_t *world, entity_t id) {
            ecs_remove_entity(world, id, ECS_INSTANCEOF | base_entity);
        });
        return *static_cast<base_type*>(this);
    }

    base_type& remove_instanceof(const entity& base_entity) const;

    /* -- set -- */

    template <typename T>
    const base_type& set(const T&& value) const {
        static_cast<base_type*>(this)->invoke(
        [&value](world_t *world, entity_t id) {
            ecs_set_ptr_w_entity(world, id, component_base<T>::s_entity, sizeof(T), &value);
        });
        return *static_cast<base_type*>(this);
    }   

    template <typename T>
    const base_type& set(const T& value) const {
        static_cast<base_type*>(this)->invoke(
        [&value](world_t *world, entity_t id) {
            ecs_set_ptr_w_entity(world, id, component_base<T>::s_entity, sizeof(T), &value);
        });
        return *static_cast<base_type*>(this);
    }

    template <typename T>
    const base_type& replace(std::function<void(T&, bool)> func) const {
        static_cast<base_type*>(this)->invoke(
        [&func](world_t *world, entity_t id) {
            bool is_added;

            T *ptr = static_cast<T*>(ecs_get_mut_w_entity(
                world, id, component_base<T>::s_entity, &is_added));

            if (ptr) {
                func(*ptr, !is_added);
                ecs_modified_w_entity(world, id, component_base<T>::s_entity);
            }
        });
        return *static_cast<base_type*>(this);
    }      

    template <typename T>
    const base_type& replace(std::function<void(T&)> func) const {
        static_cast<base_type*>(this)->invoke(
        [&func](world_t *world, entity_t id) {
            bool is_added;

            T *ptr = static_cast<T*>(ecs_get_mut_w_entity(
                world, id, component_base<T>::s_entity, &is_added));

            if (ptr) {
                if (is_added) {
                    // Allow constructor to initialize value
                    T value;
                    *ptr = value;
                }

                func(*ptr);
                ecs_modified_w_entity(world, id, component_base<T>::s_entity);
            }
        });
        return *static_cast<base_type*>(this);
    }            
};

////////////////////////////////////////////////////////////////////////////////
//// Cached component pointer
////////////////////////////////////////////////////////////////////////////////

template <typename T>
class ref {
public:
    ref()
        : m_world( nullptr )
        , m_entity( 0 )
        , m_ref({0}) { }

    ref(world_t *world, entity_t entity) 
        : m_world( world )
        , m_entity( entity )
        , m_ref({0}) {
        _ecs_get_ref(
            m_world, &m_ref, m_entity, component_base<T>::s_entity);
    }

    T* operator->() {
        T* result = static_cast<T*>(_ecs_get_ref(
            m_world, &m_ref, m_entity, component_base<T>::s_entity));

        ecs_assert(result != NULL, ECS_INVALID_PARAMETER, NULL);

        return result;
    }

    T* get() {
        if (m_entity) {
            _ecs_get_ref(
                m_world, &m_ref, m_entity, component_base<T>::s_entity);    
        }

        return static_cast<T*>(m_ref.ptr);
    }

    flecs::entity entity() const;

private:
    world_t *m_world;
    entity_t m_entity;
    ecs_ref_t m_ref;
};


////////////////////////////////////////////////////////////////////////////////
//// Entity
////////////////////////////////////////////////////////////////////////////////

class entity : public entity_fluent<entity> {
public:
    explicit entity(const world& world) 
        : m_world( world.c_ptr() )
        , m_id( ecs_new_w_type(m_world, 0) ) { }

    explicit entity(world_t *world) 
        : m_world( world )
        , m_id( ecs_new_w_type(m_world, 0) ) { }

    entity(const world& world, const char *name) 
        : m_world( world.c_ptr() )
        , m_id( ecs_lookup(m_world, name) ) 
        { 
            if (!m_id) {
                EcsName id{ ecs_os_strdup(name) };
                id.alloc_value = ecs_os_strdup(name);
                id.value = id.alloc_value;
                id.symbol = NULL;            
                m_id = ecs_set_ptr(m_world, 0, EcsName, &id);
            }
        }

    entity(const world& world, std::string name) 
        : m_world( world.c_ptr() )
        , m_id( ecs_lookup(m_world, name.c_str()) ) 
        { 
            if (!m_id) {
                EcsName id;
                id.alloc_value = ecs_os_strdup(name.c_str());
                id.value = id.alloc_value;
                id.symbol = NULL;
                m_id = ecs_set_ptr(m_world, 0, EcsName, &id);
            }
        }         

    entity(const world& world, entity_t id) 
        : m_world( world.c_ptr() )
        , m_id(id) { }

    entity(world_t *world, entity_t id) 
        : m_world( world )
        , m_id(id) { }

    entity() 
        : m_world(nullptr)
        , m_id(0) { }

    static
    flecs::entity nil(const world& world) {
        return flecs::entity(world.c_ptr(), (ecs_entity_t)0);
    }

    entity_t id() const {
        return m_id;
    }

    std::string name() const {
        const EcsName *name = static_cast<const EcsName*>(
            ecs_get_w_entity(m_world, m_id, ecs_entity(EcsName)));
        if (name) {
            return std::string(name->value);
        } else {
            return std::string();
        }
    }

    flecs::world world() const {
        return flecs::world(m_world);
    }

    flecs::type type() const;

    flecs::type to_type() const;

    template <typename T>
    const T* get() const {
        return static_cast<const T*>(
            ecs_get_w_entity(m_world, m_id, component_base<T>::s_entity));
    }

    template <typename T>
    T* get_mut(bool *is_added = nullptr) const {
        return static_cast<T*>(
            ecs_get_mut_w_entity(
                m_world, m_id, component_base<T>::s_entity), is_added);
    }

    template <typename T>
    ref<T> get_ref() const {
        return ref<T>(m_world, m_id);
    }

    template <typename Func>
    void invoke(Func&& action) const {
        action(m_world, m_id);
    } 

    void destruct() const {
        ecs_delete(m_world, m_id);
    }

    entity lookup(const char *name) const {
        auto id = ecs_lookup_child(m_world, m_id, name);
        return entity(m_world, id);
    }

    /* -- has -- */

    bool has(entity_t id) const {
        return ecs_has_entity(m_world, m_id, id);
    }

    bool has(type_t type) const {
        return ecs_has_type(m_world, m_id, type);
    }

    bool has(const entity& e) const {
        return has(e.id());
    }

    template <typename T>
    bool has() const {
        return has(component_base<T>::s_entity);
    }

    bool owns(entity_t id) const {
        return ecs_owns_entity(m_world, m_id, id, true);
    }

    bool owns(type_t type) const {
        return ecs_type_owns_type(m_world, ecs_get_type(m_world, m_id), type, true);
    }

    bool owns(const entity& e) const {
        return owns(e.id());
    }

    template <typename T>
    bool owns() const {
        return owns(component_base<T>::s_entity);
    }

    float delta_time() {
        const ecs_world_info_t *stats = ecs_get_world_info(m_world);
        return stats->delta_time;
    }

    child_iterator children() const;

    operator bool() {
        return m_id != 0;
    }

protected:
    world_t *m_world;
    entity_t m_id; 
};

/** Prefab class */
class prefab final : public entity {
public:
    prefab(const flecs::world& world, const char *name) 
        : entity(world, name)
    {
        this->add(flecs::Prefab);
    }
};


////////////////////////////////////////////////////////////////////////////////
//// Entity range, allows for operating on a range of consecutive entities
////////////////////////////////////////////////////////////////////////////////

class entity_range final : public entity_fluent<entity_range> {
    using entity_iterator = range_iterator<entity_t>;
public:
    entity_range(const world& world, std::int32_t count) 
        : m_world(world.c_ptr())
        , m_id_start( ecs_bulk_new_w_type(m_world, nullptr, count))
        , m_count(count) { }

    template <typename Func>
    void invoke(Func&& action) const {
        for (auto id : *this) {
            action(m_world, id);
        }
    }
    
    entity_iterator begin() const {
        return entity_iterator(m_id_start);
    }

    entity_iterator end() const {
        return entity_iterator(m_id_start + m_count);
    }

private:
    world_t *m_world;
    entity_t m_id_start;
    std::int32_t m_count;
};


////////////////////////////////////////////////////////////////////////////////
//// A collection of component ids used to describe the contents of a table
////////////////////////////////////////////////////////////////////////////////

class type final : entity {
public:
    type(const flecs::world& world, const char *name, const char *expr = nullptr)
        : entity(world, ecs_new_type(world.c_ptr(), 0, name, expr))
    { 
        sync_from_flecs();
    }

    type(const flecs::world& world, type_t t)
        : entity( world )
        , m_type( t )
        , m_normalized( t ) { }

    type(world_t *world, type_t t)
        : entity( world )
        , m_type( t )
        , m_normalized( t ) { }

    type& add(const type& t) {
        m_type = ecs_type_add(m_world, m_type, t.id());
        m_normalized = ecs_type_merge(m_world, m_normalized, t.c_ptr(), nullptr);
        sync_from_me();
        return *this;
    }

    type& add(const entity& e) {
        m_type = ecs_type_add(m_world, m_type, e.id());
        m_normalized = ecs_type_add(m_world, m_normalized, e.id());
        sync_from_me();
        return *this;
    }

    template<typename T>
    type& add() {
        m_type = ecs_type_add(m_world, m_type, component_base<T>::s_entity);
        m_normalized = ecs_type_add(m_world, m_normalized, component_base<T>::s_entity);
        sync_from_me();
        return *this;
    }

    type& add_instanceof(const entity& e) {
        m_type = ecs_type_add(m_world, m_type, e.id() | ECS_INSTANCEOF);
        m_normalized = ecs_type_add(m_world, m_normalized, e.id() | ECS_INSTANCEOF);
        sync_from_me();
        return *this;
    }

    type& add_childof(const entity& e) {
        m_type = ecs_type_add(m_world, m_type, e.id() | ECS_CHILDOF);
        m_normalized = ecs_type_add(m_world, m_normalized, e.id() | ECS_CHILDOF);
        sync_from_me();
        return *this;
    }

    std::string str() const {
        char *str = ecs_type_str(m_world, m_type);
        std::string result(str);
        ecs_os_free(str);
        return result;
    }

    type_t c_ptr() const {
        return m_type;
    }

    type_t c_normalized() const {
        return m_normalized;
    }

    void enable() {
        ecs_enable(m_world, m_id, true);
    }

    void disable() {
        ecs_enable(m_world, m_id, false);
    }

private:
    void sync_from_me() {
        EcsType *tc = ecs_get_mut(m_world, m_id, EcsType, NULL);
        if (tc) {
            tc->type = m_type;
            tc->normalized = m_normalized;
        }
    }

    void sync_from_flecs() {
        EcsType *tc = ecs_get_mut(m_world, m_id, EcsType, NULL);
        if (tc) {
            m_type = tc->type;
            m_normalized = tc->normalized;
        }
    }

    type_t m_type;
    type_t m_normalized;
};


////////////////////////////////////////////////////////////////////////////////
//// Register component, provide global access to component handles / metadata
////////////////////////////////////////////////////////////////////////////////

template <typename T>
class component_base final {
public:
    static void init(const world& world, const char *name) {
        entity_t cur_entity = s_entity;
        type_t cur_type = s_type;

        s_entity = ecs_new_component(world.c_ptr(), 0, name, sizeof(T), alignof(T));
        s_type = ecs_type_from_entity(world.c_ptr(), s_entity);
        s_name = name;

        ecs_assert(!cur_entity || cur_entity == s_entity, ECS_INCONSISTENT_COMPONENT_NAME, name);
        ecs_assert(!cur_type || cur_type == s_type, ECS_INCONSISTENT_COMPONENT_NAME, name);
    }

    static void init_existing(entity_t entity, type_t type, const char *name) {
        s_entity = entity;
        s_type = type;
        s_name = name;
    }

    static entity_t s_entity;
    static type_t s_type;
    static const char *s_name;
};

template <typename T> entity_t component_base<T>::s_entity( 0 );
template <typename T> type_t component_base<T>::s_type( nullptr );
template <typename T> const char* component_base<T>::s_name( nullptr );


////////////////////////////////////////////////////////////////////////////////
//// Register a component with flecs
////////////////////////////////////////////////////////////////////////////////

template <typename T>
class component : public entity {
public:
    component(const flecs::world& world, const char *name) { 
        component_base<T>::init(world, name);

        /* Register as well for both const and reference versions of type */
        component_base<const T>::init_existing(
            component_base<T>::s_entity, 
            component_base<T>::s_type, 
            component_base<T>::s_name);

        component_base<T&>::init_existing(
            component_base<T>::s_entity, 
            component_base<T>::s_type, 
            component_base<T>::s_name);    

        m_id = component_base<T>::s_entity;
        m_world = world.c_ptr();
    }
};


////////////////////////////////////////////////////////////////////////////////
//// Define a module
////////////////////////////////////////////////////////////////////////////////

template <typename T>
class module final : public component<T> {
public:
    module(flecs::world& world, const char *name) : component<T>(world, name) { }
};


////////////////////////////////////////////////////////////////////////////////
//// Import a module
////////////////////////////////////////////////////////////////////////////////

template <typename T>
void import(world& world, int flags = 0) {
    if (!component_base<T>::s_name) {
        // Allocate module, so the this ptr will remain stable
        T *module_data = new T(world, flags);

        flecs::entity s(world, EcsSingleton);

        s.set<T>(*module_data);
    }
}

////////////////////////////////////////////////////////////////////////////////
//// A filter is used to match subsets of tables
////////////////////////////////////////////////////////////////////////////////
 
class filter {
public:
    filter() 
        : m_world( nullptr )
        , m_filter{ } {}

    explicit filter(const world& world) 
        : m_world( world.c_ptr() )
        , m_filter{ } { }

    filter& include(type type) {
        m_filter.include = ecs_type_merge(m_world, m_filter.include, type.c_ptr(), nullptr);
        return *this;
    }

    filter& include(entity entity) {
        m_filter.include = ecs_type_add(m_world, m_filter.include, entity.id());
        return *this;
    }

    template <typename T>
    filter& include() {
        m_filter.include = ecs_type_add(m_world, m_filter.include, component_base<T>::s_entity);
        return *this;
    }

    filter& include_kind(match_kind kind) {
        m_filter.include_kind = static_cast<ecs_match_kind_t>(kind);
        return *this;
    }

    type include() {
        return type(m_world, m_filter.include);
    }

    filter& exclude(type type) {
        m_filter.exclude = ecs_type_merge(m_world, m_filter.exclude, type.c_ptr(), nullptr);
        return *this;
    }

    filter& exclude(entity entity) {
        m_filter.exclude = ecs_type_add(m_world, m_filter.exclude, entity.id());
        return *this;
    }

    template <typename T>
    filter& exclude() {
        m_filter.exclude = ecs_type_add(m_world, m_filter.exclude, component_base<T>::s_entity);
        return *this;
    }
 
    filter& exclude_kind(match_kind kind) {
        m_filter.exclude_kind = static_cast<ecs_match_kind_t>(kind);
        return *this;
    }

    type exclude() {
        return type(m_world, m_filter.exclude);
    }  

    const filter_t* c_ptr() const {
        if (m_world) {
            return &m_filter;
        } else {
            return nullptr;
        }
    }

private:
    world_t *m_world;
    filter_t m_filter;
};


////////////////////////////////////////////////////////////////////////////////
//// Utility to convert template argument pack to array of columns
////////////////////////////////////////////////////////////////////////////////

template <typename ... Components>
class column_args {
public:    
    struct Column {
        void *ptr;
        bool is_shared;
    };

    using Columns = std::array<Column, sizeof...(Components)>;

    column_args(ecs_iter_t* iter) {
        populate_columns(iter, 0, (typename std::remove_reference<Components>::type*)nullptr...);
    }

    Columns m_columns;

private:
    /* Dummy function when last component has been added */
    void populate_columns(ecs_iter_t *iter, int index) { }

    /* Populate columns array recursively */
    template <typename T, typename... Targs>
    void populate_columns(ecs_iter_t *iter, int index, T comp, Targs... comps) {
        m_columns[index].ptr = ecs_column_w_size(iter, sizeof(*comp), index + 1);
        m_columns[index].is_shared = !ecs_is_owned(iter, index + 1);
        populate_columns(iter, index + 1, comps ...);
    }
};

////////////////////////////////////////////////////////////////////////////////
//// Utility class to invoke a system each
////////////////////////////////////////////////////////////////////////////////

template <typename Func, typename ... Components>
class each_invoker {
    using Columns = typename column_args<Components ...>::Columns;

public:
    explicit each_invoker(Func func) : m_func(func) { }

    // Invoke system
    template <typename... Targs,
        typename std::enable_if<sizeof...(Targs) == sizeof...(Components), void>::type* = nullptr>
    static void call_system(ecs_iter_t *iter, Func func, int index, Columns& columns, Targs... comps) {
        flecs::iter iter_wrapper(iter);

        // Use auto_column so we can transparently use shared components
        for (auto row : iter_wrapper) {
            func(iter_wrapper.entity(row), (auto_column<typename std::remove_reference<Components>::type>(
                 (typename std::remove_reference<Components>::type*)comps.ptr, iter->count, comps.is_shared))[row]...);
        }
    }

    // Add components one by one to parameter pack
    template <typename... Targs,
        typename std::enable_if<sizeof...(Targs) != sizeof...(Components), void>::type* = nullptr>
    static void call_system(ecs_iter_t *iter, Func func, int index, Columns& columns, Targs... comps) {
        call_system(iter, func, index + 1, columns, comps..., columns[index]);
    }

    // Callback provided to flecs system
    static void run(ecs_iter_t *iter) {
        const Context *ctx = ecs_get(iter->world, iter->system, EcsContext);
        each_invoker *self = (each_invoker*)ctx->ctx;
        Func func = self->m_func;        
        column_args<Components...> columns(iter);
        call_system(iter, func, 0, columns.m_columns);
    }   

private:
    Func m_func;
};

////////////////////////////////////////////////////////////////////////////////
//// Persistent queries
////////////////////////////////////////////////////////////////////////////////

template<typename ... Components>
class query final {
    using Columns = typename column_args<Components...>::Columns;

public:
    query() : m_query(nullptr) { }

    explicit query(world& world) {
        std::stringstream str;
        if (!pack_args_to_string(str)) {
            ecs_abort(ECS_INVALID_PARAMETER, NULL);
        }

        m_query = ecs_query_new(world.c_ptr(), str.str().c_str());
    }

    explicit query(world& world, const char *expr) {
        std::stringstream str;
        if (!pack_args_to_string(str)) {
            m_query = ecs_query_new(world.c_ptr(), expr);
        } else {
            str << "," << expr;
            m_query = ecs_query_new(world.c_ptr(), str.str().c_str());
        }
    }

    query_iterator<Components...> begin() const;

    query_iterator<Components...> end() const;

    template <typename Func>
    void each(Func func) const {
        ecs_iter_t iter = ecs_query_iter(m_query);

        while (ecs_query_next(&iter)) {
            column_args<Components...> columns(&iter);
            each_invoker<Func, Components...> ctx(func);
            ctx.call_system(&iter, func, 0, columns.m_columns);
        }
    }

    query_t* c_ptr() const {
        return m_query;
    }

private:
    bool pack_args_to_string(std::stringstream& str) {
        std::array<const char*, sizeof...(Components)> ids = {
            component_base<Components>::s_name...
        };

        int i = 0;
        for (auto id : ids) {
            if (i) {
                str << ",";
            }

            str << "ANY:";
            str << id;
            i ++;
        }

        return i != 0;
    }

    query_t *m_query;
};


////////////////////////////////////////////////////////////////////////////////
//// Snapshots make a copy of the world state that can be restored
////////////////////////////////////////////////////////////////////////////////

class snapshot final {
public:
    explicit snapshot(const world& world)
        : m_world( world )
        , m_snapshot( nullptr ) { }

    snapshot(const snapshot& obj) 
        : m_world( obj.m_world )
    { 
        ecs_iter_t it = ecs_snapshot_iter(obj.m_snapshot, nullptr);
        m_snapshot = ecs_snapshot_take_w_iter(&it, ecs_snapshot_next);
    }

    snapshot(snapshot&& obj) 
        : m_world(obj.m_world)
        , m_snapshot(obj.m_snapshot)
    {
        obj.m_snapshot = nullptr;
    }

    snapshot& operator=(const snapshot& obj) {
        ecs_assert(m_world.c_ptr() == obj.m_world.c_ptr(), ECS_INVALID_PARAMETER, NULL);
        ecs_iter_t it = ecs_snapshot_iter(obj.m_snapshot, nullptr);
        m_snapshot = ecs_snapshot_take_w_iter(&it, ecs_snapshot_next);        
        return *this;
    }

    snapshot& operator=(snapshot&& obj) {
        ecs_assert(m_world.c_ptr() == obj.m_world.c_ptr(), ECS_INVALID_PARAMETER, NULL);
        m_snapshot = obj.m_snapshot;
        obj.m_snapshot = nullptr;
        return *this;
    }

    void take() {
        if (m_snapshot) {
            ecs_snapshot_free(m_snapshot);
        }

        m_snapshot = ecs_snapshot_take(m_world.c_ptr());
    }

    void take(flecs::filter filter) {
        if (m_snapshot) {
            ecs_snapshot_free(m_snapshot);
        }

        ecs_iter_t it = ecs_filter_iter(m_world.c_ptr(), filter.c_ptr());
        m_snapshot = ecs_snapshot_take_w_iter(
            &it, ecs_filter_next);
    }

    void restore() {
        if (m_snapshot) {
            ecs_snapshot_restore(m_world.c_ptr(), m_snapshot);
            m_snapshot = nullptr;
        }
    }

    ~snapshot() {
        if (m_snapshot) {
            ecs_snapshot_free(m_snapshot);
        }
    }

    snapshot_t* c_ptr() const {
        return m_snapshot;
    }

    snapshot_filter filter(const filter& filter);

    filter_iterator begin();

    filter_iterator end();
private:
    const world& m_world;
    snapshot_t *m_snapshot;
};


////////////////////////////////////////////////////////////////////////////////
//// Utility class to invoke a system action
////////////////////////////////////////////////////////////////////////////////

template <typename Func, typename ... Components>
class action_invoker {
    using Columns = typename column_args<Components ...>::Columns;

public:
    explicit action_invoker(Func func) 
        : m_func(func) { }

    /* Invoke system */
    template <typename... Targs,
        typename std::enable_if<sizeof...(Targs) == sizeof...(Components), void>::type* = nullptr>
    static void call_system(ecs_iter_t *iter, int index, Columns& columns, Targs... comps) {
        const Context *ctx = ecs_get(iter->world, iter->system, EcsContext);
        action_invoker *self = (action_invoker*)ctx->ctx;

        Func func = self->m_func;

        flecs::iter iter_wrapper(iter);
        
        func(iter_wrapper, (column<typename std::remove_reference<Components>::type>(
            (typename std::remove_reference<Components>::type*)comps.ptr, iter->count, comps.is_shared))...);
    }

    /** Add components one by one to parameter pack */
    template <typename... Targs,
        typename std::enable_if<sizeof...(Targs) != sizeof...(Components), void>::type* = nullptr>
    static void call_system(ecs_iter_t *iter, int index, Columns& columns, Targs... comps) {
        call_system(iter, index + 1, columns, comps..., columns[index]);
    }

    /** Callback provided to flecs */
    static void run(ecs_iter_t *iter) {
        column_args<Components...> columns(iter);
        call_system(iter, 0, columns.m_columns);
    }   

private:
    Func m_func;
};


////////////////////////////////////////////////////////////////////////////////
//// Fluent interface to run a system manually
////////////////////////////////////////////////////////////////////////////////

class system_runner_fluent {
public:
    system_runner_fluent(world_t *world, entity_t id, float delta_time, void *param)
        : m_world(world)
        , m_id(id)
        , m_delta_time(delta_time)
        , m_param(param)
        , m_filter()
        , m_offset(0)
        , m_limit(0) { }

    system_runner_fluent& filter(filter filter) {
        m_filter = filter;
        return *this;
    }

    system_runner_fluent& offset(std::int32_t offset) {
        m_offset = offset;
        return *this;
    }

    system_runner_fluent& limit(std::int32_t limit) {
        m_limit = limit;
        return *this;
    }

    ~system_runner_fluent() {
        ecs_run_w_filter(
            m_world, m_id, m_delta_time, m_offset, m_limit, m_filter.c_ptr(), m_param);
    }
private:
    world_t *m_world;
    entity_t m_id;
    float m_delta_time;
    void *m_param;
    flecs::filter m_filter;
    std::int32_t m_offset;
    std::int32_t m_limit;
};


////////////////////////////////////////////////////////////////////////////////
//// Register a system with Flecs
////////////////////////////////////////////////////////////////////////////////

template<typename ... Components>
class system final : public entity {
public:
    system(const flecs::world& world, const char *name = nullptr)
        : m_kind(static_cast<ecs_entity_t>(OnUpdate))
        , m_name(name) 
        , m_period(0.0)
        , m_on_demand(false)
        , m_hidden(false)
        , m_finalized(false) { 
            m_world = world.c_ptr();
        }

    system& signature(const char *signature) {
        ecs_assert(!m_finalized, ECS_INVALID_PARAMETER, NULL);
        m_signature = signature;
        return *this;
    }

    system& kind(entity_t kind) {
        ecs_assert(!m_finalized, ECS_INVALID_PARAMETER, NULL);
        m_kind = static_cast<ecs_entity_t>(kind);
        return *this;
    }

    system& period(float period) {
        ecs_assert(!m_finalized, ECS_INVALID_PARAMETER, NULL);
        m_period = period;
        return *this;
    }

    system& on_demand() {
        ecs_assert(!m_finalized, ECS_INVALID_PARAMETER, NULL);
        m_on_demand = true;
        return *this;
    }

    system& hidden() {
        ecs_assert(!m_finalized, ECS_INVALID_PARAMETER, NULL);
        m_hidden = true;
        return *this;
    }

    void enable() {
        ecs_enable(m_world, m_id, true);
    }

    void disable() {
        ecs_enable(m_world, m_id, false);
    }

    void set_period(float period) const {
        ecs_set_interval(m_world, m_id, period);
    }

    void set_context(void *ctx) const {
        EcsContext ctx_value = { ctx };
        ecs_set_ptr(m_world, m_id, EcsContext, &ctx_value);
    }

    void* get_context() const {
        const EcsContext *ctx = ecs_get(m_world, m_id, EcsContext);
        if (ctx) {
            return (void*)ctx->ctx;
        } else {
            return NULL;
        }
    }

    system_runner_fluent run(float delta_time = 0.0f, void *param = nullptr) const {
        return system_runner_fluent(m_world, m_id, delta_time, param);
    }

    /* Action (or each) is mandatory and always the last thing that is added in 
     * the fluent method chain. Create system signature from both template 
     * parameters and anything provided by the signature method. */
    template <typename Func>
    system& action(Func func) {
        ecs_assert(!m_finalized, ECS_INVALID_PARAMETER, NULL);
        auto ctx = new action_invoker<Func, Components...>(func);

        std::string signature = build_signature(false);

        entity_t e = ecs_new_system(
            m_world, 
            0,
            m_name, 
            m_kind, 
            signature.c_str(), 
            action_invoker<Func, Components...>::run);

        EcsContext ctx_value = {ctx};
        ecs_set_ptr(m_world, e, EcsContext, &ctx_value);

        if (m_period) {
            ecs_set_interval(m_world, e, m_period);
        }

        m_id = e;

        return *this;
    }

    /* Each is similar to action, but accepts a function that operates on a
     * single entity */
    template <typename Func>
    system& each(Func func) {
        auto ctx = new each_invoker<Func, Components...>(func);

        std::string signature = build_signature(true);

        if (!signature.length()) {
            signature = "0";
        }

        entity_t e = ecs_new_system(
            m_world, 
            0,
            m_name, 
            m_kind, 
            signature.c_str(), 
            each_invoker<Func, Components...>::run);

        EcsContext ctx_value = {ctx};
        ecs_set_ptr(m_world, e, EcsContext, &ctx_value);

        if (m_period) {
            ecs_set_interval(m_world, e, m_period);
        }        

        m_id = e;

        return *this;
    }

    ~system() = default;
private:
    std::string build_signature(bool is_each) {
        bool is_set = false;

        std::stringstream str;
        if (pack_args_to_string(str, is_each)) {
            is_set = true;
        }

        if (m_signature) {
            if (is_set) {
                str << ",";
            }
            str << m_signature;
            is_set = true;
        }

        if (m_hidden) {
            if (is_set) {
                str << ",";
            }            
            str << "SYSTEM:Hidden";
            is_set = true;
        }    

        if (m_on_demand) {
            if (is_set) {
                str << ",";
            }            
            str << "SYSTEM:EcsOnDemand";
            is_set = true;
        } 

        return str.str();       
    }

    bool pack_args_to_string(std::stringstream& str, bool is_each) {
        std::array<const char*, sizeof...(Components)> ids = {
            component_base<Components>::s_name...
        };

        std::array<const char*, sizeof...(Components)> inout_modifiers = {
            inout_modifier<Components>()...
        };    

        int i = 0;
        for (auto id : ids) {
            if (i) {
                str << ",";
            }
            str << inout_modifiers[i];
            if (is_each) {
                str << "ANY:";
            }
            str << id;
            i ++;
        }  

        return i != 0;
    }

    /** Utilities to convert type trait to flecs signature syntax */
    template <typename T,
        typename std::enable_if< std::is_const<T>::value == true, void>::type* = nullptr>
    constexpr const char *inout_modifier() const {
        return "[in] ";
    }

    template <typename T,
        typename std::enable_if< std::is_reference<T>::value == true, void>::type* = nullptr>
    constexpr const char *inout_modifier() const {
        return "[out] ";
    }

    template <typename T,
        typename std::enable_if<std::is_const<T>::value == false && std::is_reference<T>::value == false, void>::type* = nullptr>
    constexpr const char *inout_modifier() const {
        return "";
    }

    ecs_entity_t m_kind;
    const char *m_name;
    const char *m_signature = nullptr;
    float m_period;
    bool m_on_demand;
    bool m_hidden;
    bool m_finalized; // After set to true, call no more fluent functions
};


////////////////////////////////////////////////////////////////////////////////
//// Persistent queries
////////////////////////////////////////////////////////////////////////////////

template<typename ... Components>
class query_iterator
{
public:
    query_iterator()
        : m_has_next(false)
        , m_iter{ } { }

    query_iterator(const query<Components...>& query) 
        : m_iter( ecs_query_iter(query.c_ptr()) )
    {
        m_has_next = ecs_query_next(&m_iter);
    }

    bool operator!=(query_iterator const& other) const {
        return m_has_next != other.m_has_next;
    }

    flecs::iter const operator*() const {
        return flecs::iter(&m_iter);
    }

    query_iterator& operator++() {
        m_has_next = ecs_query_next(&m_iter);
        return *this;
    }

private:
    bool m_has_next;
    ecs_iter_t m_iter;
};


////////////////////////////////////////////////////////////////////////////////
//// Utility for iterating over tables that match a filter
////////////////////////////////////////////////////////////////////////////////

class filter_iterator
{
public:
    filter_iterator()
        : m_world(nullptr)
        , m_has_next(false)
        , m_iter{ } { }

    filter_iterator(const world& world, const filter& filter)
        : m_world( world.c_ptr() )
        , m_iter( ecs_filter_iter(m_world, filter.c_ptr()) ) 
    { 
        m_has_next = ecs_filter_next(&m_iter);
    }

    filter_iterator(const world& world, const snapshot& snapshot, const filter& filter) 
        : m_world( world.c_ptr() )
        , m_iter( ecs_snapshot_iter(snapshot.c_ptr(), filter.c_ptr()) )
    {
        m_has_next = ecs_filter_next(&m_iter);
    }

    bool operator!=(filter_iterator const& other) const {
        return m_has_next != other.m_has_next;
    }

    flecs::iter const operator*() const {
        return flecs::iter(&m_iter);
    }

    filter_iterator& operator++() {
        m_has_next = ecs_filter_next(&m_iter);
        return *this;
    }

private:
    world_t *m_world;
    bool m_has_next;
    ecs_iter_t m_iter;
};


////////////////////////////////////////////////////////////////////////////////
//// Tree iterator
////////////////////////////////////////////////////////////////////////////////

class tree_iterator
{
public:
    tree_iterator()
        : m_has_next(false)
        , m_iter{ } { }

    tree_iterator(flecs::entity entity) 
        : m_iter( ecs_scope_iter(entity.world().c_ptr(), entity.id()) )
    {
        m_has_next = ecs_scope_next(&m_iter);
    }

    bool operator!=(tree_iterator const& other) const {
        return m_has_next != other.m_has_next;
    }

    flecs::iter const operator*() const {
        return flecs::iter(&m_iter);
    }

    tree_iterator& operator++() {
        m_has_next = ecs_scope_next(&m_iter);
        return *this;
    }

private:
    bool m_has_next;
    ecs_iter_t m_iter;
};

////////////////////////////////////////////////////////////////////////////////
//// Utility for creating a world-based filter iterator
////////////////////////////////////////////////////////////////////////////////

class world_filter {
public:
    world_filter(const world& world, const filter& filter) 
        : m_world( world )
        , m_filter( filter ) { }

    inline filter_iterator begin() const {
        return filter_iterator(m_world, m_filter);
    }

    inline filter_iterator end() const {
        return filter_iterator();
    }

private:
    const world& m_world;
    const filter& m_filter;
};


////////////////////////////////////////////////////////////////////////////////
//// Utility for creating a snapshot-based filter iterator
////////////////////////////////////////////////////////////////////////////////

class snapshot_filter {
public:
    snapshot_filter(const world& world, const snapshot& snapshot, const filter& filter) 
        : m_world( world )
        , m_snapshot( snapshot )
        , m_filter( filter ) { }

    inline filter_iterator begin() const {
        return filter_iterator(m_world, m_snapshot, m_filter);
    }

    inline filter_iterator end() const {
        return filter_iterator();
    }

private:
    const world& m_world;
    const snapshot& m_snapshot;
    const filter& m_filter;
};

////////////////////////////////////////////////////////////////////////////////
//// Utility for creating a child table iterator
////////////////////////////////////////////////////////////////////////////////

class child_iterator {
public:
    child_iterator(const entity& entity) 
        : m_parent( entity ) { }

    inline tree_iterator begin() const {
        return tree_iterator(m_parent);
    }

    inline tree_iterator end() const {
        return tree_iterator();
    }

private:
    const entity& m_parent;
};


////////////////////////////////////////////////////////////////////////////////
//// Reader for world/snapshot serialization
////////////////////////////////////////////////////////////////////////////////

class reader final {
public:
    reader(world& world) {
        m_reader = ecs_reader_init(world.c_ptr());
    }

    reader(world& world, snapshot& snapshot) {
        ecs_iter_t it = ecs_snapshot_iter(snapshot.c_ptr(), nullptr);
        m_reader = ecs_reader_init_w_iter(&it, ecs_snapshot_next);
    }

    std::size_t read(char *buffer, std::size_t size) {
        return ecs_reader_read(buffer, size, &m_reader);
    }

private:
    ecs_reader_t m_reader;
};


////////////////////////////////////////////////////////////////////////////////
//// Writer for world deserialization
////////////////////////////////////////////////////////////////////////////////

class writer final {
public:
    writer(world& world) {
        m_writer = ecs_writer_init(world.c_ptr());
    }

    int write(const char *buffer, std::size_t size) {
        return ecs_writer_write(buffer, size, &m_writer);
    }

private:
    ecs_writer_t m_writer;
};


////////////////////////////////////////////////////////////////////////////////
//// Filter fwd declared functions
////////////////////////////////////////////////////////////////////////////////

inline snapshot_filter snapshot::filter(const flecs::filter& filter) {
    return snapshot_filter(m_world, *this, filter);
}

inline filter_iterator snapshot::begin() {
    return filter_iterator(m_world, *this, flecs::filter(m_world));
}

inline filter_iterator snapshot::end() {
    return filter_iterator();
}


////////////////////////////////////////////////////////////////////////////////
//// Query fwd declared functions
////////////////////////////////////////////////////////////////////////////////

template<typename ... Components>
inline query_iterator<Components...> query<Components...>::begin() const {
    return query_iterator<Components...>(*this);
}

template<typename ... Components>
inline query_iterator<Components...> query<Components...>::end() const {
    return query_iterator<Components...>();
}


////////////////////////////////////////////////////////////////////////////////
//// Cached ptr fwd declared functions
////////////////////////////////////////////////////////////////////////////////

template <typename T>
flecs::entity ref<T>::entity() const {
    return flecs::entity(m_world, m_entity);
}


////////////////////////////////////////////////////////////////////////////////
//// Entity fwd declared functions
////////////////////////////////////////////////////////////////////////////////

inline flecs::type entity::type() const {
    return flecs::type(m_world, ecs_get_type(m_world, m_id));
}

inline flecs::type entity::to_type() const {
    ecs_type_t type = ecs_type_from_entity(m_world, m_id);
    return flecs::type(m_world, type);
}

inline child_iterator entity::children() const {
    return flecs::child_iterator(*this);
}

////////////////////////////////////////////////////////////////////////////////
//// Entity fluent fwd declared functions
////////////////////////////////////////////////////////////////////////////////

template <typename base>
inline typename entity_fluent<base>::base_type& entity_fluent<base>::add(const entity& entity) const {
    return add(entity.id());
}

template <typename base>
inline typename entity_fluent<base>::base_type& entity_fluent<base>::add(type type) const {
    return add(type.c_ptr());
}

template <typename base>
inline typename entity_fluent<base>::base_type& entity_fluent<base>::remove(const entity& entity) const {
    return remove(entity.id());
}

template <typename base>
inline typename entity_fluent<base>::base_type& entity_fluent<base>::remove(type type) const {
    return remove(type.c_ptr());
}

template <typename base>
inline typename entity_fluent<base>::base_type& entity_fluent<base>::add_childof(const entity& entity) const {
    return add_childof(entity.id());
}

template <typename base>
inline typename entity_fluent<base>::base_type& entity_fluent<base>::remove_childof(const entity& entity) const {
    return remove_childof(entity.id());
}

template <typename base>
inline typename entity_fluent<base>::base_type& entity_fluent<base>::add_instanceof(const entity& entity) const {
    return add_instanceof(entity.id());
}

template <typename base>
inline typename entity_fluent<base>::base_type& entity_fluent<base>::remove_instanceof(const entity& entity) const {
    return remove_instanceof(entity.id());
}

inline entity world::lookup(const char *name) const {
    auto id = ecs_lookup(m_world, name);
    return entity(*this, id);
}


////////////////////////////////////////////////////////////////////////////////
//// Iter fwd declared functions
////////////////////////////////////////////////////////////////////////////////

inline flecs::entity iter::system() const {
    return flecs::entity(m_iter->world, m_iter->system);
}

inline flecs::world iter::world() const {
    return flecs::world(m_iter->world);
}

inline flecs::entity iter::entity(int32_t row) const {
    ecs_assert(row < m_iter->count, ECS_COLUMN_INDEX_OUT_OF_RANGE, NULL);
    return flecs::entity(m_iter->world, m_iter->entities[row]);
}

/* Obtain column source (0 if self) */
inline flecs::entity iter::column_source(int32_t col) const {
    return flecs::entity(m_iter->world, ecs_column_source(m_iter, col));
}

/* Obtain component/tag entity of column */
inline flecs::entity iter::column_entity(int32_t col) const {
    return flecs::entity(m_iter->world, ecs_column_entity(m_iter, col));
}

/* Obtain type of column */
inline type iter::column_type(int32_t col) const {
    return flecs::type(m_iter->world, ecs_column_type(m_iter, col));
}

/* Obtain type of table being iterated over */
inline type iter::table_type() const {
    return flecs::type(m_iter->world, ecs_iter_type(m_iter));
}


////////////////////////////////////////////////////////////////////////////////
//// World fwd declared functions
////////////////////////////////////////////////////////////////////////////////

inline void world::delete_entities(flecs::filter filter) const {
    ecs_bulk_delete(m_world, filter.c_ptr());
}

template <typename T>
inline void world::add() const {
    ecs_bulk_add_remove_type(m_world, component_base<T>::s_type, nullptr, nullptr);
}

template <typename T>
inline void world::add(flecs::filter filter) const {
    ecs_bulk_add_remove_type(m_world, component_base<T>::s_type, nullptr, filter.c_ptr());
}

inline void world::add(type type) const {
    ecs_bulk_add_remove_type(m_world, type.c_ptr(), nullptr, nullptr);
}

inline void world::add(type type, flecs::filter filter) const {
    ecs_bulk_add_remove_type(m_world, type.c_ptr(), nullptr, filter.c_ptr());
}

inline void world::add(entity entity) const {
    ecs_bulk_add_remove_type(m_world, entity.to_type().c_ptr(), nullptr, nullptr);
}

inline void world::add(entity entity, flecs::filter filter) const {
    ecs_bulk_add_remove_type(m_world, entity.to_type().c_ptr(), nullptr, filter.c_ptr());
}

template <typename T>
inline void world::remove() const {
    ecs_bulk_add_remove_type(m_world, nullptr, component_base<T>::s_type, nullptr);
}

template <typename T>
inline void world::remove(flecs::filter filter) const {
    ecs_bulk_add_remove_type(m_world, nullptr, component_base<T>::s_type, filter.c_ptr());
}

inline void world::remove(type type) const {
    ecs_bulk_add_remove_type(m_world, nullptr, type.c_ptr(), nullptr);
}

inline void world::remove(type type, flecs::filter filter) const {
    ecs_bulk_add_remove_type(m_world, nullptr, type.c_ptr(), filter.c_ptr());
}

inline void world::remove(entity entity) const {
    ecs_bulk_add_remove_type(m_world, nullptr, entity.to_type().c_ptr(), nullptr);
}

inline void world::remove(entity entity, flecs::filter filter) const {
    ecs_bulk_add_remove_type(m_world, nullptr, entity.to_type().c_ptr(), filter.c_ptr());
}

inline flecs::world_filter world::filter(const flecs::filter& filter) const {
    return flecs::world_filter(*this, filter);
}

inline filter_iterator world::begin() const {
    return filter_iterator(*this, flecs::filter(*this));
}

inline filter_iterator world::end() const {
    return filter_iterator();
}

inline int world::count(flecs::filter filter) const {
    return ecs_count_w_filter(m_world, filter.c_ptr());
}

inline void world::init_builtin_components() {
    component<Component>(*this, "EcsComponent");
    component<Type>(*this, "EcsType");
    component<Name>(*this, "EcsName");
}

}
