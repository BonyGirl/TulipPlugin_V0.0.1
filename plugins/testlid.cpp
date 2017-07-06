//
// Created by zliu58 on 7/6/17.
//

#include<fstream>
#include <algorithm>
#include <string>
#include <set>
#include "testlid.h"

#include <tulip/GlScene.h>
#include <tulip/BooleanProperty.h>
#include <tulip/StringProperty.h>
#include <tulip/IntegerProperty.h>
#include "fabric.h"
#include "ibautils/ib_fabric.h"
#include "ibautils/ib_parser.h"
#include "ibautils/ib_port.h"
#include "ibautils/regex.h"

using namespace std;
using namespace tlp;

PLUGIN(TestLid)

static const char * paramHelp[] = {
        // File to Open
        HTML_HELP_OPEN() \
  HTML_HELP_DEF( "type", "pathname" ) \
  HTML_HELP_BODY() \
  "Path to ibdiagnet2.fdbs file to import" \
  HTML_HELP_CLOSE()
};

TestLid::TestLid(tlp::PluginContext* context)
        : tlp::Algorithm(context)
{
    addInParameter<std::string>("file::filename", paramHelp[0],"");
}


namespace ib = infiniband;
namespace ibp = infiniband::parser;

bool TestLid::run(){
    assert(graph);

    static const size_t STEPS = 5;
    if(pluginProgress)
    {
        pluginProgress->showPreview(false);
        pluginProgress->setComment("Starting to Import Routes");
        pluginProgress->progress(0, STEPS);
    }

    /**
     * while this does not import
     * nodes/edges, it imports properties
     * for an existing fabric
     */

    ib::tulip_fabric_t * const fabric = ib::tulip_fabric_t::find_fabric(graph, false);
    if(!fabric)
    {
        if(pluginProgress)
            pluginProgress->setError("Unable find fabric. Make sure to preserve data when importing data.");

        return false;
    }

    if(pluginProgress)
    {
        pluginProgress->setComment("Found Fabric");
        pluginProgress->progress(1, STEPS);
    }

    /**
     * Open file to read and import per type
     */
    std::string filename;

    dataSet->get("file::filename", filename);
    std::ifstream ifs(filename.c_str());
    if(!ifs)
    {
        if(pluginProgress)
            pluginProgress->setError("Unable open source file.");

        return false;
    }

    if(pluginProgress)
    {
        pluginProgress->progress(2, STEPS);
        pluginProgress->setComment("Parsing Routes.");
    }

    ibp::ibdiagnet_fwd_db parser;
    if(!parser.parse(*fabric, ifs))
    {
        if(pluginProgress)
            pluginProgress->setError("Unable parse routes file.");

        return false;
    }

    if(pluginProgress)
    {
        pluginProgress->setComment("Parsing Routes complete.");
        pluginProgress->progress(3, STEPS);
    }

    ifs.close();


    //find the source and target nodes of the path
    BooleanProperty *selectBool = graph->getLocalProperty<BooleanProperty>("viewSelection");
    StringProperty *getGuid = graph->getLocalProperty<StringProperty>("ibGuid");
 
    vector<tlp::node> nodes_guid;

    tlp::Iterator<tlp::node> *selections = selectBool->getNodesEqualTo(true,NULL);
    const ib::fabric_t::entities_t &entities_map = fabric->get_entities();

    while(selections->hasNext()){
        const tlp::node &mynode = selections->next();
        nodes_guid.push_back(mynode);
    }

    //const unsigned long long int key1 = std::stol((getGuid->getNodeStringValue(nodes_guid[0])).c_str(),NULL,0);
    //const unsigned long long int key2 = std::stol((getGuid->getNodeStringValue(nodes_guid[1])).c_str(),NULL,0);

    const ib::fabric_t::entities_t::const_iterator  s = entities_map.find(std::stol((getGuid->getNodeStringValue(nodes_guid[0])).c_str(),NULL,0));
    if(s != entities_map.end()){
        const ib::entity_t & source_node = s->second;
        cout<<"This is source guid "<<source_node.guid<<" This is source lid: "<<source_node.lid()<<endl;
            
        const ib::entity_t::portmap_t::const_iterator Myport = source_node.ports.begin();

        cout<<"find the port: "<<Myport->first<<endl;
        //use the typedef std::map<port_t*, tlp::edge> port_edges_t to find the edge
        ib::tulip_fabric_t::port_edges_t::iterator Myedge = fabric->port_edges.find(Myport->second);
        cout<<"find the edge: "<<Myedge->second.id<<endl;
        selectBool->setEdgeValue(Myedge->second, true);
        const tlp::edge &e = Myedge->second;
        cout<<e.id<<endl;

        selectBool->setEdgeValue(Myedge->second, true);
    }else{
        cout<<"No exist"<<endl;
    }
    

    //tmp.push_back(const_cast<ib::entity_t *> (&source_node));

    /*for (
            ib::entity_t::routes_t::const_iterator
                    ritr = source_node.get_routes().begin(),
                    reitr = source_node.get_routes().end();
            ritr != reitr;
            ++ritr
            ) {

        cout<< ritr->second.size()<<endl;
        for(std::set<ib::lid_t>::iterator citr = ritr->second.begin();  citr != ritr->second.end(); citr++){
            cout<< *citr <<endl;
        }
    }*/
        
        
    if (pluginProgress) {
        pluginProgress->setComment("Found path source and target");
        pluginProgress->progress(4, STEPS);
    }

    if(pluginProgress)
    {
        pluginProgress->setComment("Print the Real hops");
        pluginProgress->progress(STEPS, STEPS);
    }

    return true;
}

