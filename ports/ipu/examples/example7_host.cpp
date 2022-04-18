#include <algorithm>
#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <iostream>

#include <poplar/DeviceManager.hpp>
#include <poplar/Engine.hpp>
#include <poplar/Graph.hpp>
#include <poplar/IPUModel.hpp>
#include <poplar/Program.hpp>

#include "example7.hpp"

using namespace poplar;

Device getIPU(bool use_hardware = true, int num_ipus = 1);

// ----------------------- Globals ----------------------- //
const unsigned printBufSize = 2048;
const int VfsBlockSize = 512;
const int VfsNumBlocks = 40;
Tensor printBuf;
Tensor changeSubprocess;
Tensor actionFlag;
Tensor vfsPos;
Tensor vfsDataBlock;
DataStream printBufStream;
char printBuf_h[printBufSize];

void create_globals(Graph& graph) {
  const int tile = 0;

  printBuf = graph.addVariable(CHAR, {printBufSize}, "printBuf");
  actionFlag = graph.addVariable(INT, {}, "actionFlag");
  changeSubprocess = graph.addVariable(BOOL, {}, "changeSubprocess");
  vfsPos = graph.addVariable(INT, {}, "vfsPos");
  vfsDataBlock = graph.addVariable(CHAR, {VfsBlockSize}, "vfsDataBlock");

  graph.setTileMapping(printBuf, tile);
  graph.setTileMapping(changeSubprocess, tile);
  graph.setTileMapping(actionFlag, tile);
  graph.setTileMapping(vfsPos, tile);
  graph.setTileMapping(vfsDataBlock, tile);

  printBufStream = graph.addDeviceToHostFIFO("printBuf-stream", poplar::CHAR, printBufSize);
}


// ----------------------- Main terminal ----------------------- //
const unsigned terminalInBufSize = 256;
char terminalInBuf_h[terminalInBufSize];
Tensor terminalInBuf;
DataStream terminalInBufStream;

program::Sequence create_terminal(Graph& graph) {
  int tile = 0;
  
  terminalInBuf = graph.addVariable(CHAR, {terminalInBufSize}, "terminalInBuf");
  graph.setTileMapping(terminalInBuf, tile);
  terminalInBufStream = graph.addHostToDeviceFIFO("terminalInBuf-stream", poplar::CHAR, terminalInBufSize);
  
  ComputeSet initCS = graph.addComputeSet("TerminalInitCS");
  VertexRef initVtx = graph.addVertex(initCS, "TerminalInit", {
    {"printBuf", printBuf}, {"changeSubprocess", changeSubprocess}, {"actionFlag", actionFlag}, {"vfsPos", vfsPos}, 
    {"vfsDataBlock", vfsDataBlock}, {"terminalInBuf", terminalInBuf}
  });
  graph.setTileMapping(initVtx, tile);

  ComputeSet bodyCS = graph.addComputeSet("TerminalBodyCS");
  VertexRef bodyVtx = graph.addVertex(initCS, "TerminalBody", {
    {"printBuf", printBuf}, {"changeSubprocess", changeSubprocess}, {"actionFlag", actionFlag}, {"vfsPos", vfsPos}, 
    {"vfsDataBlock", vfsDataBlock}, {"terminalInBuf", terminalInBuf}
  });
  graph.setTileMapping(bodyVtx, tile);

  
  program::Sequence program({
    program::Execute(initCS),
    program::Copy(printBuf, printBufStream),
    program::RepeatWhileTrue(program::Sequence(), terminalIsLive, program::Sequence({
      program::Execute(bodyCS)
  });
}


int main() {

  Device device = getIPU(true);
  Graph graph(device.getTarget());

  // Create variable in IPU memory to store program outputs
  unsigned diskSize = 512 * 32;
  char diskImg_h[diskSize];
  
  std::ifstream infile("disk.img", std::ios::in);
  if (!infile.read(diskImg_h, diskSize)) {
    std::cout << "Failed to read file disk.img\n";
    return EXIT_FAILURE;
  }
  infile.close();


  Tensor diskImg = graph.addVariable(CHAR, {diskSize}, "diskImg");
  Tensor inBuf = graph.addVariable(CHAR, {inBufSize}, "inBuf");
  Tensor doneFlag = graph.addVariable(BOOL, {}, "doneFlag");
  graph.setTileMapping(printBuf, 0);
  graph.setTileMapping(diskImg, 0);
  graph.setTileMapping(inBuf, 0);
  graph.setTileMapping(doneFlag, 0);
  DataStream printBufStream = graph.addDeviceToHostFIFO("printBuf-stream", poplar::CHAR, printBufSize);
  DataStream inBufStream = graph.addHostToDeviceFIFO("inBuf-stream", poplar::CHAR, inBufSize);
  DataStream diskInStream = graph.addHostToDeviceFIFO("diskImg-in-stream", poplar::CHAR, diskSize);
  DataStream diskOutStream = graph.addDeviceToHostFIFO("diskImg-out-stream", poplar::CHAR, diskSize);

  // Add computation vertext to IPU
  graph.addCodelets("example4_codelets.gp");
  ComputeSet init_computeset = graph.addComputeSet("InitCS");
  ComputeSet body_computeset = graph.addComputeSet("BodyCS");
  VertexRef int_vtx = graph.addVertex(init_computeset, "InitVertex", {
    {"printBuf", printBuf}, {"inBuf", inBuf}, {"doneFlag", doneFlag}, {"diskImg", diskImg}});
  VertexRef body_vtx = graph.addVertex(body_computeset, "RTVertex", {
    {"printBuf", printBuf}, {"inBuf", inBuf}, {"doneFlag", doneFlag}, {"diskImg", diskImg}});
  graph.setTileMapping(int_vtx, 0);
  graph.setTileMapping(body_vtx, 0);
  
  // Create and run program 
  program::Sequence program({
    program::Copy(diskInStream, diskImg),
    program::Execute(init_computeset),
    program::Copy(printBuf, printBufStream),
    program::RepeatWhileFalse(program::Sequence(), doneFlag, program::Sequence({
      program::Copy(inBufStream, inBuf),
      program::Execute(body_computeset),
      program::Copy(printBuf, printBufStream)
    })),
    program::Copy(diskImg, diskOutStream),
  });
  Engine engine(graph, program);
  engine.connectStream("printBuf-stream", printBuf_h);
  engine.connectStream("inBuf-stream", inBuf_h);
  engine.connectStream("diskImg-in-stream", diskImg_h);
  engine.connectStream("diskImg-out-stream", diskImg_h);
  engine.connectStreamToCallback("printBuf-stream", [printBufSize](void* p){printf("%.*s", printBufSize, (char*)p);});
  engine.connectStreamToCallback("inBuf-stream", [](void* p){*((char*)p) = getchar();});
  engine.load(device);

  // Put terminal in raw mode so we can send singel key pressed
  system("stty raw opost -echo");
  engine.run(0);
  system("stty cooked opost echo");

  printf("Shutting Down - Goodbye \e[01;31m✿\e[0m\n\n");

  // Write filesystem back to *real* disk
  std::ofstream outfile("disk.img", std::ios::out);
  if (!outfile.write(diskImg_h, diskSize)) {
    std::cout << "Failed to write file disk.img\n";
    return EXIT_FAILURE;
  }
  outfile.close();

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
