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

//------------------------------------------------------------------------
// createPatch
//------------------------------------------------------------------------
Resource::Patch createPatch(XMLDocument const &iDoc, PatchParser::sample_reference_resolver iSampleResolver)
{
  Resource::Patch patch;

  auto e = iDoc.FirstChildElement("JukeboxPatch");
  RE_MOCK_ASSERT(e != nullptr, "Missing JukeboxPatch root element");

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
      auto property = fmt::printf("/%s/%s", objectName, fmt::trim(getAttributeValue(v, "property")));
      auto type = fmt::trim(getAttributeValue(v, "type"));
      auto value = fmt::trim(v->GetText());

      if(type == "boolean")
      {
        RE_MOCK_ASSERT(value == "true" || value == "false", "Invalid boolean property [%s] line [%d]", value, v->GetLineNum());
        patch.boolean(property, value == "true");
      }
      else if(type == "number")
      {
        patch.number(property, std::stod(value));
      }
      else if(type == "sample")
      {
        RE_MOCK_ASSERT(static_cast<bool>(iSampleResolver),
                       "Patch contains sample properties so you must provide a resolver (parsing of <Samples> section not implemented)");
        patch.sample(property, iSampleResolver(std::stoi(value)));
      }
      else if(type == "string")
      {
        std::string s = "";
        while(!value.empty())
        {
          s += static_cast<char>(std::stoi(value.substr(0, 2), nullptr, 16));
          value = value.substr(2);
        }
        patch.string(property, s);
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
Resource::Patch PatchParser::from(ConfigFile iPatchFile, sample_reference_resolver iSampleResolver)
{
  XMLDocument doc;
  auto res = doc.LoadFile(iPatchFile.fFilename.c_str());
  RE_MOCK_ASSERT(res == XMLError::XML_SUCCESS, "Error [%d], while parsing patch file %s", res, iPatchFile.fFilename);
  auto patch = impl::createPatch(doc, iSampleResolver);
  return patch;
}

//------------------------------------------------------------------------
// Patch::from (String)
//------------------------------------------------------------------------
Resource::Patch PatchParser::from(ConfigString iPatchString, sample_reference_resolver iSampleResolver)
{
  XMLDocument doc;
  auto res = doc.Parse(iPatchString.fString.c_str());
  RE_MOCK_ASSERT(res == XMLError::XML_SUCCESS, "Error [%d], while parsing patch string %s", res, iPatchString.fString);
  return impl::createPatch(doc, iSampleResolver);
}

}