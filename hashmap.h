#ifndef HASHMAP_H
#define HASHMAP_H

#define DefFreer(type) typedef void type##Freer(type *);

#define NewHashMapType(type, name, freer)                                      \
  typedef struct {                                                             \
    char ***keys;                                                              \
    type **vals;                                                               \
    size_t *sizes, *maxs;                                                      \
  } name;                                                                      \
                                                                               \
  name name##_new() {                                                          \
    name map;                                                                  \
    map.keys = (char ***)malloc(sizeof(char **) * HASH_MAX);                   \
    map.vals = (type **)malloc(sizeof(type *) * HASH_MAX);                     \
    map.sizes = (size_t *)malloc(sizeof(size_t) * HASH_MAX);                   \
    map.maxs = (size_t *)malloc(sizeof(size_t) * HASH_MAX);                    \
    for (size_t i = 0; i < HASH_MAX; i++) {                                    \
      map.sizes[i] = 0;                                                        \
      map.maxs[i] = 2;                                                         \
      map.keys[i] = (char **)malloc(sizeof(char *) * map.maxs[i]);             \
      map.vals[i] = (type *)malloc(sizeof(type) * map.maxs[i]);                \
    }                                                                          \
    return map;                                                                \
  }                                                                            \
                                                                               \
  void name##_insert(name *map, char *key, type value) {                       \
    assert(map &&key);                                                         \
    size_t hash = str_hash(key);                                               \
    for (size_t i = 0; i < map->sizes[hash]; i++) {                            \
      if (!strcmp(key, map->keys[hash][i])) {                                  \
        map->vals[hash][i] = value;                                            \
        return;                                                                \
      }                                                                        \
    }                                                                          \
    if (map->sizes[hash] == map->maxs[hash]) {                                 \
      map->maxs[hash] *= 2;                                                    \
      map->keys[hash] =                                                        \
          (char **)realloc(map->keys[hash], sizeof(char *) * map->maxs[hash]); \
      map->vals[hash] =                                                        \
          (type *)realloc(map->vals[hash], sizeof(type) * map->maxs[hash]);    \
    }                                                                          \
    map->keys[hash][map->sizes[hash]] = str_clone(key);                        \
    map->vals[hash][map->sizes[hash]++] = value;                               \
  }                                                                            \
                                                                               \
  type *name##_find(name *map, char *key) {                                    \
    assert(map &&key);                                                         \
    size_t hash = str_hash(key);                                               \
    for (size_t i = 0; i < map->sizes[hash]; i++) {                            \
      if (!strcmp(key, map->keys[hash][i])) {                                  \
        return &map->vals[hash][i];                                            \
      }                                                                        \
    }                                                                          \
    return NULL;                                                               \
  }                                                                            \
                                                                               \
  void name##_free(name map) {                                                 \
    for (size_t i = 0; i < HASH_MAX; i++) {                                    \
      for (size_t j = 0; j < map.sizes[i]; j++) {                              \
        freer(map.vals[i][j]);                                                 \
        free(map.keys[i][j]);                                                  \
      }                                                                        \
      free(map.keys[i]);                                                       \
      free(map.vals[i]);                                                       \
    }                                                                          \
    free(map.keys);                                                            \
    free(map.vals);                                                            \
    free(map.sizes);                                                           \
    free(map.maxs);                                                            \
  }                                                                            \
                                                                               \
  char *name##_identifier = #name

#endif
