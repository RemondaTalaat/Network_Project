//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU Lesser General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
// 
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU Lesser General Public License for more details.
// 
// You should have received a copy of the GNU Lesser General Public License
// along with this program.  If not, see http://www.gnu.org/licenses/.
// 

#ifndef __NETWORK_PROJECT_HUB_H_
#define __NETWORK_PROJECT_HUB_H_

#include <omnetpp.h>
#include <bitset>
#include <vector>
#include <algorithm>
#include <numeric>
#include <string>
#include <cstdlib>
#include "Imessage_m.h"
using namespace omnetpp;

/**
 * TODO - Generated class
 */
class Hub : public cSimpleModule
{
  private:

    int sender;                                        // hold the first node in a session
    int receiver;                                      // hold the second node in a session
    double n;                                          // total number of nodes connected to the hub

    // session pairs table data members
    std::vector<int> senders {};
    std:: vector<int> receivers {};
    int indexer;                                       // to loop over the table pairs

    cMessage * start_session;                          // the self-message used to start a session

    // statistics gathering
    std::vector<std::vector<int>> generated_frames;
    std::vector<std::vector<int>> frame_sizes;
    std::vector<std::vector<int>> is_dropped;
    std::vector<std::vector<int>> is_duplicated;
    std::vector<std::vector<int>> retransmitted_frames;

    cMessage * print_stats;

  protected:
    virtual void initialize();                         // initialize the hub data members and generate pairs table
    virtual void handleMessage(cMessage *msg);         // handle any received message


    void generatePairs();                              // generate the pairs of nodes table
    void startSession();                               // start a new session to navigate their messages
    void parseMessage(Imessage_Base * msg);            // navigate a received message from its sender to a specific receiver
    int applyNoise(Imessage_Base * msg);               // mimic the channel noise effect on the message

    // statistics functions
    void initializeStats();
    void updateStats(int node_idx, int seq_num, int frame_size, bool is_dropped, bool is_duplicated);
    void printStats(bool collective);
};

#endif
