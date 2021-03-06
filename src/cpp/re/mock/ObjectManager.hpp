/*
 * Copyright (c) 2021 pongasoft
 *
 * Licensed under the Apache License, Version 2.0 (the "License"); you may not
 * use this file except in compliance with the License. You may obtain a copy of
 * the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS, WITHOUT
 * WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. See the
 * License for the specific language governing permissions and limitations under
 * the License.
 *
 * @author Yan Pujante
 */

#pragma once
#ifndef __Pongasoft_re_mock_object_manager_h__
#define __Pongasoft_re_mock_object_manager_h__

#include <map>
#include <atomic>
#include <functional>
#include "Errors.h"

namespace re::mock {

template<typename O, typename K = int>
class ObjectManager
{
public:
  using object_type = O;
  using key_type = K;

  K add(std::function<O(int)> iObjectFactory);
  K add(O &&iObject);
  void replace(K id, O &&iObject);
  O const &get(K id) const;
  O &get(K id);
  void remove(K id);
  void clear();
  void reset();
  size_t size() const;
  typename std::map<K, O>::iterator begin() noexcept { return fObjects.begin(); }
  typename std::map<K, O>::iterator end() noexcept { return fObjects.end(); }
  typename std::map<K, O>::const_iterator begin() const noexcept { return fObjects.begin(); }
  typename std::map<K, O>::const_iterator end() const noexcept { return fObjects.end(); }
  typename std::map<K, O>::const_iterator cbegin() const noexcept { return begin(); }
  typename std::map<K, O>::const_iterator cend() const noexcept { return end(); }

protected:
  std::map<K, O> fObjects{};

private:
  std::atomic<K> fCounter{1};
};

//------------------------------------------------------------------------
// ObjectManager::add
//------------------------------------------------------------------------
template<typename O, typename K>
K ObjectManager<O, K>::add(O &&iObject)
{
  auto id = fCounter++;
  fObjects[id] = std::move(iObject);
  return id;
}

//------------------------------------------------------------------------
// ObjectManager::add
//------------------------------------------------------------------------
template<typename O, typename K>
K ObjectManager<O, K>::add(std::function<O(int)> iObjectFactory)
{
  auto id = fCounter++;
  fObjects[id] = iObjectFactory(id);
  return id;
}


//------------------------------------------------------------------------
// ObjectManager::get
//------------------------------------------------------------------------
template<typename O, typename K>
O const &ObjectManager<O, K>::get(K id) const
{
  auto o = fObjects.find(id);
  RE_MOCK_ASSERT(o != fObjects.end(), "Missing object for key [%d]", id);
  return o->second;
}

//------------------------------------------------------------------------
// ObjectManager::get
//------------------------------------------------------------------------
template<typename O, typename K>
O &ObjectManager<O, K>::get(K id)
{
  auto o = fObjects.find(id);
  RE_MOCK_ASSERT(o != fObjects.end(), "Missing object for key [%d]", id);
  return o->second;
}

//------------------------------------------------------------------------
// ObjectManager::remove
//------------------------------------------------------------------------
template<typename O, typename K>
void ObjectManager<O, K>::remove(K id)
{
  fObjects.erase(id);
}

//------------------------------------------------------------------------
// ObjectManager::replace
//------------------------------------------------------------------------
template<typename O, typename K>
void ObjectManager<O, K>::replace(K id, O &&iObject)
{
  RE_MOCK_ASSERT(fObjects.find(id) != fObjects.end(), "Missing object for key [%d]", id);
  fObjects[id] = std::move(iObject);
}

//------------------------------------------------------------------------
// ObjectManager::clear
//------------------------------------------------------------------------
template<typename O, typename K>
void ObjectManager<O, K>::clear()
{
  fObjects.clear();
}

//------------------------------------------------------------------------
// ObjectManager::reset
//------------------------------------------------------------------------
template<typename O, typename K>
void ObjectManager<O, K>::reset()
{
  clear();
  fCounter = 1;
}

//------------------------------------------------------------------------
// ObjectManager::size
//------------------------------------------------------------------------
template<typename O, typename K>
size_t ObjectManager<O, K>::size() const
{
  return fObjects.size();
}

}

#endif