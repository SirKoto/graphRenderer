#pragma once

#include "Mesh.h"
#include "Texture.h"
#include "../utils/ConstExprHelp.h"

#include <unordered_map>
#include <set>
#include <shared_mutex>
#include <memory>

namespace gr
{

class FrameContext;
class GlobalContext;

class ResourceDictionary
{
public:

	// Set list of Types that the dictionary will handle
	using ResourceTypesList = 
		typename ctools::TypelistBuilder<Mesh, Texture>::typelist;
	static constexpr bool CONFIG_USE_DYNAMIC_CAST = true;

	ResourceDictionary& operator=(const ResourceDictionary&) = delete;

	typedef uint64_t ResId;

	template<typename T>
	ResId allocateObject(std::string objectName, T** outPtr = nullptr);

	template<typename T>
	void get(ResId id, T** object) const;

	void erase(ResId id);

	ResId getId(const std::string&  name) const;
	std::string getName(const ResId id) const;
	bool existsName(const std::string& name) const;
	bool exists(const ResId id) const;
	void rename(ResId id, const std::string& newName);

	template <typename T>
	std::vector<ResId> getAllObjectsOfType() const;


	void destroy(GlobalContext* gc);
	void flushDataAndFree(FrameContext* fc);

protected:

	std::vector<ResId> mObjectsToFree;
	std::unordered_map<ResId, std::unique_ptr<IObject>> mObjectsDictionary;
	std::array<std::set<ResId>, ctools::length<ResourceTypesList>()> mObjectsByType;

	std::unordered_map<std::string, ResId> mName2Id;

	ResId mNextId = 1;

	mutable std::mutex mEraseObjectMutex;
	mutable std::shared_mutex mObjectsMutex;
	mutable std::mutex mNextIdMutex;


	ResId getAndUpdateId();
	// Create new unique name from string. Does not lock!
	std::string createUniqueName(const std::string& string);

};

template<typename T>
ResourceDictionary::ResId ResourceDictionary::allocateObject(
	std::string objectName,
	T** outPtr)
{
	constexpr size_t typeIdx = ctools::indexOf<ResourceTypesList, T>();
	static_assert(typeIdx != -1, "Type not added to Dictionary");
	const ResId id = getAndUpdateId();


	// check if the name is unique or create new
	{
		std::unique_lock lock(mObjectsMutex);

		if (mName2Id.count(objectName) != 0) {
			objectName = createUniqueName(objectName);
		}

		std::pair<decltype(mName2Id)::const_iterator, bool> itName =
			mName2Id.emplace(std::move(objectName), id);
		assert(itName.second);
		// Create object and reference
	
		std::pair<decltype(mObjectsDictionary)::iterator, bool> itObj =
			mObjectsDictionary.emplace(id, std::unique_ptr<IObject>(new T));
		assert(itObj.second); // assert inserted
		mObjectsByType[typeIdx].insert(id);
		itObj.first->second->setObjectName(itName.first->first);

		// return reference
		if (outPtr != nullptr) {
			if (CONFIG_USE_DYNAMIC_CAST) {
				*outPtr = dynamic_cast<T*>(itObj.first->second.get());
				assert(*outPtr != nullptr);
			}
			else {
				*outPtr = reinterpret_cast<T*>(itObj.first->second.get());
			}
		}
	}

	return id;
}

template<typename T>
void gr::ResourceDictionary::get(ResId id, T** object) const
{
	assert(object != nullptr);
	std::shared_lock lock(mObjectsMutex);

	IObject* ptr = mObjectsDictionary.at(id).get();
	if (CONFIG_USE_DYNAMIC_CAST) {
		*object = dynamic_cast<T*>(ptr);
		assert(*object != nullptr);
	}
	else {
		*object = reinterpret_cast<T*>(ptr);
	}
}


template<typename T>
std::vector<gr::ResourceDictionary::ResId> 
gr::ResourceDictionary::getAllObjectsOfType() const
{
	constexpr size_t typeIdx = ctools::indexOf<ResourceTypesList, T>();
	static_assert(typeIdx != -1, "Type not added to Dictionary");
	std::shared_lock lock(mObjectsMutex);

	std::vector<ResId> res;
	res.reserve(mObjectsByType[typeIdx].size());
	for (const ResId& id : mObjectsByType[typeIdx]) {
		res.push_back(id);
	}

	return res;
}

} // namespace gr
