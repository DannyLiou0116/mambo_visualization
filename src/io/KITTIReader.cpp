#include "KITTIReader.h"

#include <rv/FileUtil.h>
#include <algorithm>
#include <cmath>
#include <fstream>
#include <thread>

#include <glow/glutil.h>
#include <rv/XmlDocument.h>
#include <rv/string_utils.h>
#include <boost/lexical_cast.hpp>

#include <rv/PrimitiveParameters.h>

namespace rv {

KITTIReader::KITTIReader(const std::string& scan_filename, ParameterList params, uint32_t buffer_size)
    : currentScan(0), bufferedScans(buffer_size), firstBufferedScan(0) {

  params_ = params;

  initScanFilenames(scan_filename);

  // initialize the rangenet
  net = std::shared_ptr<RangenetAPI> (new RangenetAPI(params_));
  label_map_ = net->getLabelMap();
  color_map_ = net->getColorMap();
}

void KITTIReader::initScanFilenames(const std::string& scan_filename) {
  scan_filenames.clear();

  std::vector<std::string> files = FileUtil::getDirectoryListing(FileUtil::dirName(scan_filename));
  /** filter irrelevant files. **/
  for (uint32_t i = 0; i < files.size(); ++i) {
    if (FileUtil::extension(files[i]) == ".bin") {
      scan_filenames.push_back(files[i]);
    }
  }

  std::sort(scan_filenames.begin(), scan_filenames.end());
}

void KITTIReader::reset() {
  currentScan = 0;
  bufferedScans.clear();
  firstBufferedScan = 0;
}

bool KITTIReader::read(Laserscan& scan) {
  bool result = false;
  scan.clear();

  if (currentScan >= (int32_t)scan_filenames.size()) {
    return false;
  }

  if (currentScan - firstBufferedScan < bufferedScans.size()) /** scan already in buffer, no need to read scan. **/
  {
    scan = bufferedScans[currentScan - firstBufferedScan];

    result = true;
  } else {
    result = read(currentScan, scan);
    if (result) {
      if (bufferedScans.capacity() == bufferedScans.size()) ++firstBufferedScan;
      bufferedScans.push_back(scan);
    }
  }

  ++currentScan;

  return result;
}

bool KITTIReader::isSeekable() const {
  return true;
}

void KITTIReader::seek(uint32_t scannr) {
  assert(scannr < scan_filenames.size());

  /** scan already in buffer, nothing to read just set current scan **/
  if (scannr - firstBufferedScan < bufferedScans.size()) {
    currentScan = scannr;
  } else if (currentScan < (int32_t)scannr) {
    /** if we don't have to read everything again than read missing scans. **/
    if ((scannr - 1) - currentScan < bufferedScans.capacity()) {
      currentScan =
          firstBufferedScan + bufferedScans.size(); /** advance to last scan in buffer to read no scans twice. **/
      while (currentScan < (int32_t)scannr) {
        Laserscan scan;

        if (bufferedScans.capacity() == bufferedScans.size()) ++firstBufferedScan;
        read(currentScan, scan);
        bufferedScans.push_back(scan);

        ++currentScan;
      }
    } else /** otherwise we just reset the buffer and start buffering again. **/
    {
      currentScan = scannr;
      firstBufferedScan = scannr;
      bufferedScans.clear();
    }
  } else if (currentScan > (int32_t)scannr) /** we have to add scans at the beginning **/
  {
    /** if we don't have to read every thing new, than read missing scans. **/
    if (currentScan - scannr < bufferedScans.capacity()) {
      currentScan = firstBufferedScan;

      while (currentScan > (int32_t)scannr) {
        --currentScan;

        Laserscan scan;

        read(currentScan, scan);
        bufferedScans.push_front(scan);
      }

      firstBufferedScan = currentScan;
    } else /** otherwise we just reset the buffer and start buffering again. **/
    {
      currentScan = scannr;
      firstBufferedScan = scannr;
      bufferedScans.clear();
    }
  }
}

uint32_t KITTIReader::count() const {
  return scan_filenames.size();
}

bool KITTIReader::read(uint32_t scan_idx, Laserscan& scan) {

  if (scan_idx > scan_filenames.size()) return false;

  std::ifstream in(scan_filenames[scan_idx].c_str(), std::ios::binary);
  if (!in.is_open()) return false;

  scan.clear();

  in.seekg(0, std::ios::end);
  uint32_t num_points = in.tellg() / (5 * sizeof(float));
  in.seekg(0, std::ios::beg);

  std::vector<float> values(5 * num_points);
  in.read((char*)&values[0], 5 * num_points * sizeof(float));

  in.close();
  std::vector<Point3f>& points = scan.points_;
  std::vector<float>& remissions = scan.remissions_;

  points.resize(num_points);
  remissions.resize(num_points);

  float max_remission = 0;

  for (uint32_t i = 0; i < num_points; ++i) {
    points[i].x() = values[5 * i];
    points[i].y() = values[5 * i + 1];
    points[i].z() = values[5 * i + 2];
    remissions[i] = values[5 * i + 3];
    max_remission = std::max(remissions[i], max_remission);
  }

  for (uint32_t i = 0; i < num_points; ++i) {
    remissions[i] /= max_remission;
  }
  // for semantic map
  std::vector<float> color_mask;

  std::vector<std::vector<float>> semantic_points = net->infer(values, num_points);

  for (auto point : semantic_points) {
    for (float ele : point){
      color_mask.push_back(ele);
    }
  }

// JLLiU
// 只是先試試，values[5 * i + 4]的資訊是learning map的資訊
  std::vector<float>& labels = scan.labels_float;
  std::vector<float>& labels_prob = scan.labels_prob;

  labels.resize(num_points);
  labels_prob.resize(num_points);

  for (uint32_t i = 0; i < num_points; ++i) {
    labels[i] = values[5 * i + 4];
    labels_prob[i] = 1;
	if (labels[i] == 1){
	labels[i] = 10;
	}
	else if(labels[i] == 2){
	labels[i] = 11;
	}
	else if(labels[i] == 3){
	labels[i] = 15;
	}
	else if(labels[i] == 4){
	labels[i] = 18;
	}
	else if(labels[i] == 5){
	labels[i] = 20;
	}
	else if(labels[i] == 6){
	labels[i] = 30;
	}
	else if(labels[i] == 7){
	labels[i] = 31;
	}
	else if(labels[i] == 8){
	labels[i] = 32;
	}
	else if(labels[i] == 9){
	labels[i] = 40;
	}
	else if(labels[i] == 10){
	labels[i] = 44;
	}
	else if(labels[i] == 11){
	labels[i] = 48;
	}
	else if(labels[i] == 12){
	labels[i] = 49;
	}
	else if(labels[i] == 13){
	labels[i] = 50;
	}
	else if(labels[i] == 14){
	labels[i] = 51;
	}
	else if(labels[i] == 15){
	labels[i] = 70;
	}
	else if(labels[i] == 16){
	labels[i] = 71;
	}
	else if(labels[i] == 17){
	labels[i] = 72;
	}
	else if(labels[i] == 18){
	labels[i] = 80;
	}
	else if(labels[i] == 19){
	labels[i] = 81;
	}
	else{
	labels[i] = 0;
        }
  }

/** By JLLiu
1. num_points 代表這個frame有幾個點
2. labels 代表semantic出來的class
**/

//int doggy;
//doggy = 0;


        /* 
	  rangenet++ 是 labels[i] = label_map_[j]
	  labels[i] = 0 是紅色
	  labels[i] = 20 是藍色 
	  labels[i] = 40 是粉色
	  由上述可知 labels[i] 代表各個點的顏色，前面的 i < num_points; 可知 i 就是第幾個點
	  顏色資訊可看 https://github.com/PRBonn/semantic-kitti-api/blob/master/config/semantic-kitti-all.yaml ，裡面的 learning map 其實就是 label_map_[j] ，且sizeof(label_map_)=24 
	*/
	/*若label[i] = doggy
	if (i<10000){
	doggy = 0;
	}
	else if(i<20000){
	doggy = 20;
	}
	else if(i<30000){
	doggy = 11;
	}
	else if(i<40000){
	doggy = 15;
	}
	else if(i<50000){
	doggy = 18;
	}
	else if(i<60000){
	doggy = 20;
	}
	else if(i<70000){
	doggy = 30;
	}
	else if(i<80000){
	doggy = 31;
	}
	else if(i<90000){
	doggy = 32;
	}
	else{
	doggy = 52;
	}*/



	/* 
	  rangenet++ 是 labels_prob[i] = color_mask[i*20+j];
	  labels_prob[i]代表semantic map上的點(並非顏色)，由於還沒釐清是什麼，就先都設定為可看到的1
	*/

        
        
        //std::cerr << "labels["<< i <<"]: " << labels[i] << std::endl;
        //std::cerr << "labels_prob["<< i <<"]: " << labels_prob[i] << std::endl;

  
  //std::cerr << "\n---------- ./src/io/KITTIReader.cpp semantic map ----------\n" << std::endl;

  return true;
}
}
