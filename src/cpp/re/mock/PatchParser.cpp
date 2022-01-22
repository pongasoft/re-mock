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

#include "PatchParser.h"
#include "stl.h"
#include "fmt.h"
#include <tinyxml2.h>

using namespace tinyxml2;

namespace re::mock {

using sample_reference_resolver = std::function<std::optional<std::string>(int)>;

namespace impl {

//------------------------------------------------------------------------
// findAttributeValue
//------------------------------------------------------------------------
std::optional<std::string> findAttributeValue(XMLElement const *iElement, char const *iAttributeName)
{
  if(iElement)
  {
    auto a = iElement->FindAttribute(iAttributeName);
    if(a && a->Value())
      return a->Value();
  }
  return std::nullopt;
}

//------------------------------------------------------------------------
// getAttributeValue
//------------------------------------------------------------------------
std::string getAttributeValue(XMLElement const *iElement, char const *iAttributeName)
{
  RE_MOCK_ASSERT(iElement != nullptr);
  auto v = findAttributeValue(iElement, iAttributeName);
  RE_MOCK_ASSERT(v != std::nullopt, "Missing required attribute %s at line [%d]", iAttributeName, iElement->GetLineNum());
  return *v;
}

//------------------------------------------------------------------------
// findElement
//------------------------------------------------------------------------
XMLElement const *findElement(XMLElement const *iElement, char const *iSubPath)
{
  RE_MOCK_ASSERT(iSubPath != nullptr);

  auto e = iElement;

  auto elts = fmt::split(iSubPath, '/');
  for(auto elt: elts)
  {
    if(e)
      e = e->FirstChildElement(elt.c_str());
  }

  return e;
}

//------------------------------------------------------------------------
// extractSampleReferenceResolver
//------------------------------------------------------------------------
sample_reference_resolver extractSampleReferenceResolver(XMLElement const *e)
{
  if(e == nullptr)
    return {};

  std::vector<std::string> paths{};

  auto sr = e->FirstChildElement("SampleReference");
  while(sr != nullptr)
  {
    std::optional<std::string> ref{};

    // Try AbsolutePath/NativeURL
    {
      auto pathElt = findElement(sr, "AbsolutePath/NativeURL");
      if(pathElt && pathElt->GetText())
      {
        std::string url{fmt::trim(pathElt->GetText())};
        RE_MOCK_ASSERT(stl::starts_with(url, "file://"), "Unsupported url format [%s]", url);
        ref = fmt::url_decode(url.substr(7));
      }
    }

    // Try DatabasePath/Path (jukebox)
    if(!ref)
    {
      auto pathElt = findElement(sr, "DatabasePath/Path");
      auto kind = findAttributeValue(pathElt, "pathKind");
      if(kind && *kind == "jukebox" && pathElt->GetText())
      {
        auto path = fmt::trim(pathElt->GetText());
        auto splittedPath = fmt::split(path, '/');
        if(splittedPath.size() < 2)
        {
          RE_MOCK_LOG_ERROR("Unknown syntax: expecting jukebox path, but got [%s]", path);
          return {};
        }
        std::vector<std::string> relativePath{};
        // remove /Rack extensions/<product_id>
        std::copy(splittedPath.begin() + 2, splittedPath.end(), std::back_inserter(relativePath));
        ref = fmt::printf("/%s", stl::join_to_string(relativePath, "/"));
      }
    }

    if(ref)
      paths.emplace_back(fmt::trim(*ref));
    else
    {
      RE_MOCK_LOG_ERROR("Unknown syntax. Could not extract path from patch file");
      return {};
    }

    sr = sr->NextSiblingElement("SampleReference");
  }

  return [p = std::move(paths)](int i) -> std::optional<std::string> {
    if(i < p.size())
      return p.at(i);
    else
      return std::nullopt;
  };
}

//------------------------------------------------------------------------
// createPatch
//------------------------------------------------------------------------
std::unique_ptr<resource::Patch> createPatch(XMLDocument const &iDoc)
{
  auto patch = std::make_unique<resource::Patch>();

  auto e = iDoc.FirstChildElement("JukeboxPatch");
  RE_MOCK_ASSERT(e != nullptr, "Missing JukeboxPatch root element");

  auto resolver = extractSampleReferenceResolver(e->FirstChildElement("Samples"));

  // <Properties>
  e = e->FirstChildElement("Properties");

  // no Properties (empty patch)
  if(!e)
    return patch;

  auto o = e->FirstChildElement("Object");

  while(o != nullptr)
  {
    auto objectName = fmt::trim(getAttributeValue(o, "name"));
    // <Value property="prop_number" type="number" >0.5</Value>
    auto v = o->FirstChildElement("Value");
    while(v)
    {
      if(v->GetText()) // ignore empty nodes
      {
        auto property = fmt::printf("/%s/%s", objectName, fmt::trim(getAttributeValue(v, "property")));
        auto type = fmt::trim(getAttributeValue(v, "type"));
        auto value = fmt::trim(v->GetText());

        if(type == "boolean")
        {
          RE_MOCK_ASSERT(value == "true" || value == "false", "Invalid boolean property [%s] line [%d]", value, v->GetLineNum());
          patch->boolean(property, value == "true");
        }
        else if(type == "number")
        {
          patch->number(property, std::stod(value));
        }
        else if(type == "sample")
        {
          auto sampleIndex = std::stoi(value);
          auto samplePath = resolver ? resolver(sampleIndex) : std::nullopt;
          RE_MOCK_ASSERT(static_cast<bool>(samplePath),
                         "Sample path [%d] could not be resolved in patch file <Samples> section.", sampleIndex);
          patch->sample(property, *samplePath);
        }
        else if(type == "string")
        {
          std::string s = "";
          while(!value.empty())
          {
            s += static_cast<char>(std::stoi(value.substr(0, 2), nullptr, 16));
            value = value.substr(2);
          }
          patch->string(property, s);
        }
      }

      v = v->NextSiblingElement("Value");
    }
    o = o->NextSiblingElement("Object");
  }

  return patch;
}

}

//------------------------------------------------------------------------
// Patch::from (file)
//------------------------------------------------------------------------
std::unique_ptr<resource::Patch> PatchParser::from(resource::File iPatchFile)
{
  XMLDocument doc;
  auto res = doc.LoadFile(iPatchFile.fFilePath.c_str());
  RE_MOCK_ASSERT(res == XMLError::XML_SUCCESS, "Error [%d], while parsing patch file %s", res, iPatchFile.fFilePath);
  auto patch = impl::createPatch(doc);
  return patch;
}

//------------------------------------------------------------------------
// Patch::from (String)
//------------------------------------------------------------------------
std::unique_ptr<resource::Patch> PatchParser::from(resource::String iPatchString)
{
  XMLDocument doc;
  auto res = doc.Parse(iPatchString.fString.c_str());
  RE_MOCK_ASSERT(res == XMLError::XML_SUCCESS, "Error [%d], while parsing patch string %s", res, iPatchString.fString);
  return impl::createPatch(doc);
}

}