#pragma once
#include "Engine/Renderer/MeshBuilder.hpp"
#include <vector>
#include <queue>
#include "../Core/Memory/UntrackedAllocator.hpp"

class Mesh;
class Matrix4x4;
class Skeleton;
class AnimationMotion;

class SceneImport
{
public:
	std::vector<MeshBuilder> meshes;
	std::vector<Skeleton*> skeletons;
	std::vector<AnimationMotion*> motions;
};

//STANDALONE FUNCTIONS//////////////////////////////////////////////////////////////////////////
void FbxListScene(const char* filename);
SceneImport* FbxLoadSceneFromFile(const char* fbxFilename, const Matrix4x4& engineBasis, bool isEngineBasisRightHanded, const Matrix4x4& transform);

extern std::queue<Mesh*> g_loadedMeshes;

