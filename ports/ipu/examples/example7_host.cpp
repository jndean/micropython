#include <algorithm>
#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <string>

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
const unsigned inBufSize = 1;
const unsigned hostCallBufSize = 256;
const int vfsBlockSize = 512;
const int vfsNumBlocks = 40;
Tensor printBuf;
Tensor inBuf;
Tensor constFalse;
Tensor constTrue;
Tensor shutdownFlag;
Tensor syscallArg;
Tensor programSelector;
Tensor hostCallBuf;
Tensor vfsPos;
Tensor vfsDataBlock;
Tensor vfsDisk;
DataStream printBufStream;
DataStream inBufStream;
DataStream hostCallBufStream;
char printBuf_h[printBufSize];
char inBuf_h[inBufSize];
char hostCallBuf_h[hostCallBufSize];

void createGlobalVars(Graph& graph) {
  const int tile = 0;

  bool falseValue = false;
  bool trueValue = true;
  constFalse = graph.addConstant(BOOL, {}, &falseValue, "constFalse");
  constTrue = graph.addConstant(BOOL, {}, &trueValue, "constTrue");
  printBuf = graph.addVariable(CHAR, {printBufSize}, "printBuf");
  inBuf = graph.addVariable(CHAR, {inBufSize}, "inBuf");
  hostCallBuf = graph.addVariable(CHAR, {hostCallBufSize}, "hostCallBuf");
  shutdownFlag = graph.addVariable(BOOL, {}, "shutdownFlag");
  syscallArg = graph.addVariable(INT, {}, "syscallArg");
  programSelector = graph.addVariable(INT, {}, "programSelector");
  vfsPos = graph.addVariable(INT, {}, "vfsPos");
  vfsDataBlock = graph.addVariable(CHAR, {vfsBlockSize}, "vfsDataBlock");
  vfsDisk = graph.addVariable(CHAR, {vfsBlockSize * vfsNumBlocks}, "vfsDisk");

  graph.setTileMapping(printBuf, tile);
  graph.setTileMapping(inBuf, tile);
  graph.setTileMapping(hostCallBuf, tile);
  graph.setTileMapping(constFalse, tile);
  graph.setTileMapping(constTrue, tile);
  graph.setTileMapping(shutdownFlag, tile);
  graph.setTileMapping(syscallArg, tile);
  graph.setTileMapping(programSelector, tile);
  graph.setTileMapping(vfsPos, tile);
  graph.setTileMapping(vfsDataBlock, tile);

  printBufStream = graph.addDeviceToHostFIFO("printBuf-stream", poplar::CHAR, printBufSize);
  inBufStream = graph.addHostToDeviceFIFO("inBuf-stream", poplar::CHAR, inBufSize);
  hostCallBufStream = graph.addDeviceToHostFIFO("hostCallBuf-stream", poplar::CHAR, hostCallBufSize);
}

void initGlobalVars(Engine& engine) {
  engine.connectStream("printBuf-stream", printBuf_h);
  engine.connectStream("inBuf-stream", inBuf_h);
  engine.connectStream("hostCallBuf-stream", hostCallBuf_h);
  engine.connectStreamToCallback("printBuf-stream", [](void* p){
    printf("%.*s", printBufSize, (char*)p);
  });
  engine.connectStreamToCallback("inBuf-stream", [](void* p){
    *((char*)p) = getchar();
  });
  engine.connectStreamToCallback("hostCallBuf-stream", [](void* p) {
    char tmp[hostCallBufSize];
    strncpy(tmp, (char*)p, hostCallBufSize);
    system(tmp);
  });
}


// ----------------------- Example1 ----------------------- //

const unsigned ex1_N = 15;
int ex1Input_h[ex1_N] = {1, 0, 0, 1, 1, 0, 0, 0, 0, 1, 0, 1, 0, 0, 0};
program::Sequence createExample1Prog(Graph& graph) {

  int tile = 1;
  
  Tensor input = graph.addVariable(INT, {ex1_N}, "ex1Input");
  DataStream inputStream = graph.addHostToDeviceFIFO("ex1Input-stream", poplar::INT, ex1_N);
  graph.setTileMapping(input, tile);
  
  ComputeSet computeset = graph.addComputeSet("cs");
  VertexRef vtx = graph.addVertex(computeset, "Example1Body", {
    {"input", input}, {"printBuf", printBuf},
  });
  graph.setTileMapping(vtx, tile);

  program::Sequence program({
    program::Copy(inputStream, input),
    program::Execute(computeset),
    program::Copy(printBuf, printBufStream),
  });
  return program;
}

void initExample1Vars(Engine& engine) {
  engine.connectStream("ex1Input-stream", ex1Input_h);
}


// ----------------------- Example2 ----------------------- //

const unsigned ex2_N = 10;
float ex2Input_h[ex2_N] = {-4., 0., 0.1, 9., 10., -2., -7., 1.5, -1., 0.};
program::Sequence createExample2Prog(Graph& graph) {

  int tile = 2;
  
  Tensor input = graph.addVariable(FLOAT, {ex2_N}, "ex2Input");
  DataStream inputStream = graph.addHostToDeviceFIFO("ex2Input-stream", poplar::FLOAT, ex2_N);
  graph.setTileMapping(input, tile);
  
  ComputeSet computeset = graph.addComputeSet("cs");
  VertexRef vtx = graph.addVertex(computeset, "Example2Body", {
    {"X", input}, {"printBuf", printBuf},
  });
  graph.setTileMapping(vtx, tile);

  program::Sequence program({
    program::Copy(inputStream, input),
    program::Execute(computeset),
    program::Copy(printBuf, printBufStream),
  });
  return program;
}

void initExample2Vars(Engine& engine) {
  engine.connectStream("ex2Input-stream", ex2Input_h);
}

// ----------------------- Example3 ----------------------- //


int main() {

  Device device = getIPU(true);
  Graph graph(device.getTarget());


  // Add computation vertext to IPU
  graph.addCodelets("example7_codelets.gp");
  createGlobalVars(graph);
  
  // Create compute sets for terminal
  int tile = 0;
  ComputeSet termInitCS = graph.addComputeSet("TerminalInitCS");
  ComputeSet termBodyCS = graph.addComputeSet("TerminalBodyCS");
  VertexRef initVtx = graph.addVertex(termInitCS, "TerminalInit", {
    {"printBuf", printBuf}, {"inBuf", inBuf}, {"syscallArg", syscallArg}, {"hostCallBuf", hostCallBuf},
    {"programSelector", programSelector}, {"vfsPos", vfsPos}, {"vfsDataBlock", vfsDataBlock}, 
  });
  VertexRef bodyVtx = graph.addVertex(termBodyCS, "TerminalBody", {
    {"printBuf", printBuf}, {"inBuf", inBuf}, {"syscallArg", syscallArg}, {"hostCallBuf", hostCallBuf},
    {"programSelector", programSelector}, {"vfsPos", vfsPos}, {"vfsDataBlock", vfsDataBlock}, 
  });
  graph.setTileMapping(initVtx, tile);
  graph.setTileMapping(bodyVtx, tile);

  // Compute sets for VFS
  int diskTileStart = 10;
  ComputeSet vfsCS = graph.addComputeSet("VfsCS");
  for (int i = 0; i < vfsBlockSize; ++i) {
    auto diskSlice = vfsDisk.slice(i * vfsNumBlocks, (i + 1) * vfsNumBlocks);
    VertexRef readwriteVtx = graph.addVertex(vfsCS, "VfsReadWrite", {
      {"vfsDisk", diskSlice}, {"vfsDataBlock", vfsDataBlock[i]}, {"vfsPos", vfsPos}, {"syscallArg", syscallArg}
    });
    graph.setTileMapping(diskSlice, diskTileStart + i);
    graph.setTileMapping(readwriteVtx, diskTileStart + i);
  }


  
  // Create main program loop
  program::Sequence mainProgram;
  {
    using namespace poplar::program;

    mainProgram = Sequence({
      Execute(termInitCS),
      Copy(printBuf, printBufStream),
      Copy(constFalse, shutdownFlag),
      RepeatWhileFalse(Sequence(), shutdownFlag, Sequence({
        Execute(termBodyCS),
        Copy(printBuf, printBufStream),
        Switch(syscallArg, {
          {syscall_none, Sequence()},
          {syscall_get_char, Copy(inBufStream, inBuf)},
          {syscall_flush_stdout, ErrorProgram("syscall_flush_stdout not implemented", constFalse)},
          {syscall_vfs_read, Execute(vfsCS)},
          {syscall_vfs_write, Execute(vfsCS)},
          {syscall_shutdown, Copy(constTrue, shutdownFlag)},
          {syscall_host_run, Copy(hostCallBuf, hostCallBufStream)},
          {syscall_run, Switch(programSelector, {
                    {0, Sequence()},
                    {1, createExample1Prog(graph)},
                    {2, createExample2Prog(graph)},
              })},
        }),
      })),
    });
  }
  
  // Create session
  Engine engine(graph, mainProgram);
  initGlobalVars(engine);
  initExample1Vars(engine);
  initExample2Vars(engine);

  engine.load(device);

  // // Put terminal in raw mode so we can send single key presses
  
  printf("\e[01;31m✿\e[0m  Entering poppysh\e[01;31m ✿\e[0m\nPress ^D^C to escape...\n");
  system("stty raw opost -echo");
  engine.run(0);
  system("stty cooked opost echo");
  printf("^D^C\n\e[01;31m✿\e[0m  Leaving poppysh\e[01;31m ✿\e[0m\n");

  

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
