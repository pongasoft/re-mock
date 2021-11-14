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

#include "Patch.h"
#include "stl.h"
#include <tinyxml2.h>

using namespace tinyxml2;

/*
<?xml version="1.0"?>
<JukeboxPatch version="2.0"  deviceProductID="com.acme.Kooza"  deviceVersion="1.0.0d1" >
    <DeviceNameInEnglish>
        Kooza
    </DeviceNameInEnglish>
    <Properties>
        <Object name="custom_properties" >
            <Value property="prop_number"  type="number" >
                0.5
            </Value>
            <Value property="prop_boolean"  type="boolean" >
                true
            </Value>
            <Value property="prop_string"  type="string" >
                414243
            </Value>
        </Object>
        <Object name="transport" />
    </Properties>
</JukeboxPatch>
 */

namespace re::mock {

namespace impl {

std::optional<std::string> findAttributeValue(XMLElement const *iElement, char const *iAttributeName)
{
  if(iElement)
  {
    auto a = iElement->FindAttribute(iAttributeName);
    if(a)
      return a->Value();
  }
  return std::nullopt;
}

std::string getAttributeValue(XMLElement const *iElement, char const *iAttributeName)
{
  RE_MOCK_ASSERT(iElement != nullptr);
  auto v = findAttributeValue(iElement, iAttributeName);
  RE_MOCK_ASSERT(v != std::nullopt, "Missing required attribute %s at line [%d]", iAttributeName, iElement->GetLineNum());
  return *v;
}


XMLElement const *findChildElementWithAttribute(XMLElement const *iParent,
                                                char const *iElementName,
                                                char const* iAttributeName,
                                                char const *iAttributeValue)
{
  if(!iParent)
    return nullptr;

  auto e = iParent->FirstChildElement(iElementName);

  while(e)
  {
    auto v = findAttributeValue(e, iAttributeName);

    if(v && v == std::string(iAttributeValue))
      return e;

    e = e->NextSiblingElement(iElementName);
  }

  return nullptr;
}

//------------------------------------------------------------------------
// createPatch
//------------------------------------------------------------------------
Patch createPatch(XMLDocument const &iDoc)
{
  Patch patch;

  auto e = iDoc.FirstChildElement("JukeboxPatch");
  RE_MOCK_ASSERT(e != nullptr, "Missing JukeboxPatch root element");

  // <Properties>
  e = e->FirstChildElement("Properties");

  // no Properties (empty patch)
  if(!e)
    return patch;

  // <Object name="custom_properties" >
  e = findChildElementWithAttribute(e, "Object", "name", "custom_properties");

  // no custom_properties
  if(!e)
    return patch;

  // <Value property="prop_number" type="number" >0.5</Value>
  auto v = e->FirstChildElement("Value");

  while(v)
  {
    auto property = stl::trim(getAttributeValue(v, "property"));
    auto type = stl::trim(getAttributeValue(v, "type"));
    auto value = stl::trim(v->GetText());

    if(type == "boolean")
    {
      RE_MOCK_ASSERT(value == "true" || value == "false", "Invalid boolean property [%s] line [%d]", value.c_str(), v->GetLineNum());
      patch.value(property, value == "true");
    }
    else if(type == "number")
    {
      patch.value(property, std::stod(value));
    }
    else if(type == "string")
    {
      std::string s = "";
      while(!value.empty())
      {
        s += static_cast<char>(std::stoi(value.substr(0, 2), nullptr, 16));
        value = value.substr(2);
      }
      patch.value(property, s);
    }

    v = v->NextSiblingElement("Value");
  }

  return patch;

}

}

//------------------------------------------------------------------------
// Patch::from (file)
//------------------------------------------------------------------------
Patch Patch::from(ConfigFile iPatchFile)
{
  XMLDocument doc;
  auto res = doc.LoadFile(iPatchFile.fFilename.c_str());
  RE_MOCK_ASSERT(res == XMLError::XML_SUCCESS, "Error [%d], while parsing patch file %s", res, iPatchFile.fFilename.c_str());
  return impl::createPatch(doc);
}

//------------------------------------------------------------------------
// Patch::from (String)
//------------------------------------------------------------------------
Patch Patch::from(ConfigString iPatchString)
{
  XMLDocument doc;
  auto res = doc.Parse(iPatchString.fString.c_str());
  RE_MOCK_ASSERT(res == XMLError::XML_SUCCESS, "Error [%d], while parsing patch string %s", res, iPatchString.fString.c_str());
  return impl::createPatch(doc);
}

}