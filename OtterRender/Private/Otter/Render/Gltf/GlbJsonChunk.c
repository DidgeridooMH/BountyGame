#include "Otter/Render/Gltf/GlbJsonChunk.h"

#include "Otter/Math/Mat.h"
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
  if ((x->type != JT_INTEGER && x->type != JT_FLOAT)
      || (y->type != JT_INTEGER && y->type != JT_FLOAT)
      || (z->type != JT_INTEGER && z->type != JT_FLOAT))
  {
    return false;
  }

  result->x =
      x->type == JT_INTEGER ? (float) x->integer : (float) x->floatingPoint;
  result->y =
      y->type == JT_INTEGER ? (float) y->integer : (float) y->floatingPoint;
  result->z =
      z->type == JT_INTEGER ? (float) z->integer : (float) z->floatingPoint;

  return true;
}

static bool glb_json_chunk_parse_vec4(JsonValue* jsonValue, Vec4* result)
{
  if (jsonValue->type != JT_ARRAY || jsonValue->array.size != 4)
  {
    return false;
  }

  JsonValue* x = *(JsonValue**) auto_array_get(&jsonValue->array, 0);
  JsonValue* y = *(JsonValue**) auto_array_get(&jsonValue->array, 1);
  JsonValue* z = *(JsonValue**) auto_array_get(&jsonValue->array, 2);
  JsonValue* w = *(JsonValue**) auto_array_get(&jsonValue->array, 3);
  if ((x->type != JT_INTEGER && x->type != JT_FLOAT)
      || (y->type != JT_INTEGER && y->type != JT_FLOAT)
      || (z->type != JT_INTEGER && z->type != JT_FLOAT)
      || (w->type != JT_INTEGER && w->type != JT_FLOAT))
  {
    return false;
  }

  result->x =
      x->type == JT_INTEGER ? (float) x->integer : (float) x->floatingPoint;
  result->y =
      y->type == JT_INTEGER ? (float) y->integer : (float) y->floatingPoint;
  result->z =
      z->type == JT_INTEGER ? (float) z->integer : (float) z->floatingPoint;
  result->w =
      w->type == JT_INTEGER ? (float) w->integer : (float) w->floatingPoint;

  return true;
}

static bool glb_json_chunk_parse_node_transformation(
    JsonValue* nodeElement, GlbNode* newNode)
{
  JsonValue* transformMatrix =
      hash_map_get_value(&nodeElement->object, "matrix", strlen("matrix"));
  if (transformMatrix != NULL)
  {
    if (transformMatrix->type != JT_ARRAY || transformMatrix->array.size != 16)
    {
      LOG_WARNING("Matrix was not in the right format for node.");
      return false;
    }

    for (uint32_t m = 0; m < 16; m++)
    {
      JsonValue* matrixElement =
          *(JsonValue**) auto_array_get(&transformMatrix->array, m);
      if (matrixElement->type != JT_FLOAT && matrixElement->type != JT_INTEGER)
      {
        LOG_WARNING("Matrix element was not a number for node.");
        return false;
      }
      newNode->transform[m / 4][m % 4] = matrixElement->type == JT_FLOAT
                                           ? matrixElement->floatingPoint
                                           : matrixElement->integer;
    }
  }
  else
  {
    JsonValue* translation = hash_map_get_value(
        &nodeElement->object, "translation", strlen("translation"));
    if (translation != NULL)
    {
      Vec3 translationTransform = {0};
      if (!glb_json_chunk_parse_vec3(translation, &translationTransform))
      {
        LOG_WARNING("Translation was not in the right format for node.");
        return false;
      }
      mat4_translate(newNode->transform, translationTransform.x,
          translationTransform.y, translationTransform.z);
    }

    JsonValue* rotation = hash_map_get_value(
        &nodeElement->object, "rotation", strlen("rotation"));
    if (rotation != NULL)
    {
      Vec4 rotationTransform = {0};
      if (!glb_json_chunk_parse_vec4(rotation, &rotationTransform))
      {
        LOG_WARNING("Rotation was not in the right format for node.");
        return false;
      }
      mat4_rotate_quaternion(newNode->transform, rotationTransform.x,
          rotationTransform.y, rotationTransform.z, rotationTransform.w);
    }

    JsonValue* scale =
        hash_map_get_value(&nodeElement->object, "scale", strlen("scale"));
    if (scale != NULL)
    {
      Vec3 scaleTransform = {0};
      if (!glb_json_chunk_parse_vec3(scale, &scaleTransform))
      {
        LOG_WARNING("Scale was not in the right format for node.");
        return false;
      }
      mat4_scale(newNode->transform, scaleTransform.x, scaleTransform.y,
          scaleTransform.z);
    }
  }

  return true;
}

static void glb_json_chunk_parse_nodes(JsonValue* nodes, AutoArray* array)
{
  auto_array_create(array, sizeof(GlbNode));

  for (uint32_t i = 0; i < nodes->array.size; i++)
  {
    JsonValue* nodeElement = *(JsonValue**) auto_array_get(&nodes->array, i);
    if (nodeElement->type != JT_OBJECT)
    {
      LOG_WARNING("Node was not an object for node %d", i);
      continue;
    }

    GlbNode* newNode = auto_array_allocate(array);
    newNode->type    = NT_EMPTY;
    mat4_identity(newNode->transform);

    JsonValue* meshIndex =
        hash_map_get_value(&nodeElement->object, "mesh", strlen("mesh"));
    if (meshIndex != NULL && meshIndex->type == JT_INTEGER)
    {
      newNode->type = NT_MESH;
      newNode->mesh = meshIndex->integer;
    }

    if (!glb_json_chunk_parse_node_transformation(nodeElement, newNode))
    {
      newNode->type = NT_EMPTY;
      LOG_WARNING("Node transformation was not parsed properly for node %d", i);
      continue;
    }

    auto_array_create(&newNode->children, sizeof(uint32_t));
    JsonValue* children = hash_map_get_value(
        &nodeElement->object, "children", strlen("children"));
    if (children != NULL && children->type == JT_ARRAY)
    {
      for (uint32_t c = 0; c < children->array.size; c++)
      {
        JsonValue* child = *(JsonValue**) auto_array_get(&children->array, c);
        if (child->type == JT_INTEGER)
        {
          *(uint32_t*) auto_array_allocate(&newNode->children) = child->integer;
        }
        else
        {
          LOG_WARNING("Child was not a number for node %d", i);
        }
      }
    }
  }
}

static void glb_json_chunk_parse_meshes(JsonValue* meshes, AutoArray* array)
{
  auto_array_create(array, sizeof(GlbMesh));

  auto_array_allocate_many(array, meshes->array.size);
  for (uint32_t i = 0; i < meshes->array.size; i++)
  {
    GlbMesh* mesh = auto_array_get(array, i);
    auto_array_create(&mesh->primitives, sizeof(GlbMeshPrimitive));

    JsonValue* meshElement = *(JsonValue**) auto_array_get(&meshes->array, i);
    if (meshElement->type != JT_OBJECT)
    {
      LOG_ERROR("Mesh was not an object.");
      continue;
    }

    JsonValue* primitives = hash_map_get_value(
        &meshElement->object, "primitives", strlen("primitives"));
    if (primitives == NULL || primitives->type != JT_ARRAY)
    {
      LOG_ERROR("Primitives were not present.");
      continue;
    }

    for (uint32_t p = 0; p < primitives->array.size; p++)
    {
      JsonValue* primitive =
          *(JsonValue**) auto_array_get(&primitives->array, p);
      if (primitive == NULL || primitive->type != JT_OBJECT)
      {
        LOG_ERROR("Primitive was not an object.");
        continue;
      }
      JsonValue* attributes = hash_map_get_value(
          &primitive->object, "attributes", strlen("attributes"));
      JsonValue* indices =
          hash_map_get_value(&primitive->object, "indices", strlen("indices"));
      if (attributes == NULL || attributes->type != JT_OBJECT || indices == NULL
          || indices->type != JT_INTEGER)
      {
        LOG_ERROR("Attributes or indices were not present.");
        continue;
      }

      JsonValue* position = hash_map_get_value(
          &attributes->object, "POSITION", strlen("POSITION"));
      JsonValue* normal =
          hash_map_get_value(&attributes->object, "NORMAL", strlen("NORMAL"));
      JsonValue* tangent =
          hash_map_get_value(&attributes->object, "TANGENT", strlen("TANGENT"));
      JsonValue* uv = hash_map_get_value(
          &attributes->object, "TEXCOORD_0", strlen("TEXCOORD_0"));
      JsonValue* material = hash_map_get_value(
          &primitive->object, "material", strlen("material"));
      if (position == NULL || position->type != JT_INTEGER || normal == NULL
          || normal->type != JT_INTEGER || uv == NULL || uv->type != JT_INTEGER
          || material == NULL || material->type != JT_INTEGER || tangent == NULL
          || tangent->type != JT_INTEGER)
      {
        LOG_ERROR(
            "position, normal, uv, or material were not in the right format.");
        continue;
      }

      GlbMeshPrimitive* meshPrimitives = auto_array_allocate(&mesh->primitives);
      meshPrimitives->position         = position->integer;
      meshPrimitives->normal           = normal->integer;
      meshPrimitives->tangent          = tangent->integer;
      meshPrimitives->uv               = uv->integer;
      meshPrimitives->indices          = indices->integer;
      meshPrimitives->material         = material->integer;
    }
  }
}

static void glb_json_chunk_parse_materials(
    JsonValue* materials, AutoArray* array)
{
  auto_array_create(array, sizeof(GlbMaterial));
  auto_array_allocate_many(array, materials->array.size);

  for (uint32_t i = 0; i < materials->array.size; i++)
  {
    GlbMaterial* material              = auto_array_get(array, i);
    material->baseColorFactor.x        = 1.0f;
    material->baseColorFactor.y        = 1.0f;
    material->baseColorFactor.z        = 1.0f;
    material->baseColorFactor.w        = 1.0f;
    material->baseColorTexture         = -1;
    material->metallicFactor           = 1.0f;
    material->roughnessFactor          = 1.0f;
    material->metallicRoughnessTexture = -1;
    material->normalTexture            = -1;
    material->occlusionTexture         = -1;
    material->occlusionStrength        = 0.0f;
    material->emissiveFactor.x         = 0.0f;
    material->emissiveFactor.y         = 0.0f;
    material->emissiveFactor.z         = 0.0f;
    material->emissiveTexture          = -1;

    JsonValue* materialElement =
        *(JsonValue**) auto_array_get(&materials->array, i);
    if (materialElement->type != JT_OBJECT)
    {
      LOG_ERROR("Material was not an object.");
      continue;
    }

    JsonValue* name =
        hash_map_get_value(&materialElement->object, "name", strlen("name"));
    if (name != NULL && name->type == JT_STRING)
    {
      LOG_DEBUG("==== Material %d: %s ====", i, name->string);
    }
    else
    {
      LOG_DEBUG("==== Material %d ====", i);
    }

    JsonValue* pbr = hash_map_get_value(&materialElement->object,
        "pbrMetallicRoughness", strlen("pbrMetallicRoughness"));
    if (pbr != NULL && pbr->type == JT_OBJECT)
    {
      JsonValue* baseColorFactor = hash_map_get_value(
          &pbr->object, "baseColorFactor", strlen("baseColorFactor"));
      if (baseColorFactor != NULL)
      {
        if (!glb_json_chunk_parse_vec4(
                baseColorFactor, &material->baseColorFactor))
        {
          LOG_ERROR("Base color factor was not in the right format.");
        }

        LOG_DEBUG("Base color factor: %f %f %f %f", material->baseColorFactor.x,
            material->baseColorFactor.y, material->baseColorFactor.z,
            material->baseColorFactor.w);
      }

      JsonValue* baseColorTexture = hash_map_get_value(
          &pbr->object, "baseColorTexture", strlen("baseColorTexture"));
      if (baseColorTexture != NULL && baseColorTexture->type == JT_OBJECT)
      {
        JsonValue* baseColorTextureSource = hash_map_get_value(
            &baseColorTexture->object, "index", strlen("index"));
        if (baseColorTextureSource != NULL
            && baseColorTextureSource->type == JT_INTEGER)
        {
          material->baseColorTexture = baseColorTextureSource->integer;
          LOG_DEBUG("Base color texture: %d", material->baseColorTexture);
        }
        else
        {
          LOG_WARNING("Base color texture source was not an integer.");
        }
      }

      JsonValue* metallicFactor = hash_map_get_value(
          &pbr->object, "metallicFactor", strlen("metallicFactor"));
      if (metallicFactor != NULL && metallicFactor->type == JT_FLOAT)
      {
        material->metallicFactor = metallicFactor->floatingPoint;
        LOG_DEBUG("Metallic factor: %f", material->metallicFactor);
      }

      JsonValue* roughnessFactor = hash_map_get_value(
          &pbr->object, "roughnessFactor", strlen("roughnessFactor"));
      if (roughnessFactor != NULL && roughnessFactor->type == JT_FLOAT)
      {
        material->roughnessFactor = roughnessFactor->floatingPoint;
        LOG_DEBUG("Roughness factor: %f", material->roughnessFactor);
      }

      JsonValue* metallicRoughnessTexture = hash_map_get_value(&pbr->object,
          "metallicRoughnessTexture", strlen("metallicRoughnessTexture"));
      if (metallicRoughnessTexture != NULL
          && metallicRoughnessTexture->type == JT_OBJECT)
      {
        JsonValue* metallicRoughnessTextureSource = hash_map_get_value(
            &metallicRoughnessTexture->object, "index", strlen("index"));
        if (metallicRoughnessTextureSource != NULL
            && metallicRoughnessTextureSource->type == JT_INTEGER)
        {
          material->metallicRoughnessTexture =
              metallicRoughnessTextureSource->integer;
          LOG_DEBUG("Metallic roughness texture: %d",
              material->metallicRoughnessTexture);
        }
        else
        {
          LOG_WARNING("Metallic roughness texture source was not an integer.");
        }
      }
    }
    else
    {
      LOG_WARNING("PBR was not an object.");
    }

    JsonValue* normalTexture = hash_map_get_value(
        &materialElement->object, "normalTexture", strlen("normalTexture"));
    if (normalTexture != NULL && normalTexture->type == JT_OBJECT)
    {
      JsonValue* normalTextureSource =
          hash_map_get_value(&normalTexture->object, "index", strlen("index"));
      if (normalTextureSource != NULL
          && normalTextureSource->type == JT_INTEGER)
      {
        material->normalTexture = normalTextureSource->integer;
      }
      else
      {
        LOG_WARNING("Normal texture source was not an integer.");
      }
    }

    JsonValue* occlusionTexture = hash_map_get_value(&materialElement->object,
        "occlusionTexture", strlen("occlusionTexture"));
    if (occlusionTexture != NULL && occlusionTexture->type == JT_OBJECT)
    {
      JsonValue* occlusionTextureSource = hash_map_get_value(
          &occlusionTexture->object, "index", strlen("index"));
      if (occlusionTextureSource != NULL
          && occlusionTextureSource->type == JT_INTEGER)
      {
        material->occlusionTexture = occlusionTextureSource->integer;
      }
      else
      {
        LOG_WARNING("Occlusion texture source was not an integer.");
      }

      JsonValue* occlusionStrength = hash_map_get_value(
          &occlusionTexture->object, "strength", strlen("strength"));
      if (occlusionStrength != NULL && occlusionStrength->type == JT_FLOAT)
      {
        material->occlusionStrength = occlusionStrength->floatingPoint;
      }
    }

    JsonValue* emissiveFactor = hash_map_get_value(
        &materialElement->object, "emissiveFactor", strlen("emissiveFactor"));
    if (emissiveFactor != NULL)
    {
      if (!glb_json_chunk_parse_vec3(emissiveFactor, &material->emissiveFactor))
      {
        LOG_ERROR("Emissive factor was not in the right format.");
      }
    }

    JsonValue* emissiveTexture = hash_map_get_value(
        &materialElement->object, "emissiveTexture", strlen("emissiveTexture"));
    if (emissiveTexture != NULL && emissiveTexture->type == JT_OBJECT)
    {
      JsonValue* emissiveTextureSource = hash_map_get_value(
          &emissiveTexture->object, "index", strlen("index"));
      if (emissiveTextureSource != NULL
          && emissiveTextureSource->type == JT_INTEGER)
      {
        material->emissiveTexture = emissiveTextureSource->integer;
      }
      else
      {
        LOG_WARNING("Emissive texture source was not an integer.");
      }
    }
  }
}

static void glb_json_chunk_parse_textures(JsonValue* textures, AutoArray* array)
{
  auto_array_create(array, sizeof(GlbTexture));

  for (uint32_t i = 0; i < textures->array.size; i++)
  {
    GlbTexture* texture = auto_array_allocate(array);
    texture->source     = -1;
    texture->sampler    = -1;

    JsonValue* textureElement =
        *(JsonValue**) auto_array_get(&textures->array, i);
    if (textureElement->type != JT_OBJECT)
    {
      LOG_ERROR("Texture was not an object.");
      continue;
    }

    JsonValue* source =
        hash_map_get_value(&textureElement->object, "source", strlen("source"));
    if (source != NULL && source->type == JT_INTEGER)
    {
      texture->source = source->integer;
    }

    JsonValue* sampler = hash_map_get_value(
        &textureElement->object, "sampler", strlen("sampler"));
    if (sampler != NULL && sampler->type == JT_INTEGER)
    {
      texture->sampler = sampler->integer;
    }
  }
}

static void glb_json_chunk_parse_images(JsonValue* images, AutoArray* array)
{
  auto_array_create(array, sizeof(GlbImage));

  for (uint32_t i = 0; i < images->array.size; i++)
  {
    GlbImage* image   = auto_array_allocate(array);
    image->bufferView = -1;
    image->colorType  = GICT_SRGB;
    image->mimeType   = GIM_JPEG;

    JsonValue* imageElement = *(JsonValue**) auto_array_get(&images->array, i);
    if (imageElement->type != JT_OBJECT)
    {
      LOG_ERROR("Image was not an object.");
      continue;
    }

    JsonValue* bufferView = hash_map_get_value(
        &imageElement->object, "bufferView", strlen("bufferView"));
    if (bufferView != NULL && bufferView->type == JT_INTEGER)
    {
      image->bufferView = bufferView->integer;
    }

    JsonValue* name =
        hash_map_get_value(&imageElement->object, "name", strlen("name"));
    if (name != NULL && name->type == JT_STRING
        && strstr(name->string, "_BaseColor") == NULL)
    {
      image->colorType = GICT_LINEAR;
    }

    JsonValue* mimeType = hash_map_get_value(
        &imageElement->object, "mimeType", strlen("mimeType"));
    if (mimeType != NULL && mimeType->type == JT_STRING)
    {
      if (strcmp(mimeType->string, "image/jpeg") == 0)
      {
        image->mimeType = GIM_JPEG;
      }
      else if (strcmp(mimeType->string, "image/png") == 0)
      {
        image->mimeType = GIM_PNG;
      }
      else
      {
        LOG_ERROR("Mime type was not recognized.");
      }
    }
  }
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
    if (bufferView == NULL || bufferView->type != JT_INTEGER
        || componentType == NULL || componentType->type != JT_INTEGER
        || count == NULL || count->type != JT_INTEGER || type == NULL
        || type->type != JT_STRING)
    {
      LOG_ERROR("Accessor property was not in the right format.");
      return false;
    }

    GlbAccessor* accessor   = auto_array_allocate(array);
    accessor->bufferView    = bufferView->integer;
    accessor->componentType = componentType->integer;
    accessor->count         = count->integer;

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
    if (buffer == NULL || buffer->type != JT_INTEGER || length == NULL
        || length->type != JT_INTEGER || offset == NULL
        || offset->type != JT_INTEGER)
    {
      LOG_ERROR("Buffer view property was not in the right format");
      return false;
    }

    GlbBufferView* bufferView = auto_array_allocate(array);
    bufferView->buffer        = buffer->integer;
    bufferView->length        = length->integer;
    bufferView->offset        = offset->integer;
  }

  return true;
}

bool glb_json_chunk_parse_buffers(JsonValue* buffers, AutoArray* array)
{
  auto_array_create(array, sizeof(GlbBuffer));

  LOG_DEBUG("Buffers size: %d", buffers->array.size);
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
    if (length == NULL || length->type != JT_INTEGER)
    {
      LOG_ERROR("Buffer length not found.");
      return false;
    }

    GlbBuffer* buffer  = auto_array_allocate(array);
    buffer->byteLength = length->integer;
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
  JsonValue* materials =
      hash_map_get_value(&json->object, "materials", strlen("materials"));
  JsonValue* textures =
      hash_map_get_value(&json->object, "textures", strlen("textures"));
  JsonValue* images =
      hash_map_get_value(&json->object, "images", strlen("images"));
  JsonValue* accessors =
      hash_map_get_value(&json->object, "accessors", strlen("accessors"));
  JsonValue* bufferViews =
      hash_map_get_value(&json->object, "bufferViews", strlen("bufferViews"));
  JsonValue* buffers =
      hash_map_get_value(&json->object, "buffers", strlen("buffers"));
  if (nodes == NULL || nodes->type != JT_ARRAY || meshes == NULL
      || meshes->type != JT_ARRAY || materials == NULL
      || materials->type != JT_ARRAY || textures == NULL
      || textures->type != JT_ARRAY || images == NULL
      || images->type != JT_ARRAY || accessors == NULL
      || accessors->type != JT_ARRAY || bufferViews == NULL
      || bufferViews->type != JT_ARRAY || buffers == NULL
      || buffers->type != JT_ARRAY)
  {
    LOG_ERROR("Fields missing from GLTF.");
    return false;
  }

  glb_json_chunk_parse_nodes(nodes, &jsonChunk->nodes);
  glb_json_chunk_parse_meshes(meshes, &jsonChunk->meshes);
  glb_json_chunk_parse_materials(materials, &jsonChunk->materials);
  glb_json_chunk_parse_textures(textures, &jsonChunk->textures);
  glb_json_chunk_parse_images(images, &jsonChunk->images);
  if (!glb_json_chunk_parse_accessors(accessors, &jsonChunk->accessors)
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
  auto_array_destroy(&jsonChunk->materials);
  auto_array_destroy(&jsonChunk->textures);

  for (uint32_t i = 0; i < jsonChunk->nodes.size; i++)
  {
    GlbNode* node = auto_array_get(&jsonChunk->nodes, i);
    auto_array_destroy(&node->children);
  }

  auto_array_destroy(&jsonChunk->nodes);

  for (uint32_t i = 0; i < jsonChunk->meshes.size; i++)
  {
    auto_array_destroy(
        &((GlbMesh*) auto_array_get(&jsonChunk->meshes, i))->primitives);
  }
  auto_array_destroy(&jsonChunk->meshes);
}

