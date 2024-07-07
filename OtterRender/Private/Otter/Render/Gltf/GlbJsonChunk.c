#include "Otter/Render/Gltf/GlbJsonChunk.h"

#include "Otter/Util/Json/Json.h"
#include "Otter/Util/Log.h"

static bool glb_json_chunk_parse_vec3(JsonValue* jsonValue, Vec3* result)
{
  if (jsonValue->type != JT_ARRAY || jsonValue->array.size != 3)
  {
    return false;
  }

  JsonValue* x = *(JsonValue**) auto_array_get(&jsonValue->array, 0);
  JsonValue* y = *(JsonValue**) auto_array_get(&jsonValue->array, 1);
  JsonValue* z = *(JsonValue**) auto_array_get(&jsonValue->array, 2);
  if (x->type != JT_NUMBER && y->type != JT_NUMBER && z->type != JT_NUMBER)
  {
    return false;
  }

  result->x = x->number;
  result->y = y->number;
  result->z = z->number;

  return true;
}

static bool glb_json_chunk_parse_nodes(JsonValue* nodes, AutoArray* array)
{
  auto_array_create(array, sizeof(GlbNode));

  for (uint32_t i = 0; i < nodes->array.size; i++)
  {
    JsonValue* nodeElement = *(JsonValue**) auto_array_get(&nodes->array, i);
    if (nodeElement->type != JT_OBJECT)
    {
      continue;
    }

    JsonValue* meshIndex =
        hash_map_get_value(&nodeElement->object, "mesh", strlen("mesh"));
    if (meshIndex == NULL || meshIndex->type != JT_NUMBER)
    {
      continue;
    }

    GlbNode* newNode = auto_array_allocate(array);
    newNode->mesh    = (uint32_t) meshIndex->number;
    transform_identity(&newNode->transform);

    JsonValue* translation = hash_map_get_value(
        &nodeElement->object, "translation", strlen("translation"));
    if (translation != NULL)
    {
      glb_json_chunk_parse_vec3(translation, &newNode->transform.position);
    }

    JsonValue* rotation = hash_map_get_value(
        &nodeElement->object, "rotation", strlen("rotation"));
    if (rotation != NULL)
    {
      glb_json_chunk_parse_vec3(rotation, &newNode->transform.rotation);
    }

    JsonValue* scale =
        hash_map_get_value(&nodeElement->object, "scale", strlen("scale"));
    if (scale != NULL)
    {
      glb_json_chunk_parse_vec3(scale, &newNode->transform.scale);
    }
  }

  return true;
}

static bool glb_json_chunk_parse_meshes(JsonValue* meshes, AutoArray* array)
{
  auto_array_create(array, sizeof(GlbMesh));

  for (uint32_t i = 0; i < meshes->array.size; i++)
  {
    JsonValue* meshElement = *(JsonValue**) auto_array_get(&meshes->array, i);
    if (meshElement->type != JT_OBJECT)
    {
      continue;
    }

    JsonValue* primitives = hash_map_get_value(
        &meshElement->object, "primitives", strlen("primitives"));
    if (primitives == NULL || primitives->type != JT_ARRAY)
    {
      return false;
    }

    GlbMesh* mesh = auto_array_allocate(array);
    auto_array_create(&mesh->primitives, sizeof(GlbMeshPrimitive));
    for (uint32_t p = 0; p < primitives->array.size; p++)
    {
      JsonValue* primitive =
          *(JsonValue**) auto_array_get(&primitives->array, p);
      if (primitive == NULL || primitive->type != JT_OBJECT)
      {
        LOG_ERROR("Primitive was not an object.");
        return false;
      }
      JsonValue* attributes = hash_map_get_value(
          &primitive->object, "attributes", strlen("attributes"));
      JsonValue* indices =
          hash_map_get_value(&primitive->object, "indices", strlen("indices"));
      if (attributes == NULL || attributes->type != JT_OBJECT || indices == NULL
          || indices->type != JT_NUMBER)
      {
        LOG_ERROR("Attributes or indices were not present.");
        return false;
      }

      JsonValue* position = hash_map_get_value(
          &attributes->object, "POSITION", strlen("POSITION"));
      JsonValue* normal =
          hash_map_get_value(&attributes->object, "NORMAL", strlen("NORMAL"));
      JsonValue* uv = hash_map_get_value(
          &attributes->object, "TEXCOORD_0", strlen("TEXCOORD_0"));
      if (position == NULL || position->type != JT_NUMBER || normal == NULL
          || normal->type != JT_NUMBER || uv == NULL || uv->type != JT_NUMBER)
      {
        LOG_ERROR("position, normal, or uv were not in the right format.");
        return false;
      }

      GlbMeshPrimitive* meshPrimitives = auto_array_allocate(&mesh->primitives);
      meshPrimitives->position         = (uint32_t) position->number;
      meshPrimitives->normal           = (uint32_t) normal->number;
      meshPrimitives->uv               = (uint32_t) uv->number;
      meshPrimitives->indices          = (uint32_t) indices->number;
    }
  }

  return true;
}

static bool glb_json_chunk_parse_accessors(
    JsonValue* accessors, AutoArray* array)
{
  auto_array_create(array, sizeof(GlbAccessor));

  for (uint32_t i = 0; i < accessors->array.size; i++)
  {
    JsonValue* accessorElement =
        *(JsonValue**) auto_array_get(&accessors->array, i);
    if (accessorElement->type != JT_OBJECT)
    {
      LOG_ERROR("Accessor was not an object.");
      return false;
    }

    JsonValue* bufferView = hash_map_get_value(
        &accessorElement->object, "bufferView", strlen("bufferView"));
    JsonValue* componentType = hash_map_get_value(
        &accessorElement->object, "componentType", strlen("componentType"));
    JsonValue* count =
        hash_map_get_value(&accessorElement->object, "count", strlen("count"));
    JsonValue* type =
        hash_map_get_value(&accessorElement->object, "type", strlen("type"));
    if (bufferView == NULL || bufferView->type != JT_NUMBER
        || componentType == NULL || componentType->type != JT_NUMBER
        || count == NULL || count->type != JT_NUMBER || type == NULL
        || type->type != JT_STRING)
    {
      LOG_ERROR("Accessor property was not in the right format.");
      return false;
    }

    GlbAccessor* accessor   = auto_array_allocate(array);
    accessor->bufferView    = (uint32_t) bufferView->number;
    accessor->componentType = (uint32_t) componentType->number;
    accessor->count         = (uint32_t) count->number;

    if (strcmp(type->string, "SCALAR") == 0)
    {
      accessor->rank = GR_SCALAR;
    }
    else if (strcmp(type->string, "VEC2") == 0)
    {
      accessor->rank = GR_VEC2;
    }
    else if (strcmp(type->string, "VEC3") == 0)
    {
      accessor->rank = GR_VEC3;
    }
    else if (strcmp(type->string, "VEC4") == 0)
    {
      accessor->rank = GR_VEC4;
    }
    else if (strcmp(type->string, "MAT2") == 0)
    {
      accessor->rank = GR_MAT2;
    }
    else if (strcmp(type->string, "MAT3") == 0)
    {
      accessor->rank = GR_MAT3;
    }
    else if (strcmp(type->string, "MAT4") == 0)
    {
      accessor->rank = GR_MAT4;
    }
    else
    {
      LOG_ERROR("Unknown accessor rank found %s", type->string);
      return false;
    }

    accessor->useBounds = false;
    JsonValue* max =
        hash_map_get_value(&accessorElement->object, "max", strlen("max"));
    JsonValue* min =
        hash_map_get_value(&accessorElement->object, "min", strlen("min"));
    if (min != NULL && glb_json_chunk_parse_vec3(min, &accessor->min)
        && max != NULL && glb_json_chunk_parse_vec3(max, &accessor->max))
    {
      accessor->useBounds = true;
    }
  }

  return true;
}

static bool glb_json_chunk_parse_buffer_views(
    JsonValue* bufferViews, AutoArray* array)
{
  auto_array_create(array, sizeof(GlbBufferView));

  for (uint32_t i = 0; i < bufferViews->array.size; i++)
  {
    JsonValue* bufferViewElement =
        *(JsonValue**) auto_array_get(&bufferViews->array, i);
    if (bufferViewElement->type != JT_OBJECT)
    {
      LOG_ERROR("Buffer view was not an object");
      return false;
    }

    JsonValue* buffer = hash_map_get_value(
        &bufferViewElement->object, "buffer", strlen("buffer"));
    JsonValue* length = hash_map_get_value(
        &bufferViewElement->object, "byteLength", strlen("byteLength"));
    JsonValue* offset = hash_map_get_value(
        &bufferViewElement->object, "byteOffset", strlen("byteOffset"));
    if (buffer == NULL || buffer->type != JT_NUMBER || length == NULL
        || length->type != JT_NUMBER || offset == NULL
        || offset->type != JT_NUMBER)
    {
      LOG_ERROR("Buffer view property was not in the right format");
      return false;
    }

    GlbBufferView* bufferView = auto_array_allocate(array);
    bufferView->buffer        = (uint32_t) buffer->number;
    bufferView->length        = (uint32_t) length->number;
    bufferView->offset        = (uint32_t) offset->number;
  }

  return true;
}

bool glb_json_chunk_parse_buffers(JsonValue* buffers, AutoArray* array)
{
  auto_array_create(array, sizeof(GlbBuffer));

  for (uint32_t i = 0; i < buffers->array.size; i++)
  {
    JsonValue* bufferElement =
        *(JsonValue**) auto_array_get(&buffers->array, i);
    if (bufferElement->type != JT_OBJECT)
    {
      LOG_ERROR("Buffer was not an object");
      return false;
    }

    JsonValue* length = hash_map_get_value(
        &bufferElement->object, "byteLength", strlen("byteLength"));
    if (length == NULL || length->type != JT_NUMBER)
    {
      LOG_ERROR("Buffer length not found.");
      return false;
    }

    GlbBuffer* buffer  = auto_array_allocate(array);
    buffer->byteLength = (uint32_t) length->number;
  }

  return true;
}

bool glb_json_chunk_parse(JsonValue* json, GlbJsonChunk* jsonChunk)
{
  if (json->type != JT_OBJECT)
  {
    LOG_ERROR("Top-level GLTF was not an object.");
    return false;
  }

  JsonValue* nodes =
      hash_map_get_value(&json->object, "nodes", strlen("nodes"));
  JsonValue* meshes =
      hash_map_get_value(&json->object, "meshes", strlen("meshes"));
  JsonValue* accessors =
      hash_map_get_value(&json->object, "accessors", strlen("accessors"));
  JsonValue* bufferViews =
      hash_map_get_value(&json->object, "bufferViews", strlen("bufferViews"));
  JsonValue* buffers =
      hash_map_get_value(&json->object, "buffers", strlen("buffers"));
  if (nodes == NULL || nodes->type != JT_ARRAY || meshes == NULL
      || meshes->type != JT_ARRAY || accessors == NULL
      || accessors->type != JT_ARRAY || bufferViews == NULL
      || bufferViews->type != JT_ARRAY || buffers == NULL
      || buffers->type != JT_ARRAY)
  {
    LOG_ERROR("Fields missing from GLTF.");
    return false;
  }

  if (!glb_json_chunk_parse_nodes(nodes, &jsonChunk->nodes)
      || !glb_json_chunk_parse_meshes(meshes, &jsonChunk->meshes)
      || !glb_json_chunk_parse_accessors(accessors, &jsonChunk->accessors)
      || !glb_json_chunk_parse_buffer_views(
          bufferViews, &jsonChunk->bufferViews)
      || !glb_json_chunk_parse_buffers(buffers, &jsonChunk->buffers))
  {
    glb_json_chunk_destroy(jsonChunk);
    return false;
  }

  return true;
}

void glb_json_chunk_destroy(GlbJsonChunk* jsonChunk)
{
  auto_array_destroy(&jsonChunk->accessors);
  auto_array_destroy(&jsonChunk->buffers);
  auto_array_destroy(&jsonChunk->bufferViews);
  auto_array_destroy(&jsonChunk->nodes);

  for (uint32_t i = 0; i < jsonChunk->meshes.size; i++)
  {
    auto_array_destroy(
        &((GlbMesh*) auto_array_get(&jsonChunk->meshes, i))->primitives);
  }
  auto_array_destroy(&jsonChunk->meshes);
}
