/**
 * Copyright 2014 University of Bremen, Institute for Artificial Intelligence
 * Author(s): Ferenc Balint-Benczedi <balintbe@cs.uni-bremen.de>
 *         Thiemo Wiedemeyer <wiedemeyer@cs.uni-bremen.de>
 *         Jan-Hendrik Worch <jworch@cs.uni-bremen.de>
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

/*
 * scene_cas.cpp
 *
 *  Created on: Mar 29, 2012
 *      Author(s): christiankerl
 */

#include <robosherlock/scene_cas.h>
#include <robosherlock/utils/output.h>
#include "uima/api.hpp"
#include "uima/xmlwriter.hpp"

namespace rs
{

//std::vector<int> SceneCas::cam_ids_ = {};

std::map<std::string, std::vector<int>> SceneCas::camera_ids_in_aae_;

SceneCas::SceneCas(uima::CAS &cas) :
  cas(cas), active_cam_id_(0)
{
  mutex.reset(new std::mutex);
}

SceneCas::~SceneCas()
{
}

bool SceneCas::getView(const char *name, uima::CAS *&view)
{
  try
  {
    view = cas.getView(name);
    outDebug("Got view: " << name);
    return true;
  }
  catch(const uima::CASException &e)
  {
    outDebug("There is no view: " << name);
  }
  return false;
}

rs::Scene SceneCas::getScene(int cam_id)
{
  uima::FeatureStructure fs;
  std::string view_name = appendCamIdToViewName(VIEW_SCENE, cam_id);
  if(!getFS(view_name.c_str(), fs))
  {
    rs::Scene scene = rs::create<rs::Scene>(cas);
    setFS(view_name.c_str(), (uima::FeatureStructure)scene);
    return scene;
  }
  return rs::Scene(fs);
}

bool SceneCas::has(const char *name)
{
  uima::CAS *view;
  return getView(name, view);
}

bool SceneCas::hasObjets()
{
  uima::CAS *view;
  std::stringstream name;
  name<<"cam"<<active_cam_id_<<"."<<VIEW_OBJECTS;
  return getView(name.str().c_str(), view);
}


bool SceneCas::getFS(const char *name, uima::FeatureStructure &fs)
{
  uima::CAS *view;
  if(!getView(name, view))
  {
    outDebug("View '" << name << "' does not exist!");
    return false;
  }
  fs = view->getSofaDataArray();
  return true;
}

void SceneCas::setFS(const char *name, const uima::FeatureStructure &fs)
{
  std::lock_guard<std::mutex> lock(*mutex);
  uima::CAS *view;
  if(!getView(name, view))
  {
    view = cas.createView(name);
  }
  const std::string mime = std::string("application/x-") + name;

  view->setSofaDataArray(fs, UnicodeString::fromUTF8(mime));
}

std::string SceneCas::getCAStoString(uima::CAS &tcas, bool saveWithPointCloud)
{
  outInfo("saving cas to xml process starts");
  uima::XCASWriter writer(tcas, true);

  std::stringstream casAsStringStream;
  // writing whole cas to stringstream
  writer.write(casAsStringStream);

  // define parameter for point clouds stuff
  std::string byteArrayB = "<uima.cas.ByteArray";
  std::string byteArrayE = " </uima.cas.ByteArray>";

  std::string intArrayB = "<uima.cas.IntegerArray";
  std::string intArrayE = "</uima.cas.IntegerArray>";

  std::string removedPointsString = casAsStringStream.str();

  if (!saveWithPointCloud)
  {
    while (removedPointsString.find(byteArrayB) != std::string::npos)
    {
      // find the specific beginning and ending of uima.cas.ByteArray child
      std::string::size_type ByteABegin = removedPointsString.find(byteArrayB);
      std::string::size_type ByteAEnd = removedPointsString.find(byteArrayE);

      // remove ByteArray from CAS
      removedPointsString = removedPointsString.erase(ByteABegin, ((ByteAEnd - ByteABegin) + byteArrayE.length()));

      if (removedPointsString.find(intArrayB) != std::string::npos)
      {
        // find the specific beginning and ending of uima.cas.IntegerArray child
        std::string::size_type IntegerABegin = removedPointsString.find(intArrayB);
        std::string::size_type IntegerAEnd = removedPointsString.find(intArrayE);

        // remove IntegerArray from CAS
        removedPointsString =
            removedPointsString.erase(IntegerABegin, ((IntegerAEnd - IntegerABegin) + intArrayE.length()));
      }
    }
    outInfo("saving cas to xml process finished");
  }

  return removedPointsString;
}

void SceneCas::saveCASToXML(uima::CAS &tcas, std::string strOutDir, bool saveWithPointCloud, int docnum)
{
  std::string removedPointsString = getCAStoString(tcas, saveWithPointCloud);

  std::string createFileName = "" + strOutDir + "/doc" + std::to_string(docnum++) + ".xml";
  std::ofstream file(createFileName);
  file << removedPointsString;
}

}
