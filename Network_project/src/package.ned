package network_project;

@license(LGPL);

import ned.IdealChannel;

network Star
{
    parameters:
        int n = default(5);
    submodules:
        nodes[n]: Node ;
        hub     : Hub;
    connections: 
       for i=0..sizeof(nodes)-1
       {
           hub.outs++ --> IdealChannel --> nodes[i].in;
           hub.ins++  <-- IdealChannel <-- nodes[i].out;
       }  
}
