#include "RangenetAPI.hpp"

RangenetAPI::RangenetAPI(const rv::ParameterList& params){

  std::cerr << "\n\n==============================\nStart from RangenetAPI.cpp" << std::endl;
  std::string model_path;
  if (params.hasParam("model_path")) {
    std::cerr << "\Find model!" << std::endl;
    model_path = std::string(params["model_path"]);
  }
  else{
    std::cerr << "No model could be found!" << std::endl;
  }

  std::string backend = "tensorrt";

  // initialize a network
  net = cl::make_net(model_path, backend);
}

std::vector<std::vector<float>> RangenetAPI::infer(const std::vector<float>& scan,
                                                   const uint32_t num_points){
  //std::cerr << "End of RangenetAPI.cpp\n==============================\n\n" << std::endl;
  return net->infer(scan, num_points);
}
