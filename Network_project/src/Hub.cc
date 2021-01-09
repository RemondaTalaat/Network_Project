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

#include "Hub.h"

Define_Module(Hub);

void Hub::generatePairs()
{

    int node1,node2;
    // loop over nodes twice
    for(int i =0; i < this->n*this->n; ++i) {

        // pick random node to be the sender
        node1 = uniform(0, this->n);
        // push it in senders vector
        this->senders.push_back(node1);

        // pick random node not equal the sender node to be the receiver
        do {
            node2 = uniform(0, this->n);
        } while(node2 == node1);

        // push it to the receivers vector
        this->receivers.push_back(node2);
    }

    for(int i = 0; i < this->n*this->n; ++i) {
        EV <<  "senders[" << i << "] = " << this->senders[i] <<endl;
        EV << " Receivers[" << i << "] = " << this->receivers[i] <<endl;
    }

}

void Hub::initialize()
{
    this->n = par("n");
    this->indexer = -1;
    //generate the pairs table
    generatePairs();
    // initialize statistics gathering variables
    initializeStats();
    // initialize the sender and receiver nodes
    this->sender = this->senders[0];
    this->receiver = this->receivers[0];
    this->start_session = new cMessage("");
    this->start_session->setKind(1);
    scheduleAt(simTime() + par("session_time"), this->start_session); // session time default value = 15 second
    this->print_stats = new cMessage("");
    this->print_stats->setKind(2);
    scheduleAt(simTime() + par("stats_time"), this->print_stats); // stats print time default value = 180 second
}

void Hub::startSession()
{

    // advance the indexer to schedule the next pairs
    this->indexer ++;
    if(this->indexer == this->senders.size()){
        this->indexer = 0;
    }

    this->sender = this->senders[this->indexer];
    this->receiver = this->receivers[this->indexer];

    // send messages to notify the nodes the session started

    cMessage* msg = new cMessage("sender");
    msg ->setKind(4);
    send(msg, "outs", sender);
    cMessage* msg2 = new cMessage("receiver");
    msg2 ->setKind(4);
    send(msg2, "outs", receiver);

    // schedule the next session if none of the nodes finishes during the session time
    EV << ". Scheduled a new session after " << par("session_time").doubleValue()  << "s" << endl;
    scheduleAt(simTime() + par("session_time") , this->start_session);
}

// return int represent what to do with the message
// 0 -> lose message
// 1 -> delayed
// 2 -> delayed duplicate
// 3 -> duplicate
// 4 -> send the message as is
// any modification is added to the msg in this function so no need for case for modification
int Hub::applyNoise(Imessage_Base * msg) {
    // apply any noise to the node

    int choice =uniform(0,3);  // 0 -> lose , 1 -> do nothing , 2 -> delayed
    EV << "choice = " << choice <<endl;
    // in case of lose return 0 so the message deleted
    if (choice == 0)
    {
        EV << "Message lost !" <<endl;
        return 0;
    }

    // calculate the probability to modify a message
    int prob_modify  = uniform(0,1)*10;
    EV << "prob modification = " << prob_modify <<endl;
    // in case of message modification
    if(prob_modify > par("prob_modification").doubleValue()) {

        std::string s = msg ->getMessage_payload();
        EV << " message before modification " << s << endl;
        int size = s.size();
        int random_var = uniform(0, size);
        std::bitset<8> bits(s[random_var]);
        EV << bits.to_string() << endl;
        int random_bit = uniform(0,8);
        bits[random_bit] = ! bits[random_bit];
        EV << bits.to_string() << endl;
        s[random_var] = (char)bits.to_ulong();
        std::stringstream ss;
        ss << s;
        msg->setMessage_payload(ss.str().c_str());
        EV << " message after modification " << s << endl;

        EV << " Message got modified !" << endl;

    }

    int prob_duplicate  = uniform(0,1)*10;

    EV << "prob duplication = " << prob_duplicate <<endl;

    if (prob_duplicate > par("prob_duplication").doubleValue()){
        EV << "message duplicated" <<endl;
        if (choice == 2) {
            return 2;
        } else {
            return 3;
        }
    }
    if (choice == 2) {
        return 1;
    }
    EV << " message send normally" <<endl;
    return 4;

}

void Hub::handleMessage(cMessage *msg)
{

    if (msg->isSelfMessage()) {
        if (msg->getKind() == 1) {
            // when receiving a self message of kind 1 allocate new session for 2 random nodes to communicate with each other
            startSession();
        } else {
            // when receiving a self message of kind 2 print statistics (collective)
            this->printStats(true);
            scheduleAt(simTime() + par("stats_time"), this->print_stats);
        }
     // receiving a message from node
    } else{
        // normal message process it normally
        try {
            // get the message sender
            Imessage_Base * rmsg = check_and_cast<Imessage_Base *> (msg);
            parseMessage(rmsg);
        }
        // end session message
        catch(...) {
            EV << "session ends early and starting a new one now" <<endl;
            cancelAndDelete(msg);
            cancelEvent(this->start_session);
            startSession();
            // do something here to end the current session and start new one
        }
    }
}


void Hub::parseMessage(Imessage_Base * msg) {
    // get the message sender
    int msg_sender = msg->getSenderModule()->getIndex();

    // logs for debugging
    EV << " the message sender id = "<< msg_sender << endl;
    EV << " the hub sender id = " << sender << endl;
    EV << " the hub receiver id = " << receiver << endl;

    // if a message was received from previous session ignore it
    if (msg_sender != this->sender && msg_sender != this->receiver) {

            cancelAndDelete(msg);
            return;
    }

    // get the message receiver
    int msg_receiver;

    if (msg_sender == this->sender) {

        msg_receiver = this->receiver;

    } else {
        msg_receiver = this->sender;
    }
    // apply the transmission noise
    int noise = applyNoise(msg);
    int delay = exponential( par("lambda_delay").doubleValue());
    std::string msg_payload = msg->getMessage_payload();
    EV << msg_payload << endl;
    //send the message
    switch(noise)
    {
        // lose the message
        case 0:
        {
            this->updateStats(msg_sender, msg->getSequence_number(), msg_payload.size(), true, false);
            cancelAndDelete(msg);
            return;
        }
        // delayed , delayed and modified
        case 1:
        {
            EV <<  " message delayed"  <<endl;
            this->updateStats(msg_sender, msg->getSequence_number(), msg_payload.size(), false, false);
            sendDelayed(msg, delay, "outs", msg_receiver);
            break;
        }
        // delayed duplicate , delayed duplicate and modified
        case 2:
        {
            EV << "message delayed " <<endl;
            this->updateStats(msg_sender, msg->getSequence_number(), msg_payload.size(), false, true);
            Imessage_Base * msg2 = msg->dup();
            sendDelayed(msg, delay, "outs", msg_receiver);
            sendDelayed(msg2,delay, "outs", msg_receiver);
            break;
        }
        case 3:
        {
            this->updateStats(msg_sender, msg->getSequence_number(), msg_payload.size(), false, true);
            Imessage_Base * msg2 = msg->dup();
            send(msg, "outs", msg_receiver);
            send(msg2, "outs", msg_receiver);
            break;
        }
        // send the message normally or modified only
        default :
            this->updateStats(msg_sender, msg->getSequence_number(), msg_payload.size(), false, false);
            send(msg, "outs", msg_receiver );
            break;
    }

}

void Hub::initializeStats() {
    for (int i = 0; i < this->n; i++) {
        this->generated_frames.push_back(std::vector<int>());
        this->frame_sizes.push_back(std::vector<int>());
        this->is_dropped.push_back(std::vector<int>());
        this->is_duplicated.push_back(std::vector<int>());
        this->retransmitted_frames.push_back(std::vector<int>());
    }
}

void Hub::updateStats(int node_idx, int seq_num, int frame_size, bool is_dropped, bool is_duplicated) {
    auto it = std::find(this->generated_frames[node_idx].begin(), this->generated_frames[node_idx].end(), seq_num);
    if(it != this->generated_frames[node_idx].end()) {
        int index = it - this->generated_frames[node_idx].begin();
        this->retransmitted_frames[node_idx][index] ++;
        if (is_dropped) {
            this->is_dropped[node_idx][index] ++;
        }
        if (is_duplicated) {
            this->is_duplicated[node_idx][index] ++;
        }
    } else {
        this->generated_frames[node_idx].push_back(seq_num);
        this->frame_sizes[node_idx].push_back(frame_size);
        this->retransmitted_frames[node_idx].push_back(0);
        if (is_dropped) {
            this->is_dropped[node_idx].push_back(1);
        } else {
            this->is_dropped[node_idx].push_back(0);
        }
        if (is_duplicated) {
            this->is_duplicated[node_idx].push_back(1);
        } else {
            this->is_duplicated[node_idx].push_back(0);
        }
    }
}

void Hub::printStats(bool collective) {
    if (collective) {
        int total_generated = 0;
        for (int i = 0; i < this->n; i++) {
            total_generated += this->generated_frames[i].size();
        }
        int total_dropped = 0;
        for (int i = 0; i < this->n; i++) {
            total_dropped += std::accumulate(this->is_dropped[i].begin(), this->is_dropped[i].end(), 0);
        }
        int total_retransmitted = 0;
        for (int i = 0; i < this->n; i++) {
            total_retransmitted += std::accumulate(this->retransmitted_frames[i].begin(), this->retransmitted_frames[i].end(), 0);
        }
        float useful_data = 0;
        float all_data = 0;
        for (int i = 0; i < this->n; i++) {
            for (int j = 0; j < this->generated_frames[i].size(); j++) {
                useful_data += this->frame_sizes[i][j];
                all_data += (this->retransmitted_frames[i][j] + this->is_duplicated[i][j] + 1) * (4*3 + this->frame_sizes[i][j]);
            }
        }
        float efficieny = useful_data/all_data;
        EV << "####################### Collective Statistics #######################" << endl;
        EV << "Total number of generated frames = " << total_generated << endl;
        EV << "Total number of dropped frames = " << total_dropped << endl;
        EV << "Total number of retransmitted frames = " << total_retransmitted << endl;
        EV << "Percentage of useful transmitted data = " << efficieny*100 << "%" << endl;
    } else {
        for (int i = 0; i < this->n; i++) {
            int total_generated = this->generated_frames[i].size();
            int total_dropped = std::accumulate(this->is_dropped[i].begin(), this->is_dropped[i].end(), 0);
            int total_retransmitted = std::accumulate(this->retransmitted_frames[i].begin(), this->retransmitted_frames[i].end(), 0);
            float useful_data = 0;
            float all_data = 0;
            for (int j = 0; j < this->generated_frames[i].size(); j++) {
                useful_data += this->frame_sizes[i][j];
                all_data += (this->retransmitted_frames[i][j] + this->is_duplicated[i][j] + 1) * (4*3 + this->frame_sizes[i][j]);
            }
            float efficieny = useful_data/all_data;
            EV << "####################### Collective Statistics #######################" << endl;
            EV << "####################### Node [" << i << "] #######################" << endl;
            EV << "Total number of generated frames = " << total_generated << endl;
            EV << "Total number of dropped frames = " << total_dropped << endl;
            EV << "Total number of retransmitted frames = " << total_retransmitted << endl;
            EV << "Percentage of useful transmitted data = " << efficieny*100 << "%" << endl;
        }
    }
}

