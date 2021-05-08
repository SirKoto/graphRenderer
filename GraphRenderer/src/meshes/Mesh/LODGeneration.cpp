#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/hash.hpp>

#include "../Mesh.h"

#include <Eigen/Dense>
#include <Eigen/src/SVD/JacobiSVD.h>
#include <bitset>


typedef Eigen::Matrix4d Mat4;
typedef Eigen::Vector4d Vec4;


struct Task {
	std::vector<uint32_t> vertices;
	uint32_t depth;
	glm::vec3 midCoord;
};

void gr::Mesh::regenerateLODs(FrameContext* fc)
{
	//std::vector<Node> nodes;

	std::stack<Task> tasks;

	if (mLODs.empty()) {
		return;
	}

	// map depth to index of LOD
	std::map<uint32_t, uint32_t> depthToLod;
	for (uint32_t i = 0; i < (uint32_t)mLODs.size(); ++i) {
		mLODs[i].indices.clear();
		mLODs[i].vertices.clear();
		depthToLod.insert({ mLODs[i].depth, i });
	}

	uint32_t maxDepth = mLODs.front().depth;

	const float octreeSize = std::max(mBBox.getSize().x, std::max(mBBox.getSize().y, mBBox.getSize().z));


	{
		Task root;
		root.vertices.resize(mVertices.size());
		std::iota(root.vertices.begin(), root.vertices.end(), 0);
		root.depth = 0;
		root.midCoord = (mBBox.getMin() + octreeSize * 0.5f);
		tasks.push(std::move(root));
	}


	std::array<std::vector<uint32_t>, 8> childsVert = {};
	for (auto& v : childsVert) v.reserve(mVertices.size() / 6);

	std::vector<std::vector<uint32_t>> old2newVerticesInLod;
	old2newVerticesInLod.resize(mLODs.size());
	for (auto& v : old2newVerticesInLod) v.resize(mVertices.size());


	// Compute the planes of all triangles
	std::vector<Vec4> trianglePlanes(mIndices.size() / 3);
	for (uint32_t t = 0; t < (uint32_t)mIndices.size() / 3; ++t) {
		const glm::vec3& v0 = mVertices[mIndices[3 * t + 0]].pos;
		const glm::vec3& v1 = mVertices[mIndices[3 * t + 1]].pos;
		const glm::vec3& v2 = mVertices[mIndices[3 * t + 2]].pos;

		glm::vec3 n = glm::cross(v1 - v0, v2 - v0);
		n = glm::normalize(n);

		float d = -glm::dot(n, v0);

		trianglePlanes[t] = Vec4(n.x, n.y, n.z, d);
	}

	// Compute V:{F}
	// Vertex positions on the structure.
	std::vector<std::vector<uint32_t>> vert2faces(mVertices.size());
	std::vector<uint32_t> vertexArity(mVertices.size(), 0);
	for (uint32_t t = 0; t < (uint32_t)mIndices.size() / 3; ++t) {
		vertexArity[mIndices[3 * t + 0]] += 1;
		vertexArity[mIndices[3 * t + 1]] += 1;
		vertexArity[mIndices[3 * t + 2]] += 1;
	}
	for (uint32_t v = 0; v < (uint32_t)mVertices.size(); ++v) {
		vert2faces[v].reserve(vertexArity[v]);
	}
	for (uint32_t t = 0; t < (uint32_t)mIndices.size() / 3; ++t) {
		vert2faces[mIndices[3 * t + 0]].push_back(t);
		vert2faces[mIndices[3 * t + 1]].push_back(t);
		vert2faces[mIndices[3 * t + 2]].push_back(t);
	}

	while (!tasks.empty()) {

		const Task task = std::move(tasks.top());
		tasks.pop();

		float size = octreeSize / static_cast<float>(1 << task.depth);

		for (auto& v : childsVert) v.clear();

		for (const uint32_t i : task.vertices) {
			const glm::vec3 dir = mVertices[i].pos - task.midCoord;
			uint32_t k = 
						((dir.x >= 0.f ? 1 : 0) << 0) +
						((dir.y >= 0.f ? 1 : 0) << 1) +
						((dir.z >= 0.f ? 1 : 0) << 2);
			childsVert[k].push_back(i);
		}

		// keep generating nodes....
		if (maxDepth  > task.depth) {

			for (uint32_t k = 0; k < 8; ++k) {

				if (!childsVert[k].empty()) {
					glm::vec3 dir = { k & 0b1 ? 1 : -1, k & 0b10 ? 1 : -1, k & 0b100 ? 1 : -1 };
					Task newT;
					newT.depth = task.depth + 1;
					newT.midCoord = task.midCoord + 0.25f * size * dir;
					newT.vertices = childsVert[k];
					tasks.push(std::move(newT));
				}
			}
		}

		// if this depth is one of the LODs, store vertex
		decltype(depthToLod)::const_iterator it = depthToLod.find(task.depth);
		if (it != depthToLod.end()) {
			Vertex v;
			v.pos = task.midCoord;
			v.normal = glm::vec3(0);

			// Quadric error metric
			{
				Mat4 K = Mat4::Zero();

				for (const uint32_t& vId : task.vertices) {
					for (const uint32_t& f : vert2faces[vId]) {
						const Vec4& P = trianglePlanes[f];

						const Vec4 p = Vec4(P[0], P[1], P[2], P[3] + P.dot(Vec4(task.midCoord.x, task.midCoord.y, task.midCoord.z, 0)));
						K += p * p.transpose();
					}
					v.normal += mVertices[vId].normal;
				}
				v.normal /= task.vertices.size();

				const Vec4 rhs = Vec4(0, 0, 0, 1.0);
				// Set last row to 0, 0, 0, 1
				K.row(3) = rhs;
				// Compute SVD
				Eigen::JacobiSVD<Mat4> svd(K, Eigen::ComputeFullU | Eigen::ComputeFullV);
				svd.setThreshold(1e-6);
				const Vec4 res = svd.solve(rhs);

				v.pos = glm::vec3(res[0], res[1], res[2]) + task.midCoord;
			}


			// store new vertex
			uint32_t vertexIdx = (uint32_t) mLODs[it->second].vertices.size();
			mLODs[it->second].vertices.push_back(v);

			// Store translation table of the new vertex
			for (uint32_t i : task.vertices) {
				old2newVerticesInLod[it->second][i] = vertexIdx;
			}
		}

	}

	// create faces from indices
	std::unordered_set<glm::uvec3> alreadyAddedFaces;
	alreadyAddedFaces.reserve(mIndices.size() / 3);
	for (uint32_t lod = 0; lod < (uint32_t)mLODs.size(); ++lod) {
		alreadyAddedFaces.clear();

		for (uint32_t i = 0; i < mIndices.size() / 3; ++i) {
			glm::uvec3 f = {	old2newVerticesInLod[lod][mIndices[3 * i + 0]],
								old2newVerticesInLod[lod][mIndices[3 * i + 1]],
								old2newVerticesInLod[lod][mIndices[3 * i + 2]] };

			// if face has area, and does not exist yet
			if (f.x != f.y && f.x != f.z && f.y != f.z && alreadyAddedFaces.count(f) == 0) {
				alreadyAddedFaces.insert(f);
				mLODs[lod].indices.push_back(f.x);
				mLODs[lod].indices.push_back(f.y);
				mLODs[lod].indices.push_back(f.z);

			}

		}


	}

}