#include <dephier/dephier.hpp>
#include "netcdf.hpp"
#include <iostream>
#include <richdem/common/Array2D.hpp>
#include <string>
#include <stdexcept>

namespace rd = richdem;
namespace dh = richdem::dephier;

int main(int argc, char **argv){
  if(argc!=4){
    std::cout<<"Syntax: "<<argv[0]<<" <Input> <Output> <Ocean Level>"<<std::endl;
    return -1;
  }

  const std::string in_name     = argv[1];
  const std::string out_name    = argv[2];
  const float       ocean_level = std::stod(argv[3]);

  std::cout<<"m Processing    = "<<argv[1]<<std::endl;
  std::cout<<"m Output prefix = "<<argv[2]<<std::endl;
  std::cout<<"m Ocean level   = "<<argv[3]<<std::endl;

  rd::Timer timer_io;
  timer_io.start();
  rd::Array2D<float> topo(in_name);   //Recharge (Percipitation minus Evapotranspiration)
  timer_io.stop();

  std::cout<<"m Data width  = "<<topo.width ()<<std::endl;
  std::cout<<"m Data height = "<<topo.height()<<std::endl;
  std::cout<<"m Data cells  = "<<topo.numDataCells()<<std::endl;

  rd::Array2D<float>          wtd     (topo.width(), topo.height(), 1000       ); //All cells have some water
  rd::Array2D<dh::dh_label_t> label   (topo.width(), topo.height(), dh::NO_DEP ); //No cells are part of a depression
  rd::Array2D<rd::flowdir_t>  flowdirs(topo.width(), topo.height(), rd::NO_FLOW); //No cells flow anywhere

  wtd.setNoData(topo.noData());

  //Label the ocean cells. This is a precondition for using
  //`GetDepressionHierarchy()`.
  #pragma omp parallel for
  for(unsigned int i=0;i<label.size();i++){
    if(topo.isNoData(i) || topo(i)==ocean_level){ //Ocean Level is assumed to be lower than any other cells (even Death Valley)
      label(i) = dh::OCEAN;
      wtd  (i) = 0;
    }
  }

  //Generate flow directions, label all the depressions, and get the hierarchy
  //connecting them
  auto deps = dh::GetDepressionHierarchy<float,rd::Topology::D8>(topo, label, flowdirs);

  timer_io.start();

  //GraphViz dot-style output for drawing depression hierarchy graphs.
  {
    std::ofstream fgraph(out_name+"-graph.csv");
    fgraph<<"parent,child,oceanlink\n";
    for(unsigned int i=1;i<deps.size();i++){
      fgraph<<deps[i].parent<<","<<i<<",";
      //Is it an ocean link?
      fgraph<<(deps[i].parent!=dh::NO_VALUE && (deps[i].parent==dh::OCEAN || !(deps[deps[i].parent].lchild==i || deps[deps[i].parent].rchild==i)))<<"\n";
    }
  }

  //Adjust flood heights to absolute elevations
  for(unsigned int i=0;i<topo.size();i++)
    if(!topo.isNoData(i))
      wtd(i) += topo(i);

  wtd.saveGDAL  (out_name+"-flooded.tif");
  label.saveGDAL(out_name+"-label.tif");

  timer_io.stop();

  // NOTE: Demonstrates how to save to NetCDF
  // SaveAsNetCDF(wtd, out_name+"-flooded.nc", "value");

  std::cout<<"Finished"<<std::endl;
  std::cout<<"IO time   = "<<timer_io.accumulated()<<" s"<<std::endl;

  return 0;
}
