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
  char stack_h[N] = {1, 0, 0, 0, 0};
  Tensor stack = graph.addVariable(CHAR, {N}, "stack");
  graph.setTileMapping(stack, 0);
  graph.createHostWrite("stack-write", stack);
  graph.createHostRead("stack-read", stack);

  // Add computation vertext to IPU
  graph.addCodelets("ipubuild/main.gp");
  ComputeSet computeset = graph.addComputeSet("cs");
  VertexRef vtx = graph.addVertex(computeset, "pyvertex", {{"stack", stack}});
  graph.setTileMapping(vtx, 0);
  
  // Create and run program 
  poplar::program::Execute program(computeset);
  Engine engine(graph, program);
  engine.load(device);
  engine.writeTensor("stack-write", stack_h, &stack_h[N]);
  engine.run(0);
  engine.readTensor("stack-read", stack_h, &stack_h[N]);

  // Print and check for expected output
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
