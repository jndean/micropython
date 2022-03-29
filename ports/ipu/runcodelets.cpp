#include <algorithm>
#include <chrono>
#include <cstdlib>
#include <iostream>

#include <poplar/DeviceManager.hpp>
#include <poplar/Engine.hpp>
#include <poplar/Graph.hpp>
#include <poplar/IPUModel.hpp>
#include <poplar/Program.hpp>

using namespace poplar;


Device getIPU(bool use_hardware = true, int num_ipus = 1);


int main() {

  Device device = getIPU(true);
  Graph graph(device.getTarget());

  // Create variable in IPU memory to store program outputs
  unsigned N = 4096;  
  char stdout_tensor_h[N] = {0};
  unsigned v1size = 10;
  int variable1_h[v1size] = {1, 2, 3};
  Tensor stdout_tensor = graph.addVariable(CHAR, {N}, "stdout_tensor");
  Tensor variable1 = graph.addVariable(INT, {v1size}, "stdout_tensor");
  graph.setTileMapping(stdout_tensor, 0);
  graph.setTileMapping(variable1, 0);
  graph.createHostRead("stdout_tensor-read", stdout_tensor);
  graph.createHostWrite("variable1-write", variable1);
  graph.createHostRead("variable1-read", variable1);

  // Add computation vertext to IPU
  graph.addCodelets("ipubuild/main.gp");
  ComputeSet computeset = graph.addComputeSet("cs");
  VertexRef vtx = graph.addVertex(computeset, "pyvertex", {
    {"stdout_tensor", stdout_tensor},
    {"variable1", variable1}
    });
  graph.setTileMapping(vtx, 0);
  
  // Create and run program 
  poplar::program::Execute program(computeset);
  Engine engine(graph, program);
  engine.load(device);
  engine.writeTensor("variable1-write", variable1_h, &variable1_h[v1size]);
  engine.run(0);
  engine.readTensor("stdout_tensor-read", stdout_tensor_h, &stdout_tensor_h[N]);
  engine.readTensor("variable1-read", variable1_h, &variable1_h[v1size]);

  // Print output
  for (int i = 0; i < N; ++i) {
    if (stdout_tensor_h[i] == '\0') break;
    printf("%c", stdout_tensor_h[i]);
  }
  printf("\n");

  for (int i = 0; i < v1size; ++i) {
    printf("%d ", variable1_h[i]);
  }
  printf("\n");

  return EXIT_SUCCESS;
}


// Helper utility //
Device getIPU(bool use_hardware, int num_ipus) {

  if (use_hardware) {
auto manager = DeviceManager::createDeviceManager();
    auto devices = manager.getDevices(TargetType::IPU, num_ipus);
    auto it = std::find_if(devices.begin(), devices.end(), [](Device &device) {
	return device.attach();
      });
    if (it == devices.end()) {
      std::cerr << "Error attaching to device\n";
      exit(EXIT_FAILURE);
    }
    std::cout << "Attached to IPU " << it->getId() << std::endl;
    return std::move(*it);
    
  } else {
    IPUModel ipuModel;
    return ipuModel.createDevice(); 
  }
}
