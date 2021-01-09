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
    int sender;
    int receiver;
    double n;
    std::vector<int> senders {};
    std:: vector<int> receivers {};
    int indexer;
    // statistics gathering
    std::vector<std::vector<int>> generated_frames;
    std::vector<std::vector<int>> frame_sizes;
    std::vector<std::vector<int>> is_dropped;
    std::vector<std::vector<int>> is_duplicated;
    std::vector<std::vector<int>> retransmitted_frames;

  protected:
    virtual void initialize();
    virtual void handleMessage(cMessage *msg);
    cMessage * start_session;
    cMessage * print_stats;
    void generatePairs();
    void startSession();
    void parseMessage(Imessage_Base * msg);
    int applyNoise(Imessage_Base * msg);
    void initializeStats();
    void updateStats(int node_idx, int seq_num, int frame_size, bool is_dropped, bool is_duplicated);
    void printStats(bool collective);
};

#endif
