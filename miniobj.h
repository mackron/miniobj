// Public domain. See "unlicense" statement at the end of this file.

// ABOUT
//
// miniobj is a simple library for loading OBJ mesh files. This does not load MTL files, but does keep track of them
// to allow an application to do their own loading. Only a tiny subset of features are currently supported:
//   - Only basic triangle/face polygonal meshes are supported.
//   - Freeform meshes are not supported (Bazier curves, etc.)
//   - Groups are ignored.
//
//
//
// USAGE
//
// This is a single-file library. To use it, do something like the following in one .c file.
//   #define MINIOBJ_IMPLEMENTATION
//   #include "miniobj.h"
//
// You can then #include this file in other parts of the program as you would with any other header file.
// 
//
//
// OPTIONS
// #define these options before including this file.
//
// #define MINIOBJ_NO_STDIO
//   Disable miniobj_load_file().
//
//
//
// QUICK NOTES
// - Only triangle polygonal geometry is currently supported.
//
//
//
// TODO
// 

#ifndef miniobj_h
#define miniobj_h

#include <stddef.h> /* For size_t. */

/* Sized types. Prefer built-in types. Fall back to stdint. */
#ifdef _MSC_VER
    #if defined(__clang__)
        #pragma GCC diagnostic push
        #pragma GCC diagnostic ignored "-Wlanguage-extension-token"
        #pragma GCC diagnostic ignored "-Wc++11-long-long"
    #endif
    typedef   signed __int8  miniobj_int8;
    typedef unsigned __int8  miniobj_uint8;
    typedef   signed __int16 miniobj_int16;
    typedef unsigned __int16 miniobj_uint16;
    typedef   signed __int32 miniobj_int32;
    typedef unsigned __int32 miniobj_uint32;
    typedef   signed __int64 miniobj_int64;
    typedef unsigned __int64 miniobj_uint64;
    #if defined(__clang__)
        #pragma GCC diagnostic pop
    #endif
#else
    #define MINIOBJ_HAS_STDINT
    #include <stdint.h>
    typedef int8_t   miniobj_int8;
    typedef uint8_t  miniobj_uint8;
    typedef int16_t  miniobj_int16;
    typedef uint16_t miniobj_uint16;
    typedef int32_t  miniobj_int32;
    typedef uint32_t miniobj_uint32;
    typedef int64_t  miniobj_int64;
    typedef uint64_t miniobj_uint64;
#endif

#ifdef MINIOBJ_HAS_STDINT
    typedef uintptr_t miniobj_uintptr;
#else
    #if defined(_WIN32)
        #if defined(_WIN64)
            typedef miniobj_uint64 miniobj_uintptr;
        #else
            typedef miniobj_uint32 miniobj_uintptr;
        #endif
    #elif defined(__GNUC__)
        #if defined(__LP64__)
            typedef miniobj_uint64 miniobj_uintptr;
        #else
            typedef miniobj_uint32 miniobj_uintptr;
        #endif
    #else
        typedef miniobj_uint64 miniobj_uintptr;   /* Fallback. */
    #endif
#endif

typedef miniobj_uint8  miniobj_bool8;
typedef miniobj_uint32 miniobj_bool32;
#define MINIOBJ_TRUE   1
#define MINIOBJ_FALSE  0

#ifdef __cplusplus
extern "C" {
#endif

// Callback for when data is read. Return value is the number of bytes actually read.
typedef size_t (* miniobj_read_proc)(void* userData, void* bufferOut, size_t bytesToRead);

// Callback for when the loader needs to be seeked back to the start of the file.
typedef miniobj_bool32 (* miniobj_seek_to_start_proc)(void* userData);

typedef struct
{
    // The name.
    char* name;

} miniobj_mtllib;

typedef struct
{
    // The index of the first face that uses this material.
    miniobj_uint32 firstFace;

    // The number of faces that use this material (starting from <firstFace>).
    miniobj_uint32 faceCount;

    // The name of the material.
    char* name;

} miniobj_material;

typedef struct
{
    float v[3];
} miniobj_vec3;

typedef struct
{
    float v[4];
} miniobj_vec4;

typedef struct
{
    int positionIndex;
    int texcoordIndex;
    int normalIndex;
} miniobj_face_vertex;

typedef struct
{
    miniobj_face_vertex v[4];     // <-- The 4th component is only used internally. All faces are converted to triangles.
} miniobj_face;

typedef struct
{
    // The material library count.
    miniobj_uint32 materialLibCount;

    // A pointer to the list of material libraries.
    miniobj_mtllib* pMaterialLibs;


    // The number of materials used by the mesh.
    miniobj_uint32 materialCount;

    // A pointer to the list of materials used by the mesh.
    miniobj_material* pMaterials;


    // The number of positions.
    miniobj_uint32 positionCount;
    
    // The buffer containing the vertex positions.
    miniobj_vec4* pPositions;


    // The number of texture coordinates.
    miniobj_uint32 texCoordCount;

    // The buffer containing the text coordinates.
    miniobj_vec3* pTexCoords;


    // The number of normals.
    miniobj_uint32 normalCount;

    // The buffer containing the normal positions.
    miniobj_vec3* pNormals;


    // The face count.
    miniobj_uint32 faceCount;

    // A pointer to the face data.
    miniobj_face* pFaces;


    // A pointer variable length strings that contains the names of materials and whatnot. This is an offset of
    // pData and every string is stored immediately after each other and are null terminated.
    char* pStrings;

    // A pointer to the raw data. This contains the vertex and index data.
    char pData[1];

} miniobj;


// Loads an OBJ file using the given callbacks.
miniobj* miniobj_load(miniobj_read_proc onRead, miniobj_seek_to_start_proc onSeek, void* pUserData);

// Deletes the given OBJ file.
void miniobj_delete(miniobj* pOBJ);


#ifndef MINIOBJ_NO_STDIO
// Loads an OBJ file from an actual file.
miniobj* miniobj_load_file(const char* pFile);
#endif

// Helper for loading an OBJ file from a block of memory.
miniobj* miniobj_load_memory(const void* pData, size_t dataSize);


// Frees an internal allocation.
void miniobj_free(void* pData);


// Helper for interleaving the vertex data of the given OBJ object.
//
// Free the returned pointers with miniobj_free().
void miniobj_interleave_p3t2n3(miniobj* pOBJ, miniobj_uint32* pVertexCountOut, float** ppVertexDataOut, miniobj_uint32* pIndexCountOut, miniobj_uint32** ppIndexDataOut);
void miniobj_interleave_p3t2n3_material(miniobj* pOBJ, miniobj_uint32 materialIndex, miniobj_uint32* pVertexCountOut, float** ppVertexDataOut, miniobj_uint32* pIndexCountOut, miniobj_uint32** ppIndexDataOut);


#ifdef __cplusplus
}
#endif

#endif  // miniobj_h


///////////////////////////////////////////////////////////////////////////////
//
// IMPLEMENTATION
//
///////////////////////////////////////////////////////////////////////////////
#ifdef MINIOBJ_IMPLEMENTATION
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <math.h>   // Use for powf() - can probably be removed later.

#ifndef MINIOBJ_NO_STDIO
#include <stdio.h>

static size_t miniobj__on_read_stdio(void* pUserData, void* bufferOut, size_t bytesToRead)
{
    return fread(bufferOut, 1, bytesToRead, (FILE*)pUserData);
}

static miniobj_bool32 miniobj__on_seek_stdio(void* pUserData)
{
    return fseek((FILE*)pUserData, 0, SEEK_SET) == 0;
}

miniobj* miniobj_load_file(const char* filename)
{
    FILE* pFile;
#ifdef _MSC_VER
    if (fopen_s(&pFile, filename, "rb") != 0) {
        return NULL;
    }
#else
    pFile = fopen(filename, "rb");
    if (pFile == NULL) {
        return NULL;
    }
#endif

    miniobj* pOBJ = miniobj_load(miniobj__on_read_stdio, miniobj__on_seek_stdio, pFile);
    
    fclose(pFile);
    return pOBJ;
}
#endif  // MINIOBJ_NO_STDIO


typedef struct
{
    // A pointer to the beginning of the data. We use a char as the type here for easy offsetting.
    const unsigned char* data;
    size_t dataSize;
    size_t currentReadPos;
} miniobj_memory;

static size_t miniobj__on_read_memory(void* pUserData, void* bufferOut, size_t bytesToRead)
{
    miniobj_memory* memory = (miniobj_memory*)pUserData;
    assert(memory != NULL);
    assert(memory->dataSize >= memory->currentReadPos);

    size_t bytesRemaining = memory->dataSize - memory->currentReadPos;
    if (bytesToRead > bytesRemaining) {
        bytesToRead = bytesRemaining;
    }

    if (bytesToRead > 0) {
        memcpy(bufferOut, memory->data + memory->currentReadPos, bytesToRead);
        memory->currentReadPos += bytesToRead;
    }

    return bytesToRead;
}

static miniobj_bool32 miniobj__on_seek_memory(void* pUserData)
{
    miniobj_memory* memory = (miniobj_memory*)pUserData;
    assert(memory != NULL);

    memory->currentReadPos = 0;
    return 1;
}

miniobj* miniobj_load_memory(const void* data, size_t dataSize)
{
    miniobj_memory memory;
    memory.data = (const unsigned char*)data;
    memory.dataSize = dataSize;
    memory.currentReadPos = 0;
    return miniobj_load(miniobj__on_read_memory, miniobj__on_seek_memory, &memory);
}


void miniobj_free(void* pData)
{
    free(pData);
}


miniobj_bool32 miniobj__find_face_vertex(miniobj_uint32 vertexCount, miniobj_face_vertex* pVertices, miniobj_face_vertex vertex, miniobj_uint32* pIndexOut)
{
    for (miniobj_uint32 i = 0; i < vertexCount; ++i) {
        if (pVertices[i].positionIndex == vertex.positionIndex && pVertices[i].texcoordIndex == vertex.texcoordIndex && pVertices[i].normalIndex == vertex.normalIndex) {
            *pIndexOut = i;
            return MINIOBJ_TRUE;
        }
    }

    return MINIOBJ_FALSE;
}

void miniobj_interleave_p3t2n3(miniobj* pOBJ, miniobj_uint32* pVertexCountOut, float** ppVertexDataOut, miniobj_uint32* pIndexCountOut, miniobj_uint32** ppIndexDataOut)
{
    // When interleaving we want to ensure we don't copy over duplicate vertices. For example, a quad will be made up of two triangles, with two
    // vertices being shared by both faces (along the common edge dividing the two triangles). We don't want to duplicate that data, so when creating
    // an index for a face, we first want to check that it hasn't already been added.

    // Create output buffers large enough to contain the interleaved data.
    miniobj_uint32 indexCount = 0;
    miniobj_uint32* pIndexData = (miniobj_uint32*)malloc(sizeof(miniobj_uint32) * pOBJ->faceCount*3);

    miniobj_uint32 vertexCount = 0;
    float* pVertexData = (float*)malloc((sizeof(float)*(3+2+3)) * pOBJ->faceCount*3);


    miniobj_uint32 uniqueVertexCount = 0;
    miniobj_face_vertex* pUniqueVertices = (miniobj_face_vertex*)malloc(sizeof(miniobj_face_vertex) * pOBJ->faceCount*3);

    for (miniobj_uint32 iFace = 0; iFace < pOBJ->faceCount; ++iFace)
    {
        for (miniobj_uint32 iFaceVertex = 0; iFaceVertex < 3; ++iFaceVertex)
        {
            miniobj_uint32 index;
            if (!miniobj__find_face_vertex(uniqueVertexCount, pUniqueVertices, pOBJ->pFaces[iFace].v[iFaceVertex], &index))
            {
                pUniqueVertices[uniqueVertexCount] = pOBJ->pFaces[iFace].v[iFaceVertex];
                index = uniqueVertexCount++;

                pVertexData[(index*8) + 0] = pOBJ->pPositions[pOBJ->pFaces[iFace].v[iFaceVertex].positionIndex].v[0];
                pVertexData[(index*8) + 1] = pOBJ->pPositions[pOBJ->pFaces[iFace].v[iFaceVertex].positionIndex].v[1];
                pVertexData[(index*8) + 2] = pOBJ->pPositions[pOBJ->pFaces[iFace].v[iFaceVertex].positionIndex].v[2];

                pVertexData[(index*8) + 3] = pOBJ->pTexCoords[pOBJ->pFaces[iFace].v[iFaceVertex].texcoordIndex].v[0];
                pVertexData[(index*8) + 4] = pOBJ->pTexCoords[pOBJ->pFaces[iFace].v[iFaceVertex].texcoordIndex].v[1];

                pVertexData[(index*8) + 5] = pOBJ->pNormals[pOBJ->pFaces[iFace].v[iFaceVertex].normalIndex].v[0];
                pVertexData[(index*8) + 6] = pOBJ->pNormals[pOBJ->pFaces[iFace].v[iFaceVertex].normalIndex].v[1];
                pVertexData[(index*8) + 7] = pOBJ->pNormals[pOBJ->pFaces[iFace].v[iFaceVertex].normalIndex].v[2];

                vertexCount += 1;
            }

            pIndexData[indexCount++] = index;
        }
    }


    free(pUniqueVertices);

    if (pIndexCountOut) {
        *pIndexCountOut = indexCount;
    }

    if (pVertexCountOut) {
        *pVertexCountOut = vertexCount;
    }

    
    if (ppIndexDataOut != NULL) {
        *ppIndexDataOut = pIndexData;
    } else {
        free(pIndexData);
    }

    if (ppVertexDataOut != NULL) {
        *ppVertexDataOut = pVertexData;
    } else {
        free(pIndexData);
    }
}

void miniobj_interleave_p3t2n3_material(miniobj* pOBJ, miniobj_uint32 materialIndex, miniobj_uint32* pVertexCountOut, float** ppVertexDataOut, miniobj_uint32* pIndexCountOut, miniobj_uint32** ppIndexDataOut)
{
    // When interleaving we want to ensure we don't copy over duplicate vertices. For example, a quad will be made up of two triangles, with two
    // vertices being shared by both faces (along the common edge dividing the two triangles). We don't want to duplicate that data, so when creating
    // an index for a face, we first want to check that it hasn't already been added.
    miniobj_material* pMaterial = &pOBJ->pMaterials[materialIndex];

    // Create output buffers large enough to contain the interleaved data.
    miniobj_uint32 indexCount = 0;
    miniobj_uint32* pIndexData = (miniobj_uint32*)malloc(sizeof(miniobj_uint32) * pMaterial->faceCount*3);

    miniobj_uint32 vertexCount = 0;
    float* pVertexData = (float*)malloc((sizeof(float)*(3+2+3)) * pMaterial->faceCount*3);


    miniobj_uint32 uniqueVertexCount = 0;
    miniobj_face_vertex* pUniqueVertices = (miniobj_face_vertex*)malloc(sizeof(miniobj_face_vertex) * pMaterial->faceCount*3);

    for (miniobj_uint32 iFace = 0; iFace < pMaterial->faceCount; ++iFace)
    {
        for (miniobj_uint32 iFaceVertex = 0; iFaceVertex < 3; ++iFaceVertex)
        {
            miniobj_uint32 index;
            if (!miniobj__find_face_vertex(uniqueVertexCount, pUniqueVertices, pOBJ->pFaces[pMaterial->firstFace + iFace].v[iFaceVertex], &index))
            {
                pUniqueVertices[uniqueVertexCount] = pOBJ->pFaces[pMaterial->firstFace + iFace].v[iFaceVertex];
                index = uniqueVertexCount++;

                pVertexData[(index*8) + 0] = pOBJ->pPositions[pOBJ->pFaces[pMaterial->firstFace + iFace].v[iFaceVertex].positionIndex].v[0];
                pVertexData[(index*8) + 1] = pOBJ->pPositions[pOBJ->pFaces[pMaterial->firstFace + iFace].v[iFaceVertex].positionIndex].v[1];
                pVertexData[(index*8) + 2] = pOBJ->pPositions[pOBJ->pFaces[pMaterial->firstFace + iFace].v[iFaceVertex].positionIndex].v[2];

                pVertexData[(index*8) + 3] = pOBJ->pTexCoords[pOBJ->pFaces[pMaterial->firstFace + iFace].v[iFaceVertex].texcoordIndex].v[0];
                pVertexData[(index*8) + 4] = pOBJ->pTexCoords[pOBJ->pFaces[pMaterial->firstFace + iFace].v[iFaceVertex].texcoordIndex].v[1];

                pVertexData[(index*8) + 5] = pOBJ->pNormals[pOBJ->pFaces[pMaterial->firstFace + iFace].v[iFaceVertex].normalIndex].v[0];
                pVertexData[(index*8) + 6] = pOBJ->pNormals[pOBJ->pFaces[pMaterial->firstFace + iFace].v[iFaceVertex].normalIndex].v[1];
                pVertexData[(index*8) + 7] = pOBJ->pNormals[pOBJ->pFaces[pMaterial->firstFace + iFace].v[iFaceVertex].normalIndex].v[2];

                vertexCount += 1;
            }

            pIndexData[indexCount++] = index;
        }
    }


    free(pUniqueVertices);

    if (pIndexCountOut) {
        *pIndexCountOut = indexCount;
    }

    if (pVertexCountOut) {
        *pVertexCountOut = vertexCount;
    }

    
    if (ppIndexDataOut != NULL) {
        *ppIndexDataOut = pIndexData;
    } else {
        free(pIndexData);
    }

    if (ppVertexDataOut != NULL) {
        *ppVertexDataOut = pVertexData;
    } else {
        free(pIndexData);
    }
}



typedef struct
{
    miniobj_read_proc onRead;
    miniobj_seek_to_start_proc onSeek;
    void* pUserData;

    
    char buffer[4096];
    char* pNextBytes;
    size_t bytesRemaining;

    miniobj_uint32 materialLibCount;
    miniobj_uint32 materialCount;
    miniobj_uint32 positionCount;
    miniobj_uint32 texcoordCount;
    miniobj_uint32 normalCount;
    miniobj_uint32 faceCount;

    size_t totalStringLength;   // <-- Includes null terminators.
    size_t allocationSize;
    
} miniobj_load_context;

static inline miniobj_bool32 miniobj__is_whitespace(char c)
{
    return c == '\0' || c == ' ' || c == '\t' || c == '\n' || c == '\v' || c == '\f' || c == '\r';
}

static inline miniobj_bool32 miniobj__is_valid_digit(char c)
{
    return c >= '0' && c <= '9';
}

miniobj_bool32 miniobj__load_next_chunk(miniobj_load_context* pLoadContext)
{
    assert(pLoadContext != NULL);

    pLoadContext->bytesRemaining = pLoadContext->onRead(pLoadContext->pUserData, pLoadContext->buffer, sizeof(pLoadContext->buffer));
    if (pLoadContext->bytesRemaining == 0) {
        pLoadContext->pNextBytes = NULL;
        return MINIOBJ_FALSE;
    }


    pLoadContext->pNextBytes = pLoadContext->buffer;
    return MINIOBJ_TRUE;
}

miniobj_bool32 miniobj__seek_past_whitespace(miniobj_load_context* pLoadContext)
{
    assert(pLoadContext != NULL);

    for (;;)
    {
        while (pLoadContext->bytesRemaining > 0)
        {
            if (!miniobj__is_whitespace(pLoadContext->pNextBytes[0])) {
                return MINIOBJ_TRUE;
            }

            pLoadContext->pNextBytes     += 1;
            pLoadContext->bytesRemaining -= 1;
        }

        if (!miniobj__load_next_chunk(pLoadContext)) {
            return MINIOBJ_FALSE;
        }
    }
}

char* miniobj__find_end_of_line(const miniobj_load_context* pLoadContext)
{
    assert(pLoadContext != NULL);

    // This function does not load a new chunk. If it runs out of data in the chunk it will return MINIOBJ_FALSE.
    char* pLineEnd = pLoadContext->pNextBytes;
    size_t bytesRemaining = pLoadContext->bytesRemaining;
    while (bytesRemaining > 0)
    {
        if (pLineEnd[0] == '\0' || pLineEnd[0] == '\n' || (pLineEnd[0] == '\r' && bytesRemaining > 1 && pLineEnd[1] == '\n') || pLineEnd[0] == '#') {
            return pLineEnd;
        }

        pLineEnd       += 1;
        bytesRemaining -= 1;
    }

    // If we get here it means we've run out of data in the buffer. We don't want to read the next chunk in this function so
    // we return MINIOBJ_FALSE in this case.
    return NULL;
}

miniobj_bool32 miniobj__seek_to_next_line(miniobj_load_context* pLoadContext)
{
    assert(pLoadContext != NULL);

    for (;;)
    {
        while (pLoadContext->bytesRemaining > 0)
        {
            if (pLoadContext->pNextBytes[0] == '\n')
            {
                pLoadContext->pNextBytes     += 1;
                pLoadContext->bytesRemaining -= 1;

                return MINIOBJ_TRUE;
            }
            else if (pLoadContext->pNextBytes[0] == '\r' && pLoadContext->bytesRemaining > 1 && pLoadContext->pNextBytes[1] == '\n')     // Win32 line endings.
            {
                pLoadContext->pNextBytes     += 2;
                pLoadContext->bytesRemaining -= 2;

                return MINIOBJ_TRUE;
            }
            else
            {
                pLoadContext->pNextBytes     += 1;
                pLoadContext->bytesRemaining -= 1;
            }
        }

        if (!miniobj__load_next_chunk(pLoadContext)) {
            return MINIOBJ_FALSE;
        }
    }
}

miniobj_bool32 miniobj__read_next_line(miniobj_load_context* pLoadContext, char** pLineBeg, char** pLineEnd)
{
    assert(pLineBeg != NULL);
    assert(pLineEnd != NULL);

    // We don't want to include whitespace in the result.
    if (!miniobj__seek_past_whitespace(pLoadContext)) {
        return MINIOBJ_FALSE;
    }

    // The entire line must be contained within pLoadContext->buffer.

    char* lineBeg = pLoadContext->pNextBytes;
    char* lineEnd = miniobj__find_end_of_line(pLoadContext);
    if (lineEnd == NULL)
    {
        // If we get here it means we've either reached the end of the file or ran out of data in the buffer. If we've simply reached the
        // end of the file it's not an error. If we've run out of data we need to load the next chunk.
        size_t lineLengthSoFar = pLoadContext->bytesRemaining;
        memmove(pLoadContext->buffer, lineBeg, lineLengthSoFar);

        lineBeg = pLoadContext->buffer;
        lineEnd = lineBeg + lineLengthSoFar;

        pLoadContext->bytesRemaining = pLoadContext->onRead(pLoadContext->pUserData, pLoadContext->buffer + lineLengthSoFar, sizeof(pLoadContext->buffer) - lineLengthSoFar) + lineLengthSoFar;
        if (pLoadContext->bytesRemaining > 0)
        {
            // At this point we should have the rest of the line in the buffer so we just try finding the end of it again.
            pLoadContext->pNextBytes = pLoadContext->buffer;
            lineEnd = miniobj__find_end_of_line(pLoadContext);
        }
        
        if (lineEnd == NULL)
        {
            pLoadContext->pNextBytes    += pLoadContext->bytesRemaining;
            pLoadContext->bytesRemaining = 0;

            *pLineBeg = lineBeg;
            *pLineEnd = lineBeg + lineLengthSoFar;
            return MINIOBJ_TRUE;
        }
    }



    // We don't want to return empty lines, so if the line is empty, just try getting the next one.
    if (lineEnd == lineBeg)
    {
        if (!miniobj__seek_to_next_line(pLoadContext)) {
            return MINIOBJ_FALSE;
        }

        return miniobj__read_next_line(pLoadContext, pLineBeg, pLineEnd);
    }
    else
    {
        // At this point we should be good. We need to move to the start of the next line.
        size_t lineLength = (lineEnd - lineBeg);
        if (lineLength > pLoadContext->bytesRemaining) {
            lineLength = pLoadContext->bytesRemaining;
        }

        pLoadContext->pNextBytes     += lineLength;
        pLoadContext->bytesRemaining -= lineLength;
        miniobj__seek_to_next_line(pLoadContext);

        *pLineBeg = lineBeg;
        *pLineEnd = lineEnd;
        return MINIOBJ_TRUE;
    }
}

size_t miniobj__parse_mtl_name(const char* pMtlName, const char* pLineEnd, char* pMtlNameOut)
{
    while (pMtlName < pLineEnd && miniobj__is_whitespace(pMtlName[0])) {
        pMtlName += 1;
    }

    const char* pNameBeg = pMtlName;
    const char* pLastNonWhitespaceChar = pMtlName;

    while (pMtlName[0] != '#' && pMtlName[0] != '\n' && pMtlName < pLineEnd) {
        if (!miniobj__is_whitespace(pMtlName[0])) {
            pLastNonWhitespaceChar = pMtlName;
        }

        pMtlName += 1;
    }

    size_t nameLength = pLastNonWhitespaceChar - pNameBeg + 1;

    if (pMtlNameOut != NULL)
    {
        if (nameLength > 0) {
            memcpy(pMtlNameOut, pNameBeg, nameLength);
        }

        pMtlNameOut[nameLength] = '\0';
    }

    return nameLength;
}




// Special implementation of atof() for converting a string to a float.
miniobj_bool32 miniobj__atof(const char* str, const char* strEnd, float* pResultOut, const char** strEndOut)
{
    // Skip leading whitespace.
    while (str < strEnd && miniobj__is_whitespace(*str)) {
        str += 1;
    }


    // Check that we have a string after moving the whitespace.
    if (str < strEnd)
    {
        float sign  = 1.0f;
        float value = 0.0f;

        // Sign.
        if (*str == '-')
        {
            sign = -1.0f;
            str += 1;
        }
        else if (*str == '+')
        {
            sign = 1.0f;
            str += 1;
        }


        // Digits before the decimal point.
        while (str < strEnd && miniobj__is_valid_digit(*str))
        {
            value = value * 10.0f + (*str - '0');

            str += 1;
        }

        // Digits after the decimal point.
        if (*str == '.')
        {
            float pow10 = 10.0f;

            str += 1;
            while (str < strEnd && miniobj__is_valid_digit(*str))
            {
                value += (*str - '0') / pow10;
                pow10 *= 10.0f;

                str += 1;
            }


            // Exponential notation.
            if (*str == 'e') {
                float esign = 1;
                float evalue = 0;

                str += 1;
                if (*str == '-') {
                    esign = -1;
                    str += 1;
                } else if (*str == '+') {
                    str += 1;
                }
                
                while (str < strEnd && miniobj__is_valid_digit(*str)) {
                    evalue = evalue * 10.0f + (*str - '0');
                    str += 1;
                }

                if (esign < 0) {
                    value /= powf(10.0f, evalue);
                } else {
                    value *= powf(10.0f, evalue);
                }
            }
        }

            
        if (strEndOut != NULL) {
            *strEndOut = str;
        }

        *pResultOut = sign * value;
        return MINIOBJ_TRUE;
    }
    else
    {
        // Empty string.
        *pResultOut = 0;
        return MINIOBJ_FALSE;
    }
}


// Parses a single index in a face's p/t/n string.
int miniobj__parse_face_vertex_index(const char* str, const char* strEnd, const char** strEndOut)
{
    int sign  = 1;
    int value = 0;

    if (str < strEnd)
    {
        // Sign.
        if (*str == '-')
        {
            sign = -1;
            str += 1;
        }
        else if (*str == '+')
        {
            sign = 1;
            str += 1;
        }


        // Value.
        while (str < strEnd && miniobj__is_valid_digit(*str))
        {
            value = value * 10 + (*str - '0');
            str += 1;
        }
    }


    if (strEndOut != NULL) {
        *strEndOut = str;
    }
        
    return sign * value;
}


// Parses a face vertex index string in p/t/n format.
miniobj_bool32 miniobj__parse_face_vertex(const char* str, const char* strEnd, const char** strEndOut, miniobj_face_vertex* pVertexOut)
{
    assert(pVertexOut != NULL);

    miniobj_face_vertex result;
    result.positionIndex = 0;
    result.texcoordIndex = 0;
    result.normalIndex   = 0;


    // Skip leading whitespace.
    while (str < strEnd && miniobj__is_whitespace(*str)) {
        str += 1;
    }


    // Check that we have a string after moving past the whitespace.
    if (str < strEnd)
    {
        result.positionIndex = miniobj__parse_face_vertex_index(str, strEnd, &str);

        if (str[0] == '/') {
            str += 1;
            result.texcoordIndex = miniobj__parse_face_vertex_index(str, strEnd, &str);

            if (str[0] == '/') {
                str += 1;
                result.normalIndex = miniobj__parse_face_vertex_index(str, strEnd, &str);
            }
        }

        if (strEndOut != NULL) {
            *strEndOut = str;
        }

        *pVertexOut = result;
        return MINIOBJ_TRUE;
    }
    else
    {
        if (strEndOut != NULL) {
            *strEndOut = str;
        }

        return MINIOBJ_FALSE;
    }
}

// Parses a face index string.
miniobj_uint32 miniobj__parse_face(const char* str, const char* strEnd, miniobj_face* pFaceOut)
{
    assert(str      != NULL);
    assert(strEnd   != NULL);
    assert(pFaceOut != NULL);

    miniobj__parse_face_vertex(str, strEnd, &str, pFaceOut->v + 0);
    miniobj__parse_face_vertex(str, strEnd, &str, pFaceOut->v + 1);
    miniobj__parse_face_vertex(str, strEnd, &str, pFaceOut->v + 2);

    miniobj_uint32 faceCount = 3;
    if (miniobj__parse_face_vertex(str, strEnd, &str, pFaceOut->v + 3)) {
        faceCount += 1;
    }

    return faceCount;
}


// Parses the value of a "v" element.
void miniobj__parse_v(const char* str, const char* strEnd, miniobj_vec4* pResultOut)
{
    assert(str        != NULL);
    assert(strEnd     != NULL);
    assert(pResultOut != NULL);

    // Format: x y z w
    // w is optional.

    miniobj__atof(str, strEnd, &pResultOut->v[0], &str);
    miniobj__atof(str, strEnd, &pResultOut->v[1], &str);
    miniobj__atof(str, strEnd, &pResultOut->v[2], &str);

    if (!miniobj__atof(str, strEnd, &pResultOut->v[3], &str)) {   // <-- The w component is optional. Defaults to 1.
        pResultOut->v[3] = 1;
    }
}

// Parses the valid of a "vn" element.
void miniobj__parse_vn(const char* str, const char* strEnd, miniobj_vec3* pResultOut)
{
    assert(str        != NULL);
    assert(strEnd     != NULL);
    assert(pResultOut != NULL);

    // Format: x y z
    miniobj__atof(str, strEnd, &pResultOut->v[0], &str);
    miniobj__atof(str, strEnd, &pResultOut->v[1], &str);
    miniobj__atof(str, strEnd, &pResultOut->v[2], &str);
}

// Parses the valid of a "vt" element.
void miniobj__parse_vt(const char* str, const char* strEnd, miniobj_vec3* pResultOut)
{
    assert(str        != NULL);
    assert(strEnd     != NULL);
    assert(pResultOut != NULL);

    // Format: x y z
    // y and z are optional. Both default to zero.
    miniobj__atof(str, strEnd, &pResultOut->v[0], &str);
    
    if (!miniobj__atof(str, strEnd, &pResultOut->v[1], &str)) {
        pResultOut->v[1] = 0;
        pResultOut->v[2] = 0;
        return;
    }

    if (!miniobj__atof(str, strEnd, &pResultOut->v[2], &str)) {
        pResultOut->v[2] = 0;
        return;
    }
}

miniobj_bool32 miniobj__load_stage1(miniobj_load_context* pLoadContext)
{
    assert(pLoadContext != NULL);
    assert(pLoadContext->onRead != NULL);

    char* lineBeg;
    char* lineEnd;
    while (miniobj__read_next_line(pLoadContext, &lineBeg, &lineEnd))     // <-- This will handle comments and leading whitespace for us.
    {
        if (lineBeg[0] == 'v' && miniobj__is_whitespace(lineBeg[1]))
        {
            // Position.
            pLoadContext->positionCount += 1;
        }
        else if (lineBeg[0] == 'v' && lineBeg[1] == 't' && miniobj__is_whitespace(lineBeg[2]))
        {
            // Texture coordinate.
            pLoadContext->texcoordCount += 1;
        }
        else if (lineBeg[0] == 'v' && lineBeg[1] == 'n' && miniobj__is_whitespace(lineBeg[2]))
        {
            // Normal.
            pLoadContext->normalCount += 1;
        }
        else if (lineBeg[0] == 'f' && miniobj__is_whitespace(lineBeg[1]))
        {
            // Face.
            pLoadContext->faceCount += 1;

            // If the face has more than 3 vertices it'll need to be triangulated which means more faces. The face needs to be parsed.
            miniobj_face face;
            miniobj_uint32 vertexCount = miniobj__parse_face(lineBeg + 2, lineEnd, &face);
            if (vertexCount > 3) {
                if (vertexCount > 4) {
                    assert(MINIOBJ_FALSE);  // Not currently supporting more than 4 vertices per face.
                }

                pLoadContext->faceCount += 1;
            }
        }
        else if (lineBeg[0] == 'm' && lineBeg[1] == 't' && lineBeg[2] == 'l' && lineBeg[3] == 'l' && lineBeg[4] == 'i' && lineBeg[5] == 'b' && miniobj__is_whitespace(lineBeg[6]))
        {
            // mtllib.
            pLoadContext->materialLibCount += 1;
            pLoadContext->totalStringLength += miniobj__parse_mtl_name(lineBeg + 6, lineEnd, NULL) + 1;   // +1 for null terminator.
        }
        else if (lineBeg[0] == 'u' && lineBeg[1] == 's' && lineBeg[2] == 'e' && lineBeg[3] == 'm' && lineBeg[4] == 't' && lineBeg[5] == 'l' && miniobj__is_whitespace(lineBeg[6]))
        {
            // usemtl.
            pLoadContext->materialCount += 1;   
            pLoadContext->totalStringLength += miniobj__parse_mtl_name(lineBeg + 6, lineEnd, NULL) + 1;   // +1 for null terminator.
        }
        else
        {
            // Either an unknown label, or we simply don't care about it. Skip the line.
            continue;
        }
    }

    // We always want at least one position, texture coordinate and normal.
    if (pLoadContext->positionCount == 0) {
        pLoadContext->positionCount = 1;
    }
    if (pLoadContext->texcoordCount == 0) {
        pLoadContext->texcoordCount = 1;
    }
    if (pLoadContext->normalCount == 0) {
        pLoadContext->normalCount = 1;
    }

    // Always want at least one material.
    if (pLoadContext->materialCount == 0) {
        pLoadContext->materialCount = 1;
    }
    

    pLoadContext->allocationSize =
        pLoadContext->materialLibCount * sizeof(miniobj_mtllib) +
        pLoadContext->materialCount    * sizeof(miniobj_material) +
        pLoadContext->positionCount    * sizeof(miniobj_vec4) + 
        pLoadContext->texcoordCount    * sizeof(miniobj_vec3) +
        pLoadContext->normalCount      * sizeof(miniobj_vec3) +
        pLoadContext->faceCount        * sizeof(miniobj_face) +
        pLoadContext->totalStringLength;

    return MINIOBJ_TRUE;
}

miniobj_bool32 miniobj__load_stage2(miniobj* pOBJ, miniobj_load_context* pLoadContext)
{
    assert(pOBJ != NULL);

    pLoadContext->materialLibCount  = 0;
    pLoadContext->materialCount     = 0;
    pLoadContext->positionCount     = 0;
    pLoadContext->texcoordCount     = 0;
    pLoadContext->normalCount       = 0;
    pLoadContext->faceCount         = 0;
    pLoadContext->totalStringLength = 0;

    char* lineBeg;
    char* lineEnd;
    while (miniobj__read_next_line(pLoadContext, &lineBeg, &lineEnd))     // <-- This will handle comments and leading whitespace for us.
    {
        if (lineBeg[0] == 'v' && miniobj__is_whitespace(lineBeg[1]))
        {
            // Position.
            miniobj__parse_v(lineBeg + 2, lineEnd, &pOBJ->pPositions[pLoadContext->positionCount]);
            pLoadContext->positionCount += 1;
        }
        else if (lineBeg[0] == 'v' && lineBeg[1] == 't' && miniobj__is_whitespace(lineBeg[2]))
        {
            // Texture coordinate.
            miniobj__parse_vt(lineBeg + 3, lineEnd, &pOBJ->pTexCoords[pLoadContext->texcoordCount]);
            pLoadContext->texcoordCount += 1;
        }
        else if (lineBeg[0] == 'v' && lineBeg[1] == 'n' && miniobj__is_whitespace(lineBeg[2]))
        {
            // Normal.
            miniobj__parse_vn(lineBeg + 3, lineEnd, &pOBJ->pNormals[pLoadContext->normalCount]);
            pLoadContext->normalCount += 1;
        }
        else if (lineBeg[0] == 'f' && miniobj__is_whitespace(lineBeg[1]))
        {
            // Face.
            miniobj_face face;
            miniobj_uint32 vertexCount = miniobj__parse_face(lineBeg + 2, lineEnd, &face);

            // Faces can have negative indices which are interpreted as being relative. Positive values are one based so they need to be changed to 0 based, also.
            for (miniobj_uint32 i = 0; i < vertexCount; ++i) {
                if (face.v[i].positionIndex > 0) {
                    face.v[i].positionIndex -= 1;
                } else {
                    face.v[i].positionIndex += pLoadContext->positionCount;
                }

                if (face.v[i].texcoordIndex > 0) {
                    face.v[i].texcoordIndex -= 1;
                } else {
                    face.v[i].texcoordIndex += pLoadContext->texcoordCount;
                }

                if (face.v[i].normalIndex > 0) {
                    face.v[i].normalIndex -= 1;
                } else {
                    face.v[i].normalIndex += pLoadContext->normalCount;
                }
            }

            pOBJ->pFaces[pLoadContext->faceCount] = face;
            pLoadContext->faceCount += 1;

            // Quads need to be converted to triangles.
            if (vertexCount > 3)
            {
                // TODO: Add support for polygons with an arbitrary number of triangles.
                miniobj_face face2;
                face2.v[0].positionIndex = face.v[2].positionIndex;
                face2.v[0].texcoordIndex = face.v[2].texcoordIndex;
                face2.v[0].normalIndex   = face.v[2].normalIndex;

                face2.v[1].positionIndex = face.v[3].positionIndex;
                face2.v[1].texcoordIndex = face.v[3].texcoordIndex;
                face2.v[1].normalIndex   = face.v[3].normalIndex;

                face2.v[2].positionIndex = face.v[0].positionIndex;
                face2.v[2].texcoordIndex = face.v[0].texcoordIndex;
                face2.v[2].normalIndex   = face.v[0].normalIndex;

                pOBJ->pFaces[pLoadContext->faceCount] = face2;
                pLoadContext->faceCount += 1;
            }
        }
        else if (lineBeg[0] == 'm' && lineBeg[1] == 't' && lineBeg[2] == 'l' && lineBeg[3] == 'l' && lineBeg[4] == 'i' && lineBeg[5] == 'b' && miniobj__is_whitespace(lineBeg[6]))
        {
            // mtllib.
            size_t nameLength = miniobj__parse_mtl_name(lineBeg + 6, lineEnd, pOBJ->pStrings + pLoadContext->totalStringLength);
            pOBJ->pMaterialLibs[pLoadContext->materialLibCount].name = pOBJ->pStrings + pLoadContext->totalStringLength;

            pLoadContext->totalStringLength += nameLength;
            pOBJ->pStrings[pLoadContext->totalStringLength++] = '\0';

            pLoadContext->materialLibCount += 1;
        }
        else if (lineBeg[0] == 'u' && lineBeg[1] == 's' && lineBeg[2] == 'e' && lineBeg[3] == 'm' && lineBeg[4] == 't' && lineBeg[5] == 'l' && miniobj__is_whitespace(lineBeg[6]))
        {
            // usemtl.
            size_t nameLength = miniobj__parse_mtl_name(lineBeg + 6, lineEnd, pOBJ->pStrings + pLoadContext->totalStringLength);
            pOBJ->pMaterials[pLoadContext->materialCount].name = pOBJ->pStrings + pLoadContext->totalStringLength;
            pOBJ->pMaterials[pLoadContext->materialCount].firstFace = pLoadContext->faceCount;
            pOBJ->pMaterials[pLoadContext->materialCount].faceCount = 0;

            // Previous material needs to have its face count updated.
            if (pLoadContext->materialCount > 0) {
                pOBJ->pMaterials[pLoadContext->materialCount-1].faceCount = pLoadContext->faceCount - pOBJ->pMaterials[pLoadContext->materialCount-1].firstFace;
            }
            

            pLoadContext->totalStringLength += nameLength;
            pOBJ->pStrings[pLoadContext->totalStringLength++] = '\0';

            pLoadContext->materialCount += 1;
        }
        else
        {
            // Either an unknown label, or we simply don't care about it. Skip the line.
            continue;
        }
    }

    // Previous material needs to have its face count updated.
    if (pLoadContext->materialCount > 0) {
        pOBJ->pMaterials[pLoadContext->materialCount-1].faceCount = pLoadContext->faceCount - pOBJ->pMaterials[pLoadContext->materialCount-1].firstFace;
    }

    // We always want at least one position, texture coordinate and normal. The first pass will have allocated memory for at least one of each.
    if (pLoadContext->positionCount == 0) {
        pOBJ->pPositions[0].v[0] = 0;
        pOBJ->pPositions[0].v[1] = 0;
        pOBJ->pPositions[0].v[2] = 0;
        pOBJ->pPositions[0].v[3] = 1;
    }
    if (pLoadContext->texcoordCount == 0) {
        pOBJ->pTexCoords[0].v[0] = 0;
        pOBJ->pTexCoords[0].v[1] = 0;
        pOBJ->pTexCoords[0].v[2] = 0;
    }
    if (pLoadContext->normalCount == 0) {
        pOBJ->pNormals[0].v[0] = 0;
        pOBJ->pNormals[0].v[1] = 0;
        pOBJ->pNormals[0].v[2] = 0;
    }

    // Always want at least one material.
    if (pLoadContext->materialCount == 0) {
        pLoadContext->materialCount = 1;
        pOBJ->pMaterials[0].firstFace = 0;
        pOBJ->pMaterials[0].faceCount = pLoadContext->faceCount;
        pOBJ->pMaterials[0].name = NULL;
    }


    return MINIOBJ_TRUE;
}

miniobj* miniobj_load(miniobj_read_proc onRead, miniobj_seek_to_start_proc onSeek, void* pUserData)
{
    if (onRead == NULL || onSeek == NULL) {
        return NULL;
    }

    // Loading is done is two stages. The first stage is a pre-load to determine the size of the allocation. The second stage
    // is where the data is actually loaded.
    miniobj_load_context loadContext;
    memset(&loadContext, 0, sizeof(loadContext));
    loadContext.onRead    = onRead;
    loadContext.onSeek    = onSeek;
    loadContext.pUserData = pUserData;

    miniobj_bool32 result = miniobj__load_stage1(&loadContext);
    if (!result) {
        return NULL;
    }

    // After the pre-load we need to seek back to the start and do the actual load.
    if (!onSeek(pUserData)) {
        return NULL;
    }

    miniobj* pOBJ = (miniobj*)malloc(sizeof(*pOBJ) + loadContext.allocationSize);
    if (pOBJ == NULL) {
        return NULL;
    }

    pOBJ->materialLibCount = loadContext.materialLibCount;
    pOBJ->pMaterialLibs    = (miniobj_mtllib*)pOBJ->pData;
    pOBJ->materialCount    = loadContext.materialCount;
    pOBJ->pMaterials       = (miniobj_material*)(((char*)pOBJ->pData) + (loadContext.materialLibCount * sizeof(*pOBJ->pMaterialLibs)));
    pOBJ->positionCount    = loadContext.positionCount;
    pOBJ->pPositions       = (miniobj_vec4*)(((char*)pOBJ->pMaterials) + (loadContext.materialCount * sizeof(*pOBJ->pMaterials)));
    pOBJ->texCoordCount    = loadContext.texcoordCount;
    pOBJ->pTexCoords       = (miniobj_vec3*)(((char*)pOBJ->pPositions) + (loadContext.positionCount * sizeof(*pOBJ->pPositions)));
    pOBJ->normalCount      = loadContext.normalCount;
    pOBJ->pNormals         = (miniobj_vec3*)(((char*)pOBJ->pTexCoords) + (loadContext.texcoordCount * sizeof(*pOBJ->pTexCoords)));
    pOBJ->faceCount        = loadContext.faceCount;
    pOBJ->pFaces           = (miniobj_face*)(((char*)pOBJ->pNormals) + (loadContext.normalCount * sizeof(*pOBJ->pNormals)));
    pOBJ->pStrings         = (char*)pOBJ->pFaces + (loadContext.faceCount * sizeof(*pOBJ->pFaces));

    result = miniobj__load_stage2(pOBJ, &loadContext);
    if (!result) {
        free(pOBJ);
        return NULL;
    }

    return pOBJ;
}

void miniobj_delete(miniobj* pOBJ)
{
    free(pOBJ);
}

#endif // MINIOBJ_IMPLEMENTATION